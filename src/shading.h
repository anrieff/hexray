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
#include "bitmap.h"

extern Vector lightPos;
extern Color lightColor;
extern float lightIntensity;


class Shader {
public:
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) = 0;
};

class Texture {
public:
    virtual Color sample(Ray ray, const IntersectionInfo& info) = 0;
};

class CheckerTexture: public Texture {
public:
    Color col1, col2;
    float scaling;
    CheckerTexture(const Color& c1 = Color(1, 1, 1), const Color& c2 = Color(0, 0, 0), float scaling = 20): col1(c1), col2(c2), scaling(scaling) {}
    virtual Color sample(Ray ray, const IntersectionInfo& info) override;
};

class BitmapTexture: public Texture {
    Bitmap m_bitmap;
public:
    float scaling;
    BitmapTexture(const char* filename, float scaling = 100.0f);
    virtual Color sample(Ray ray, const IntersectionInfo& info) override;
};

class ConstantShader: public Shader {
public:
    Color color;
    ConstantShader(Color col = Color(0.5, 0.5, 0.5)): color(col) {}
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
};
//    float scaling = 20.0f;

class Lambert: public Shader {
public:
    Color diffuse;
    Texture* diffuseTex = nullptr;
    Lambert(
        Color color = Color(0.5, 0.5, 0.5),
        Texture* tex = nullptr
        ): diffuse(color), diffuseTex(tex) {}
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
};

class Phong: public Shader {
public:
    Color diffuse, specular;
    float specularExponent;
    Texture* diffuseTex;
    Phong(
        Color color = Color(0.5, 0.5, 0.5),
        Color spec = Color(1, 1, 1),
        float specularExponent = 10.0,
        Texture* diffuseTex = nullptr);
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
};

class Reflection: public Shader {
public:
    float glossiness = 1.0f;
    Color reflColor = Color(0.95f, 0.95f, 0.95f);
    int numSamples = 50;
    Reflection(float g = 1.0, Color rc = Color(0.95f, 0.95f, 0.95f)): glossiness(g), reflColor(rc) {}
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
};

class Refraction: public Shader {
public:
    Color refrColor = Color(0.95f, 0.95f, 0.95f);
    float ior = 1.33;
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
};

class Layered: public Shader {
    struct Layer {
        Shader* shader;
        Color blend;
        Texture* blendTex;
    };
    std::vector<Layer> m_layers;
public:
    void addLayer(Shader* shader, Color blend = Color(1, 1, 1), Texture* blendTex = nullptr);
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
};

class Fresnel: public Texture {
public:
    float ior = 1.33;
    virtual Color sample(Ray ray, const IntersectionInfo& info) override;
};
