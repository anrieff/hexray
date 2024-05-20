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

vector<string> tokenize(string s)
{
	int i = 0, j, l = (int) s.length();
	vector<string> result;
	while (i < l) {
		while (i < l && isspace(s[i])) i++;
		if (i >= l) break;
		j = i;
		while (j < l && !isspace(s[j])) j++;
		result.push_back(s.substr(i, j - i));
		i = j;
	}
	return result;
}

vector<string> split(string s, char separator)
{
	int i = 0, j, l = (int) s.length();
	vector<string> result;
	while (i < l) {
		j = i;
		while (j < l && s[j] != separator) j++;
		result.push_back(s.substr(i, j - i));
		i = j + 1;
		if (j == l - 1) result.push_back("");
	}
	return result;
}

static std::mt19937 generator; // mersenne twister generator

int randInt(int a, int b)
{
	static std::uniform_int_distribution<int> sampler(a, b);
	return sampler(generator);
}

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

Vector hemisphereSample(const Vector& normal)
{
	double u = randDouble();
	double v = randDouble();

	double theta = 2 * PI * u;
	double cosPhi = 2 * v - 1;
	double sinPhi = sqrt(1 - cosPhi * cosPhi);

	Vector vec(
		cos(theta) * sinPhi,
		cosPhi,
		sin(theta) * sinPhi
	);

	/// restrict to the hemisphere: it is on the wrong side, just flip the result:
	if (dot(vec, normal) < 0)
		vec = -vec;

	return vec;
}
