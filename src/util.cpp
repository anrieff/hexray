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
 * @File util.cpp
 * @Brief a few useful short functions
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <random>
#include "util.h"

#include <string>
#include <chrono>
using namespace std;

string extensionUpper(const char* fileName)
{
	int l = (int) strlen(fileName);
	if (l < 2) return "";

	for (int i = l - 1; i >= 0; i--) {
		if (fileName[i] == '.') {
			string result = "";
			for  (int j = i + 1; j < l; j++) result += toupper(fileName[j]);
			return result;
		}
	}
	return "";
}

static std::mt19937 generator; // mersenne twister generator

float randFloat()
{
	static std::uniform_real_distribution<float> sampler;
	return sampler(generator);
}

double randDouble()
{
	static std::uniform_real_distribution<double> sampler;
	return sampler(generator);
}

void unitDiskSample(double& x, double& y)
{
	do {
		x = randDouble() * 2 - 1;
		y = randDouble() * 2 - 1;
	} while (x * x + y * y > 1);
}