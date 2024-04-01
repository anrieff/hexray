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
 * @File geometry.cpp
 * @Brief Contains implementations of geometry primitives' intersection methods.
 */

#include "geometry.h"
#include "util.h"
#include <algorithm>

bool Plane::intersect(Ray ray, IntersectionInfo& info)
{
    if (ray.start.y > y && ray.dir.y >= 0) return false;
    if (ray.start.y < y && ray.dir.y <= 0) return false;
    //
    double going = ray.dir.y; // -1  * X ->    vv
    double toGo = (this->y) - ray.start.y; // -5
    //
    double m = toGo / going;
    info.dist = m;
    info.ip = ray.start + ray.dir * m;
    if (fabs(info.ip.x) > limit || fabs(info.ip.z) > limit) return false;
    info.norm.set(0, (ray.start.y > y) ? 1 : -1, 0);
    info.u = info.ip.x;
    info.v = info.ip.z;
    info.geom = this;
    return true;
}

bool Sphere::intersect(Ray ray, IntersectionInfo& info)
{
    double A = ray.dir.lengthSqr();
    Vector H = ray.start - O;
    double B = 2 * dot(ray.dir, H);
    double C = H.lengthSqr() - R*R;
    //p^2*A + p*B + C = 0
    double D = B*B - 4*A*C;
    if (D < 0) return false;
    //
    double sqrtD = sqrt(D);
    double p1 = (-B-sqrtD)/(2*A);
    double p2 = (-B+sqrtD)/(2*A);
    double p;
    // p2 >= p1!
    if (p2 < 0) return false;
    if (p1 < 0) p = p2;
    else p = p1;
    //
    info.dist = p;
    info.ip = ray.start + ray.dir * p;
    info.norm = info.ip - O;
    info.norm.normalize();
    info.v = asin(info.norm.y); // [-pi/2..+pi/2]
    info.u = atan2(info.norm.z, info.norm.x); // [-pi..+pi]
    info.v = -(info.v / PI + 0.5f); // [0..1]
    info.u = info.u / (2*PI) + 0.5f; // [0..1]
    if (uvscaling != 1) {
        info.u *= uvscaling;
        info.v *= uvscaling;
    }
    info.geom = this;
    return true;
}

static inline bool inBounds(double x, double center, double halfSide)
{
    // example: Cube is in (2.0, 0.0, -1.0), side = 1
    // X: cube is from  1.5 ..  2.5
    // Y: cube is from -0.5 ..  0.5
    // Z: cube is from -1.5 .. -0.5
    // eg., for X we'll call inBounds(x, 2, 0.5)
    return (x > center - halfSide - 1e-6 && x < center + halfSide + 1e-6);
}

int Cube::intersectCubeSide(
            const Vector& norm,
            double startCoord, // "A"
            double dir,
            double target,     // "B"
            const Ray& ray,
            IntersectionInfo& info,
            std::function<void(IntersectionInfo&)> genUV)
{
    // startCoord + dir * p == target
    if (fabs(dir) < 1e-9) return 0;
    if (startCoord < target && dir < 0) return 0;
    if (startCoord > target && dir > 0) return 0;
    //
    double p = (target - startCoord) / dir;
    if (p < info.dist) {
        Vector ip = ray.start + ray.dir * p;
        if (!inBounds(ip.x, O.x, m_halfSide)
         || !inBounds(ip.y, O.y, m_halfSide)
         || !inBounds(ip.z, O.z, m_halfSide)) return 0;
        info.dist = p;
        info.ip = ip;
        info.norm = norm;
        genUV(info);
        info.geom = this;

        return 1;
    }
    return 0;
}

bool Cube::intersect(Ray ray, IntersectionInfo& info)
{
    auto UV_X = [] (IntersectionInfo& info) { info.u = info.ip.y; info.v = info.ip.z; };
    auto UV_Y = [] (IntersectionInfo& info) { info.u = info.ip.x; info.v = info.ip.z; };
    auto UV_Z = [] (IntersectionInfo& info) { info.u = info.ip.x; info.v = info.ip.y; };
    int numIntersections = 0;
    //
    // +-X:
    info.dist = INF;
    numIntersections += intersectCubeSide(Vector(-1, 0, 0), ray.start.x, ray.dir.x, O.x - m_halfSide, ray, info, UV_X);
    numIntersections += intersectCubeSide(Vector(+1, 0, 0), ray.start.x, ray.dir.x, O.x + m_halfSide, ray, info, UV_X);
    // +-Y:
    numIntersections += intersectCubeSide(Vector( 0,-1, 0), ray.start.y, ray.dir.y, O.y - m_halfSide, ray, info, UV_Y);
    numIntersections += intersectCubeSide(Vector( 0,+1, 0), ray.start.y, ray.dir.y, O.y + m_halfSide, ray, info, UV_Y);
    // +-Z:
    numIntersections += intersectCubeSide(Vector( 0, 0,-1), ray.start.z, ray.dir.z, O.z - m_halfSide, ray, info, UV_Z);
    numIntersections += intersectCubeSide(Vector( 0, 0,+1), ray.start.z, ray.dir.z, O.z + m_halfSide, ray, info, UV_Z);
    //
    return numIntersections > 0;
}

std::vector<IntersectionInfo> findAllIntersections(Ray ray, Geometry* geom)
{
    std::vector<IntersectionInfo> result;
    Vector origin = ray.start;
    // doing a for loop just to ensure we eventually terminate. Typically though,
    // the loop will terminate after 1-2 iterations due to the geom->intersect() break.
    for (int counter = 0; counter < 30; counter++) {
        IntersectionInfo info;
        info.dist = INF;
        if (!geom->intersect(ray, info)) break;
        //
        result.push_back(info);
        ray.start = info.ip + ray.dir * 1e-6;
    }
    for (auto& info: result) info.dist = distance(origin, info.ip);
    return result;
}

bool CSGBase::intersect(Ray ray, IntersectionInfo& info)
{
    std::vector<IntersectionInfo> xLeft = findAllIntersections(ray, left);
    std::vector<IntersectionInfo> xRight = findAllIntersections(ray, right);

    std::vector<IntersectionInfo> allIntersections = xLeft;
    allIntersections.insert(allIntersections.end(), xRight.cbegin(), xRight.cend());
    //
    std::sort(allIntersections.begin(), allIntersections.end(),
        [] (const IntersectionInfo& left, const IntersectionInfo& right) -> bool {
            return left.dist < right.dist;
    });
    //
    bool inA = (xLeft.size() % 2);
    bool inB = (xRight.size() % 2);
    bool initial = inside(inA, inB);
    for (auto& infoCandidate: allIntersections) {
        if (infoCandidate.geom == left) inA = !inA;
        else                            inB = !inB;
        if (inside(inA, inB) != initial) {
            info = infoCandidate;
            info.norm = faceforward(ray.dir, info.norm);
            info.geom = this;
            return true;
        }
    }
    return false;
}
