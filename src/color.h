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
 * @File color.h
 * @Brief Defines the Color class, which we'll use to represent light.
 */
#pragma once

#include "util.h"

inline unsigned convertTo8bit(float x)
{
	if (x < 0) x = 0;
	if (x > 1) x = 1;
	return nearestInt(x * 255.0f);
}

inline unsigned convertTo8bit_sRGB(float x)
{
	const float a = 0.055f;
	if (x <= 0) return 0;
	if (x >= 1) return 255;
	// sRGB transform:
	if (x <= 0.0031308f)
		x = x * 12.02f;
	else
		x = (1.0f + a) * powf(x, 1.0f / 2.4f) - a;
	return nearestInt(x * 255.0f);
}

unsigned convertTo8bit_sRGB_cached(float x);

/// Represents a color, using floatingpoint components in [0..1]
struct Color {
	// a union, that allows us to refer to the channels by name (::r, ::g, ::b),
	// or by index (::components[0] ...). See operator [].
	union {
		   struct { float r, g, b; };
		   float components[3];
	};
	//
	Color() {}
	Color(float _r, float _g, float _b) //!< Construct a color from floatingpoint values
	{
		setColor(_r, _g, _b);
	}
	explicit Color(unsigned rgbcolor) //!< Construct a color from R8G8B8 value like "0xffce08"
	{
		b = (rgbcolor & 0xff) / 255.0f;
		g = ((rgbcolor >> 8) & 0xff) / 255.0f;
		r = ((rgbcolor >> 16) & 0xff) / 255.0f;
	}
	static void init_sRGB_cache(); // must be called at program startup, otherwise toRBG32() won't work
	/// convert to RGB32, with channel shift specifications. The default values are for
	/// the blue channel occupying the least-significant byte
	unsigned toRGB32(int redShift = 16, int greenShift = 8, int blueShift = 0) const
	{
		unsigned ir = convertTo8bit_sRGB_cached(r);
		unsigned ig = convertTo8bit_sRGB_cached(g);
		unsigned ib = convertTo8bit_sRGB_cached(b);
		return (ib << blueShift) | (ig << greenShift) | (ir << redShift);
	}
	/// make black
	inline void makeZero(void)
	{
		r = g = b = 0;
	}
	/// check if it's black
	inline bool isZero() const
	{
		return (r == 0 && g == 0 && b == 0);
	}
	/// set the color explicitly
	void setColor(float _r, float _g, float _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}
	/// get the intensity of the color (direct)
	float intensity(void)
	{
		return (r + g + b) / 3;
	}
	/// get the perceptual intensity of the color
	float intensityPerceptual(void)
	{
		return (r * 0.299 + g * 0.587 + b * 0.114);
	}
	/// Accumulates some color to the current
	void operator += (const Color& rhs)
	{
		r += rhs.r;
		g += rhs.g;
		b += rhs.b;
	}
	/// multiplies the color
	void operator *= (float multiplier)
	{
		r *= multiplier;
		g *= multiplier;
		b *= multiplier;
	}
	/// divides the color
	void operator /= (float divider)
	{
		r /= divider;
		g /= divider;
		b /= divider;
	}
	/// fetch r, g or b (depending on index)
	inline const float& operator[] (int index) const
	{
		return components[index];
	}
	/// fetch r, g or b (depending on index)
	inline float& operator[] (int index)
	{
		return components[index];
	}
};

/// adds two colors
inline Color operator + (const Color& a, const Color& b)
{
	return Color(a.r + b.r, a.g + b.g, a.b + b.b);
}

/// subtracts two colors
inline Color operator - (const Color& a, const Color& b)
{
	return Color(a.r - b.r, a.g - b.g, a.b - b.b);
}

/// multiplies two colors
inline Color operator * (const Color& a, const Color& b)
{
	return Color(a.r * b.r, a.g * b.g, a.b * b.b);
}

/// multiplies a color by some multiplier
inline Color operator * (const Color& a, float multiplier)
{
	return Color(a.r * multiplier, a.g * multiplier, a.b * multiplier);
}

/// multiplies a color by some multiplier
inline Color operator * (float multiplier, const Color& a)
{
	return Color(a.r * multiplier, a.g * multiplier, a.b * multiplier);
}

/// divides some color
inline Color operator / (const Color& a, float divider)
{
	return Color(a.r / divider, a.g / divider, a.b / divider);
}
