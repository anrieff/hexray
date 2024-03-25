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

Color CheckerTexture::sample(const IntersectionInfo& info)
{
    int u1 = int(floor(info.u / scaling));
    int v1 = int(floor(info.v / scaling));
    return ((u1 + v1) % 2 == 0) ? col1 : col2;
}

static inline float getLambertTerm(const IntersectionInfo& info, double& distSqr)
{
    Vector lightToIp = info.ip - lightPos;
    distSqr = lightToIp.lengthSqr();
    //
    Vector dirToLight = -lightToIp;
    dirToLight.normalize();
    return std::max(0.0, dot(info.norm, dirToLight));
}

Color Lambert::computeColor(Ray ray, const IntersectionInfo& info)
{
    double distSqr;
    float lambertTerm = getLambertTerm(info, distSqr);
    //
    Color diffuseColor = this->diffuseTex ? diffuseTex->sample(info) : this->diffuse;
    //
    Color direct;
    if (visible(lightPos, info.ip))
        direct = diffuseColor * lightColor * (lambertTerm * lightIntensity / distSqr);
    else
        direct = Color(0, 0, 0);
    Color ambient = diffuseColor * AMBIENT_LIGHT;
    return direct + ambient;
}

Phong::Phong(Color color, Color spec, float specularExponent, Texture* diffuseTex):
    diffuse(color), specular(spec), specularExponent(specularExponent), diffuseTex(diffuseTex) {}

Color Phong::computeColor(Ray ray, const IntersectionInfo& info)
{
    double distSqr;
    float lambertTerm = getLambertTerm(info, distSqr);
    //
    Color diffuseColor = this->diffuseTex ? this->diffuseTex->sample(info) : this->diffuse;
    Color result = diffuseColor * AMBIENT_LIGHT;
    if (visible(lightPos, info.ip)) {
        result += diffuseColor * lightColor * (lambertTerm * lightIntensity / distSqr);
        // add specular:
        Vector fromLight = info.ip - lightPos;
        fromLight.normalize();

        Vector reflLight = reflect(fromLight, faceforward(fromLight, info.norm));
        float cosGamma = dot(-ray.dir, reflLight);
        if (cosGamma > 0) {
            result += specular * lightColor * pow(cosGamma, specularExponent);
        }
    }
    return result;
}

BitmapTexture::BitmapTexture(const char* filename, float scaling)
{
    m_bitmap.loadBMP(filename);
    this->scaling = scaling;
}
Color BitmapTexture::sample(const IntersectionInfo& info)
{
    float u = (info.u / scaling);
    float v = (info.v / scaling);
    // one square in the new u,v represents a whole copy of m_bitmap
    u -= floor(u);
    v -= floor(v);
    u *= m_bitmap.getWidth();
    v *= m_bitmap.getHeight();
    return m_bitmap.getPixel(int(u), int(v));
}
