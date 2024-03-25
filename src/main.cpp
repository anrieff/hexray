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
#include "util.h"
#include "sdl.h"
#include "color.h"
#include "vector.h"
#include "camera.h"
#include "geometry.h"
#include "shading.h"
#include <vector>

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];
Camera camera;
Color backgroundColor(0, 0, 0);

struct Node {
	Geometry* geom;
	Shader* shader;
};

std::vector<Node> nodes;

Sphere sphere(Vector(-10, 60, 0), 30);
//Sphere sphere(Vector(5, 75, -15), 10);
void setupScene()
{
	camera.pos.set(0, 60, -120);
	camera.beginFrame();
	Texture* floor  = new BitmapTexture("../data/floor.bmp");
	Texture* world  = new BitmapTexture("../data/world.bmp", 1);
	Phong* phong = new Phong(Color(0.6, 0.4, 0.1), Color(1, 1, 1), 120.0f, world);
	nodes.push_back(Node{ new Plane(15), new Lambert(Color(0, 0, 0.8), floor)});
	nodes.push_back(Node{ &sphere, phong});
	/*nodes.push_back(Node{ &sphere, phong});*/
	//nodes.push_back(Node{ new CSGDiff(&cube, &sphere), phong});
	lightPos.set(+30, +100, -70);
	lightColor.setColor(1, 1, 1);
	lightIntensity = 5000.0;
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
	initGraphics(1024, 768);
	setupScene();
	Uint32 start = SDL_GetTicks();
	for (double angle = 0; angle < 360; angle += 10) {
//		sphere.O.y = y;
		double a_rad = toRadians(angle);
		camera.pos = Vector(sin(a_rad) * 120, 85, -cos(a_rad) * 120);
		camera.yaw = angle;
		camera.beginFrame();
		render();
		displayVFB(vfb);
	}
	Uint32 end = SDL_GetTicks();
	printf("Elapsed time: %.2f seconds.\n", (end - start) / 1000.0);
	waitForUserExit();
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
