/*
 *	videoLeapMotion.h
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
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef AR_VIDEO_LEAPMOTION_H
#define AR_VIDEO_LEAPMOTION_H


#include <ARX/ARVideo/video.h>
#include "LeapC.h"

#ifdef  __cplusplus
extern "C" {
#endif


typedef struct alloc_state {
	uint8_t *memory;
	int index;
} alloc_state;

typedef struct {
	alloc_state        allocator_state;
	LEAP_ALLOCATOR     allocator;
    AR2VideoBufferT    buffer;
    int                width;
    int                height;
    AR_PIXEL_FORMAT    format;
    int                bufWidth;
    int                bufHeight;
    int                stereo_part;
    int                rectified;
    double             gain;
} AR2VideoParamLeapMotionT;

int                    ar2VideoDispOptionLeapMotion     ( void );
AR2VideoParamLeapMotionT   *ar2VideoOpenLeapMotion           ( const char *config );
int                    ar2VideoCloseLeapMotion          ( AR2VideoParamLeapMotionT *vid );
int                    ar2VideoGetIdLeapMotion          ( AR2VideoParamLeapMotionT *vid, ARUint32 *id0, ARUint32 *id1 );
int                    ar2VideoGetSizeLeapMotion        ( AR2VideoParamLeapMotionT *vid, int *x,int *y );
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatLeapMotion ( AR2VideoParamLeapMotionT *vid );
AR2VideoBufferT       *ar2VideoGetImageLeapMotion       ( AR2VideoParamLeapMotionT *vid );
int                    ar2VideoCapStartLeapMotion       ( AR2VideoParamLeapMotionT *vid );
int                    ar2VideoCapStopLeapMotion        ( AR2VideoParamLeapMotionT *vid );
int                    ar2VideoGetCParamLeapMotion      ( AR2VideoParamLeapMotionT *vid, ARParam *cparam );

int                    ar2VideoGetParamiLeapMotion      ( AR2VideoParamLeapMotionT *vid, int paramName, int *value );
int                    ar2VideoSetParamiLeapMotion      ( AR2VideoParamLeapMotionT *vid, int paramName, int  value );
int                    ar2VideoGetParamdLeapMotion      ( AR2VideoParamLeapMotionT *vid, int paramName, double *value );
int                    ar2VideoSetParamdLeapMotion      ( AR2VideoParamLeapMotionT *vid, int paramName, double  value );
int                    ar2VideoGetParamsLeapMotion      ( AR2VideoParamLeapMotionT *vid, const int paramName, char **value );
int                    ar2VideoSetParamsLeapMotion      ( AR2VideoParamLeapMotionT *vid, const int paramName, const char  *value );

int ar2VideoSetBufferSizeLeapMotion(AR2VideoParamLeapMotionT *vid, const int width, const int height);
int ar2VideoGetBufferSizeLeapMotion(AR2VideoParamLeapMotionT *vid, int *width, int *height);

#ifdef  __cplusplus
}
#endif
#endif // AR_VIDEO_LEAPMOTION_H
