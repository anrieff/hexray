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
#include <string.h>

#include <optional>

Vector lightPos(30, 100, -70);
Color lightColor(1, 1, 1);
float lightIntensity = 10000.0f;

Color ConstantShader::computeColor(Ray ray, const IntersectionInfo& info)
{
    return color;
}

Color CheckerTexture::sample(Ray ray, const IntersectionInfo& info)
{
    int u1 = int(floor(info.u / scaling));
    int v1 = int(floor(info.v / scaling));
    return ((u1 + v1) % 2 == 0) ? color1 : color2;
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
    Color diffuseColor = this->diffuseTex ? diffuseTex->sample(ray, info) : this->diffuse;
    //
    Color direct;
    if (visible(lightPos, info.ip))
        direct = diffuseColor * lightColor * (lambertTerm * lightIntensity / distSqr);
    else
        direct = Color(0, 0, 0);
    Color ambient = diffuseColor * scene.settings.ambientLight;
    return direct + ambient;
}

Color Phong::computeColor(Ray ray, const IntersectionInfo& info)
{
    double distSqr;
    float lambertTerm = getLambertTerm(info, distSqr);
    //
    Color diffuseColor = this->diffuseTex ? this->diffuseTex->sample(ray, info) : this->diffuse;
    Color result = diffuseColor * scene.settings.ambientLight;
    if (visible(lightPos, info.ip)) {
        result += diffuseColor * lightColor * (lambertTerm * lightIntensity / distSqr);
        // add specular:
        Vector fromLight = info.ip - lightPos;
        fromLight.normalize();

        Vector reflLight = reflect(fromLight, faceforward(fromLight, info.norm));
        float cosGamma = dot(-ray.dir, reflLight);
        if (cosGamma > 0) {
            result += specular * lightColor * pow(cosGamma, exponent);
        }
    }
    return result;
}

/*BitmapTexture::BitmapTexture(const char* filename, float scaling)
{
    m_bitmap.loadBMP(filename);
    this->scaling = scaling;
}*/

Color BitmapTexture::sample(Ray ray, const IntersectionInfo& info)
{
    float u = (info.u / scaling);
    float v = (info.v / scaling);
    // one square in the new u,v represents a whole copy of m_bitmap
    u -= floor(u);
    v -= floor(v);
    u *= m_bitmap.getWidth();
    v *= m_bitmap.getHeight();
    return m_bitmap.getFilteredPixel(u, v);
}

Color Reflection::computeColor(Ray ray, const IntersectionInfo& info)
{
    Vector n = faceforward(ray.dir, info.norm);
    Ray newRay = ray;
    newRay.start = info.ip + n * 1e-6;
    newRay.depth = ray.depth + 1;
    if (glossiness < 1.0f) {
        float scaling = pow(10.0f, 2 - 8*glossiness);
        auto [u, v] = othonormedBasis(n);
        Color sum(0, 0, 0);
        for (int i = 0; i < numSamples; i++) {
            Vector reflected;
            do {
                double x, y;
                unitDiskSample(x, y);
                x *= scaling;
                y *= scaling;
                Vector modifiedNormal = n + u * x + v * y;
                modifiedNormal.normalize();
                reflected = reflect(ray.dir, modifiedNormal);
            } while (dot(reflected, n) < 0);
            newRay.dir = reflected;
            sum += raytrace(newRay) * reflColor;
        }
        return sum / numSamples;
    }
    newRay.dir = reflect(ray.dir, n);
    return raytrace(newRay) * reflColor; // account for attenuation
}

inline std::optional<Vector> refract(const Vector& i, const Vector& n, float ior)
{
    float NdotI = dot(i, n);
    float k = 1 - (ior * ior) * (1 - NdotI * NdotI);
    if (k < 0) return {};
    return ior * i - (ior * NdotI + sqrt(k)) * n;
}

