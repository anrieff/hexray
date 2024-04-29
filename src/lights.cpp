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
    samplePos = T.transformPoint(Vector(lx - 0.5, 0, ly - 0.5));
    //
    Vector shadePos_LS = T.untransformPoint(shadePos);
    if (shadePos_LS.y < 0) {
        color = this->color * -shadePos_LS.y / shadePos_LS.length();
    } else {
        color.makeZero();
    }
}
