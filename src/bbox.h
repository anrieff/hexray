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
 * @File bbox.h
 * @Brief Contains the BBox (bounding box) class.
 */
#pragma once

#include <algorithm>
#include <string>
#include <math.h>
#include "vector.h"
#include "util.h"
using std::min;
using std::max;

/// A structure to represent a single triangle in the mesh
struct Triangle {
	int v[3]; //!< holds indices to the three vertices of the triangle (indexes in the `vertices' array in the Mesh)
	int n[3]; //!< holds indices to the three normals of the triangle (indexes in the `normals' array)
	int t[3]; //!< holds indices to the three texture coordinates of the triangle (indexes in the `uvs' array)
	Vector gnormal; //!< The geometric normal of the mesh (AB ^ AC, normalized)
	Vector AB, AC, ABcrossAC; //!< precomputed AB, AC and AB^AC
	Vector dNdx, dNdy;
};
// the C vertex of triangle with index 5 is:
// mesh.vertices[mesh.triangles[5].v[2]];

enum Axis {
	AXIS_X,
	AXIS_Y,
	AXIS_Z,

	AXIS_NONE,
};

// from mesh.cpp
bool intersectTriangleFast(const Ray& ray, const Vector& A, const Vector& B, const Vector& C, double& dist);

/**
 * @Brief a class that represents an axis-aligned bounding box around some object
 * The more precise definition of our BBox is the volume, bounded by the vectors vmin, vmax, so that any point p inside the volume satisfies
 * (vmin.x <= p.x <= vmax.x) && (vmin.y <= p.y <= vmax.y) && (vmin.z <= p.z <= vmax.z)
 */
struct BBox {
	Vector vmin, vmax;
	BBox() {}
	/// makes the box empty (so it has no volume)
	inline void makeEmpty() {
		vmin.set(+INF, +INF, +INF);
		vmax.set(-INF, -INF, -INF);
	}
	/// add a point to the bounding box, possibly expanding it. If the point is inside the current box, nothing happens.
	/// if it is outside, the box grows just enough so that in encompasses the new point.
	inline void add(const Vector& vec)
	{
		vmin.x = min(vmin.x, vec.x); vmax.x = max(vmax.x, vec.x);
		vmin.y = min(vmin.y, vec.y); vmax.y = max(vmax.y, vec.y);
		vmin.z = min(vmin.z, vec.z); vmax.z = max(vmax.z, vec.z);
	}
	/// Checks if a point is inside the bounding box (borders-inclusive)
	inline bool inside(const Vector& v) const
	{
		return (vmin.x - 1e-6 <= v.x && v.x <= vmax.x + 1e-6 &&
		        vmin.y - 1e-6 <= v.y && v.y <= vmax.y + 1e-6 &&
		        vmin.z - 1e-6 <= v.z && v.z <= vmax.z + 1e-6);
	}
	/// Extend with another bounding box
	inline void extend(const BBox& other)
	{
		add(other.vmin);
		add(other.vmax);
	}
	/// Test for ray-box intersection
	/// @returns true if an intersection exists; false otherwise.
	inline bool testIntersect(const Ray& ray) const
	{
		if (inside(ray.start)) return true;
		for (int dim = 0; dim < 3; dim++) {
			if ((ray.dir[dim] < 0 && ray.start[dim] < vmin[dim]) || (ray.dir[dim] > 0 && ray.start[dim] > vmax[dim])) return false;
			if (fabs(ray.dir[dim]) < 1e-9) continue;
			double mul = 1 / ray.dir[dim];
			int u = (dim == 0) ? 1 : 0;
			int v = (dim == 2) ? 1 : 2;
			double dist, x, y;
			dist = (vmin[dim] - ray.start[dim]) * mul;
			if (dist < 0) continue; //*
			/* (*) this is a good optimization I found out by chance. Consider the following scenario
			 *
			 *   ---+  ^  (ray)
			 *      |   \
			 * bbox |    \
			 *      |     \
			 *      |      * (camera)
			 * -----+
			 *
			 * if we're considering the walls up and down of the bbox (which belong to the same axis),
			 * the optimization in (*) says that we can skip testing with the "up" wall, if the "down"
			 * wall is behind us. The rationale for that is, that we can never intersect the "down" wall,
			 * and even if we have the chance to intersect the "up" wall, we'd be intersection the "right"
			 * wall first. So we can just skip any further intersection tests for this axis.
			 * This may seem bogus at first, as it doesn't work if the camera is inside the BBox, but then we would
			 * have quitted the function because of the inside(ray.start) condition in the first line of the function.
			 */
			x = ray.start[u] + ray.dir[u] * dist;
			if (vmin[u] <= x && x <= vmax[u]) {
				y = ray.start[v] + ray.dir[v] * dist;
				if (vmin[v] <= y && y <= vmax[v]) {
					return true;
				}
			}
			dist = (vmax[dim] - ray.start[dim]) * mul;
			if (dist < 0) continue;
			x = ray.start[u] + ray.dir[u] * dist;
			if (vmin[u] <= x && x <= vmax[u]) {
				y = ray.start[v] + ray.dir[v] * dist;
				if (vmin[v] <= y && y <= vmax[v]) {
					return true;
				}
			}
		}
		return false;
	}
	/// returns the distance to the closest intersection of the ray and the BBox, or +INF if such an intersection doesn't exist.
	/// please note that this is heavier than using just testIntersect() - testIntersect needs only to consider *ANY* intersection,
	/// whereas closestIntersection() also needs to find the nearest one.
	inline double closestIntersectionPeriphery(const Ray& ray) const
	{
		double minDist = INF;
		for (int dim = 0; dim < 3; dim++) {
			if ((ray.dir[dim] < 0 && ray.start[dim] < vmin[dim]) || (ray.dir[dim] > 0 && ray.start[dim] > vmax[dim])) return INF;
			if (fabs(ray.dir[dim]) < 1e-9) continue;
			double mul = 1 / ray.dir[dim];
			double xs[2] = { vmin[dim], vmax[dim] };
			int u = (dim == 0) ? 1 : 0;
			int v = (dim == 2) ? 1 : 2;
			for (int j = 0; j < 2; j++) {
				double dist = (xs[j] - ray.start[dim]) * mul;
				if (dist < 0) continue;
				double x = ray.start[u] + ray.dir[u] * dist;
				if (vmin[u] <= x && x <= vmax[u]) {
					double y = ray.start[v] + ray.dir[v] * dist;
					if (vmin[v] <= y && y <= vmax[v]) {
						minDist = min(minDist, dist);
					}
				}
			}
		}
		return minDist;
	}

