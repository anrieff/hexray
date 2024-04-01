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
 * @File environment.cpp
 * @Brief Implementations of the Environment classes (as if there're many of them ...)
 */
#include <string.h>
#include <filesystem>
#include "environment.h"
#include "vector.h"
#include "bitmap.h"
#include "util.h"

CubemapEnvironment::CubemapEnvironment(const char* folder)
{
	loadMaps(folder);
}

bool CubemapEnvironment::loadMaps(const char* folder)
{
	// the maps are stored in order - negx, negy, negz, posx, posy, posz
	const char* prefixes[2] = {"neg", "pos"};
	const char* axes[3] = {"x", "y", "z"};
	const char* suffixes[2] = {".bmp", ".exr"};
	int n = 0;
	for (int pi = 0; pi < 2; pi++)
		for (int axis = 0; axis < 3; axis++) {
			char fn[256];
			for (int si = 0; si < 2; si++) {
				sprintf(fn, "%s/%s%s%s", folder, prefixes[pi], axes[axis], suffixes[si]);
				if (std::filesystem::exists(fn) && m_sides[n].loadImage(fn)) break;
			}
			if (!m_sides[n++].isOK()) return false;
		}
	loaded = true;
	return true;
}

Color CubemapEnvironment::getSide(const Bitmap& bmp, double x, double y)
{
	// X: [-1, 1] -> [0, width]
	// Y: [-1, 1] -> [0, height]

	int ix = ((x + 1) / 2) * bmp.getWidth();
	int iy = ((y + 1) / 2) * bmp.getHeight();

	return bmp.getPixel(ix, iy);
}

Color CubemapEnvironment::getEnvironment(const Vector& dir)
{
	if (!loaded) return Color(0, 0, 0);
	int dim = dir.maxDimension();
	Vector onSide = dir;
	bool positive = dir[dim] > 0; // dir.x == dir[0]; dir.y == dir[1]
	onSide /= fabs(dir[dim]);

	int caseNum = (positive ? 3 : 0) + dim;
	Bitmap& side = m_sides[caseNum];
	switch (caseNum) {
		case NEGX: return getSide(side,  onSide.z, -onSide.y);
		case NEGY: return getSide(side,  onSide.x, -onSide.z);
		case NEGZ: return getSide(side,  onSide.x,  onSide.y);
		case POSX: return getSide(side, -onSide.z, -onSide.y);
		case POSY: return getSide(side,  onSide.x,  onSide.z);
		case POSZ: return getSide(side,  onSide.x, -onSide.y);
	};
	return Color(0, 0, 0);
}
