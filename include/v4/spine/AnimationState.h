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

#ifndef SPINE_V4_ANIMATIONSTATE_H_
#define SPINE_V4_ANIMATIONSTATE_H_

#include <v4/spine/dll.h>
#include <v4/spine/Animation.h>
#include <v4/spine/AnimationStateData.h>
#include <v4/spine/Event.h>
#include <v4/spine/Array.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SP4_ANIMATION_START,
	SP4_ANIMATION_INTERRUPT,
	SP4_ANIMATION_END,
	SP4_ANIMATION_COMPLETE,
	SP4_ANIMATION_DISPOSE,
	SP4_ANIMATION_EVENT
} sp4EventType;

typedef struct sp4AnimationState sp4AnimationState;
typedef struct sp4TrackEntry sp4TrackEntry;

typedef void (*sp4AnimationStateListener)(sp4AnimationState *state, sp4EventType type, sp4TrackEntry *entry,
										 sp4Event *event);

_SP4_ARRAY_DECLARE_TYPE(sp4TrackEntryArray, sp4TrackEntry*)

struct sp4TrackEntry {
	sp4Animation *animation;
	sp4TrackEntry *previous;
	sp4TrackEntry *next;
	sp4TrackEntry *mixingFrom;
	sp4TrackEntry *mixingTo;
	sp4AnimationStateListener listener;
	int trackIndex;
	int /*boolean*/ loop;
	int /*boolean*/ holdPrevious;
	int /*boolean*/ reverse;
	float eventThreshold, attachmentThreshold, drawOrderThreshold;
	float animationStart, animationEnd, animationLast, nextAnimationLast;
	float delay, trackTime, trackLast, nextTrackLast, trackEnd, timeScale;
	float alpha, mixTime, mixDuration, interruptAlpha, totalAlpha;
	sp4MixBlend mixBlend;
	sp4IntArray *timelineMode;
	sp4TrackEntryArray *timelineHoldMix;
	float *timelinesRotation;
	int timelinesRotationCount;
	void *rendererObject;
	void *userData;
};

struct sp4AnimationState {
	sp4AnimationStateData *const data;

	int tracksCount;
	sp4TrackEntry **tracks;

	sp4AnimationStateListener listener;

	float timeScale;

	void *rendererObject;
	void *userData;

	int unkeyedState;
};

/* @param data May be 0 for no mixing. */
SP4_API sp4AnimationState *sp4AnimationState_create(sp4AnimationStateData *data);

SP4_API void sp4AnimationState_dispose(sp4AnimationState *self);

SP4_API void sp4AnimationState_update(sp4AnimationState *self, float delta);

SP4_API int /**bool**/ sp4AnimationState_apply(sp4AnimationState *self, struct sp4Skeleton *skeleton);

SP4_API void sp4AnimationState_clearTracks(sp4AnimationState *self);

SP4_API void sp4AnimationState_clearTrack(sp4AnimationState *self, int trackIndex);

/** Set the current animation. Any queued animations are cleared. */
SP4_API sp4TrackEntry *
sp4AnimationState_setAnimationByName(sp4AnimationState *self, int trackIndex, const char *animationName,
									int/*bool*/loop);

SP4_API sp4TrackEntry *
sp4AnimationState_setAnimation(sp4AnimationState *self, int trackIndex, sp4Animation *animation, int/*bool*/loop);

/** Adds an animation to be played delay seconds after the current or last queued animation, taking into account any mix
 * duration. */
SP4_API sp4TrackEntry *
sp4AnimationState_addAnimationByName(sp4AnimationState *self, int trackIndex, const char *animationName,
									int/*bool*/loop, float delay);

SP4_API sp4TrackEntry *
sp4AnimationState_addAnimation(sp4AnimationState *self, int trackIndex, sp4Animation *animation, int/*bool*/loop,
							  float delay);

SP4_API sp4TrackEntry *sp4AnimationState_setEmptyAnimation(sp4AnimationState *self, int trackIndex, float mixDuration);

SP4_API sp4TrackEntry *
sp4AnimationState_addEmptyAnimation(sp4AnimationState *self, int trackIndex, float mixDuration, float delay);

SP4_API void sp4AnimationState_setEmptyAnimations(sp4AnimationState *self, float mixDuration);

SP4_API sp4TrackEntry *sp4AnimationState_getCurrent(sp4AnimationState *self, int trackIndex);

SP4_API void sp4AnimationState_clearListenerNotifications(sp4AnimationState *self);

SP4_API float sp4TrackEntry_getAnimationTime(sp4TrackEntry *entry);

SP4_API float sp4TrackEntry_getTrackComplete(sp4TrackEntry *entry);

SP4_API void sp4AnimationState_clearNext(sp4AnimationState *self, sp4TrackEntry *entry);

/** Use this to dispose static memory before your app exits to appease your memory leak detector*/
SP4_API void sp4AnimationState_disposeStatics();

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_ANIMATIONSTATE_H_ */
