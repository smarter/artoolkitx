//
//  main.cpp
//  artoolkitX Square Tracking Example
//
//  Copyright 2018 Realmax, Inc. All Rights Reserved.
//
//  Author(s): Philip Lamb
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its
//  contributors may be used to endorse or promote products derived from this
//  software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef DEBUG
#  ifdef _WIN32
#    define MAXPATHLEN MAX_PATH
#    include <direct.h>               // _getcwd()
#    define getcwd _getcwd
#  else
#    include <unistd.h>
#    include <sys/param.h>
#  endif
#endif
#include <string>

#include <ARX/ARController.h>
#include <ARX/ARUtil/time.h>
#include <ARX/AR/icpCore.h>
#include <ARX/AR/icpCalib.h>
#include <ARX/AR/icp.h>
#include <ARX/AR/paramGL.h>

#ifdef __APPLE__
#  include <SDL2/SDL.h>
#  if (HAVE_GL || HAVE_GL3)
#    include <SDL2/SDL_opengl.h>
#  elif HAVE_GLES2
#    include <SDL2/SDL_opengles2.h>
#  endif
#else
#  include "SDL2/SDL.h"
#  if (HAVE_GL || HAVE_GL3)
#    include "SDL2/SDL_opengl.h"
#  elif HAVE_GLES2
#    include "SDL2/SDL_opengles2.h"
#  endif
#endif

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc.hpp"

#include "draw.h"

#if ARX_TARGET_PLATFORM_WINDOWS
const char *vconfL = "-module=LeapMotion -no-deallocate";
const char *vconfR = "-module=LeapMotion -no-deallocate -stereo_part=right";
#else
const char *vconfL = NULL;
const char *vconfR = NULL;
#endif
const char *cparaL = "C:\\Users\\smarter\\leftx.dat";
const char *cparaR = "C:\\Users\\smarter\\rightx.dat";

// Window and GL context.
static SDL_GLContext gSDLContext = NULL;
static int contextWidth = 0;
static int contextHeight = 0;
static bool contextWasUpdated = false;
static int32_t viewportL[4];
static int32_t viewportR[4];
static float projection[16];
static SDL_Window* gSDLWindow = NULL;

static ARController* arController = NULL;
static ARG_API drawAPI = ARG_API_None;


int                  chessboardCornerNumX = 7;
int                  chessboardCornerNumY = 5;
int                  calibImageNum = 10;
int                  capturedImageNum = 0;
float                patternWidth = 28.5f;
int                  cornerFlag = 0;
CvPoint2D32f        *cornersL;
CvPoint2D32f        *cornersR;
IplImage            *calibImageL;
IplImage            *calibImageR;
ICPCalibDataT       *calibData;
ICP3DCoordT         *worldCoord;

static AR2VideoBufferT *gVideoBuffL = NULL;
static AR2VideoBufferT *gVideoBuffR = NULL;


ARVideoSource *sourceL = new ARVideoSource;
ARVideoSource *sourceR = new ARVideoSource;

ARVideoView *viewL = new ARVideoView;
ARVideoView *viewR = new ARVideoView;

ARParam paramL;
ARParam paramR;

AR2VideoParamT      *vidL;
AR2VideoParamT      *vidR;
int                  xsizeL, ysizeL;
int                  xsizeR, ysizeR;
int                  pixFormatL;
int                  pixFormatR;

static long gFrameNo = 0;

struct marker {
    const char *name;
    float height;
};
static const struct marker markers[] = {
    {"hiro.patt", 80.0},
    {"kanji.patt", 80.0}
};
static const int markerCount = (sizeof(markers)/sizeof(markers[0]));

//
//
//

static void processCommandLineOptions(int argc, char *argv[]);
static void usage(char *com);
static void quit(int rc);
static void reshape(int w, int h);

#define MAXPATHLEN 128

