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
#include <vector>
#include "util.h"
#include "sdl.h"
#include "color.h"
#include "vector.h"
#include "camera.h"
#include "geometry.h"
#include "shading.h"

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];
Camera camera;
Color backgroundColor(0, 0, 0);

struct Node {
	Geometry* geom;
	Shader* shader;
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
Cube cube(csgCenter, 40);
Sphere sphere(csgCenter, 26);
void setupScene()
{
	camera.pos.set(0, 60, -120);
	camera.pitch = -10;
	camera.beginFrame();
	sphere.uvscaling = 50;
	// Create the floor:
	Texture* floor  = new BitmapTexture("../data/floor.bmp");
	nodes.push_back(Node{ new Plane(0), new Lambert(Color(0, 0, 0.8), floor)});
	// Create a globe with the world map:
	Phong* globeMat = new Phong;
	globeMat->specularExponent = 120.0f;
	globeMat->diffuseTex = new BitmapTexture("../data/world.bmp", 1);
	nodes.push_back(Node{ new Sphere(Vector(50, 30, 15), 30), globeMat});
	// Create a CSG object: a cube with a cut out sphere in the middle
	Texture* checker = new CheckerTexture(Color(0x8d3d3d), Color(0x9c9c9c), 5);
	Phong* phong = new Phong(Color(0.6, 0.4, 0.1), Color(1, 1, 1), 120.0f, checker);
	nodes.push_back(Node{ new CSGDiff(&cube, &sphere), phong});
	// Create a complex CSG object: cube with rounded edges:
	nodes.push_back(Node{ createRoundedEdgesCube(Vector(0, 20, 15), 40, 10),
	                      new Phong(Color(0.9, 0.4, 0.1), Color(1, 1, 1), 125.0f)});
	/*
	nodes.push_back(Node{ createRoundedEdgesCube(Vector(0, 10, -15), 20, 8),
	                      new Phong(Color(0.1, 0.4, 0.9), Color(1, 1, 1), 125.0f)});
	nodes.push_back(Node{ createRoundedEdgesCube(Vector(0, 40, 100), 80, 10),
	                      new Phong(Color(0.4, 0.9, 0.1), Color(1, 1, 1), 125.0f)});
	*/
	lightPos.set(+30, +100, -70);
	lightColor.setColor(1, 1, 1);
	lightIntensity = 10000.0;
}

Color raytrace(Ray ray)
{
	IntersectionInfo closestIntersection;
	closestIntersection.dist = INF;
	Node closestNode;
	//
	for (auto& node: nodes) {
		IntersectionInfo info;
		if (node.geom->intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	//
	if (closestIntersection.dist >= INF) return backgroundColor;
	return closestNode.shader->computeColor(ray, closestIntersection);
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
		if (node.geom->intersect(ray, info) && info.dist < D) {
			return false;
		}
	}
	//
	return true;
}

void render()
{
	for (int y = 0; y < frameHeight(); y++)
		for (int x = 0; x < frameWidth(); x++) {
			Ray ray = camera.getScreenRay(x, y);
			vfb[y][x] = raytrace(ray);
		}
}

int main(int argc, char** argv)
{
	initGraphics(800, 600);
	setupScene();
	Uint32 start = SDL_GetTicks();
	bool shouldExit = false;
	for (double angle = 0; angle < 360 && !shouldExit; angle += 30) {
		double a_rad = toRadians(angle);
		camera.pos = Vector(sin(a_rad) * 120, 60, -cos(a_rad) * 120);
		camera.yaw = angle;
		camera.beginFrame();
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
