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
	virtual int getNumSamples() const = 0;
	virtual void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) = 0;
	// from SceneElement
	virtual ElementType getElementType() const override { return ELEM_LIGHT; }
	/**
	 * intersects a ray with the light. The param intersectionDist is in/out;
	 * it's behaviour is similar to Intersectable::intersect()'s treatment of distances.
	 * @retval true, if the ray intersects the light, and the intersection distance is smaller
	 *               than the current value of intersectionDist (which is updated upon return)
	 * @retval false, otherwise.
	 */
	virtual bool intersect(const Ray& ray, double& intersectionDist) = 0;

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
	bool intersect(const Ray& ray, double& intersectionDist) override;
};

class RectLight: public Light {
	Transform T;
	int xSubd = 3, ySubd = 3;
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
	bool intersect(const Ray& ray, double& intersectionDist) override;
};

