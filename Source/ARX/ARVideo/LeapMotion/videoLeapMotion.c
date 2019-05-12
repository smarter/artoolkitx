/*
 *  videoDummy.c
 *  artoolkitX
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include "videoLeapMotion.h"

#ifdef ARVIDEO_INPUT_LEAPMOTION

#include "ExampleConnection.h"

#include <string.h> // memset()
#ifdef _WIN32
#  define _USE_MATH_DEFINES
#endif
#include <math.h>

#define AR_VIDEO_LEAPMOTION_XSIZE   640
#define AR_VIDEO_LEAPMOTION_YSIZE   240

#define AR_VIDEO_LEAPMOTION_RECTIFIED_XSIZE   400
#define AR_VIDEO_LEAPMOTION_RECTIFIED_YSIZE   400

#define ARVIDEO_INPUT_LEAPMOTION_DEFAULT_PIXEL_FORMAT   AR_PIXEL_FORMAT_MONO

#define ARVIDEO_INPUT_LEAPMOTION_LEFT_STEREO_PART 0
#define ARVIDEO_INPUT_LEAPMOTION_RIGHT_STEREO_PART 1
#define ARVIDEO_INPUT_LEAPMOTION_DEFAULT_STEREO_PART ARVIDEO_INPUT_LEAPMOTION_LEFT_STEREO_PART

static LEAP_CONNECTION *connection = NULL;
static int connectionRef = 0;

static void *allocate(uint32_t size, eLeapAllocatorType typeHint, void *state)
{
	void* ptr = malloc(size);
	return ptr;
}

static void deallocate(void *ptr, void* state)
{
	if (!ptr)
		return;
	//LockMutex(&dataLock);
	free(ptr);
	//UnlockMutex(&dataLock);
}

static LEAP_ALLOCATOR allocator = { allocate, deallocate, NULL };

// Seems to be the same in every Leap Motion device, so no need to fetch them dynamically
static const int v_fov = 2.007129;
static const int h_fov = 2.303835;

int ar2VideoDispOptionLeapMotion( void )
{
    ARPRINT(" -module=LeapMotion\n");
    ARPRINT("\n");
    ARPRINT(" -stereo_part=N\n");
    ARPRINT("    Either \"left\" or \"right\" stereo part, default to \"left\".\n");
    ARPRINT(" -rectified\n");
    ARPRINT("    Rectify image distortion.\n");
    ARPRINT(" -gain=N\n");
    ARPRINT("    Multiply each output pixel by this floating-point value.\n");
    ARPRINT("\n");

    return 0;
}

AR2VideoParamLeapMotionT *ar2VideoOpenLeapMotion( const char *config )
{
    AR2VideoParamLeapMotionT *vid;
    const char *a;
#   define B_SIZE ((unsigned int)256)
    char b[B_SIZE];
    int bufSizeX;
    int bufSizeY;
    char bufferpow2 = 0;

    //ConnectionCallbacks.on_frame = OnFrame;
    //ConnectionCallbacks.on_image = OnImage;
    //if (connection == NULL) {
        connection = OpenConnection();
		LeapSetAllocator(*connection, &allocator);
        LeapSetPolicyFlags(*connection, eLeapPolicyFlag_Images, 0);
        ARLOGi("Connected.\n");
    //} else {
    //    ARLOGi("Reusing existing connection (not thread-safe).\n");
    //}
    connectionRef++;

    // while(!IsConnected) {
    //   ARLOGi("Waiting.\n");
    //   millisleep(100); //wait a bit to let the connection complete
    // }


    arMalloc(vid, AR2VideoParamLeapMotionT, 1);
    vid->buffer.buff = vid->buffer.buffLuma = NULL;
    vid->buffer.bufPlanes = NULL;
    vid->buffer.bufPlaneCount = 0;
    vid->buffer.fillFlag = 0;
    vid->width  = AR_VIDEO_LEAPMOTION_XSIZE;
    vid->height = AR_VIDEO_LEAPMOTION_YSIZE;
    vid->format = ARVIDEO_INPUT_LEAPMOTION_DEFAULT_PIXEL_FORMAT;
    vid->stereo_part = ARVIDEO_INPUT_LEAPMOTION_DEFAULT_STEREO_PART;
    vid->rectified = false;
    vid->gain = 1.0;

    a = config;
    if (a != NULL) {
        for (;;) {
            while (*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", b) == 0) break;
            if (strncmp(b, "-stereo_part=", 13) == 0) {
                if (strcmp(b+13, "left") == 0) {
                    vid->stereo_part = ARVIDEO_INPUT_LEAPMOTION_LEFT_STEREO_PART;
                } else if (strcmp(b+13, "right") == 0) {
                    vid->stereo_part = ARVIDEO_INPUT_LEAPMOTION_RIGHT_STEREO_PART;
                }
            }
            else if (strcmp(b, "-rectified") == 0) {
                vid->rectified = true;
            }
            else if (strncmp(b, "-gain", 5) == 0) {
                sscanf(&b[6], "%lf", &vid->gain);
            }
            else if (strcmp(b, "-module=LeapMotion") == 0) {
            }
            else {
                ARLOGe("Error: unrecognised video configuration option \"%s\".\n", a);
                ar2VideoDispOptionLeapMotion();
                goto bail;
            }

            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    if (vid->rectified) {
        vid->width = AR_VIDEO_LEAPMOTION_RECTIFIED_XSIZE;
        vid->height = AR_VIDEO_LEAPMOTION_RECTIFIED_YSIZE;
    }

    bufSizeX = vid->width;
    bufSizeY = vid->height;

    if (!bufSizeX || !bufSizeY || ar2VideoSetBufferSizeLeapMotion(vid, bufSizeX, bufSizeY) != 0) {
        goto bail;
    }

    ARLOGi("LeapMotion video size %dx%d@%dBpp.\n", vid->width, vid->height, arVideoUtilGetPixelSize(vid->format));

    return vid;
bail:
    free(vid);
    return (NULL);
}

int ar2VideoCloseLeapMotion( AR2VideoParamLeapMotionT *vid )
{
    if (!vid) return (-1); // Sanity check.

    //connectionRef--;
    //if (connectionRef == 0) {
        CloseConnection();
        connection = NULL;
    //}

    ar2VideoSetBufferSizeLeapMotion(vid, 0, 0);
    free( vid );

    return 0;
}

int ar2VideoCapStartLeapMotion( AR2VideoParamLeapMotionT *vid )
{
    return 0;
}

int ar2VideoCapStopLeapMotion( AR2VideoParamLeapMotionT *vid )
{
    return 0;
}

int64_t lastFrameID = 0; //The last frame received

AR2VideoBufferT *ar2VideoGetImageLeapMotion( AR2VideoParamLeapMotionT *vid )
{
    if (!vid) return (NULL); // Sanity check.

    // LEAP_TRACKING_EVENT *frame = GetFrame();
    // if (frame && (frame->tracking_frame_id > lastFrameID)) {
    //   lastFrameID = frame->tracking_frame_id;
    //   ARLOGi("Frame %lli with %i hands.\n", (long long int)frame->tracking_frame_id, frame->nHands);
    //   for(uint32_t h = 0; h < frame->nHands; h++){
    //     LEAP_HAND* hand = &frame->pHands[h];
    //     ARLOGi("    Hand id %i is a %s hand with position (%f, %f, %f).\n",
    //                 hand->id,
    //                 (hand->type == eLeapHandType_Left ? "left" : "right"),
    //                 hand->palm.position.x,
    //                 hand->palm.position.y,
    //                 hand->palm.position.z);
    //   }
    // } else {
    //   ARLOGi("No frame.\n");
    // }
	//printf("BEGIN getImage\n");
	LockMutex(&dataLock);

	//printf("BEGIN getDevice\n");
	LEAP_DEVICE_INFO* deviceProps = GetDeviceProperties();
	//printf("END getDevice\n");
	//if (deviceProps)
    //  ARLOGd("Using device %s, h_fov = %f, v_fov = %f\n", deviceProps->serial, deviceProps->h_fov, deviceProps->v_fov);


    LEAP_IMAGE_EVENT *image = GetImage();
    int hasFrame = false;
    ARUint8 *src = NULL;
    if (image /*&& image->info.frame_id > lastFrameID*/) {
      hasFrame = true;
      lastFrameID = image->info.frame_id;
      LEAP_IMAGE img = image->image[vid->stereo_part];

      /*ARLOGd("Received image for frame %lli with size %lli*%lli, stereo_part = %d.\n",
           (long long int)image->info.frame_id,
           (long long int)img.properties.width,
           (long long int)img.properties.height,
           vid->stereo_part);*/
      src = (ARUint8*)img.data + img.offset;
    } else {
      ARLOGi("No frame.\n");
    }

    ARUint8     *dst, *dst1;
    if (!vid->buffer.bufPlaneCount) {
        dst = vid->buffer.buff;
        dst1 = NULL;
    } else {
        dst = vid->buffer.bufPlanes[0];
        dst1 = vid->buffer.bufPlanes[1];
    }

    int width = 640;
    int height = 240;
    // Clear buffer to white.
    memset(dst, 255, vid->bufWidth*vid->bufHeight*arVideoUtilGetPixelSize(vid->format));
    if (dst1) memset(dst1, 128, vid->bufWidth*vid->bufHeight/2);

    if (hasFrame) {
        if (vid->rectified) {
            // tan(h_fov/2)
            const int MAX_FOV = 2.24603794466;
            if (vid->stereo_part == ARVIDEO_INPUT_LEAPMOTION_LEFT_STEREO_PART) {
                for (float row = 0; row < vid->height; row++) {
                    for (float col = 0; col < vid->width; col++) {
                        //Normalize from pixel xy to range [0..1]
                        LEAP_VECTOR input;
                        input.x = col/vid->width;
                        input.y = row/vid->height;

                        //Convert from normalized [0..1] to ray slopes
                        input.x = (input.x - .5) * MAX_FOV;
                        input.y = (input.y - .5) * MAX_FOV;

                        LEAP_VECTOR pixel = LeapRectilinearToPixel(*connection,
                                                                eLeapPerspectiveType_stereo_left,
                                                                input);
                        int dindex = (int)floor(row * vid->width + col);
                        int pindex = (int)roundf(pixel.y) * width + (int)roundf(pixel.x);
                        if(pixel.x >= 0 && pixel.x < width && pixel.y >=0 && pixel.y < height){
                            dst[dindex] = min(src[pindex]*vid->gain, 255);
                        } else {
                            dst[dindex] = 128;
                        }
                    }
                }
            } else {
                for( float row = 0; row < vid->height; row++ ) {
                    for( float col = 0; col < vid->width; col++ ) {

                        //Normalize from pixel xy to range [0..1]
                        LEAP_VECTOR input;
                        input.x = col/vid->width;
                        input.y = row/vid->height;

                        //Convert from normalized [0..1] to ray slopes
                        input.x = (input.x - .5) * MAX_FOV;
                        input.y = (input.y - .5) * MAX_FOV;

                        LEAP_VECTOR pixel = LeapRectilinearToPixel(*connection,
                                                                eLeapPerspectiveType_stereo_right,
                                                                input);
                        int dindex = (int)floor(row * vid->width + col);
                        int pindex = (int)roundf(pixel.y + height) * width + (int)roundf(pixel.x);

                        if(pixel.x >= 0 && pixel.x < width && pixel.y >=0 && pixel.y < height){
                            dst[dindex] = min(src[pindex]*vid->gain, 255);
                        } else {
                            dst[dindex] = 128;
                        }
                    }
                }
            }
        } else {
            for (int y = 0; y < height; y++) {
                memcpy(dst, src, width);
                src += width;
                dst += vid->bufWidth;
            }
        }
    }
	UnlockMutex(&dataLock);
	//printf("END getImage\n");

    // TODO: check what these flags do
    vid->buffer.fillFlag  = 1;
    vid->buffer.time.sec  = 0;
    vid->buffer.time.usec = 0;

    return &(vid->buffer);
}