	inline double closestIntersection(const Ray& ray) const
	{
		if (inside(ray.start)) return 0;
		return closestIntersectionPeriphery(ray);
	}

	/// Check whether the box intersects a triangle (all three cases)
	inline bool intersectTriangle(const Vector& A, const Vector& B, const Vector& C) const
	{
		if (inside(A) || inside(B) || inside(C)) return true;
		Ray ray;
		Vector t[3] = { A, B, C };
		for (int i = 0; i < 3; i++) for (int j = i + 1; j < 3; j++) {
			ray.start = t[i];
			ray.dir = t[j] - t[i];
			if (testIntersect(ray)) {
				ray.start = t[j];
				ray.dir = t[i] - t[j];
				if (testIntersect(ray)) return true;
			}
		}
		Vector AB = B - A;
		Vector AC = C - A;
		Vector ABcrossAC = AB ^ AC;
		double D = A * ABcrossAC;
		for (int mask = 0; mask < 7; mask++) {
			for (int j = 0; j < 3; j++) {
				if (mask & (1 << j)) continue;
				ray.start.set((mask & 1) ? vmax.x : vmin.x, (mask & 2) ? vmax.y : vmin.y, (mask & 4) ? vmax.z : vmin.z);
				Vector rayEnd = ray.start;
				rayEnd[j] = vmax[j];
				if (signOf(ray.start * ABcrossAC - D) != signOf(rayEnd * ABcrossAC - D)) {
					ray.dir = rayEnd - ray.start;
					double gamma = 1.0000001;
					if (intersectTriangleFast(ray, A, B, C, gamma)) return true;
				}
			}
		}
		return false;
	}
	/// Split a bounding box along an given axis at a given position, yielding a two child bboxen
	/// @param axis - an axis to use for splitting (AXIS_X, AXIS_Y or AXIS_Z)
	/// @param where - where to put the splitting plane (must be between vmin[axis] and vmax[axis])
	/// @param left - output - this is where the left bbox is stored (lower coordinates)
	/// @param right - output - this is where the right bbox is stored.
	inline void split(Axis axis, double where, BBox& left, BBox& right) const
	{
		left = *this;
		right = *this;
		left.vmax[axis] = where;
		right.vmin[axis] = where;
	}
	/// Checks if a ray intersects a single wall inside the BBox
	/// Consider the intersection of the splitting plane as described in split(), and the BBox
	/// (i.e., the "split wall"). We want to check if the ray intersects that wall.
	inline bool intersectWall(Axis axis, double where, const Ray& ray) const
	{
	   if (fabs(ray.dir[axis]) < 1e-9) return (fabs(ray.start[axis] - where) < 1e-9);
	   int u = (axis == 0) ? 1 : 0;
	   int v = (axis == 2) ? 1 : 2;
	   double toGo = where - ray.start[axis];
	   double rdirInAxis = 1 / ray.dir[axis];
	   // check if toGo and dirInAxis are of opposing signs:
	   if ((toGo * rdirInAxis) < 0) return false;
	   double d = toGo * rdirInAxis;
	   double tu = ray.start[u] + ray.dir[u] * d;
	   if (vmin[u] <= tu && tu <= vmax[u]) {
		   double tv = ray.start[v] + ray.dir[v] * d;
		   return (vmin[v] <= tv && tv <= vmax[v]);
	   }
	   return false;
	}
};
