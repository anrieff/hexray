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
 * @File mesh.cpp
 * @Brief Contains implementation of the Mesh class
 */

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <numeric>
#include <SDL.h>
#include "mesh.h"
#include "constants.h"
#include "color.h"
using std::max;
using std::vector;
using std::string;


void Mesh::beginRender()
{
	computeBoundingGeometry();
	printf("Mesh loaded, %d triangles\n", int(triangles.size()));
}

void Mesh::computeBoundingGeometry()
{
	boundingSphere.O = Vector(0, 0, 0);
	boundingSphere.R = 0;
	for (Vector& v: vertices) boundingSphere.R = std::max(boundingSphere.R, v.length());
}

inline double det(const Vector& a, const Vector& b, const Vector& c)
{
	return (a^b) * c;
}

bool Mesh::intersectTriangle(const Ray& ray, const Triangle& t, IntersectionInfo& info)
{
	if (backfaceCulling && dot(ray.dir, t.gnormal) > 0) return false;
	const Vector& A = vertices[t.v[0]];
	const Vector& B = vertices[t.v[1]];
	const Vector& C = vertices[t.v[2]];
	const Vector& AB = t.AB;
	const Vector& AC = t.AC;
	Vector H = ray.start - A;
	double Dcr = det(AB, AC, -ray.dir);
	if (fabs(Dcr) < 1e-12) return false;
	double rDcr = 1/Dcr;
	double lambda2 = det(H, AC, -ray.dir) * rDcr;
	if (lambda2 < 0 || lambda2 > 1) return false;
	double lambda3 = det(AB, H, -ray.dir) * rDcr;
	if (lambda3 < 0 || lambda3 > 1) return false;
	double lambda1 = 1 - (lambda2 + lambda3);
	if (lambda1 < 0 || lambda1 > 1) return false;
	double gamma = det(AB, AC, H) * rDcr;
	if (gamma < 0) return false;
	//
	info.dist = gamma;
	info.ip = ray.start + gamma * ray.dir;
	// compute texture coords:
	const Vector& texA = uvs[t.t[0]];
	const Vector& texB = uvs[t.t[1]];
	const Vector& texC = uvs[t.t[2]];
	Vector texCoords = texA + (texB - texA) * lambda2 + (texC - texA) * lambda3;
	info.u = texCoords[0];
	info.v = texCoords[1];
	// compute normals:
	if (faceted) {
		info.norm = t.gnormal;
	} else {
		const Vector& nA = normals[t.n[0]];
		const Vector& nB = normals[t.n[1]];
		const Vector& nC = normals[t.n[2]];
		info.norm = nA + (nB - nA) * lambda2 + (nC - nA) * lambda3;
		info.norm.normalize();
	}
	info.dNdx = t.dNdx;
	info.dNdy = t.dNdy;
	return true;
}


bool Mesh::intersect(Ray ray, IntersectionInfo& info)
{
	if (!boundingSphere.intersect(ray, info)) return false;
	info.dist = INF;
	for (Triangle& T: triangles) {
		IntersectionInfo tempInfo;
		if (intersectTriangle(ray, T, tempInfo) && tempInfo.dist < info.dist) {
			info = tempInfo;
		}
	}
	if (info.dist < INF) {
		info.geom = this;
		return true;
	}
	return false;
}

static int toInt(const string& s)
{
	if (s.empty()) return 0;
	int x;
	if (1 == sscanf(s.c_str(), "%d", &x)) return x;
	return 0;
}

static double toDouble(const string& s)
{
	if (s.empty()) return 0;
	double x;
	if (1 == sscanf(s.c_str(), "%lf", &x)) return x;
	return 0;
}

static void parseTrio(string s, int& vertex, int& uv, int& normal)
{
	vector<string> items = split(s, '/');
	// "4" -> {"4"} , "4//5" -> {"4", "", "5" }

	vertex = toInt(items[0]);
	uv = items.size() >= 2 ? toInt(items[1]) : 0;
	normal = items.size() >= 3 ? toInt(items[2]) : 0;
}

static Triangle parseTriangle(string s0, string s1, string s2)
{
	// "3", "3/4", "3//5", "3/4/5"  (v/uv/normal)
	Triangle T;
	parseTrio(s0, T.v[0], T.t[0], T.n[0]);
	parseTrio(s1, T.v[1], T.t[1], T.n[1]);
	parseTrio(s2, T.v[2], T.t[2], T.n[2]);
	return T;
}

bool Mesh::loadFromOBJ(const char* filename)
{
	FILE* f = fopen(filename, "rt");

	if (!f) return false;

	vertices.resize(1);
	normals.resize(1);
	uvs.resize(1);

	char line[10000];
	while (fgets(line, sizeof(line), f)) {
		if (line[0] == '#') continue;
		//
		std::vector<std::string> tokens = tokenize(line);
		if (tokens.empty()) continue;
		//
		if (tokens[0] == "v") {
			vertices.push_back(Vector(
				toDouble(tokens[1]),
				toDouble(tokens[2]),
				toDouble(tokens[3])
			));
			continue;
		}
		//
		if (tokens[0] == "vn") {
			normals.push_back(Vector(
				toDouble(tokens[1]),
				toDouble(tokens[2]),
				toDouble(tokens[3])
			));
			continue;
		}
		//
		if (tokens[0] == "vt") {
			uvs.push_back(Vector(
				toDouble(tokens[1]),
				toDouble(tokens[2]),
				0
			));
			continue;
		}
		//
		if (tokens[0] == "f") {
			for (int i = 0; i < int(tokens.size() - 3); i++)
				triangles.push_back(
					parseTriangle(tokens[1], tokens[2 + i], tokens[3 + i])
				);
			continue;
		}
	}

	fclose(f);
	prepareTriangles();
	return true;
}

static void solve2D(Vector A, Vector B, Vector C, double& x, double& y)
{
	// solve: x * A + y * B = C
	double mat[2][2] = { { A.x, B.x }, { A.y, B.y }};
	double h[2] = { C.x, C.y };
	double Dcr = mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1];
	x          = h[0]      * mat[1][1] - h[1]      * mat[0][1];
	y          = mat[0][0] * h[1]      - mat[1][0] * h[0];
}

void Mesh::prepareTriangles()
{
	if (normals.size() <= 1) faceted = true;
	for (auto& t: triangles) {
		const Vector& A = vertices[t.v[0]];
		const Vector& B = vertices[t.v[1]];
		const Vector& C = vertices[t.v[2]];
		t.AB = B - A;
		t.AC = C - A;
		t.gnormal = t.AB ^ t.AC;
		t.gnormal.normalize();

		const Vector& tA = uvs[t.t[0]];
		const Vector& tB = uvs[t.t[1]];
		const Vector& tC = uvs[t.t[2]];
		Vector texAB = tB - tA;
		Vector texAC = tC - tA;

		double px, py, qx, qy;
		solve2D(texAB, texAC, Vector(1, 0, 0), px, qx);
		solve2D(texAB, texAC, Vector(0, 1, 0), py, qy);
		t.dNdx = px * t.AB + qx * t.AC;
		t.dNdy = py * t.AB + qy * t.AC;
		t.dNdx.normalize();
		t.dNdy.normalize();
	}
}