int ar2VideoGetSizeLeapMotion(AR2VideoParamLeapMotionT *vid, int *x,int *y)
{
    if (!vid) return (-1); // Sanity check.
    *x = vid->width;
    *y = vid->height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatLeapMotion( AR2VideoParamLeapMotionT *vid )
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    return (vid->format);
}

int ar2VideoSetBufferSizeLeapMotion(AR2VideoParamLeapMotionT *vid, const int width, const int height)
{
    unsigned int i;
    int rowBytes;

    if (!vid) return (-1);

    if (vid->buffer.bufPlaneCount) {
        for (i = 0; i < vid->buffer.bufPlaneCount; i++) {
            free(vid->buffer.bufPlanes[i]);
            vid->buffer.bufPlanes[i] = NULL;
            free(vid->buffer.bufPlanes);
            vid->buffer.bufPlaneCount = 0;
            vid->buffer.buff = vid->buffer.buffLuma = NULL;
        }
    } else {
        if (vid->buffer.buff) {
            free (vid->buffer.buff);
            vid->buffer.buff = vid->buffer.buffLuma = NULL;
        }
    }

    if (width && height) {
        if (width < vid->width || height < vid->height) {
            ARLOGe("Error: Requested buffer size smaller than video size.\n");
            return (-1);
        }

        if (ar2VideoGetPixelFormatLeapMotion(vid) == AR_PIXEL_FORMAT_420f || ar2VideoGetPixelFormatLeapMotion(vid) == AR_PIXEL_FORMAT_NV21) {
            arMallocClear(vid->buffer.bufPlanes, ARUint8 *, 2);
            arMalloc(vid->buffer.bufPlanes[0], ARUint8, width*height);
            arMalloc(vid->buffer.bufPlanes[1], ARUint8, width*height/2);
            vid->buffer.bufPlaneCount = 2;
            vid->buffer.buff = vid->buffer.buffLuma = vid->buffer.bufPlanes[0];
        } else {
            rowBytes = width * arVideoUtilGetPixelSize(ar2VideoGetPixelFormatLeapMotion(vid));
            arMalloc(vid->buffer.buff, ARUint8, height * rowBytes);
            if (ar2VideoGetPixelFormatLeapMotion(vid) == AR_PIXEL_FORMAT_MONO) {
                vid->buffer.buffLuma = vid->buffer.buff;
            } else {
                vid->buffer.buffLuma = NULL;
            }
        }
    }

    vid->bufWidth = width;
    vid->bufHeight = height;

    return (0);
}

