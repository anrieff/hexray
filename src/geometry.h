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
 * @File geometry.h
 * @Brief Contains declarations of geometry primitives.
 */
#pragma once

#include "vector.h"
#include <functional>
#include "scene.h"

class Geometry;

struct IntersectionInfo {
    double dist;
    Vector ip;
    Vector norm;
    Vector dNdx, dNdy;
    double u, v;
    Geometry *geom;
};

/**
 * @class Intersectable
 * @brief implements the interface to an intersectable primitive (geometry or node)
 */
class Intersectable {
public:
	virtual bool intersect(Ray ray, IntersectionInfo& info) = 0;
};

class Geometry: public Intersectable, public SceneElement {
public:
    //
   	virtual ElementType getElementType() const override { return ELEM_GEOMETRY; }
};

class Plane: public Geometry {
public:
    double y = 0;
    double limit = 100;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("y", &y);
		pb.getDoubleProp("limit", &limit);
	}
    virtual bool intersect(Ray ray, IntersectionInfo& info) override;
};

class Sphere: public Geometry {
public:
    Vector O = Vector(0, 0, 0);
    double R = 1;
    double uvscaling = 1;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getVectorProp("O", &O);
		pb.getDoubleProp("R", &R, 0.0);
        pb.getDoubleProp("uvscaling", &uvscaling, 1e-6);
	}
    virtual bool intersect(Ray ray, IntersectionInfo& info) override;
};

class Cube: public Geometry {
    double m_halfSide;
    int intersectCubeSide(
        const Vector& norm,
        double startCoord,
        double dir,
        double target,
        const Ray& ray,
        IntersectionInfo& info,
        std::function<void(IntersectionInfo&)> genUV
        );
public:
    Vector O  = Vector(0, 0, 0);
    double side = 1;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getVectorProp("O", &O);
		pb.getDoubleProp("side", &side, 0.0);
	}
    void beginFrame() override
    {
        m_halfSide = side * 0.5;
    }
    virtual bool intersect(Ray ray, IntersectionInfo& info) override;
};

class CSGBase: public Geometry {
    Geometry* left, *right;
public:
    virtual bool intersect(Ray ray, IntersectionInfo& info) override;
    virtual bool inside(bool inA, bool inB) = 0;
	void fillProperties(ParsedBlock& pb)
	{
		pb.requiredProp("left");
		pb.requiredProp("right");
		pb.getGeometryProp("left", &left);
		pb.getGeometryProp("right", &right);
	}
};

class CSGUnion: public CSGBase {
public:
    virtual bool inside(bool inA, bool inB) override { return inA || inB; }
};

class CSGInter: public CSGBase {
public:
    virtual bool inside(bool inA, bool inB) override { return inA && inB; }
};

class CSGDiff: public CSGBase { // A - B
public:
    virtual bool inside(bool inA, bool inB) override { return inA && !inB; }
};