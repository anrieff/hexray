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
#include "environment.h"

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];
Camera camera;
Color backgroundColor(0, 0, 0);
const int MAX_RAY_DEPTH = 10;

struct Node {
	Geometry* geom;
	Shader* shader;
	Transform T;

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
	nodes.push_back(Node{ new Sphere(Vector(50, 30, 15), 30), glossy});
	// Create a CSG object: a cube with a cut out sphere in the middle
	Texture* checker = new CheckerTexture(Color(0x8d3d3d), Color(0x9c9c9c), 5);
	Phong* phong = new Phong(Color(0.6, 0.4, 0.1), Color(1, 1, 1), 120.0f, checker);
	Reflection* refl1 = new Reflection;
	Reflection* refl2 = new Reflection;
	refl1->reflColor = Color(0.8, 0.8, 0.8);
	refl2->reflColor = Color(0.4, 0.4, 0.9);
	auto csg = new CSGDiff(&cube, &sphere);
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

void render()
{
	static const float AA_KERNEL[5][2] {
		{ 0.0f, 0.0f },
		{ 0.6f, 0.0f },
		{ 0.3f, 0.3f },
		{ 0.0f, 0.6f },
		{ 0.6f, 0.6f },
	};
	memset(vfb, 0, sizeof(vfb));
	for (int y = 0; y < frameHeight(); y++) {
		for (int x = 0; x < frameWidth(); x++) {
			Color sum(0, 0, 0);
			for (int i = 0; i < 1; i++) {
				Ray ray = camera.getScreenRay(x + AA_KERNEL[i][0], y + AA_KERNEL[i][1]);
				sum += raytrace(ray);
			}
			vfb[y][x] = sum * 1.0f;
		}
	}
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

int main(int argc, char** argv)
{
	ensureDataIsVisible();
	initGraphics(800, 600);
	setupScene();
	Uint32 start = SDL_GetTicks();
	bool shouldExit = false;
	for (double angle = 0; !shouldExit && angle < 360; angle += 10) {
		double a_rad = toRadians(angle);
		camera.pos = Vector(sin(a_rad) * 120, 60, -cos(a_rad) * 120);
		camera.yaw = angle;
		camera.beginFrame();
		//nodes.back().T.rotate(30, 15, 0);
		cube.beginFrame();
		render();
		displayVFB(vfb);
		if (checkForUserExit()) {
			printf("Exited early.\n");
			shouldExit = true;
		}
	}
	if (!shouldExit) {
		Uint32 end = SDL_GetTicks();
		printf("Elapsed time: %.2f seconds.\n", (end - start) / 1000.0);
		waitForUserExit();
	}
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
