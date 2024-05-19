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
 * @File camera.cpp
 * @Brief Implementation of the raytracing camera.
 */

#include "camera.h"
#include "sdl.h"
#include "matrix.h"

void Camera::beginFrame()
{
	m_width = frameWidth();
	m_height = frameHeight();
	//
	m_topLeft.set(-aspectRatio, +1, +1);
	m_topRight.set(+aspectRatio, +1, +1);
	m_bottomLeft.set(-aspectRatio, -1, +1);
	// fixup FOV:
	double BC = distance(Vector(0, 0, 1), m_topLeft);
	double BC_wanted = tan(toRadians(fov/2));
	double m = BC_wanted / BC;
	m_topLeft.set(-aspectRatio * m, +m, +1);
	m_topRight.set(+aspectRatio * m, +m, +1);
	m_bottomLeft.set(-aspectRatio * m, -m, +1);
	// rotate:
	Matrix R = rotationAroundZ(toRadians(roll))
			 * rotationAroundX(toRadians(pitch))
			 * rotationAroundY(toRadians(yaw));
	m_topLeft *= R;
	m_topRight *= R;
	m_bottomLeft *= R;
	// translate in front of the camera:
	m_topLeft += this->pos;
	m_topRight += this->pos;
	m_bottomLeft += this->pos;

	//
	m_upDir = normalize(m_topLeft - m_bottomLeft);
	m_rightDir = normalize(m_topRight - m_topLeft);
	m_frontDir = m_rightDir ^ m_upDir;
	//
	m_apertureSize = 2.5 / fNumber;
}

Ray Camera::getScreenRay(double x, double y, double stereoOffset)
{
	Ray result;
	result.start = this->pos;
	Vector through = m_topLeft + (m_topRight   - m_topLeft) * (x / m_width)
							   + (m_bottomLeft - m_topLeft) * (y / m_height);
	result.dir = through - result.start;
	result.dir.normalize();
	if (stereoOffset != 0)
		result.start += m_rightDir * (stereoOffset * stereoSeparation);
	return result;
}

Ray Camera::getDOFScreenRay(double x, double y, double u, double v, double stereoOffset)
{
	Ray ray = getScreenRay(x, y, stereoOffset);
	double M = focalPlaneDist / dot(m_frontDir, ray.dir);
	Vector T = ray.start + ray.dir * M;
	ray.start = this->pos + (u * m_apertureSize) * m_rightDir + (v * m_apertureSize) * m_upDir;
	ray.dir = normalize(T - ray.start);
	return ray;
}
