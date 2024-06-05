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
 * @File bitmap.cpp
 * @Brief Implementation of the Bitmap class, loading/saving .BMP files.
 */
#include <stdio.h>
#include <string.h>
#include "color.h"
#include "constants.h"
#include "bitmap.h"
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <Iex.h>

Bitmap::Bitmap()
{
	m_width = m_height = -1;
}

void Bitmap::freeMem(void)
{
	m_width = m_height = -1;
    m_data.clear();
    m_data.shrink_to_fit();
}

int Bitmap::getWidth(void) const { return m_width; }
int Bitmap::getHeight(void) const { return m_height; }
bool Bitmap::isOK(void) const { return !m_data.empty(); }

void Bitmap::generateEmptyImage(int w, int h)
{
	freeMem();
	if (w <= 0 || h <= 0) return;
	m_width = w;
	m_height = h;
	m_data.resize(w * h);
	for (auto& pixel: m_data) pixel.makeZero();
}

Color Bitmap::getPixel(int x, int y) const
{
	if (m_data.empty() || x < 0 || x >= m_width || y < 0 || y >= m_height) return Color(0.0f, 0.0f, 0.0f);
	return m_data[x + y * m_width];
}

Color Bitmap::getFilteredPixel(float x, float y) const
{
	if (m_data.empty() || !m_width || !m_height || x < 0 || x >= m_width || y < 0 || y >= m_height) return Color(0.0f, 0.0f, 0.0f);
	int tx = (int) floor(x);
	int ty = (int) floor(y);
	if (tx < 0 || ty < 0) return Color(0.0f, 0.0f, 0.0f);
	int tx_next = (tx + 1) % m_width;
	int ty_next = (ty + 1) % m_height;
	float p = x - tx;
	float q = y - ty;
	return
		  m_data[ty      * m_width + tx     ] * ((1.0f - p) * (1.0f - q))
		+ m_data[ty      * m_width + tx_next] * (        p  * (1.0f - q))
		+ m_data[ty_next * m_width + tx     ] * ((1.0f - p) *         q )
		+ m_data[ty_next * m_width + tx_next] * (        p  *         q );
}


void Bitmap::setPixel(int x, int y, const Color& color)
{
	if (m_data.empty() || x < 0 || x >= m_width || y < 0 || y >= m_height) return;
	m_data[x + y * m_width] = color;
}

class ImageOpenRAII {
	Bitmap &bmp;
public:
	bool imageIsOk;
	FILE* fp;
	ImageOpenRAII(Bitmap &bitmap): bmp(bitmap)
	{
		fp = NULL;
		imageIsOk = false;
	}
	~ImageOpenRAII()
	{
		if (!imageIsOk) bmp.freeMem();
		if (fp) fclose(fp);
		fp = NULL;
	}
};

const int BM_MAGIC = 19778;

struct BmpHeader {
	int fs;       // filesize
	int lzero;
	int bfImgOffset;  // basic header size
};
struct BmpInfoHeader {
	int ihdrsize; 	// info header size
	int x,y;      	// image dimensions
	unsigned short channels;// number of planes
	unsigned short bitsperpixel;
	int compression; // 0 = no compression
	int biSizeImage; // used for compression; otherwise 0
	int pixPerMeterX, pixPerMeterY; // dots per meter
	int colors;	 // number of used colors. If all specified by the bitsize are used, then it should be 0
	int colorsImportant; // number of "important" colors (wtf?..)
};