int ar2VideoGetCParamLeapMotion(AR2VideoParamLeapMotionT *vid, ARParam *cparam) {
    // In rectified mode use the real v_fov, otherwise use the default one which performs better.
    double fov = vid->rectified ? v_fov : M_PI_4;
    arParamClearWithFOVy(cparam, vid->width, vid->height, v_fov);
    return (0);
}

int ar2VideoGetBufferSizeLeapMotion(AR2VideoParamLeapMotionT *vid, int *width, int *height)
{
    if (!vid) return (-1);
    if (width) *width = vid->bufWidth;
    if (height) *height = vid->bufHeight;
    return (0);
}

int ar2VideoGetIdLeapMotion( AR2VideoParamLeapMotionT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiLeapMotion( AR2VideoParamLeapMotionT *vid, int paramName, int *value )
{
    if (!value) return -1;

    if (paramName == AR_VIDEO_PARAM_GET_IMAGE_ASYNC) {
        *value = 0;
        return 0;
    }

    return -1;
}

int ar2VideoSetParamiLeapMotion( AR2VideoParamLeapMotionT *vid, int paramName, int  value )
{
    return -1;
}

int ar2VideoGetParamdLeapMotion( AR2VideoParamLeapMotionT *vid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamdLeapMotion( AR2VideoParamLeapMotionT *vid, int paramName, double  value )
{
    return -1;
}

int ar2VideoGetParamsLeapMotion( AR2VideoParamLeapMotionT *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsLeapMotion( AR2VideoParamLeapMotionT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);

    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

#endif //  ARVIDEO_INPUT_LEAPMOTION
