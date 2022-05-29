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

#ifndef SPINE_V4_PATHCONSTRAINT_H_
#define SPINE_V4_PATHCONSTRAINT_H_

#include <v4/spine/dll.h>
#include <v4/spine/PathConstraintData.h>
#include <v4/spine/Bone.h>
#include <v4/spine/Slot.h>
#include "PathAttachment.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sp4Skeleton;

typedef struct sp4PathConstraint {
	sp4PathConstraintData *const data;
	int bonesCount;
	sp4Bone **const bones;
	sp4Slot *target;
	float position, spacing;
	float mixRotate, mixX, mixY;

	int spacesCount;
	float *spaces;

	int positionsCount;
	float *positions;

	int worldCount;
	float *world;

	int curvesCount;
	float *curves;

	int lengthsCount;
	float *lengths;

	float segments[10];

	int /*boolean*/ active;
} sp4PathConstraint;

#define SP4_PATHCONSTRAINT_

SP4_API sp4PathConstraint *sp4PathConstraint_create(sp4PathConstraintData *data, const struct sp4Skeleton *skeleton);

SP4_API void sp4PathConstraint_dispose(sp4PathConstraint *self);

SP4_API void sp4PathConstraint_update(sp4PathConstraint *self);

SP4_API float *sp4PathConstraint_computeWorldPositions(sp4PathConstraint *self, sp4PathAttachment *path, int spacesCount,
													 int/*bool*/ tangents);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_PATHCONSTRAINT_H_ */