bool Bitmap::loadBMP(const char* filename)
{
	freeMem();
	ImageOpenRAII helper(*this);
    //
	BmpHeader hd;
	BmpInfoHeader hi;
	Color palette[256];
	int toread = 0;
	int rowsz;
	unsigned short sign;
	FILE* fp = fopen(filename, "rb");

	if (fp == NULL) {
		printf("loadBMP: Can't open file: `%s'\n", filename);
		return false;
	}
	helper.fp = fp;
	if (!fread(&sign, 2, 1, fp)) return false;
	if (sign != BM_MAGIC) {
		printf("loadBMP: `%s' is not a BMP file.\n", filename);
		return false;
	}
	if (!fread(&hd, sizeof(hd), 1, fp)) return false;
	if (!fread(&hi, sizeof(hi), 1, fp)) return false;

	/* header correctness checks */
	if (!(hi.bitsperpixel == 8 || hi.bitsperpixel == 24 ||  hi.bitsperpixel == 32)) {
		printf("loadBMP: Cannot handle file format at %d bpp.\n", hi.bitsperpixel);
		return false;
	}
	if (hi.channels != 1) {
		printf("loadBMP: cannot load multichannel .bmp!\n");
		return false;
	}
	/* ****** header is OK *******/

	// if image is 8 bits per pixel or less (indexed mode), read some pallete data
	if (hi.bitsperpixel <= 8) {
		toread = (1 << hi.bitsperpixel);
		if (hi.colors) toread = hi.colors;
		for (int i = 0; i < toread; i++) {
			unsigned temp;
			if (!fread(&temp, 1, 4, fp)) return false;
			palette[i] = Color(temp);
		}
	}
	toread = hd.bfImgOffset - (54 + toread*4);
	fseek(fp, toread, SEEK_CUR); // skip the rest of the header
	int k = hi.bitsperpixel / 8;
	rowsz = hi.x * k;
	if (rowsz % 4 != 0)
		rowsz = (rowsz / 4 + 1) * 4; // round the row size to the next exact multiple of 4
	std::vector<unsigned char> xx(rowsz);
	generateEmptyImage(hi.x, hi.y);
	if (!isOK()) {
		printf("loadBMP: cannot allocate memory for bitmap! Check file integrity!\n");
		return false;
	}
	for (int j = hi.y - 1; j >= 0; j--) {// bitmaps are saved in inverted y
		if (!fread(&xx[0], 1, rowsz, fp)) {
			printf("loadBMP: short read while opening `%s', file is probably incomplete!\n", filename);
			return false;
		}
		for (int i = 0; i < hi.x; i++){ // actually read the pixels
			if (hi.bitsperpixel > 8)
				setPixel(i, j, Color(xx[i*k+2]/255.0f, xx[i*k+1]/255.0f, xx[i*k]/255.0f));
			else
				setPixel(i, j,  palette[xx[i*k]]);
		}
	}
	helper.imageIsOk = true;
	return true;
}

bool Bitmap::saveBMP(const char* filename)
{
	FILE* fp = fopen(filename, "wb");
	if (!fp) return false;
	BmpHeader hd;
	BmpInfoHeader hi;
	char xx[VFB_MAX_SIZE * 3];

	// fill in the header:
	int rowsz = m_width * 3;
	if (rowsz % 4)
		rowsz += 4 - (rowsz % 4); // each row in of the image should be filled with zeroes to the next multiple-of-four boundary
	hd.fs = rowsz * m_height + 54; //std image size
	hd.lzero = 0;
	hd.bfImgOffset = 54;
	hi.ihdrsize = 40;
	hi.x = m_width; hi.y = m_height;
	hi.channels = 1;
	hi.bitsperpixel = 24; //RGB format
	// set the rest of the header to default values:
	hi.compression = hi.biSizeImage = 0;
	hi.pixPerMeterX = hi.pixPerMeterY = 0;
	hi.colors = hi.colorsImportant = 0;

	fwrite(&BM_MAGIC, 2, 1, fp); // write 'BM'
	fwrite(&hd, sizeof(hd), 1, fp); // write file header
	fwrite(&hi, sizeof(hi), 1, fp); // write image header
	for (int y = m_height - 1; y >= 0; y--) {
		for (int x = 0; x < m_width; x++) {
			unsigned t = getPixel(x, y).toRGB32();
			xx[x * 3    ] = (0xff     & t);
			xx[x * 3 + 1] = (0xff00   & t) >> 8;
			xx[x * 3 + 2] = (0xff0000 & t) >> 16;
		}
		fwrite(xx, rowsz, 1, fp);
	}
	fclose(fp);
	return true;
}

