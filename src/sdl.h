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
 * @File sdl.h
 * @Brief Contains the interface to SDL
 */
#pragma once

#include "color.h"
#include "constants.h"
#include <vector>
#include <SDL.h>

struct Rect {
	int x0, y0, x1, y1, w, h;
	Rect() {}
	Rect(int _x0, int _y0, int _x1, int _y1)
	{
		x0 = _x0, y0 = _y0, x1 = _x1, y1 = _y1;
		h = y1 - y0;
		w = x1 - x0;
	}
	void clip(int maxX, int maxY); // clips the rectangle against image size
};

extern bool isInteractive;

bool initGraphics(int frameWidth, int frameHeight);
void closeGraphics(void);
void displayVFB(Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]); //!< displays the VFB (Virtual framebuffer) to the real one.
bool checkForUserExit(void); //!< check if the user wants to close the application (returns true if so)
/**
 * Gets the keyboard, mouse and potentially other inputs from SDL:
 * @param keystate - a byte-addressable array of key codes which are pressed at the time of calling getSDLInputs()
 *                   e.g. keystate[SDL_F1] will be nonzero if the F1 is being held pressed right now
 * @param mouseDeltaX - the relative mouse motion since the last call to getSDLInputs()
 * @param mouseDeltaY - ditto
 * @param events      - all non-system events queued for processing. The queue is cleared after this call and
 *                      starts collecting new events
 */
void getSDLInputs(const Uint8*& keystate, int& mouseDeltaX, int& mouseDeltaY, std::vector<SDL_Event>& events);
int frameWidth(void); //!< returns the frame width (pixels)
int frameHeight(void); //!< returns the frame height (pixels)

/// generate a list of buckets (image sub-rectangles) to be rendered, in a zigzag pattern
std::vector<Rect> getBucketsList(int bucketSize = 64);
/// updates a block of the screen (similar to displayVFB(), but for the specified rectangle only)
bool displayVFBRect(Rect r, Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]);
/// draws a rectangle on the screen with a solid color
bool drawRect(Rect r, const Color& c);
/// shows any updates to the screen buffer
void showUpdatedFullscreen();
void showUpdated(Rect r);
/// draws marking "brackets" on the given rectangle on the screen
bool markRegion(Rect r, const Color& bracketColor = Color(0, 0, 0.5f));
/// displays a mask of pixels on top of the currently shown screen contents
void markAApixels(bool needsAA[VFB_MAX_SIZE][VFB_MAX_SIZE]);