Color Refraction::computeColor(Ray ray, const IntersectionInfo& info)
{
	std::optional<Vector> refr;
	if (dot(ray.dir, info.norm) < 0) {
		// entering the geometry
		refr = refract(ray.dir, info.norm, 1 / ior);
	} else {
		// leaving the geometry
		refr = refract(ray.dir, -info.norm, ior);
	}
	if (!refr) return Color(0, 0, 0);
	Ray newRay = ray;
	newRay.start = info.ip - faceforward(ray.dir, info.norm) * 0.000001;
	newRay.dir = refr.value();
	newRay.depth++;
	return raytrace(newRay) * refrColor;
}

void Layered::addLayer(Shader* shader, Color blend, Texture* blendTex)
{
    m_layers.emplace_back(Layer{shader, blend, blendTex});
}

void Layered::fillProperties(ParsedBlock& pb)
{
	char name[128];
	char value[256];
	int srcLine;
	for (int i = 0; i < pb.getBlockLines(); i++) {
		// fetch and parse all lines like "layer <shader>, <color>[, <texture>]"
		pb.getBlockLine(i, srcLine, name, value);
		if (!strcmp(name, "layer")) {
			char shaderName[200];
			char textureName[200] = "";
			bool err = false;
			if (!getFrontToken(value, shaderName)) {
				err = true;
			} else {
				stripPunctuation(shaderName);
			}
			if (!strlen(value)) err = true;
			if (!err && value[strlen(value) - 1] != ')') {
				if (!getLastToken(value, textureName)) {
					err = true;
				} else {
					stripPunctuation(textureName);
				}
			}
			if (!err && !strcmp(textureName, "NULL")) strcpy(textureName, "");
			Shader* shader = NULL;
			Texture* texture = NULL;
			if (!err) {
				shader = pb.getParser().findShaderByName(shaderName);
				err = (shader == NULL);
			}
			if (!err && strlen(textureName)) {
				texture = pb.getParser().findTextureByName(textureName);
				err = (texture == NULL);
			}
			if (err) throw SyntaxError(srcLine, "Expected a line like `layer <shader>, <color>[, <texture>]'");
			double x, y, z;
			get3Doubles(srcLine, value, x, y, z);
			addLayer(shader, Color((float) x, (float) y, (float) z), texture);
		}
	}
}


Color Layered::computeColor(Ray ray, const IntersectionInfo& info)
{
    Color col(0, 0, 0);
    for (auto& layer: m_layers) {
        Color blend = layer.blendTex ? layer.blendTex->sample(ray, info) : layer.blend;
        Color fromShader = layer.shader->computeColor(ray, info);
        col = col * (Color(1, 1, 1) - blend) + fromShader * blend;
    }
    return col;
}

inline float fresnelSchlickApprox(float NdotI, float ior)
{
    float f = sqr((1.0f - ior) / (1.0f + ior));
    float x = 1 - NdotI;
    return f + (1 - f) * pow(x, 5.0f);
}

Color Fresnel::sample(Ray ray, const IntersectionInfo& info)
{
    float eta = ior;
    float NdotI = dot(ray.dir, info.norm);
    if (NdotI > 0)
        eta = 1/eta;
    else
        NdotI = -NdotI;
    float fr = fresnelSchlickApprox(NdotI, eta);
    return Color(fr, fr, fr);
}

void BumpTexture::modifyNormal(IntersectionInfo& info)
{
    float x = fmod(info.u * scaling * bitmap.getWidth(), bitmap.getWidth());
    float y = fmod(info.v * scaling * bitmap.getHeight(), bitmap.getHeight());
    //
    Color bump = bitmap.getFilteredPixel(x, y);
    float dx = bump.r * strength;
    float dy = bump.g * strength;

    info.norm += (info.dNdx * dx + info.dNdy * dy);
    info.norm.normalize();
}

void BumpTexture::beginRender()
{
    bitmap.differentiate();
}