static void calib(void)
{
	//COVHI10400, COVHI10352
	ICPHandleT *icpHandleL = NULL;
	ICPHandleT *icpHandleR = NULL;
	ICPDataT    icpData;
	ARdouble    initMatXw2Xc[3][4];
	ARdouble    initTransL2R[3][4], matL[3][4], matR[3][4], invMatL[3][4];
	ARdouble    transL2R[3][4];
	ARdouble    err;
	int         i;

	if ((icpHandleL = icpCreateHandle(paramL.mat)) == NULL) {
		ARLOGi("Error!! icpCreateHandle\n");
		goto done;
	}
	icpSetBreakLoopErrorThresh(icpHandleL, 0.00001);

	if ((icpHandleR = icpCreateHandle(paramR.mat)) == NULL) {
		ARLOGi("Error!! icpCreateHandle\n");
		goto done;
	}
	icpSetBreakLoopErrorThresh(icpHandleR, 0.00001);

	for (i = 0; i < calibImageNum; i++) {
		if (icpGetInitXw2Xc_from_PlanarData(paramL.mat, calibData[i].screenCoordL, calibData[i].worldCoordL, calibData[i].numL,
			calibData[i].initMatXw2Xcl) < 0) {
			ARLOGi("Error!! icpGetInitXw2Xc_from_PlanarData\n");
			goto done;
		}
		icpData.screenCoord = calibData[i].screenCoordL;
		icpData.worldCoord = calibData[i].worldCoordL;
		icpData.num = calibData[i].numL;
	}

	if (icpGetInitXw2Xc_from_PlanarData(paramL.mat, calibData[0].screenCoordL, calibData[0].worldCoordL, calibData[0].numL,
		initMatXw2Xc) < 0) {
		ARLOGi("Error!! icpGetInitXw2Xc_from_PlanarData\n");
		goto done;
	}
	icpData.screenCoord = calibData[0].screenCoordL;
	icpData.worldCoord = calibData[0].worldCoordL;
	icpData.num = calibData[0].numL;
	if (icpPoint(icpHandleL, &icpData, initMatXw2Xc, matL, &err) < 0) {
		ARLOGi("Error!! icpPoint\n");
		goto done;
	}
	if (icpGetInitXw2Xc_from_PlanarData(paramR.mat, calibData[0].screenCoordR, calibData[0].worldCoordR, calibData[0].numR,
		matR) < 0) {
		ARLOGi("Error!! icpGetInitXw2Xc_from_PlanarData\n");
		goto done;
	}
	icpData.screenCoord = calibData[0].screenCoordR;
	icpData.worldCoord = calibData[0].worldCoordR;
	icpData.num = calibData[0].numR;
	if (icpPoint(icpHandleR, &icpData, initMatXw2Xc, matR, &err) < 0) {
		ARLOGi("Error!! icpPoint\n");
		goto done;
	}
	arUtilMatInv(matL, invMatL);
	arUtilMatMul(matR, invMatL, initTransL2R);

	if (icpCalibStereo(calibData, calibImageNum, paramL.mat, paramR.mat, initTransL2R, transL2R, &err) < 0) {
		ARLOGi("Calibration error!!\n");
		goto done;
	}
	ARLOGi("Estimated transformation matrix from Left to Right.\n");
	arParamDispExt(transL2R);

	arParamSaveExt("C:\\Users\\smarter\\transL2R.dat", transL2R);

done:
	free(icpHandleL);
	free(icpHandleR);
}


int main(int argc, char *argv[])
{
	ARParam            wparam;

    processCommandLineOptions(argc, argv);
    
    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        ARLOGe("Error: SDL initialisation failed. SDL error: '%s'.\n", SDL_GetError());
        quit(1);
        return 1;
    }

    // Create a window.
#if 0
    gSDLWindow = SDL_CreateWindow("artoolkitX Square Tracking Example",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              0, 0,
                              SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                              );
#else
    gSDLWindow = SDL_CreateWindow("artoolkitX Square Tracking Example",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  1280, 720,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                                  );
