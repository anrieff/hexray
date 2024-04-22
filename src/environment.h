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
 * @File environment.h
 * @Brief declarations of the Environment classes
 */
#pragma once

#include <array>
#include "color.h"
#include "vector.h"
#include "bitmap.h"
#include "scene.h"

enum CubeOrder {
	NEGX, // 0
	NEGY, // 1
	NEGZ, // 2
	POSX, // 3
	POSY, // 4
	POSZ, // 5
};

class Environment: public SceneElement {
public:
	bool loaded = false;
	virtual ~Environment() {}
	/// gets a color from the environment at the specified direction
	virtual Color getEnvironment(const Vector& dir) = 0;
	//
	virtual ElementType getElementType() const override { return ELEM_ENVIRONMENT; }
};

class CubemapEnvironment: public Environment {
	std::array<Bitmap, 6> m_sides;
	Color getSide(const Bitmap& bmp, double x, double y);
public:
 	/// loads a cubemap from 6 separate images, from the specified folder.
 	/// The images have to be named "posx.bmp", "negx.bmp", "posy.bmp", ...
 	/// (or they may be .exr images, not .bmp).
 	/// The folder specification shouldn't include a trailing slash;
 	/// e.g. "/images/cubemaps/cathedral" is OK.
	bool loadMaps(const char* folder);

	void fillProperties(ParsedBlock& pb)
	{
		Environment::fillProperties(pb);
		char folder[256];
		if (!pb.getFilenameProp("folder", folder)) pb.requiredProp("folder");
		if (!loadMaps(folder)) {
			fprintf(stderr, "CubemapEnvironment: Could not load maps from `%s'\n", folder);
		}
	}

	virtual Color getEnvironment(const Vector& dir) override;
};
