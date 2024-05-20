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
 * @File util.h
 * @Brief a few useful short functions
 */
#pragma once

#include <stdlib.h>
#include <math.h>
#include <string>
#include <vector>
#include "vector.h"
#include "constants.h"

#define COUNT_OF(arr) int(sizeof(arr) / sizeof(arr[0]))

inline double signOf(double x) { return x > 0 ? +1 : -1; }
inline double sqr(double a) { return a * a; }
inline double toRadians(double angle) { return angle / 180.0 * PI; }
inline double toDegrees(double angle_rad) { return angle_rad / PI * 180.0; }
inline int nearestInt(float x) { return (int) floor(x + 0.5f); }

std::string extensionUpper(const char* fileName); //!< Given a filename, return its extension in UPPERCASE
std::vector<std::string> tokenize(std::string s);
std::vector<std::string> split(std::string s, char separator);

/// returns a random integer in [a..b]
int randInt(int a, int b);
/// returns a random floating-point number in [0..1).
float randFloat();
double randDouble();

/// returns a random point (x, y) inside the unit disk (sqrt(x*x + y*y) <= 1.0)
void unitDiskSample(double& x, double& y);

/// returns a random 3D point on the unit sphere, and restricts it to the hemisphere pointed by `normal'.
/// The returned points are evenly distributed on the (hemi)sphere surface (see "sphere point picking" problem).
/// If we have "result = hemisphereSample(normal);", is guaranteed that dot(result, normal) >= 0.
Vector hemisphereSample(const Vector& normal);

/// a simple RAII class for FILE* pointers.
class FileRAII {
	FILE* held;
public:
	FileRAII(FILE* init): held(init) {}
	~FileRAII() { if (held) fclose(held); held = NULL; }
	FileRAII(const FileRAII&) = delete;
	FileRAII& operator = (const FileRAII&) = delete;
};