#endif
    if (!gSDLWindow) {
        ARLOGe("Error creating window: %s.\n", SDL_GetError());
        quit(-1);
    }

    // Create an OpenGL context to draw into. If OpenGL 3.2 not available, attempt to fall back to OpenGL 1.5, then OpenGL ES 2.0
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // This is the default.
    SDL_GL_SetSwapInterval(1);
#if HAVE_GL3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    gSDLContext = SDL_GL_CreateContext(gSDLWindow);
    if (gSDLContext) {
        drawAPI = ARG_API_GL3;
        ARLOGi("Created OpenGL 3.2+ context.\n");
    } else {
        ARLOGi("Unable to create OpenGL 3.2 context: %s. Will try OpenGL 1.5.\n", SDL_GetError());
#endif // HAVE_GL3
#if HAVE_GL
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
        gSDLContext = SDL_GL_CreateContext(gSDLWindow);
        if (gSDLContext) {
            drawAPI = ARG_API_GL;
            ARLOGi("Created OpenGL 1.5+ context.\n");
#  if ARX_TARGET_PLATFORM_MACOS
        vconf = "-format=BGRA";
#  endif
        } else {
            ARLOGi("Unable to create OpenGL 1.5 context: %s. Will try OpenGL ES 2.0\n", SDL_GetError());
#endif // HAVE_GL
#if HAVE_GLES2
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            gSDLContext = SDL_GL_CreateContext(gSDLWindow);
            if (gSDLContext) {
                drawAPI = ARG_API_GLES2;
                ARLOGi("Created OpenGL ES 2.0+ context.\n");
            } else {
                ARLOGi("Unable to create OpenGL ES 2.0 context: %s.\n", SDL_GetError());
            }
#endif // HAVE_GLES2
#if HAVE_GL
        }
#endif
#if HAVE_GL3
    }
#endif
    if (drawAPI == ARG_API_None) {
        ARLOGe("No OpenGL context available. Giving up.\n", SDL_GetError());
        quit(-1);
    }

    int w, h;
    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &w, &h);
    reshape(w, h);

    // Initialise the ARController.
    arController = new ARController();
    if (!arController->initialiseBase()) {
        ARLOGe("Error initialising ARController.\n");
        quit(-1);
    }

#ifdef DEBUG
    arLogLevel = AR_LOG_LEVEL_DEBUG;
#endif

    // Add trackables.
    int markerIDs[markerCount];
    int markerModelIDs[markerCount];
#ifdef DEBUG
    char buf[MAXPATHLEN];
    ARLOGd("CWD is '%s'.\n", getcwd(buf, sizeof(buf)));
#endif
    char *resourcesDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST);
    for (int i = 0; i < markerCount; i++) {
        std::string markerConfig = "single;" + std::string(resourcesDir) + '/' + markers[i].name + ';' + std::to_string(markers[i].height);
        markerIDs[i] = arController->addTrackable(markerConfig);
        if (markerIDs[i] == -1) {
            ARLOGe("Error adding marker.\n");
            quit(-1);
        }
    }
    arController->getSquareTracker()->setPatternDetectionMode(AR_TEMPLATE_MATCHING_MONO);
    arController->getSquareTracker()->setThresholdMode(AR_LABELING_THRESH_MODE_AUTO_BRACKETING);

#ifdef DEBUG
	ARLOGd("vconf1 is '%s'.\n", vconf1);
	ARLOGd("vconf2 is '%s'.\n", vconf2);
