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

#ifndef SPINE_V4_PATHCONSTRAINTDATA_H_
#define SPINE_V4_PATHCONSTRAINTDATA_H_

#include <v4/spine/dll.h>
#include <v4/spine/BoneData.h>
#include <v4/spine/SlotData.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SP4_POSITION_MODE_FIXED, SP4_POSITION_MODE_PERCENT
} sp4PositionMode;

typedef enum {
	SP4_SPACING_MODE_LENGTH, SP4_SPACING_MODE_FIXED, SP4_SPACING_MODE_PERCENT, SP4_SPACING_MODE_PROPORTIONAL
} sp4SpacingMode;

typedef enum {
	SP4_ROTATE_MODE_TANGENT, SP4_ROTATE_MODE_CHAIN, SP4_ROTATE_MODE_CHAIN_SCALE
} sp4RotateMode;

typedef struct sp4PathConstraintData {
	const char *const name;
	int order;
	int/*bool*/ skinRequired;
	int bonesCount;
	sp4BoneData **const bones;
	sp4SlotData *target;
	sp4PositionMode positionMode;
	sp4SpacingMode spacingMode;
	sp4RotateMode rotateMode;
	float offsetRotation;
	float position, spacing;
	float mixRotate, mixX, mixY;
} sp4PathConstraintData;

SP4_API sp4PathConstraintData *sp4PathConstraintData_create(const char *name);

SP4_API void sp4PathConstraintData_dispose(sp4PathConstraintData *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_PATHCONSTRAINTDATA_H_ */
