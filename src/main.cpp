/***************************************************************************
 *   Copyright (C) 2009-2024 by Veselin Georgiev, Slavomir Kaslev,         *
 *                              Deyan Hadzhiev et al                       *
 *   admin@raytracing-bg.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @File main.cpp
 * @Brief Raytracer main file
 */
#include <SDL.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <vector>
#include <filesystem>
#include "util.h"
#include "sdl.h"
#include "color.h"
#include "vector.h"
#include "camera.h"
#include "geometry.h"
#include "node.h"
#include "shading.h"
#include "matrix.h"
#include "mesh.h"
#include "environment.h"
#include "lights.h"

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];
bool needsAA[VFB_MAX_SIZE][VFB_MAX_SIZE];
std::vector<Rect> buckets;
const float AA_THRESH = 0.075f;

Color raytrace(Ray ray)
{
	if (ray.depth > scene.settings.maxTraceDepth) return Color(1, 0, 0);
	IntersectionInfo closestIntersection;
	closestIntersection.dist = INF;
	Node* closestNode = nullptr;
	//
	for (auto& node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	//
	if (closestIntersection.dist >= INF) {
		if (scene.environment) return scene.environment->getEnvironment(ray.dir);
		return scene.settings.backgroundColor;
	}
	if (closestNode->bump) {
		closestNode->bump->modifyNormal(closestIntersection);
	}
	return closestNode->shader->computeColor(ray, closestIntersection);
}
bool visible(Vector A, Vector B)
{
	double D = distance(A, B);
	Ray ray;
	ray.start = A;
	ray.dir = B - A;
	ray.dir.normalize();
	//
	for (auto& node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < D) {
			return false;
		}
	}
	//
	return true;
}

static void detectAApixels()
{
	const int neighbours[8][2] = {
		{ -1, -1 }, { 0, -1 }, { 1, -1 },
		{ -1,  0 },            { 1,  0 },
		{ -1,  1 }, { 0,  1 }, { 1,  1 }
	};
	int W = frameWidth(), H = frameHeight();
	for (auto& r: buckets) {
		for (int y = r.y0; y < r.y1; y++)
			for (int x = r.x0; x < r.x1; x++) {
				needsAA[y][x] = false;
				const Color& me = vfb[y][x];
				for (int ni = 0; ni < COUNT_OF(neighbours); ni++) {
					int neighX = x + neighbours[ni][0];
					int neighY = y + neighbours[ni][1];
					if (neighX < 0 || neighX >= W || neighY < 0 || neighY >= H) continue;
					const Color& neighbour = vfb[neighY][neighX];
					for (int channel = 0; channel < 3; channel++) {
						if (fabs(std::min(1.0f, me[channel]) - std::min(1.0f, neighbour[channel])) > AA_THRESH) {
							needsAA[y][x] = true;
							break;
						}
					}
					if (needsAA[y][x]) break;
				}
			}
	}
}

bool renderNoDOF(bool displayProgress) // returns true if the complete frame is rendered
{
	static const float AA_KERNEL[5][2] {
		{ 0.0f, 0.0f },
		{ 0.6f, 0.0f },
		{ 0.3f, 0.3f },
		{ 0.0f, 0.6f },
		{ 0.6f, 0.6f },
	};
	static const int AA_KERNEL_SIZE = int(COUNT_OF(AA_KERNEL));

	// Pass 1: render without anti-aliasing
	for (auto& r: buckets) {
		for (int y = r.y0; y < r.y1; y++)
			for (int x = r.x0; x < r.x1; x++)
				vfb[y][x] = raytrace(scene.camera->getScreenRay(x, y)); // should be "x + AA_KERNEL[0][0]", etc.
		if (displayProgress) displayVFBRect(r, vfb);
		if (checkForUserExit()) return false;
	}
	// Do we need AA? if not, we're done
	if (!scene.settings.wantAA) return true;

	// Pass 2: detect pixels, needing AA:
	detectAApixels();
	// show them:
	if (displayProgress) markAApixels(needsAA);

	// Pass 3: recompute those pixels with the AA kernel:
	float mul = 1.0f / AA_KERNEL_SIZE;
	for (auto& r: buckets) {
		if (displayProgress) markRegion(r);
		for (int y = r.y0; y < r.y1; y++)
			for (int x = r.x0; x < r.x1; x++) if (needsAA[y][x]) {
				for (int i = 1; i < AA_KERNEL_SIZE; i++) // note that we skip index i=0, as we did it in pass 1.
					vfb[y][x] += raytrace(scene.camera->getScreenRay(x + AA_KERNEL[i][0], y + AA_KERNEL[i][1]));
				vfb[y][x] *= mul;
			}
		if (displayProgress) displayVFBRect(r, vfb);
		if (checkForUserExit()) return false;
	}

	return true;
}