bool Bitmap::loadEXR(const char* filename)
{
	try {
		Imf::RgbaInputFile exr(filename);
		Imf::Array2D<Imf::Rgba> pixels;
		Imath::Box2i dw = exr.dataWindow();
		m_width  = dw.max.x - dw.min.x + 1;
		m_height = dw.max.y - dw.min.y + 1;
		pixels.resizeErase(m_height, m_width);
		exr.setFrameBuffer(&pixels[0][0] - dw.min.x - dw.min.y * m_width, 1, m_width);
		exr.readPixels(dw.min.y, dw.max.y);
		m_data.resize(m_width * m_height);
		for (int y = 0; y < m_height; y++)
			for (int x = 0; x < m_width; x++) {
				Color& pixel = m_data[y * m_width + x];
				pixel.r = pixels[y + dw.min.y][x + dw.min.x].r;
				pixel.g = pixels[y + dw.min.y][x + dw.min.x].g;
				pixel.b = pixels[y + dw.min.y][x + dw.min.x].b;
			}
		return true;
	}
	catch (Iex::BaseExc& ex) {
		m_width = m_height = 0;
		m_data.clear();
		return false;
	}
}

bool Bitmap::saveEXR(const char* filename)
{
	try {
		Imf::RgbaOutputFile file(filename, m_width, m_height, Imf::WRITE_RGBA);
		std::vector<Imf::Rgba> temp(m_width * m_height);
		for (int i = 0; i < m_width * m_height; i++) {
			temp[i].r = m_data[i].r;
			temp[i].g = m_data[i].g;
			temp[i].b = m_data[i].b;
			temp[i].a = 1.0f;
		}
		file.setFrameBuffer(&temp[0], 1, m_width);
		file.writePixels(m_height);
	}
	catch (Iex::BaseExc& ex) {
		return false;
	}
	return true;
}

bool Bitmap::loadImage(const char* filename)
{
	if (extensionUpper(filename) == "BMP") return loadBMP(filename);
	if (extensionUpper(filename) == "EXR") return loadEXR(filename);
	return false;
}

bool Bitmap::saveImage(const char* filename)
{
	if (extensionUpper(filename) == "BMP") return saveBMP(filename);
	if (extensionUpper(filename) == "EXR") return saveEXR(filename);
	return false;
}

void Bitmap::differentiate()
{
	Bitmap bumpTex;
	bumpTex.generateEmptyImage(this->m_width, this->m_height);

	for (int y = 0; y < m_height; y++) {
		for (int x = 0; x < m_width; x++) {
			float dx = getPixel(x, y).intensity() -
					   getPixel((x + 1) % m_width, y).intensity();
			float dy = getPixel(x, y).intensity() -
					   getPixel(x, (y + 1) % m_height).intensity();
			bumpTex.setPixel(x, y, Color(dx, dy, 0));
		}
	}
	this->m_data = std::move(bumpTex.m_data);
}

void Bitmap::decompressGamma(float gamma)
{
	if (fabs(gamma - 2.2f) < 1e-6f) {
		// assume sRGB decompression:
		for (auto& pixel: m_data) {
			if (pixel.r > 0) pixel.r = decompress_sRGB(pixel.r);
			if (pixel.g > 0) pixel.g = decompress_sRGB(pixel.g);
			if (pixel.b > 0) pixel.b = decompress_sRGB(pixel.b);
		}
	} else {
		// arbitrary gamma decompression
		for (auto& pixel: m_data) {
			if (pixel.r > 0) pixel.r = pow(pixel.r, gamma);
			if (pixel.g > 0) pixel.g = pow(pixel.g, gamma);
			if (pixel.b > 0) pixel.b = pow(pixel.b, gamma);
		}
	}
}
