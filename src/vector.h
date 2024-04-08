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
 * @File vector.h
 * @Brief defines the Vector class (a 3D vector with the usual algebraic operations)
 */
#pragma once

#include <math.h>

struct Vector {
	union {
		struct { double x, y, z; };
		double v[3];
	};

	Vector () {}
	Vector(double _x, double _y, double _z) { set(_x, _y, _z); }
	void set(double _x, double _y, double _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}
	void makeZero(void)
	{
		x = y = z = 0.0;
	}
	inline double length(void) const
	{
		return sqrt(x * x + y * y + z * z);
	}
	inline double lengthSqr(void) const
	{
		return (x * x + y * y + z * z);
	}
	void scale(double multiplier)
	{
		x *= multiplier;
		y *= multiplier;
		z *= multiplier;
	}
	void operator *= (double multiplier)
	{
		scale(multiplier);
	}
	Vector& operator += (const Vector& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
	void operator /= (double divider)
	{
		scale(1.0 / divider);
	}
	void normalize(void)
	{
		double multiplier = 1.0 / length();
		scale(multiplier);
	}
	void setLength(double newLength)
	{
		scale(newLength / length());
	}
	int maxDimension() const
	{
		double maxVal = fabs(x);
		int maxDim = 0;
		if (fabs(y) > maxVal) {
			maxDim = 1;
			maxVal = fabs(y);
		}
		if (fabs(z) > maxVal) {
			maxDim = 2;
		}
		return maxDim;
	}
	inline double& operator[](const int index) { return v[index]; }
	inline const double& operator[](const int index) const { return v[index]; }
};

inline Vector operator + (const Vector& a, const Vector& b)
{
	return Vector(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vector operator - (const Vector& a, const Vector& b)
{
	return Vector(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vector operator - (const Vector& a)
{
	return Vector(-a.x, -a.y, -a.z);
}

/// dot product
inline double operator * (const Vector& a, const Vector& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
/// dot product (functional form, to make it more explicit):
inline double dot(const Vector& a, const Vector& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
/// cross product
inline Vector operator ^ (const Vector& a, const Vector& b)
{
	return Vector(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

inline Vector operator * (const Vector& a, double multiplier)
{
	return Vector(a.x * multiplier, a.y * multiplier, a.z * multiplier);
}
inline Vector operator * (double multiplier, const Vector& a)
{
	return Vector(a.x * multiplier, a.y * multiplier, a.z * multiplier);
}
inline Vector operator / (const Vector& a, double divider)
{
	double multiplier = 1.0 / divider;
	return Vector(a.x * multiplier, a.y * multiplier, a.z * multiplier);
}
/// distance between two points
inline double distance(const Vector& a, const Vector& b)
{
	return sqrt((a.x - b.x)*(a.x - b.x)
	          + (a.y - b.y)*(a.y - b.y)
	          + (a.z - b.z)*(a.z - b.z)
	);
}

inline Vector normalize(const Vector& v)
{
	double len = v.length();
	if (fabs(len - 1.0) < 1e-6) return v;
	return v * (1 / len);
}

struct Ray {
	Vector start;
	Vector dir; // ! unit vector!
	int depth = 0;
};

inline Vector faceforward(const Vector& ray, const Vector& n)
{
	return (dot(ray, n) < 0) ? n : -n;
}

inline Vector reflect(const Vector& i, const Vector& n)
{
	return 2 * dot(-i, n) * n + i;
}

// finds an orthonormed basis around v (which must be an unit vector)
// The returned pair of vectors:
//   dot(result.first, v) == 0
//   dot(result.second, v) == 0
//   dot(result.first, result.second) == 0
inline std::pair<Vector, Vector> othonormedBasis(const Vector& v)
{
	std::pair<Vector, Vector> result;
	static const Vector TEST_VECTORS[2] = {
		Vector{0.577350269, 0.577350269, 0.577350269},
		Vector{0.267261242, -0.534522484, 0.801783726},
	};
	int i = 0;
	if (fabs(dot(v, TEST_VECTORS[0])) > 0.9) i = 1;
	result.first  = TEST_VECTORS[i] ^ v;
	result.first.normalize();
	result.second = result.first ^ v;
	return result;
}
