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
 * @File geometry.cpp
 * @Brief Contains implementations of geometry primitives' intersection methods.
 */

#include "geometry.h"
#include "util.h"

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
    info.norm.set(0, (ray.start.y > y) ? 1 : -1, 0);
    info.u = info.ip.x;
    info.v = info.ip.z;
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
    info.v = toDegrees(asin(info.norm.y));
    info.u = toDegrees(atan2(info.norm.x, info.norm.z));
    return true;
}