bool renderDOF(bool displayProgress) // returns true if the complete frame is rendered
{
	if (scene.camera->autoFocus) {
		Ray midRay = scene.camera->getScreenRay(frameWidth() * 0.5, frameHeight() * 0.5);
		double closestIntersectionDist = INF;
		//
		for (auto& node: scene.nodes) {
			IntersectionInfo info;
			if (node->intersect(midRay, info) && info.dist < closestIntersectionDist)
				closestIntersectionDist = info.dist;
		}
		//
		if (closestIntersectionDist < INF)
			scene.camera->focalPlaneDist = closestIntersectionDist;
	}
	Vector frontDir = scene.camera->getFrontDir();
	for (auto& r: buckets) {
		for (int y = r.y0; y < r.y1; y++)
			for (int x = r.x0; x < r.x1; x++) {
				Color sum(0, 0, 0);
				for (int i = 0; i < scene.camera->numSamples; i++) {
					Ray ray = scene.camera->getScreenRay(x + randFloat(), y + randFloat());
					double M = scene.camera->focalPlaneDist / dot(frontDir, ray.dir);
					Vector T = ray.start + ray.dir * M;
					double u, v;
					unitDiskSample(u, v);
					ray.start = scene.camera->getDOFRayStart(u, v);
					ray.dir = normalize(T - ray.start);
					sum += raytrace(ray);
				}
				vfb[y][x] = sum / scene.camera->numSamples;
			}
		if (displayProgress) displayVFBRect(r, vfb);
		if (checkForUserExit()) return false;
	}
	return true;
}

bool render(bool displayProgress)
{
	if (scene.camera->dof) return renderDOF(displayProgress);
	else return renderNoDOF(displayProgress);
}

// makes sure we see the "data" dir:
static void ensureDataIsVisible()
{
	namespace fs = std::filesystem;
	// handle the common case where we launch "hexray" from within "hexray/build"
	if (!fs::exists("data") && fs::exists("../data"))
		fs::current_path("..");
	// otherwise, we can't continue:
	if (!fs::exists("data")) {
		printf("Error: the \"data\" directory is not visible!\n");
		printf("(make sure you run hexray from the correct directory; either \"hexray\" or \"hexray/build\")\n");
		exit(1);
	}
}

bool renderStatic()
{
	scene.beginRender();
	scene.beginFrame();
	return render(true);
}

const char* DEFAULT_SCENE = "data/meshes.hexray";

int main(int argc, char** argv)
{
	Color::init_sRGB_cache();
	ensureDataIsVisible();
	const char* sceneFile = DEFAULT_SCENE;
	if (argc > 1 && strlen(argv[1]) && argv[1][0] != '-') sceneFile = argv[1];
	if (!scene.parseScene(sceneFile)) {
		printf("Could not parse the scene file (%s)!\n", sceneFile);
		return 1;
	}
	initGraphics(scene.settings.frameWidth, scene.settings.frameHeight);
	buckets = getBucketsList();
	Uint32 start = SDL_GetTicks();
	if (renderStatic()) {
		Uint32 end = SDL_GetTicks();
		printf("Elapsed time: %.2f seconds.\n", (end - start) / 1000.0);
		waitForUserExit();
	}
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
