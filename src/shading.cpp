/***************************************************************************
 *   Copyright (C) 2009-2024 by Veselin Georgiev, Slavomir Kaslev et al    *
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
 * @File shading.cpp
 * @Brief Contains implementations of shader classes
 */

#include "shading.h"
#include "main.h"

Vector lightPos;
Color lightColor;
float lightIntensity;
const Color AMBIENT_LIGHT(0.15, 0.15, 0.15);

Color ConstantShader::computeColor(Ray ray, const IntersectionInfo& info)
{
    return color;
}

Color Checker::computeColor(Ray ray, const IntersectionInfo& info)
{
    int u1 = int(floor(info.u / scaling));
    int v1 = int(floor(info.v / scaling));
    Color toUse = ((u1 + v1) % 2 == 0) ? col1 : col2;
    //
    Vector lightToIp = info.ip - lightPos;
    double distSqr = lightToIp.lengthSqr();
    //
    Vector dirToLight = -lightToIp;
    dirToLight.normalize();
    float lambertTerm = dot(info.norm, dirToLight);
    if (lambertTerm < 0) return Color(0, 0, 0);
    //
    Color direct;
    if (visible(lightPos, info.ip))
        direct = toUse * lightColor * (lambertTerm * lightIntensity / distSqr);
    else
        direct = Color(0, 0, 0);
    Color ambient = toUse * AMBIENT_LIGHT;
    return direct + ambient;
}
