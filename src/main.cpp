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
 * @File main.cpp
 * @Brief Raytracer main file
 */
#include <SDL/SDL.h>
#include <math.h>
#include "util.h"
#include "sdl.h"
#include "color.h"
#include "vector.h"

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];

void render()
{
	double centerX = frameWidth() * 0.5;
	double centerY = frameHeight() * 0.5;
	double radius = 100.0;
	double border = 40;
	for (int y = 0; y < frameHeight(); y++)
		for (int x = 0; x < frameWidth(); x++) {
			double dx = x - centerX;
			double dy = y - centerY;
			double dist = sqrt(dx*dx + dy*dy);
			float intensity;
			if (dist < radius - border/2) intensity = 1;
			else if (dist < radius + border/2) {
				intensity = (dist - radius) / border;
				// if dist = radius - border/2 -> intensity = -0.5
				// if dist = radius            -> intensity = 0
				// if dist = radius + border/2 -> intensity = +0.5
				intensity = -intensity+0.5; // [-0.5 .. +0.5] -> [0.0 .. 1.0]
			} else intensity = 0;
			vfb[y][x] = Color(0.0, intensity * 0.4f, intensity);
		}
}

int main(int argc, char** argv)
{
	initGraphics(800, 600);
	render();
	displayVFB(vfb);
	waitForUserExit();
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
