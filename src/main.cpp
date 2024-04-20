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
#include "shading.h"
#include "matrix.h"
#include "mesh.h"
#include "environment.h"

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];
bool needsAA[VFB_MAX_SIZE][VFB_MAX_SIZE];
Camera camera;
Color backgroundColor(0, 0, 0);
const int MAX_RAY_DEPTH = 10;
std::vector<Rect> buckets;
bool wantAA = true;
const float AA_THRESH = 0.075f;

struct Node {
	Geometry* geom;
	Shader* shader;
	Transform T;
	BumpTexture* bump = nullptr;

	bool intersect(Ray ray, IntersectionInfo& info);
};

void addToUnion(Geometry*& csg, Geometry* toAdd)
{
	if (!csg) csg = toAdd;
	else      csg = new CSGUnion(csg, toAdd);
}

std::vector<Node> nodes;

Geometry* createRoundedEdgesCube(Vector center, double side, double cornerRadius)
{
	Geometry* allCornerCubes = nullptr;
	Geometry* allCornerSpheres = nullptr;
	double offset = side/2 - cornerRadius;
	assert(offset >= 0);
	double eps = side / 100;
	// the big cube is just a cube with the given center and side:
	Cube* bigCube = new Cube(center, side);
	// iterate all 8 corners of the big cube:
	for (int dz = -1; dz <= 1; dz += 2) {
		for (int dy = -1; dy <= 1; dy += 2) {
			for (int dx = -1; dx <= 1; dx += 2) {
				Cube* smallCube = new Cube(center + Vector(dx, dy, dz) * (offset + cornerRadius/2 + eps), cornerRadius + 2*eps);
				Sphere* smallSphere = new Sphere(center + Vector(dx, dy, dz) * offset, cornerRadius*sqrt(2.0));
				addToUnion(allCornerCubes, smallCube);
				addToUnion(allCornerSpheres, smallSphere);
			}
		}
	}
	CSGBase* cubeWithoutCorners = new CSGDiff(bigCube, allCornerCubes);
	CSGBase* roundedCube = new CSGUnion(cubeWithoutCorners, new CSGInter(bigCube, allCornerSpheres));
	return roundedCube;
}

Vector csgCenter(-50, 20, 15);
Cube cube(Vector(0, 0 ,0), 40);
Sphere sphere(Vector(0, 0, 0), 26);
Environment* environment = nullptr;
void setupScene()
{
	camera.pos.set(0, 60, -120);
	camera.pitch = -10;
	camera.beginFrame();
	sphere.uvscaling = 50;
	// Create the floor:
	Texture* floor  = new BitmapTexture("data/floor.bmp");
	Layered* floorShader = new Layered;
	floorShader->addLayer( new Lambert(Color(0, 0, 0.8), floor));
	//floorShader->addLayer( new Reflection, Color(0.02, 0.02, 0.02));
	nodes.push_back(Node{ new Plane(0), floorShader });
	// Create a globe with the world map:
	Phong* globeMat = new Phong;
	globeMat->specularExponent = 120.0f;
	globeMat->diffuseTex = new BitmapTexture("data/world.bmp", 1);
	Layered* glass = new Layered;
	glass->addLayer(new Refraction);
	glass->addLayer(new Reflection, Color(1, 1, 1), new Fresnel);
	Reflection* glossy = new Reflection(0.45f);
	//nodes.push_back(Node{ new Sphere(Vector(50, 30, 15), 30), glossy});
	// Create a CSG object: a cube with a cut out sphere in the middle
	Texture* checker = new CheckerTexture(Color(0x8d3d3d), Color(0x9c9c9c), 5);
	Phong* phong = new Phong(Color(0.6, 0.4, 0.1), Color(1, 1, 1), 120.0f, checker);
	Reflection* refl1 = new Reflection;
	Reflection* refl2 = new Reflection;
	refl1->reflColor = Color(0.8, 0.8, 0.8);
	refl2->reflColor = Color(0.4, 0.4, 0.9);
	auto csg = new CSGDiff(&cube, &sphere);
	Mesh* teapot = new Mesh;
	teapot->loadFromOBJ("data/geom/teapot_lowres.obj");
	teapot->beginRender();
	float f1 = 0.6, f2 = 0.7;
	Texture* teapotTex = new CheckerTexture(Color(f1, f1, f1), Color(f2, f2, f2), 0.2f);
	Texture* zarTex = new BitmapTexture("data/texture/zar-texture.bmp", 1);
	nodes.push_back(Node{ teapot, new Phong(Color(0.9, 0.1, 0.1), Color(1, 1, 1), 84.0f/*, zarTex*/)});
	nodes.back().T.translate(Vector(-50, 0, 0));
	nodes.back().T.scale(30);

	nodes.back().bump = new BumpTexture();
	nodes.back().bump->loadFile("data/texture/zar-bump.bmp");
	nodes.back().bump->scaling = 0.8f;
	nodes.back().bump->strength = 3.2f;

	/**/
	/*
	nodes.push_back(Node{ csg, refl1});
	nodes.back().T.translate(csgCenter);
	nodes.push_back(Node{ csg, refl2});
	nodes.back().T.translate(csgCenter + Vector(40, 0, 0));
	nodes.back().T.scale(0.5);
	*/
	// Create a complex CSG object: cube with rounded edges:
	/*nodes.push_back(Node{ createRoundedEdgesCube(Vector(0, 20, 15), 40, 10),
	                      new Phong(Color(0.9, 0.4, 0.1), Color(1, 1, 1), 125.0f)});*/
	/*
	nodes.push_back(Node{ createRoundedEdgesCube(Vector(0, 10, -15), 20, 8),
	                      new Phong(Color(0.1, 0.4, 0.9), Color(1, 1, 1), 125.0f)});
	nodes.push_back(Node{ createRoundedEdgesCube(Vector(0, 40, 100), 80, 10),
	                      new Phong(Color(0.4, 0.9, 0.1), Color(1, 1, 1), 125.0f)});
	*/
	lightPos.set(+30, +100, -70);
	lightColor.setColor(1, 1, 1);
	lightIntensity = 10000.0;
	//
	environment = new CubemapEnvironment("data/env/forest");
}