#endif
    // Start tracking.
    //arController->startRunning(vconf1, cpara1, NULL, 0);
	//arController->startRunningStereo(vconf1, cpara1, NULL, 0, vconf2, cpara2, NULL, 0, )

	sourceL->configure(vconfL, false, cparaL, NULL, 0);
	sourceR->configure(vconfR, false, cparaR, NULL, 0);

	if (!sourceL->open()) {
		ARLOGe("Error: %d\n", sourceL->getError());
		exit(1);
	}
	if (!sourceR->open()) {
		ARLOGe("Error: %d\n", sourceR->getError());
		exit(1);
	}
	/*

	if ((vidL = ar2VideoOpen(vconfL)) == NULL) {
		ARLOGe("Cannot found the first camera.\n");
		exit(0);
	}
	if ((vidR = ar2VideoOpen(vconfR)) == NULL) {
		ARLOGe("Cannot found the second camera.\n");
		exit(0);
	}
	if (ar2VideoGetSize(vidL, &xsizeL, &ysizeL) < 0) exit(0);
	if (ar2VideoGetSize(vidR, &xsizeR, &ysizeR) < 0) exit(0);
	if ((pixFormatL = ar2VideoGetPixelFormat(vidL)) < 0) exit(0);
	if ((pixFormatR = ar2VideoGetPixelFormat(vidR)) < 0) exit(0);
	ARLOGi("Image size for the left camera  = (%d,%d)\n", xsizeL, ysizeL);
	ARLOGi("Image size for the right camera = (%d,%d)\n", xsizeR, ysizeR);

	if (arParamLoad(cparaL, 1, &wparam) < 0) {
		ARLOGe("Camera parameter load error !!   %s\n", cparaL);
		exit(0);
	}
	arParamChangeSize(&wparam, xsizeL, ysizeL, &paramL);
	ARLOGi("*** Camera Parameter for the left camera ***\n");
	arParamDisp(&paramL);
	if (arParamLoad(cparaR, 1, &wparam) < 0) {
		ARLOGe("Camera parameter load error !!   %s\n", cparaR);
		exit(0);
	}
	arParamChangeSize(&wparam, xsizeR, ysizeR, &paramR);
	ARLOGi("*** Camera Parameter for the right camera ***\n");
	arParamDisp(&paramR);*/

	int width = 640;
	int height = 240;

	calibImageL = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	calibImageR = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	arMalloc(cornersL, CvPoint2D32f, chessboardCornerNumX*chessboardCornerNumY);
	arMalloc(cornersR, CvPoint2D32f, chessboardCornerNumX*chessboardCornerNumY);
	arMalloc(worldCoord, ICP3DCoordT, chessboardCornerNumX*chessboardCornerNumY);
	for (int i = 0; i < chessboardCornerNumX; i++) {
		for (int j = 0; j < chessboardCornerNumY; j++) {
			worldCoord[i*chessboardCornerNumY + j].x = patternWidth * i;
			worldCoord[i*chessboardCornerNumY + j].y = patternWidth * j;
			worldCoord[i*chessboardCornerNumY + j].z = 0.0;
		}
	}
	arMalloc(calibData, ICPCalibDataT, calibImageNum);
	for (int i = 0; i < calibImageNum; i++) {
		arMalloc(calibData[i].screenCoordL, ICP2DCoordT, chessboardCornerNumX*chessboardCornerNumY);
		arMalloc(calibData[i].screenCoordR, ICP2DCoordT, chessboardCornerNumX*chessboardCornerNumY);
		calibData[i].worldCoordL = worldCoord;
		calibData[i].worldCoordR = worldCoord;
		calibData[i].numL = chessboardCornerNumX * chessboardCornerNumY;
		calibData[i].numR = chessboardCornerNumX * chessboardCornerNumY;
	}
	//ar2VideoCapStart(vidL);
	//ar2VideoCapStart(vidR);

	//sourceL->checkinFrame();
	//sourceR->checkinFrame();

	viewL->initWithVideoSource(*sourceL, width*2, height);
	viewL->setScalingMode(ARVideoView::ScalingMode::SCALE_MODE_1_TO_1);
	viewL->setHorizontalAlignment(ARVideoView::HorizontalAlignment::H_ALIGN_LEFT);
	viewL->setVerticalAlignment(ARVideoView::VerticalAlignment::V_ALIGN_TOP);
	viewL->getViewport(viewportL);

	viewR->initWithVideoSource(*sourceR, width*2, height);
	viewR->setScalingMode(ARVideoView::ScalingMode::SCALE_MODE_1_TO_1);
	viewR->setHorizontalAlignment(ARVideoView::HorizontalAlignment::H_ALIGN_RIGHT);
	viewR->setVerticalAlignment(ARVideoView::VerticalAlignment::V_ALIGN_TOP);
	viewR->getViewport(viewportR);

	drawSetup(drawAPI, false, false, false);
	printf("L: %d %d %d %d\n", viewportL[0], viewportL[1], viewportL[2], viewportL[3]);
	printf("R: %d %d %d %d\n", viewportR[0], viewportR[1], viewportR[2], viewportR[3]);
	drawSetViewport(viewportR);

	//arController->projectionMatrix(0, 10.0f, 10000.0f, projectionARD);

	ARParamLT *paramLTL = sourceL->getCameraParameters();
	ARParamLT *paramLTR = sourceR->getCameraParameters();
	paramL = paramLTL->param;
	paramR = paramLTR->param;
	ARdouble projectionNearPlane = 10.0f;
	ARdouble projectionFarPlane = 10000.0f;
	ARdouble projectionARD[16];
	arglCameraFrustumRH(&(paramLTL->param), 10.0f, 10000.0f, projectionARD);
	for (int i = 0; i < 16; i++) projection[i] = (float)projectionARD[i];
	drawSetCamera(projection, NULL);

    // Main loop.
    bool done = false;
    while (!done) {
        //bool gotFrame = arController->capture();

		AR2VideoBufferT *videoBuffL;
		AR2VideoBufferT *videoBuffR;
		ARUint8         *dataPtrL;
		ARUint8         *dataPtrR;
		int              cornerFlagL;
		int              cornerFlagR;
		int              cornerCountL;
		int              cornerCountR;
		int i;
/*
		if ((videoBuffL = ar2VideoGetImage(vidL))) {
			gVideoBuffL = videoBuffL;
		}
		if ((videoBuffR = ar2VideoGetImage(vidR))) {
			gVideoBuffR = videoBuffR;
		}*/
		bool captureL = sourceL->captureFrame();
		bool captureR = sourceR->captureFrame();

        if (!captureL || !captureR) {
            arUtilSleep(1);
			continue;
        }

		//ARLOGi("Got frame %ld.\n", gFrameNo);
		gFrameNo++;

		// Warn about significant time differences.
		/*i = ((int)gVideoBuffR->time.sec - (int)gVideoBuffL->time.sec) * 1000
			+ ((int)gVideoBuffR->time.usec - (int)gVideoBuffL->time.usec) / 1000;
		if (i > 20) {
			ARLOGi("Time diff = %d[msec]\n", i);
		}
		else if (i < -20) {
			ARLOGi("Time diff = %d[msec]\n", i);
		}

		dataPtrL = gVideoBuffL->buff;
		dataPtrR = gVideoBuffR->buff;*/

		/*
		if (!arController->update()) {
			ARLOGe("Error in ARController::update().\n");
			quit(-1);
		}*/
		/*
		if (contextWasUpdated) {

			if (!arController->drawVideoInit(0)) {
				ARLOGe("Error in ARController::drawVideoInit().\n");
				quit(-1);
			}
			if (!arController->drawVideoSettings(0, contextWidth, contextHeight, false, false, false, ARVideoView::HorizontalAlignment::H_ALIGN_CENTRE, ARVideoView::VerticalAlignment::V_ALIGN_CENTRE, ARVideoView::ScalingMode::SCALE_MODE_FIT, viewport)) {
				ARLOGe("Error in ARController::drawVideoSettings().\n");
				quit(-1);
			}
			drawSetup(drawAPI, false, false, false);

			drawSetViewport(viewport);
			ARdouble projectionARD[16];
			arController->projectionMatrix(0, 10.0f, 10000.0f, projectionARD);
			for (int i = 0; i < 16; i++) projection[i] = (float)projectionARD[i];
			drawSetCamera(projection, NULL);

			for (int i = 0; i < markerCount; i++) {
				markerModelIDs[i] = drawLoadModel(NULL);
			}
			contextWasUpdated = false;
		}*/

		SDL_GL_MakeCurrent(gSDLWindow, gSDLContext);

		// Clear the context.
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Display the current video frame to the current OpenGL context.
		//arController->drawVideo(0);
		viewL->draw(sourceL);
		viewR->draw(sourceR);

		draw();
		SDL_GL_SwapWindow(gSDLWindow);

		AR2VideoBufferT *imageL, *imageR = NULL;
		imageL = sourceL->checkoutFrameIfNewerThan({ 0, 0 });
		imageR = sourceR->checkoutFrameIfNewerThan({ 0, 0 });

		memcpy(calibImageL->imageData, imageL->buffLuma, 640 * 240);
		memcpy(calibImageR->imageData, imageR->buffLuma, 640 * 240);

		sourceL->checkinFrame();
		sourceR->checkinFrame();


		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)) {
				done = true;
				break;
			}
			else if (ev.type == SDL_WINDOWEVENT) {
				//ARLOGd("Window event %d.\n", ev.window.event);
				if (ev.window.event == SDL_WINDOWEVENT_RESIZED && ev.window.windowID == SDL_GetWindowID(gSDLWindow)) {
					//int32_t w = ev.window.data1;
					//int32_t h = ev.window.data2;
					int w, h;
					SDL_GL_GetDrawableSize(gSDLWindow, &w, &h);
					reshape(w, h);
				}
			}
			else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_d) {

				cornerFlagL = cvFindChessboardCorners(calibImageL, cvSize(chessboardCornerNumY, chessboardCornerNumX),
					cornersL, &cornerCountL, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);

				cornerFlagR = cvFindChessboardCorners(calibImageR, cvSize(chessboardCornerNumY, chessboardCornerNumX),
					cornersR, &cornerCountR, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);

				printf("found: %d %d\n", cornerCountL, cornerCountL);
				if (!cornerFlagL || !cornerFlagL)
					continue;

				printf("%f, %f %f, %f\n", cornersL[0].x, cornersL[0].y, cornersL[1].x, cornersL[1].y);

				cvFindCornerSubPix(calibImageL, cornersL, chessboardCornerNumX*chessboardCornerNumY, cvSize(5, 5),
					cvSize(-1, -1), cvTermCriteria(CV_TERMCRIT_ITER, 100, 0.1));
				for (i = 0; i < chessboardCornerNumX*chessboardCornerNumY; i++) {
					arParamObserv2Ideal(paramL.dist_factor, (double)cornersL[i].x, (double)cornersL[i].y,
						&calibData[capturedImageNum].screenCoordL[i].x, &calibData[capturedImageNum].screenCoordL[i].y, paramL.dist_function_version);
				}
				cvFindCornerSubPix(calibImageR, cornersR, chessboardCornerNumX*chessboardCornerNumY, cvSize(5, 5),
					cvSize(-1, -1), cvTermCriteria(CV_TERMCRIT_ITER, 100, 0.1));
				for (i = 0; i < chessboardCornerNumX*chessboardCornerNumY; i++) {
					arParamObserv2Ideal(paramR.dist_factor, (double)cornersR[i].x, (double)cornersR[i].y,
						&calibData[capturedImageNum].screenCoordR[i].x, &calibData[capturedImageNum].screenCoordR[i].y, paramR.dist_function_version);
				}
				ARLOGi("---------- %2d/%2d -----------\n", capturedImageNum + 1, calibImageNum);
				for (i = 0; i < chessboardCornerNumX*chessboardCornerNumY; i++) {
					ARLOGi("  %f, %f  ----   %f, %f\n", calibData[capturedImageNum].screenCoordL[i].x, calibData[capturedImageNum].screenCoordL[i].y,
						calibData[capturedImageNum].screenCoordR[i].x, calibData[capturedImageNum].screenCoordR[i].y);
				}
				ARLOGi("---------- %2d/%2d -----------\n", capturedImageNum + 1, calibImageNum);
				capturedImageNum++;

				if (capturedImageNum == calibImageNum) {
					calib();
				}
			}
		}

		// Look for trackables, and draw on each found one.
		/*for (int i = 0; i < markerCount; i++) {

			// Find the trackable for the given trackable ID.
			ARTrackable *marker = arController->findTrackable(markerIDs[i]);
			float view[16];
			if (marker->visible) {
				//arUtilPrintMtx16(marker->transformationMatrix);
				for (int i = 0; i < 16; i++) view[i] = (float)marker->transformationMatrix[i];
			}
			drawSetModel(markerModelIDs[i], marker->visible, view);
		}*/
    } // while (!done)

    free(resourcesDir);
    
    quit(0);
    return 0;
}

