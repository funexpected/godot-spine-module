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

#ifndef SPINE_V4_SKELETONDATA_H_
#define SPINE_V4_SKELETONDATA_H_

#include <v4/spine/dll.h>
#include <v4/spine/BoneData.h>
#include <v4/spine/SlotData.h>
#include <v4/spine/Skin.h>
#include <v4/spine/EventData.h>
#include <v4/spine/Animation.h>
#include <v4/spine/IkConstraintData.h>
#include <v4/spine/TransformConstraintData.h>
#include <v4/spine/PathConstraintData.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4SkeletonData {
	const char *version;
	const char *hash;
	float x, y, width, height;
	float fps;
	const char *imagesPath;
	const char *audioPath;

	int stringsCount;
	char **strings;

	int bonesCount;
	sp4BoneData **bones;

	int slotsCount;
	sp4SlotData **slots;

	int skinsCount;
	sp4Skin **skins;
	sp4Skin *defaultSkin;

	int eventsCount;
	sp4EventData **events;

	int animationsCount;
	sp4Animation **animations;

	int ikConstraintsCount;
	sp4IkConstraintData **ikConstraints;

	int transformConstraintsCount;
	sp4TransformConstraintData **transformConstraints;

	int pathConstraintsCount;
	sp4PathConstraintData **pathConstraints;
} sp4SkeletonData;

SP4_API sp4SkeletonData *sp4SkeletonData_create();

SP4_API void sp4SkeletonData_dispose(sp4SkeletonData *self);

SP4_API sp4BoneData *sp4SkeletonData_findBone(const sp4SkeletonData *self, const char *boneName);

SP4_API sp4SlotData *sp4SkeletonData_findSlot(const sp4SkeletonData *self, const char *slotName);

SP4_API sp4Skin *sp4SkeletonData_findSkin(const sp4SkeletonData *self, const char *skinName);

SP4_API sp4EventData *sp4SkeletonData_findEvent(const sp4SkeletonData *self, const char *eventName);

SP4_API sp4Animation *sp4SkeletonData_findAnimation(const sp4SkeletonData *self, const char *animationName);

SP4_API sp4IkConstraintData *sp4SkeletonData_findIkConstraint(const sp4SkeletonData *self, const char *constraintName);

SP4_API sp4TransformConstraintData *
sp4SkeletonData_findTransformConstraint(const sp4SkeletonData *self, const char *constraintName);

SP4_API sp4PathConstraintData *sp4SkeletonData_findPathConstraint(const sp4SkeletonData *self, const char *constraintName);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_SKELETONDATA_H_ */