Color raytrace(Ray ray)
{
	if (ray.depth > MAX_RAY_DEPTH) return Color(1, 0, 0);
	IntersectionInfo closestIntersection;
	closestIntersection.dist = INF;
	Node closestNode;
	//
	for (auto& node: nodes) {
		IntersectionInfo info;
		if (node.intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	//
	if (closestIntersection.dist >= INF) {
		if (environment) return environment->getEnvironment(ray.dir);
		return backgroundColor;
	}
	if (closestNode.bump) {
		closestNode.bump->modifyNormal(closestIntersection);
	}
	return closestNode.shader->computeColor(ray, closestIntersection);
}

bool Node::intersect(Ray ray, IntersectionInfo& info)
{
	Vector origStart = ray.start;
	ray.start = T.untransformPoint(ray.start);
	ray.dir = T.untransformDir(ray.dir);
	if (!geom->intersect(ray, info)) return false;
	//
	info.ip = T.transformPoint(info.ip);
	info.norm = T.transformDir(info.norm);
	info.dist = distance(origStart, info.ip);
	return true;
}

bool visible(Vector A, Vector B)
{
	double D = distance(A, B) - 1e-3;
	Ray ray;
	ray.start = A;
	ray.dir = B - A;
	ray.dir.normalize();
	//
	for (auto& node: nodes) {
		IntersectionInfo info;
		if (node.intersect(ray, info) && info.dist < D) {
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

bool render(bool displayProgress) // returns true if the complete frame is rendered
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
				vfb[y][x] = raytrace(camera.getScreenRay(x, y)); // should be "x + AA_KERNEL[0][0]", etc.
		if (displayProgress) displayVFBRect(r, vfb);
		if (checkForUserExit()) return false;
	}
	// Do we need AA? if not, we're done
	if (!wantAA) return true;

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
					vfb[y][x] += raytrace(camera.getScreenRay(x + AA_KERNEL[i][0], y + AA_KERNEL[i][1]));
				vfb[y][x] *= mul;
			}
		if (displayProgress) displayVFBRect(r, vfb);
		if (checkForUserExit()) return false;
	}

	return true;
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

bool renderAnimation()
{
	wantAA = false; // make the animation quicker
	for (double angle = 0; angle < 360; angle += 10) {
/*		double a_rad = toRadians(angle);
		camera.pos = Vector(sin(a_rad) * 120, 60, -cos(a_rad) * 120);
		camera.yaw = angle;
		camera.beginFrame();*/
		nodes.back().T.rotate(20, 0, 0);
		//cube.beginFrame();
		bool go = render(false);
		displayVFB(vfb);
		if (!go || checkForUserExit()) return false;
	}
	return true;
}

bool renderStatic()
{
	camera.beginFrame();
	return render(true);
}

int main(int argc, char** argv)
{
	ensureDataIsVisible();
	initGraphics(800, 600);
	buckets = getBucketsList();
	setupScene();
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
