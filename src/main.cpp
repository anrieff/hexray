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
 * @File main.cpp
 * @Brief Raytracer main file
 */
#include <SDL.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <vector>
#include <filesystem>
#include <optional>
#include <atomic>
#include <thread>
#include <memory>
#include "util.h"
#include "sdl.h"
#include "main.h"
#include "color.h"
#include "vector.h"
#include "camera.h"
#include "geometry.h"
#include "node.h"
#include "shading.h"
#include "matrix.h"
#include "mesh.h"
#include "environment.h"
#include "lights.h"
#include "threading.h"

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE];
bool needsAA[VFB_MAX_SIZE][VFB_MAX_SIZE];
std::vector<Rect> buckets;
const float AA_THRESH = 0.075f;
int vipX = -100, vipY = -100;

struct TraceContext {
	IntersectionInfo closestIntersection;
	Node* closestNode;

	std::optional<Color> raycast(const Ray& ray);
};

std::optional<Color> TraceContext::raycast(const Ray& ray)
{
	if (ray.depth > scene.settings.maxTraceDepth) return Color(0, 0, 0);
	closestIntersection.dist = INF;
	closestNode = nullptr;
	// check for ray->node intersection:
	for (auto& node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < closestIntersection.dist) {
			closestIntersection = info;
			closestNode = node;
		}
	}
	// check if the closest intersection point is actually a light:
	std::optional<Color> hitLightColor;
	for (auto& light: scene.lights) {
		switch (light->intersect(ray, closestIntersection.dist)) {
			case -1: // non-emitting face of the light
				hitLightColor = Color(0, 0, 0);
				break;
			case 1:  // emitting face of the light
				hitLightColor = light->getColor() * light->getScaleFactor();
				break;
		}
	}
	if (hitLightColor) {
		// this check exists for path tracing: we want to forbid paths
		// camera->...->diffuse->light; we handle these with explicit light sampling
		return (ray.flags & RF_GI_DIFFUSE) ? Color(0, 0, 0) : *hitLightColor;
	}
	// no intersection? fetch from the environment, if any:
	if (closestIntersection.dist >= INF) {
		if (scene.environment) return scene.environment->getEnvironment(ray.dir);
		return scene.settings.backgroundColor;
	}
	// if we intersect a node which has a bump map applied, modify our intersection point normal:
	if (closestNode->bump) {
		closestNode->bump->modifyNormal(closestIntersection);
	}
	return {};
}

Color raytrace(const Ray& ray)
{
	// Ray-tracing:
	TraceContext tc;
	auto earlyResult = tc.raycast(ray);
	if (earlyResult) return *earlyResult;
	// Shading:
	return tc.closestNode->shader->computeColor(ray, tc.closestIntersection);
}

Color pathtrace(const Ray& ray, Color pathMultiplier = Color(1, 1, 1))
{
	// early exit:
	if (pathMultiplier.intensity() < 0.001f) return Color(0, 0, 0);
	// Ray-tracing:
	TraceContext tc;
	auto earlyResult = tc.raycast(ray);
	if (earlyResult) return (*earlyResult) * pathMultiplier;
	// Continue building the path:
	// Option A: continue randomly
	Ray newRay = ray;
	Color brdfColor;
	float brdfPDF;
	newRay.depth++;
	tc.closestNode->shader->spawnRay(tc.closestIntersection, ray.dir, newRay, brdfColor, brdfPDF);
	if (brdfPDF <= 0) return Color(0, 0, 0);
	Color fromGI = pathtrace(newRay, pathMultiplier * brdfColor / brdfPDF);
	//
	// Option B: explicit light sampling
	// scheme:
	// 1) choose a random light
	// 2) pick a random point on that light
	// 3) see if we can link the current intersection point with that light
	// 4) if we can, evaluate the solid angle that this light projects onto the hemisphere
	//    over our intersection point
	// 5) evalute the BRDF (what light do we get from the proposed path extension)
	// 6) if nonzero, add to the result from pathtrace()
	if (scene.lights.empty()) return fromGI;
	//
	Light* light = scene.lights[randInt(0, scene.lights.size() - 1)];
	//
	const Vector& x = tc.closestIntersection.ip;
	double solidAngle = light->getSolidAngle(x);
	if (solidAngle <= 0) return fromGI;
	//
	int sampleIdx = randInt(0, light->getNumSamples() - 1);
	Color unused;
	Vector pointOnLight;
	light->getNthSample(sampleIdx, x, pointOnLight, unused);
	//
	if (!visible(x + tc.closestIntersection.norm * 1e-6, pointOnLight)) return fromGI;
	//
	Vector w_out = pointOnLight - x;
	w_out.normalize();
	Color fromLight =
		light->getColor() *
		light->getScaleFactor() *
		tc.closestNode->shader->eval(tc.closestIntersection, ray.dir, w_out);
	if (fromLight.intensity() <= 0) return fromGI;
	//
	float pChooseLight = 1.0f / scene.lights.size();
	float pHitLight = 1.0f / solidAngle;
	float pThisPath = pChooseLight * pHitLight;
	return fromGI + fromLight * pathMultiplier / pThisPath;
}

