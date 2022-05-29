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

#ifndef SPINE_V4_BONE_H_
#define SPINE_V4_BONE_H_

#include <v4/spine/dll.h>
#include <v4/spine/BoneData.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sp4Skeleton;

typedef struct sp4Bone sp4Bone;
struct sp4Bone {
	sp4BoneData *const data;
	struct sp4Skeleton *const skeleton;
	sp4Bone *const parent;
	int childrenCount;
	sp4Bone **const children;
	float x, y, rotation, scaleX, scaleY, shearX, shearY;
	float ax, ay, arotation, ascaleX, ascaleY, ashearX, ashearY;

	float const a, b, worldX;
	float const c, d, worldY;

	int/*bool*/ sorted;
	int/*bool*/ active;
};

SP4_API void sp4Bone_setYDown(int/*bool*/yDown);

SP4_API int/*bool*/sp4Bone_isYDown();

/* @param parent May be 0. */
SP4_API sp4Bone *sp4Bone_create(sp4BoneData *data, struct sp4Skeleton *skeleton, sp4Bone *parent);

SP4_API void sp4Bone_dispose(sp4Bone *self);

SP4_API void sp4Bone_setToSetupPose(sp4Bone *self);

SP4_API void sp4Bone_update(sp4Bone *self);

SP4_API void sp4Bone_updateWorldTransform(sp4Bone *self);

SP4_API void sp4Bone_updateWorldTransformWith(sp4Bone *self, float x, float y, float rotation, float scaleX, float scaleY,
											float shearX, float shearY);

SP4_API float sp4Bone_getWorldRotationX(sp4Bone *self);

SP4_API float sp4Bone_getWorldRotationY(sp4Bone *self);

SP4_API float sp4Bone_getWorldScaleX(sp4Bone *self);

SP4_API float sp4Bone_getWorldScaleY(sp4Bone *self);

SP4_API void sp4Bone_updateAppliedTransform(sp4Bone *self);

SP4_API void sp4Bone_worldToLocal(sp4Bone *self, float worldX, float worldY, float *localX, float *localY);

SP4_API void sp4Bone_localToWorld(sp4Bone *self, float localX, float localY, float *worldX, float *worldY);

SP4_API float sp4Bone_worldToLocalRotation(sp4Bone *self, float worldRotation);

SP4_API float sp4Bone_localToWorldRotation(sp4Bone *self, float localRotation);

SP4_API void sp4Bone_rotateWorld(sp4Bone *self, float degrees);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_BONE_H_ */
