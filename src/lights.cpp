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
 * @File lights.cpp
 * @Brief Implements the various models of light sources
 */
#include "lights.h"

int PointLight::intersect(const Ray& ray, double& intersectionDist)
{
    return 0; // you cannot intersect a point light.
}

int RectLight::getNumSamples() const
{
    return xSubd * ySubd;
}

void RectLight::getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color)
{
    // xSubd=3, ySubd=4, sampleIdx = 0..11
    double lx = ((sampleIdx % xSubd) + randDouble()) / xSubd; //lx in [0..1]
    double ly = ((sampleIdx / xSubd) + randDouble()) / ySubd; //ly in [0..1]
    //
    samplePos = T.transformPoint(Vector(lx - 0.5, -1e-6, ly - 0.5));
    //
    Vector shadePos_LS = T.untransformPoint(shadePos);
    if (shadePos_LS.y < 0) {
        color = this->color * -shadePos_LS.y / shadePos_LS.length();
    } else {
        color.makeZero();
    }
}

int RectLight::intersect(const Ray& ray, double& intersectionDist)
{
	Ray ray_LS = T.untransformRay(ray);
	if (fabs(ray_LS.dir.y) < 1e-12) return 0; // ray parallel to light, no intersection possible
	// check if ray_LS (the incoming ray, transformed in local space) hits the oriented square 1x1, resting
	// at (0, 0, 0), pointing downwards:
	double lengthToIntersection = -(ray_LS.start.y / ray_LS.dir.y); // intersect with XZ plane
	if (lengthToIntersection < 0) return 0; // hit point is not forward of the ray, no intersection possible
	Vector p = ray_LS.start + ray_LS.dir * lengthToIntersection;
	if (fabs(p.x) < 0.5 && fabs(p.z) < 0.5) {
		// the hit point is inside the 1x1 square - calculate the length to the intersection:
		double distance = (T.transformPoint(p) - ray.start).length();

		if (distance < intersectionDist) {
			// intersection found, and it improves the current closest dist
			intersectionDist = distance;
			return (ray_LS.start.y < 0) ? +1 : -1; // return side we hit the light on
		}
	}
	return 0;
}

void RectLight::beginFrame()
{
	Vector pts[3];
	for (int i = 0; i < 3; i++) {
		pts[i].x = (i % 2) ? -0.5 : 0.5;
		pts[i].y = 0;
		pts[i].z = (i < 2) ? -0.5 : 0.5;
		pts[i] = T.transformPoint(pts[i]);
	}
	double A = distance(pts[0], pts[1]);
	double B = distance(pts[0], pts[2]);
	m_area = A * B;
	scaleFactor = 1 / m_area;
}

double RectLight::getSolidAngle(const Vector& p)
{
	Vector lightDirCanonic(0, -1, 0);
	//
	Vector lightDirWorld = T.transformDir(lightDirCanonic);
	Vector lightPosWorld = T.transformPoint(Vector(0, 0, 0));
	Vector lightToP = p - lightPosWorld;
	double cosTerm = dot(lightToP, lightDirWorld);
	if (cosTerm < 0) return 0;
	double d = lightToP.length();
	cosTerm /= d;
	return m_area * cosTerm / sqr(1 + d);
}
