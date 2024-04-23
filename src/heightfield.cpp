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
 * @File heightfield.cpp
 * @Brief Implementation of the Heightfield.
 */
#include "heightfield.h"
#include "bitmap.h"
#include <SDL.h>


float Heightfield::getHeight(int x, int y) const
{
	x = min(W - 1, x);
	y = min(H - 1, y);
	x = max(0, x);
	y = max(0, y);
	return heights[y * W + x];
}

float Heightfield::getHighest(int x, int y, int k) const
{
	x = min(W - 1, x);
	y = min(H - 1, y);
	x = max(0, x);
	y = max(0, y);
	return highMap[y * W + x].h[k];
}


void Heightfield::buildHighMap()
{
	highMap.resize(W * H);
	maxK = int(ceil(log(W)/log(2.0)));
	//
	// handle k = 0
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			float& result = highMap[y * W + x].h[0];
			result = getHeight(x, y);
			for (int dy = -1; dy <= 1; dy++)
				for (int dx = -1; dx <= 1; dx++)
					result = max(result, getHeight(x + dx, y + dy));
		}
	//
	// XXXXX
	// X1X2X
	// XX.XX
	// X3X4X
	// XXXXX
	for (int k = 1; k < maxK; k++) {
		int offset = (1 << (k - 1));
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				float& result = highMap[y * W + x].h[k];
				result = getHighest(x - offset, y - offset, k - 1);
				result = max(result, getHighest(x + offset, y - offset, k - 1));
				result = max(result, getHighest(x - offset, y + offset, k - 1));
				result = max(result, getHighest(x + offset, y + offset, k - 1));
			}
		}
	}
}

Vector Heightfield::getNormal(float x, float y) const
{
	// we have precalculated the normals at each integer position.
	// Here, we do bilinear filtering on the four nearest integral positions:
	int x0 = (int) floor(x);
	int y0 = (int) floor(y);
	float p = (x - x0);
	float q = (y - y0);
	int x1 = min(W - 1, x0 + 1);
	int y1 = min(H - 1, y0 + 1);
	x0 = min(W - 1, x0);
	y0 = min(H - 1, y0);
	x0 = max(0, x0);
	y0 = max(0, y0);
	Vector v =
		normals[y0 * W + x0] * ((1 - p) * (1 - q)) +
		normals[y0 * W + x1] * ((    p) * (1 - q)) +
		normals[y1 * W + x0] * ((1 - p) * (    q)) +
		normals[y1 * W + x1] * ((    p) * (    q));
	v.normalize();
	return v;
}

bool Heightfield::intersect(Ray ray, IntersectionInfo& info)
{
	Vector step = ray.dir;
	double distHoriz = sqrt(sqr(step.x) + sqr(step.z));
	step /= distHoriz;
	double dist = bbox.closestIntersection(ray);
	Vector p = ray.start + ray.dir * (dist + 1e-6); // step firmly inside the bbox

	double mx = 1.0 / ray.dir.x; // mx = how much to go along ray.dir until the unit distance along X is traversed
	double mz = 1.0 / ray.dir.z; // same as mx, for Z

	while (bbox.inside(p)) {
		int x0 = (int) floor(p.x);
		int z0 = (int) floor(p.z);
		if (x0 < 0 || x0 >= W || z0 < 0 || z0 >= H) break; // if outside the [0..W)x[0..H) rect, get out

		if (useOptimization) {
			int k = 1;
			while (k < maxK && p.y + step.y * (1 << k) > getHighest(x0, z0, k)) k++;
			k--;
			if (k > 0) {
				p += step * (1 << k);
				continue;
			}
		}

		// calculate how much we need to go along ray.dir until we hit the next X voxel boundary:
		double lx = ray.dir.x > 0 ? (ceil(p.x) - p.x) * mx : (floor(p.x) - p.x) * mx;
		// same as lx, for the Z direction:
		double lz = ray.dir.z > 0 ? (ceil(p.z) - p.z) * mz : (floor(p.z) - p.z) * mz;
		// advance p along ray.dir until we hit the next X or Z gridline
		// also, go a little more than that, to assure we're firmly inside the next voxel:
		Vector p_next = p + step * (min(lx, lz) + 1e-6);
		// "p" is position before advancement; p_next is after we take a single step.
		// if any of those are below the height of the nearest four voxels of the heightfield,
		// we need to test the current voxel for intersection:
		if (min(p.y, p_next.y) < maxH[z0 * W + x0]) {
			double closestDist = INF;
			// form ABCD - the four corners of the current voxel, whose heights are taken from the heightmap
			// then form triangles ABD and BCD and try to intersect the ray with each of them:
			Vector A = Vector(x0, getHeight(x0, z0), z0);
			Vector B = Vector(x0 + 1, getHeight(x0 + 1, z0), z0);
			Vector C = Vector(x0 + 1, getHeight(x0 + 1, z0 + 1), z0 + 1);
			Vector D = Vector(x0, getHeight(x0, z0 + 1), z0 + 1);
			bool b1 = intersectTriangleFast(ray, A, B, D, closestDist);
			bool b2 = intersectTriangleFast(ray, B, C, D, closestDist);

			if (b1 || b2) {
				// intersection found: ray hits either triangle ABD or BCD. Which one exactly isn't
				// important, because we calculate the normals by bilinear interpolation of the
				// precalculated normals at the four corners:
				info.dist = closestDist;
				info.ip = ray.start + ray.dir * closestDist;
				info.norm = getNormal((float) info.ip.x, (float) info.ip.z);
				info.u = info.ip.x / W;
				info.v = info.ip.z / H;
				info.dNdx = Vector(1, 0, 0);
				info.dNdy = Vector(0, 0, 1);
				info.geom = this;
				return true;
			}
		}
		p = p_next;
	}
	return false;
}