bool visible(const Vector& A, const Vector& B)
{
	double D = distance(A, B);
	Ray ray;
	ray.start = A;
	ray.dir = B - A;
	ray.dir.normalize();
	//
	for (auto& node: scene.nodes) {
		IntersectionInfo info;
		if (node->intersect(ray, info) && info.dist < D) {
			return false;
		}
	}
	for (auto& light: scene.lights) {
		if (light->intersect(ray, D)) return false;
	}
	//
	return true;
}

static void detectAApixels()
{
	const int neighbours[8][2] = {
		{ -1, -1 }, { 0, -1 }, { 1, -1 },
		{ -1,  0 },            { 1,  0 },
		{ -1,  1 }, { 0,  1 }, { 1,  1 }
	};
	int W = frameWidth(), H = frameHeight();
	for (auto& r: buckets) {
		for (int y = r.y0; y < r.y1; y++)
			for (int x = r.x0; x < r.x1; x++) {
				needsAA[y][x] = false;
				const Color& me = vfb[y][x];
				for (int ni = 0; ni < COUNT_OF(neighbours); ni++) {
					int neighX = x + neighbours[ni][0];
					int neighY = y + neighbours[ni][1];
					if (neighX < 0 || neighX >= W || neighY < 0 || neighY >= H) continue;
					const Color& neighbour = vfb[neighY][neighX];
					for (int channel = 0; channel < 3; channel++) {
						if (fabs(std::min(1.0f, me[channel]) - std::min(1.0f, neighbour[channel])) > AA_THRESH) {
							needsAA[y][x] = true;
							break;
						}
					}
					if (needsAA[y][x]) break;
				}
			}
	}
}

std::function<Color(Ray)> traceFunction;
std::function<Ray(double, double, double, double, double)> rayGenerator;

Color traceSingleRay(double x, double y, bool fast = false)
{
	//if (sqr(x - vipX) + sqr(y - vipY) < 100) return Color(1, 1, 0);
	double u, v;
	if (scene.camera->dof) unitDiskSample(u, v);
	//sum += raytrace(scene.camera->getDOFScreenRay(x + randDouble(), y + randDouble(), u, v));

	if (fast || scene.camera->stereoSeparation == 0.0) {
		return traceFunction(rayGenerator(x, y, u, v, 0));
	} else {
		Color left = traceFunction(rayGenerator(x, y, u, v, -0.5));
		Color right = traceFunction(rayGenerator(x, y, u, v, +0.5));
		//
		float midLeft = left.intensity();
		float midRight = right.intensity();
		//
		return Color(
			midLeft + (left.r - midLeft) * 0.3f,
			midRight + (right.g - midRight) * 0.3f,
			midRight + (right.b - midRight) * 0.3f
		);
	}
}

std::unique_ptr<ThreadPool> threadPool;

