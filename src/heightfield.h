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
 * @File heightfield.h
 * @Brief Contains the Heightfield geometry class.
 */
#pragma once

#include "geometry.h"
#include "bbox.h"

class Heightfield: public Geometry {
	std::vector<float> heights, maxH;
	std::vector<Vector> normals;
	BBox bbox;
	int W, H;
	float getHeight(int x, int y) const;
	float getHighest(int x, int y, int k) const;
	Vector getNormal(float x, float y) const;

	void buildHighMap();

public:
	void beginRender();
	virtual bool intersect(Ray ray, IntersectionInfo& info) override;
	bool isInside(const Vector& p ) const { return false; }
	void fillProperties(ParsedBlock& pb);
};


