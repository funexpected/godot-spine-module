/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated January 1, 2020. Replaces all prior versions.
 *
 * Copyright (c) 2013-2020, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef SPINE_V4_BONEDATA_H_
#define SPINE_V4_BONEDATA_H_

#include <v4/spine/dll.h>
#include <v4/spine/Color.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SP4_TRANSFORMMODE_NORMAL,
	SP4_TRANSFORMMODE_ONLYTRANSLATION,
	SP4_TRANSFORMMODE_NOROTATIONORREFLECTION,
	SP4_TRANSFORMMODE_NOSCALE,
	SP4_TRANSFORMMODE_NOSCALEORREFLECTION
} sp4TransformMode;

typedef struct sp4BoneData sp4BoneData;
struct sp4BoneData {
	const int index;
	const char *const name;
	sp4BoneData *const parent;
	float length;
	float x, y, rotation, scaleX, scaleY, shearX, shearY;
	sp4TransformMode transformMode;
	int/*bool*/ skinRequired;
	sp4Color color;
};

SP4_API sp4BoneData *sp4BoneData_create(int index, const char *name, sp4BoneData *parent);

SP4_API void sp4BoneData_dispose(sp4BoneData *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_BONEDATA_H_ */
