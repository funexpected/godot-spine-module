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

#ifndef SPINE_V4_ANIMATIONSTATEDATA_H_
#define SPINE_V4_ANIMATIONSTATEDATA_H_

#include <v4/spine/dll.h>
#include <v4/spine/Animation.h>
#include <v4/spine/SkeletonData.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4AnimationStateData {
	sp4SkeletonData *const skeletonData;
	float defaultMix;
	const void *const entries;
} sp4AnimationStateData;

SP4_API sp4AnimationStateData *sp4AnimationStateData_create(sp4SkeletonData *skeletonData);

SP4_API void sp4AnimationStateData_dispose(sp4AnimationStateData *self);

SP4_API void
sp4AnimationStateData_setMixByName(sp4AnimationStateData *self, const char *fromName, const char *toName, float duration);

SP4_API void sp4AnimationStateData_setMix(sp4AnimationStateData *self, sp4Animation *from, sp4Animation *to, float duration);
/* Returns 0 if there is no mixing between the animations. */
SP4_API float sp4AnimationStateData_getMix(sp4AnimationStateData *self, sp4Animation *from, sp4Animation *to);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_ANIMATIONSTATEDATA_H_ */