bool renderWithoutMonteCarlo(bool displayProgress) // returns true if the complete frame is rendered
{
	static const float AA_KERNEL[5][2] {
		{ 0.0f, 0.0f },
		{ 0.6f, 0.0f },
		{ 0.3f, 0.3f },
		{ 0.0f, 0.6f },
		{ 0.6f, 0.6f },
	};
	static const int COARSE_KERNEL[5][2] {
		{ 2, 2 },
		{ 2, 6 },
		{ 4, 4 },
		{ 6, 2 },
		{ 6, 6 },
	};
	static const int AA_KERNEL_SIZE = int(COUNT_OF(AA_KERNEL));
	int foveated_thresh = sqr(scene.settings.foveatedRadius);

	// Pass 1: render without anti-aliasing
	std::atomic<int> cursor(0);
	threadPool->run([displayProgress, &cursor, foveated_thresh] (int threadIdx, int threadCount) {
		for (int i = cursor++; i < int(buckets.size()); i = cursor++) {
			auto& r = buckets[i];
			if (scene.settings.foveatedRadius <= 0) {
				// plain old rendering. Raytrace through every single pixel; shoot one ray only
				for (int y = r.y0; y < r.y1; y++) {
					if (checkForUserExit()) return;
					for (int x = r.x0; x < r.x1; x++)
						vfb[y][x] = traceSingleRay(x, y); // should be "x + AA_KERNEL[0][0]", etc.
				}
			} else {
				// foveated rendering. Split the frame into 8x8 blocks, determine which blocks are close to
				// (vipX, vipY), and only render those in full resolution. The others are approximated via
				// 5 raytrace()s only
				for (int blkY = r.y0; blkY < r.y1; blkY += 8) {
					if (checkForUserExit()) return;
					int blkYend = std::min(r.y1, blkY + 8);
					for (int blkX = r.x0; blkX < r.x1; blkX += 8) {
						int blkXend = std::min(r.x1, blkX + 8);
						int cx = blkX + 4, cy = blkY + 4;
						if (sqr(cx - vipX) + sqr(cy - vipY) > foveated_thresh) {
							// coarse quality:
							Color sum(0, 0, 0);
							for (int j = 0; j < 5; j++)
								sum += traceSingleRay(blkX + COARSE_KERNEL[j][1], blkY + COARSE_KERNEL[j][0]);
							sum *= 0.2f;
							for (int y = blkY; y < blkYend; y++)
								for (int x = blkX; x < blkXend; x++)
									vfb[y][x] = sum;
						} else {
							// inside the fovea; full quality
							for (int y = blkY; y < blkYend; y++)
								for (int x = blkX; x < blkXend; x++)
									vfb[y][x] = traceSingleRay(x, y);
						}
					}
				}
			}
			if (displayProgress) displayVFBRect(r, vfb);
		}
	});

	// Do we need AA? if not, we're done
	if (checkForUserExit()) return false;
	if (!scene.settings.wantAA) return true;

	// Pass 2: detect pixels, needing AA:
	detectAApixels();
	// show them:
	if (displayProgress) markAApixels(needsAA);

	// Pass 3: recompute those pixels with the AA kernel:
	cursor = 0;
	threadPool->run([displayProgress, &cursor] (int threadIdx, int threadCount) {
		float mul = 1.0f / AA_KERNEL_SIZE;
		for (int i = cursor++; i < int(buckets.size()); i = cursor++) {
			auto& r = buckets[i];
			if (displayProgress) markRegion(r);
			for (int y = r.y0; y < r.y1; y++) {
				if (checkForUserExit()) return;
				for (int x = r.x0; x < r.x1; x++) if (needsAA[y][x]) {
					for (int i = 1; i < AA_KERNEL_SIZE; i++) // note that we skip index i=0, as we did it in pass 1.
						vfb[y][x] += traceSingleRay(x + AA_KERNEL[i][0], y + AA_KERNEL[i][1]);
					vfb[y][x] *= mul;
				}
			}
			if (displayProgress) displayVFBRect(r, vfb);
		}
	});

	return !checkForUserExit();
}

