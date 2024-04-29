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
 * @File bitmap.h
 * @Brief A class to represent bitmap textures.
 */
#pragma once

#include "color.h"
#include <vector>

/// @brief a class that represents a bitmap (2d array of colors), e.g. a image
/// supports loading/saving to BMP
class Bitmap {
	int m_width, m_height;
	std::vector<Color> m_data;
public:
	Bitmap(); //!< Generates an empty bitmap
	void freeMem(void); //!< Deletes the memory, associated with the bitmap
	int getWidth(void) const; //!< Gets the width of the image (X-dimension)
	int getHeight(void) const; //!< Gets the height of the image (Y-dimension)
	bool isOK(void) const; //!< Returns true if the bitmap is valid
	void generateEmptyImage(int width, int height); //!< Creates an empty image with the given dimensions
	Color getPixel(int x, int y) const; //!< Gets the pixel at coordinates (x, y). Returns black if (x, y) is outside of the image
	Color getFilteredPixel(float x, float y) const; //!< Gets an interpolated pixel at coordinates (x, y), using bilinear filtering
	void setPixel(int x, int y, const Color& col); //!< Sets the pixel at coordinates (x, y).

	bool loadBMP(const char* filename); //!< Loads an image from a BMP file. Returns false in the case of an error
	bool saveBMP(const char* filename); //!< Saves the image to a BMP file (with clamping, etc). Returns false in the case of an error (e.g. read-only media)

	bool loadEXR(const char* filename); //!< Loads an EXR file
	bool saveEXR(const char* filename); //!< Saves the image into the EXR format, preserving the dynamic range, using Half for storage.

	enum OutputFormat { /// the two supported writing formats
		outputFormat_BMP,
		outputFormat_EXR,
	};

	virtual bool loadImage(const char* filename); //!< Loads an image (the format is detected from extension)
	virtual bool saveImage(const char* filename); //!< Save the bitmap to an image (the format is detected from extension)

	void differentiate();
	void decompressGamma(float gamma);
};