static void processCommandLineOptions(int argc, char *argv[])
{
    int i, gotTwoPartOption;
    
    //
    // Command-line options.
    //
    
    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
			if (strcmp(argv[i], "--vconf1") == 0) {
				i++;
				vconfL = argv[i];
				gotTwoPartOption = TRUE;
			} else if (strcmp(argv[i], "--vconf2") == 0) {
				i++;
				vconfR = argv[i];
				gotTwoPartOption = TRUE;
			} else if (strcmp(argv[i], "--cpara1") == 0) {
				i++;
				cparaL = argv[i];
				gotTwoPartOption = TRUE;
			} else if (strcmp(argv[i], "--cpara2") == 0) {
				i++;
				cparaR = argv[i];
				gotTwoPartOption = TRUE;
			}
        }
        if (!gotTwoPartOption) {
            // Look for single-part options.
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
                usage(argv[0]);
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
                ARPRINT("%s version %s\n", argv[0], AR_HEADER_VERSION_STRING);
                exit(0);
            } else if( strncmp(argv[i], "-loglevel=", 10) == 0 ) {
                if (strcmp(&(argv[i][10]), "DEBUG") == 0) arLogLevel = AR_LOG_LEVEL_DEBUG;
                else if (strcmp(&(argv[i][10]), "INFO") == 0) arLogLevel = AR_LOG_LEVEL_INFO;
                else if (strcmp(&(argv[i][10]), "WARN") == 0) arLogLevel = AR_LOG_LEVEL_WARN;
                else if (strcmp(&(argv[i][10]), "ERROR") == 0) arLogLevel = AR_LOG_LEVEL_ERROR;
                else usage(argv[0]);
            } else {
                ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
                usage(argv[0]);
            }
        }
        i++;
    }
}

static void usage(char *com)
{
    ARPRINT("Usage: %s [options]\n", com);
    ARPRINT("Options:\n");
    ARPRINT("  --vconf <video parameter for the camera>\n");
    ARPRINT("  --cpara <camera parameter file for the camera>\n");
    ARPRINT("  --version: Print artoolkitX version and exit.\n");
    ARPRINT("  -loglevel=l: Set the log level to l, where l is one of DEBUG INFO WARN ERROR.\n");
    ARPRINT("  -h -help --help: show this message\n");
    exit(0);
}

static void quit(int rc)
{
    drawCleanup();
    if (arController) {
        arController->drawVideoFinal(0);
        arController->shutdown();
        delete arController;
    }
    if (gSDLContext) {
        SDL_GL_MakeCurrent(0, NULL);
        SDL_GL_DeleteContext(gSDLContext);
    }
    if (gSDLWindow) {
        SDL_DestroyWindow(gSDLWindow);
    }
    SDL_Quit();
    exit(rc);
}

static void reshape(int w, int h)
{
    contextWidth = w;
    contextHeight = h;
    ARLOGd("Resized to %dx%d.\n", w, h);
    contextWasUpdated = true;
}

