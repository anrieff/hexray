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
 * @File lights.h
 * @Brief Implements the various models of light sources
 */
#pragma once

#include "scene.h"
#include "matrix.h"
#include "color.h"

class Light: public SceneElement {
protected:
	Color color = Color(1, 1, 1);
	float power = 1.0f;
public:
	Color getColor() const { return color * power; }
	//
	/// gets how many samples are needed to properly evaluate this light using Monte Carlo sampling
	virtual int getNumSamples() const = 0;
	/// gets the n-th sample (0 <= sampleIdx < getNumSamples()):
	/// @param shadePos - input position to be shaded, in world space
	/// @param samplePos [out] - generated sample on the light, in world space
	/// @param color [out] - the light color/brightness of the sample. Depends on shadePos for things
	///                      like falloff/obliquity
	virtual void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) = 0;

	/**
	 * intersects a ray with the light. The param intersectionDist is in/out;
	 * it's behaviour is similar to Intersectable::intersect()'s treatment of distances.
	 * @retval +1/-1, if the ray intersects the light, and the intersection distance is smaller
	 *                than the current value of intersectionDist (which is updated upon return).
	 *                The sign indicates which side we hit: +1 for the front side, -1 for the back side
	 *                (-1 is the non-emitting side).
	 * @retval 0, otherwise.
	 */
	virtual int intersect(const Ray& ray, double& intersectionDist) = 0;

	/// gets an estimate for the solid angle that this light projects onto a hemisphere above the point `p'
	/// @param p - the point being considered, in world space
	/// @returns a solid angle: [0..2*PI)
	virtual double getSolidAngle(const Vector& p) = 0;

	/// gets the scaling factor which you should apply to getColor() to get the light emission per unit area
	/// I.e. getColor() returns the total brightness of the lamp
	///      getColor() * getScaleFactor() returns the brightness of the lamp per unit area
	virtual float getScaleFactor() const { return 1.0f; }

	// from SceneElement
	virtual ElementType getElementType() const override { return ELEM_LIGHT; }
	void fillProperties(ParsedBlock& pb) override
	{
		pb.getColorProp("color", &color);
		pb.getFloatProp("power", &power);
	}
	friend class Lambert;
	friend class Phong;
};

class PointLight: public Light {
	Vector pos;
public:
	void fillProperties(ParsedBlock& pb) override
	{
		Light::fillProperties(pb);
		pb.getVectorProp("pos", &pos);
	}

	int getNumSamples() const override { return 1; }
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) override
	{
		samplePos = this->pos;
		color = this->color;
	}
	int intersect(const Ray& ray, double& intersectionDist) override;
	double getSolidAngle(const Vector& p) override { return 0; }
};

class RectLight: public Light {
	Transform T;
	int xSubd = 3, ySubd = 3;
	float scaleFactor;
	double m_area;
public:
	void fillProperties(ParsedBlock& pb) override
	{
		Light::fillProperties(pb);
		pb.getTransformProp(T);
		pb.getIntProp("xSubd", &xSubd, 1, 1000);
		pb.getIntProp("ySubd", &ySubd, 1, 1000);
	}

	int getNumSamples() const override;
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) override;
	int intersect(const Ray& ray, double& intersectionDist) override;
	void beginFrame() override;
	float getScaleFactor() const override { return scaleFactor; }
	double getSolidAngle(const Vector& p) override;
};
