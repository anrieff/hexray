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
 * @File camera.h
 * @Brief Contains declaration of the raytracing camera.
 */
#pragma once

#include "vector.h"
#include "scene.h"

class Camera: public SceneElement {
    Vector m_topLeft, m_topRight, m_bottomLeft;
    double m_width, m_height;
	Vector m_upDir, m_rightDir;
	Vector m_frontDir;
	double m_apertureSize;
public:
    Vector pos = Vector(0, 0, 0);
    double yaw = 0, pitch = 0, roll = 0; // ! in degrees
    double aspectRatio = 4.0/3.0;
    double fov = 90;
	double fNumber = 2.0;
	int    numSamples = 32;
	double focalPlaneDist = 100;
	bool   dof = false;
	bool   autoFocus = false;
	double stereoSeparation = 0;

    void beginFrame();
    Ray getScreenRay(double x, double y, double stereoOffset = 0.0);
	Ray getDOFScreenRay(double x, double y, double u, double v, double stereoOffset = 0.0);
	Vector getFrontDir() const { return m_frontDir; }
	double getApertureSize() const { return m_apertureSize; }
    //
	virtual ElementType getElementType() const override { return ELEM_CAMERA; }
	void fillProperties(ParsedBlock& pb)
	{
		if (!pb.getVectorProp("pos", &pos))
			pb.requiredProp("pos");
		pb.getDoubleProp("aspectRatio", &aspectRatio, 1e-6);
		pb.getDoubleProp("fov", &fov, 0.0001, 179);
		pb.getDoubleProp("yaw", &yaw);
		pb.getDoubleProp("pitch", &pitch, -90, 90);
		pb.getDoubleProp("roll", &roll);
		pb.getDoubleProp("fNumber", &fNumber, 0.5, 128.0);
		pb.getIntProp("numSamples", &numSamples, 1);
		pb.getDoubleProp("focalPlaneDist", &focalPlaneDist, 1e-3, 1e+6);
		pb.getBoolProp("dof", &dof);
		pb.getBoolProp("autoFocus", &autoFocus);
		pb.getDoubleProp("stereoSeparation", &stereoSeparation, 0.0);
	}
};
