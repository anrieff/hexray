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
 * @File shading.h
 * @Brief Contains declarations of shader classes
 */
#pragma once

#include "color.h"
#include "vector.h"
#include "geometry.h"

extern Vector lightPos;
extern Color lightColor;
extern float lightIntensity;


class Shader {
public:
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) = 0;
};

class ConstantShader: public Shader {
public:
    Color color;
    ConstantShader(Color col = Color(0.5, 0.5, 0.5)): color(col) {}
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
};

class Checker: public Shader {
public:
    Color col1, col2;
    float scaling = 20.0f;
    Checker(
        Color col1 = Color(0.5, 0.5, 0.5),
        Color col2 = Color(0.1, 0.1, 0.8),
        float scaling = 20.0f): col1(col1), col2(col2), scaling(scaling) {}
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
};