bool renderWithMonteCarlo(bool displayProgress, int raysPerPixel) // returns true if the complete frame is rendered
{
	// compute the auto-focus, if required:
	if (scene.camera->dof && scene.camera->autoFocus) {
		Ray midRay = scene.camera->getScreenRay(frameWidth() * 0.5, frameHeight() * 0.5);
		double closestIntersectionDist = INF;
		//
		for (auto& node: scene.nodes) {
			IntersectionInfo info;
			if (node->intersect(midRay, info) && info.dist < closestIntersectionDist)
				closestIntersectionDist = info.dist;
		}
		//
		if (closestIntersectionDist < INF)
			scene.camera->focalPlaneDist = closestIntersectionDist;
	}
	// render the image (only one pass with many rays per pixel)
	std::atomic<int> cursor(0);
	threadPool->run([displayProgress, raysPerPixel, &cursor] (int threadIdx, int threadCount) {
		float mul = 1.0f / raysPerPixel;
		for (int i = cursor++; i < int(buckets.size()); i = cursor++) {
			auto& r = buckets[i];
			for (int y = r.y0; y < r.y1; y++) {
				if (checkForUserExit()) return;
				for (int x = r.x0; x < r.x1; x++) {
					Color sum(0, 0, 0);
					for (int i = 0; i < raysPerPixel; i++) {
						sum += traceSingleRay(x + randDouble(), y + randDouble());
					}
					vfb[y][x] = sum * mul;
				}
			}
			if (displayProgress) displayVFBRect(r, vfb);
		}
	});

	return true;
}

/// render the whole frame at much lower resolution (using blocks 24x24px, and estimate each via a few rays)
bool coarseRender()
{
	float mul = 1.0f / scene.settings.prepassSamples;
	const int BLK_SIZE = 24;
	Uint32 lastUpdate = 0;
	int lastY = 0;
	for (int y = 0; y < frameHeight(); y += BLK_SIZE) {
		for (int x = 0; x < frameWidth(); x += BLK_SIZE) {
			Color sum(0, 0, 0);
			for (int i = 0; i < scene.settings.prepassSamples; i++) {
				sum += traceSingleRay(x + randDouble() * BLK_SIZE, y + randDouble() * BLK_SIZE);
			}
			Rect r(x, y, min(x + BLK_SIZE, frameWidth()), min(y + BLK_SIZE, frameHeight()));
			drawRect(r, sum * mul);
			// display on screen:
			Uint32 t = SDL_GetTicks();
			if (t - lastUpdate > 200) {
				Rect updateRect(0, lastY, frameWidth(), min(y + BLK_SIZE, frameHeight()));
				showUpdated(updateRect);
				lastY = y;
				lastUpdate = t;
			}
		}
		if (checkForUserExit()) return false;
	}
	showUpdated(Rect(0, 0, frameWidth(), frameHeight()));
	return true;
}

bool render(bool displayProgress)
{
	if (displayProgress && scene.settings.prepassSamples > 0) {
		if (!coarseRender()) return false;
	}
	int raysPerPixel = 0;
	if (scene.camera->dof) raysPerPixel = scene.camera->numSamples;
	if (scene.settings.gi) raysPerPixel = std::max(raysPerPixel, scene.settings.numPaths);
	if (raysPerPixel > 0) return renderWithMonteCarlo(displayProgress, raysPerPixel);
	else return renderWithoutMonteCarlo(displayProgress);
}

// makes sure we see the "data" dir:
static void ensureDataIsVisible()
{
	namespace fs = std::filesystem;
	// handle the common case where we launch "hexray" from within "hexray/build"
	if (!fs::exists("data") && fs::exists("../data"))
		fs::current_path("..");
	// otherwise, we can't continue:
	if (!fs::exists("data")) {
		printf("Error: the \"data\" directory is not visible!\n");
		printf("(make sure you run hexray from the correct directory; either \"hexray\" or \"hexray/build\")\n");
		exit(1);
	}
}

