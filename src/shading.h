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
#include "scene.h"

class Shader: public SceneElement {
public:
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) = 0;
	virtual ElementType getElementType() const override { return ELEM_SHADER; }
};

class Texture: public SceneElement {
public:
    virtual Color sample(Ray ray, const IntersectionInfo& info) = 0;
	virtual void modifyNormal(IntersectionInfo& info) {}
	virtual ElementType getElementType() const override { return ELEM_TEXTURE; }
};

class CheckerTexture: public Texture {
public:
    Color color1 = Color(1, 1, 1), color2 = Color(0, 0, 0);
    double scaling = 20.0;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color1", &color1);
		pb.getColorProp("color2", &color2);
		pb.getDoubleProp("scaling", &scaling);
	}
    virtual Color sample(Ray ray, const IntersectionInfo& info) override;
};

class BitmapTexture: public Texture {
    Bitmap m_bitmap;
public:
    double scaling = 100.0;
    virtual Color sample(Ray ray, const IntersectionInfo& info) override;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("scaling", &scaling);
		if (!pb.getBitmapFileProp("file", m_bitmap))
			pb.requiredProp("file");
	}
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
    Color diffuse = Color(0.5, 0.5, 0.5);
    Texture* diffuseTex = nullptr;
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &diffuse);
		pb.getTextureProp("texture", &diffuseTex);
	}
};

class Phong: public Shader {
public:
    Color diffuse = Color(0.5, 0.5, 0.5), specular = Color(1, 1, 1);
    float exponent = 10.0f;
    Texture* diffuseTex = nullptr;
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &diffuse);
        pb.getColorProp("specular", &specular);
		pb.getTextureProp("texture", &diffuseTex);
		pb.getFloatProp("exponent", &exponent);
	}
};

class Reflection: public Shader {
public:
    float glossiness = 1.0f;
    Color reflColor = Color(0.95f, 0.95f, 0.95f);
    int numSamples = 50;
    Reflection(float g = 1.0, Color rc = Color(0.95f, 0.95f, 0.95f)): glossiness(g), reflColor(rc) {}
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
	void fillProperties(ParsedBlock& pb)
	{
		double multiplier;
        if (pb.getDoubleProp("multiplier", &multiplier)) {
            reflColor = Color(multiplier, multiplier, multiplier);
        } else pb.getColorProp("reflColor", &reflColor);
		pb.getFloatProp("glossiness", &glossiness, 0, 1);
		pb.getIntProp("numSamples", &numSamples, 1);
	}
};

class Refraction: public Shader {
public:
    Color refrColor = Color(0.95f, 0.95f, 0.95f);
    double ior = 1.33;
    virtual Color computeColor(Ray ray, const IntersectionInfo& info) override;
	void fillProperties(ParsedBlock& pb)
	{
		double multiplier;
        if (pb.getDoubleProp("multiplier", &multiplier)) {
            refrColor = Color(multiplier, multiplier, multiplier);
        } else pb.getColorProp("refrColor", &refrColor);
		pb.getDoubleProp("ior", &ior, 1e-6, 10);
	}
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
	void fillProperties(ParsedBlock& pb);
};

class Fresnel: public Texture {
public:
    double ior = 1.33;
    virtual Color sample(Ray ray, const IntersectionInfo& info) override;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("ior", &ior, 1e-6, 10);
	}
};

class BumpTexture: public Texture {
	Bitmap bitmap;
    virtual Color sample(Ray ray, const IntersectionInfo& info) override { return Color(0, 0, 0); }
public:
	double strength = 1, scaling = 1;
    //
	void modifyNormal(IntersectionInfo& info) override;
	void beginRender() override;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("strength", &strength);
		pb.getDoubleProp("scaling", &scaling);
		if (!pb.getBitmapFileProp("file", bitmap))
			pb.requiredProp("file");
	}
};

class Const: public Shader {
	Color color = Color(0.5, 0.5, 0.5);
public:
	Color computeColor(Ray ray, const IntersectionInfo& info) override { return color; }
	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &color);
	}
};

// a texture that generates a slight random bumps on any geometry, which computes dNdx, dNdy
class Bumps: public Texture {
	float strength = 1;
    Color sample(Ray ray, const IntersectionInfo& info) override { return Color(0, 0, 0); }
public:
	void modifyNormal(IntersectionInfo& data) override;
	void fillProperties(ParsedBlock& pb)
	{
		pb.getFloatProp("strength", &strength);
	}

};
