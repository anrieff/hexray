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
//#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include "util.h"
#include "sdl.h"
#include "color.h"
#include "vector.h"
#include "camera.h"
#include "geometry.h"
#include "shading.h"
#include "matrix.h"
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

void printVec(const Vector& v)
{
	printf(" (%6.3f, %6.3f, %6.3f) ", v.x, v.y, v.z);
}

int main(void)
{
	const double N = 10;
	Vector a(0.9372438, -0.2811731, 0.2061936);
	Vector b = a;
	Matrix m = rotationAroundY(toRadians(360.0 / N));
	printf("Iter b                        dot(a, b) a^b                       len(a^b) norm(a^b)                dot(a, nc) dot(b, nc)\n");
	for (int i = 1; i <= int(N); i++) {
		b *= m; // rotate by 1/N turn around Y
		Vector c = a^b;
		Vector nc = c;
		nc.normalize();
		printf("%4d", i);
		printVec(b);
		printf("%8.5f", dot(a, b));
		printVec(c);
		printf("  %.5f", c.length());
		printVec(nc);
		printf(" %9.6f  %9.6f\n", dot(a, nc), dot(b, nc));
	}
	return 0;
}