void gameloop()
{
	scene.beginRender();
	Camera& cam = *scene.camera;
	const double MOVEMENT_PER_SEC = 20;
	const double ROTATION_PER_SEC = 50;
	const double SENSITIVITY = 0.1;
	Uint32 startTicks = SDL_GetTicks();
	int numFrames = 0;
	bool runMode = false;
	while (!checkForUserExit()) {
		scene.beginFrame();
		Uint32 frameStart = SDL_GetTicks();
		render(false);
		displayVFB(vfb);
		double timeDelta = (SDL_GetTicks() - frameStart) / 1000.0;
		numFrames++;
		//
		const Uint8* keystate;
		int deltax, deltay;
		std::vector<SDL_Event> events;
		getSDLInputs(keystate, deltax, deltay, events);
		//
		double movement = MOVEMENT_PER_SEC * timeDelta;
		if (runMode) movement *= 3;
		double rotation = ROTATION_PER_SEC * timeDelta;
		// arrow keys movement:
		if (keystate[SDL_SCANCODE_UP]) cam.move(0, +movement);
		if (keystate[SDL_SCANCODE_DOWN]) cam.move(0, -movement);
		if (keystate[SDL_SCANCODE_LEFT]) cam.move(-movement, 0);
		if (keystate[SDL_SCANCODE_RIGHT]) cam.move(+movement, 0);

		// keypad arrow keys look around:
		if (keystate[SDL_SCANCODE_KP_8]) cam.rotate(0, +rotation);
		if (keystate[SDL_SCANCODE_KP_2]) cam.rotate(0, -rotation);
		if (keystate[SDL_SCANCODE_KP_4]) cam.rotate(+rotation, 0);
		if (keystate[SDL_SCANCODE_KP_6]) cam.rotate(-rotation, 0);

		// mouse look around
		if (scene.settings.foveatedRadius <= 0)
			cam.rotate(-SENSITIVITY * deltax, -SENSITIVITY * deltay);

		// handle keyboard button presses (non-movement, non-hold events)
		for (auto& ev: events) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_r:
						runMode = !runMode;
						break;
					case SDLK_F5:
						printf("Last frame took %.3fs\n", timeDelta);
						break;
				}
			} else if (ev.type == SDL_MOUSEMOTION) {
				vipX = ev.motion.x;
				vipY = ev.motion.y;
			}
		}
	}
	Uint32 elapsedTicks = SDL_GetTicks() - startTicks;
	printf("%d frames in %u ms: %.2f FPS\n", numFrames, elapsedTicks, numFrames / (elapsedTicks * 0.001));
}

bool renderStatic()
{
	scene.beginRender();
	scene.beginFrame();
	return render(true);
}

void renderThreadEntry() {
	// split the screen into regions:
	buckets = getBucketsList(scene.settings.interactive ? 16 : 64);
	// render:
	Uint32 start = SDL_GetTicks();
	if (scene.settings.interactive) {
		isInteractive = true;
		gameloop();
	} else if (renderStatic()) {
		Uint32 end = SDL_GetTicks();
		displayVFB(vfb);
		printf("Elapsed time: %.2f seconds.\n", (end - start) / 1000.0);
	}
}

const char* DEFAULT_SCENE = "data/simple.hexray";

int main(int argc, char** argv)
{
	// setup:
	Color::init_sRGB_cache();
	ensureDataIsVisible();
	// parse the scene:
	const char* sceneFile = DEFAULT_SCENE;
	if (argc > 1 && strlen(argv[1]) && argv[1][0] != '-') sceneFile = argv[1];
	if (!scene.parseScene(sceneFile)) {
		printf("Could not parse the scene file (%s)!\n", sceneFile);
		return 1;
	}
	// configure the thread pool:
	if (scene.settings.numThreads <= 0) scene.settings.numThreads = std::thread::hardware_concurrency();
	printf("Rendering on %d threads\n", scene.settings.numThreads);
	threadPool = std::make_unique<ThreadPool>(scene.settings.numThreads);
	// configure the functions for ray generation and ray tracing:
	traceFunction = raytrace;
	if (scene.settings.gi) traceFunction = [] (Ray ray) { return pathtrace(ray); };
	rayGenerator = scene.camera->dof ?
		[] (double x, double y, double u, double v, double stereoOffset) {
			return scene.camera->getDOFScreenRay(x, y, u, v, stereoOffset);
		} :
		[] (double x, double y, double u, double v, double stereoOffset) {
			return scene.camera->getScreenRay(x, y, stereoOffset);
		};
	// open up the window
	initGraphics(scene.settings.frameWidth, scene.settings.frameHeight);
	void uiMainLoop(void);
	std::thread renderThread(renderThreadEntry);
	uiMainLoop();
	// in case of early exit the render thread has to be waited before closing the
	// graphics, because it may still try to show the VFB
	renderThread.join();
	// close the window
	closeGraphics();
	printf("Exited cleanly\n");
	return 0;
}