void Heightfield::fillProperties(ParsedBlock& pb)
{
	pb.getBoolProp("useOptimization", &useOptimization);
	Bitmap bmp;
	if (!pb.getBitmapFileProp("file", bmp)) pb.requiredProp("file");
	W = bmp.getWidth();
	H = bmp.getHeight();
	double blur = 0;
	pb.getDoubleProp("blur", &blur, 0, 1000);
	// do we have blur? if no, just fetch the source image and store it:
	heights.resize(W * H);
	float minY = LARGE_FLOAT, maxY = -LARGE_FLOAT;
	if (blur <= 0) {
		for (int y = 0; y < H; y++)
			for (int x = 0; x < W; x++) {
				float h = bmp.getPixel(x, y).intensity();
				heights[y * W + x] = h;
				minY = min(minY, h);
				maxY = max(maxY, h);
			}
	} else {
		// We have blur...
		// 1) convert image to greyscale (if not already):
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				float f = bmp.getPixel(x, y).intensity();
				bmp.setPixel(x, y, Color(f, f, f));
			}
		}
		// 2) calculate the gaussian coefficients, see http://en.wikipedia.org/wiki/Gaussian_blur
		static float gauss[128][128];
		int R = min(128, nearestInt(float(3 * blur)));
		for (int y = 0; y < R; y++)
			for (int x = 0; x < R; x++)
				gauss[y][x] = float(exp(-(sqr(x) + sqr(y))/(2 * sqr(blur))) / (2 * PI * sqr(blur)));
		// 3) apply gaussian blur with the specified number of blur units:
		// (this is potentially slow for large blur radii)
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				float sum = 0;
				for (int dy = -R + 1; dy < R; dy++)
					for (int dx = -R + 1; dx < R; dx++)
						sum += gauss[abs(dy)][abs(dx)] * bmp.getPixel(x + dx, y + dy).r;
				heights[y * W + x] = sum;
				minY = min(minY, sum);
				maxY = max(maxY, sum);
			}
		}
	}

	bbox.vmin = Vector(0, minY, 0);
	bbox.vmax = Vector(W, maxY, H);

	maxH.resize(W * H);
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			float& maxH = this->maxH[y * W + x];
			maxH = heights[y * W + x];
			if (x < W - 1) maxH = max(maxH, heights[y * W + x + 1]);
			if (y < H - 1) {
				maxH = max(maxH, heights[(y + 1) * W + x]);
				if (x < W - 1)
					maxH = max(maxH, heights[(y + 1) * W + x + 1]);
			}
		}

	normals.resize(W * H);
	for (int y = 0; y < H - 1; y++)
		for (int x = 0; x < W - 1; x++) {
			float h0 = heights[y * W + x];
			float hdx = heights[y * W + x + 1];
			float hdy = heights[(y + 1) * W + x];
			Vector vdx = Vector(1, hdx - h0, 0);
			Vector vdy = Vector(0, hdy - h0, 1);
			Vector norm = vdy ^ vdx;
			norm.normalize();
			normals[y * W + x] = norm;
		}
	// fill edges of the normals array:
	for (int y = 0; y < H; y++) normals[y * W + W - 1] = normals[y * W + W - 2];
	for (int x = 0; x < W; x++) normals[(H - 1) * W + x] = normals[(H - 2) * W + x];
	//
	if (useOptimization) {
		Uint32 startBuild = SDL_GetTicks();
		buildHighMap();
		Uint32 endBuild = SDL_GetTicks();
		printf("Heightfield optimization struct built in %.2fs\n", (endBuild - startBuild) / 1000.0);
	}
}

void Heightfield::beginRender()
{
	/*
	if (useOptimization) {
		Uint32 startBuild = SDL_GetTicks();
		buildHighMap();
		Uint32 endBuild = SDL_GetTicks();
		printf("Built %dx%d heightmap acceleration struct in %.2lfs.\n", W, H, (endBuild - startBuild) / 1000.0);
	}
	*/
}
