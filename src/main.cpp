/***************************************************************************
 *   Copyright (C) 2009-2024 by Veselin Georgiev, Slavomir Kaslev et al    *
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

void setupScene()
{
	camera.pos.set(0, 60, -120);
	camera.beginFrame();
	nodes.push_back(Node{ new Plane(15), new Checker(Color(0, 0, 0.8), Color(0.2, 0.3, 0.6))});
	nodes.push_back(Node{ &sphere, new Checker(Color(0.8, 0, 0), Color(0.6, 0.4, 0.1), 9)});
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
//	for (double y = 60; y >= -30; y -= 1.5) {
//		sphere.O.y = y;
		camera.beginFrame();
		render();
		displayVFB(vfb);
//	}
	waitForUserExit();
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
