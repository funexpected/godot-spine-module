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

#ifndef SPINE_V4_SKELETON_H_
#define SPINE_V4_SKELETON_H_

#include <v4/spine/dll.h>
#include <v4/spine/SkeletonData.h>
#include <v4/spine/Slot.h>
#include <v4/spine/Skin.h>
#include <v4/spine/IkConstraint.h>
#include <v4/spine/TransformConstraint.h>
#include <v4/spine/PathConstraint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4Skeleton {
	sp4SkeletonData *const data;

	int bonesCount;
	sp4Bone **bones;
	sp4Bone *const root;

	int slotsCount;
	sp4Slot **slots;
	sp4Slot **drawOrder;

	int ikConstraintsCount;
	sp4IkConstraint **ikConstraints;

	int transformConstraintsCount;
	sp4TransformConstraint **transformConstraints;

	int pathConstraintsCount;
	sp4PathConstraint **pathConstraints;

	sp4Skin *const skin;
	sp4Color color;
	float time;
	float scaleX, scaleY;
	float x, y;
} sp4Skeleton;

SP4_API sp4Skeleton *sp4Skeleton_create(sp4SkeletonData *data);

SP4_API void sp4Skeleton_dispose(sp4Skeleton *self);

/* Caches information about bones and constraints. Must be called if bones or constraints, or weighted path attachments
 * are added or removed. */
SP4_API void sp4Skeleton_updateCache(sp4Skeleton *self);

SP4_API void sp4Skeleton_updateWorldTransform(const sp4Skeleton *self);

/* Sets the bones, constraints, and slots to their setup pose values. */
SP4_API void sp4Skeleton_setToSetupPose(const sp4Skeleton *self);
/* Sets the bones and constraints to their setup pose values. */
SP4_API void sp4Skeleton_setBonesToSetupPose(const sp4Skeleton *self);

SP4_API void sp4Skeleton_setSlotsToSetupPose(const sp4Skeleton *self);

/* Returns 0 if the bone was not found. */
SP4_API sp4Bone *sp4Skeleton_findBone(const sp4Skeleton *self, const char *boneName);

/* Returns 0 if the slot was not found. */
SP4_API sp4Slot *sp4Skeleton_findSlot(const sp4Skeleton *self, const char *slotName);

/* Sets the skin used to look up attachments before looking in the SkeletonData defaultSkin. Attachments from the new skin are
 * attached if the corresponding attachment from the old skin was attached. If there was no old skin, each slot's setup mode
 * attachment is attached from the new skin.
 * @param skin May be 0.*/
SP4_API void sp4Skeleton_setSkin(sp4Skeleton *self, sp4Skin *skin);
/* Returns 0 if the skin was not found. See sp4Skeleton_setSkin.
 * @param skinName May be 0. */
SP4_API int sp4Skeleton_setSkinByName(sp4Skeleton *self, const char *skinName);

/* Returns 0 if the slot or attachment was not found. */
SP4_API sp4Attachment *
sp4Skeleton_getAttachmentForSlotName(const sp4Skeleton *self, const char *slotName, const char *attachmentName);
/* Returns 0 if the slot or attachment was not found. */
SP4_API sp4Attachment *
sp4Skeleton_getAttachmentForSlotIndex(const sp4Skeleton *self, int slotIndex, const char *attachmentName);
/* Returns 0 if the slot or attachment was not found.
 * @param attachmentName May be 0. */
SP4_API int sp4Skeleton_setAttachment(sp4Skeleton *self, const char *slotName, const char *attachmentName);

/* Returns 0 if the IK constraint was not found. */
SP4_API sp4IkConstraint *sp4Skeleton_findIkConstraint(const sp4Skeleton *self, const char *constraintName);

/* Returns 0 if the transform constraint was not found. */
SP4_API sp4TransformConstraint *sp4Skeleton_findTransformConstraint(const sp4Skeleton *self, const char *constraintName);

/* Returns 0 if the path constraint was not found. */
SP4_API sp4PathConstraint *sp4Skeleton_findPathConstraint(const sp4Skeleton *self, const char *constraintName);

SP4_API void sp4Skeleton_update(sp4Skeleton *self, float deltaTime);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_SKELETON_H_*/
