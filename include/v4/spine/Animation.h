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

#ifndef SPINE_V4_ANIMATION_H_
#define SPINE_V4_ANIMATION_H_

#include <v4/spine/dll.h>
#include <v4/spine/Event.h>
#include <v4/spine/Attachment.h>
#include <v4/spine/VertexAttachment.h>
#include <v4/spine/Array.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4Timeline sp4Timeline;
struct sp4Skeleton;
typedef uint64_t sp4PropertyId;

_SP4_ARRAY_DECLARE_TYPE(sp4PropertyIdArray, sp4PropertyId)

_SP4_ARRAY_DECLARE_TYPE(sp4TimelineArray, sp4Timeline*)

typedef struct sp4Animation {
	const char *const name;
	float duration;

	sp4TimelineArray *timelines;
	sp4PropertyIdArray *timelineIds;
} sp4Animation;

typedef enum {
	SP4_MIX_BLEND_SETUP,
	SP4_MIX_BLEND_FIRST,
	SP4_MIX_BLEND_REPLACE,
	SP4_MIX_BLEND_ADD
} sp4MixBlend;

typedef enum {
	SP4_MIX_DIRECTION_IN,
	SP4_MIX_DIRECTION_OUT
} sp4MixDirection;

SP4_API sp4Animation *sp4Animation_create(const char *name, sp4TimelineArray *timelines, float duration);

SP4_API void sp4Animation_dispose(sp4Animation *self);

SP4_API int /*bool*/ sp4Animation_hasTimeline(sp4Animation *self, sp4PropertyId *ids, int idsCount);

/** Poses the skeleton at the specified time for this animation.
 * @param lastTime The last time the animation was applied.
 * @param events Any triggered events are added. May be null.*/
SP4_API void
sp4Animation_apply(const sp4Animation *self, struct sp4Skeleton *skeleton, float lastTime, float time, int loop,
				  sp4Event **events, int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction);

/**/
typedef enum {
	SP4_TIMELINE_ATTACHMENT,
	SP4_TIMELINE_ALPHA,
	SP4_TIMELINE_PATHCONSTRAINTPOSITION,
	SP4_TIMELINE_PATHCONSTRAINTSPACING,
	SP4_TIMELINE_ROTATE,
	SP4_TIMELINE_SCALEX,
	SP4_TIMELINE_SCALEY,
	SP4_TIMELINE_SHEARX,
	SP4_TIMELINE_SHEARY,
	SP4_TIMELINE_TRANSLATEX,
	SP4_TIMELINE_TRANSLATEY,
	SP4_TIMELINE_SCALE,
	SP4_TIMELINE_SHEAR,
	SP4_TIMELINE_TRANSLATE,
	SP4_TIMELINE_DEFORM,
	SP4_TIMELINE_IKCONSTRAINT,
	SP4_TIMELINE_PATHCONSTRAINTMIX,
	SP4_TIMELINE_RGB2,
	SP4_TIMELINE_RGBA2,
	SP4_TIMELINE_RGBA,
	SP4_TIMELINE_RGB,
	SP4_TIMELINE_TRANSFORMCONSTRAINT,
	SP4_TIMELINE_DRAWORDER,
	SP4_TIMELINE_EVENT
} sp4TimelineType;

/**/

typedef enum {
	SP4_PROPERTY_ROTATE = 1 << 0,
	SP4_PROPERTY_X = 1 << 1,
	SP4_PROPERTY_Y = 1 << 2,
	SP4_PROPERTY_SCALEX = 1 << 3,
	SP4_PROPERTY_SCALEY = 1 << 4,
	SP4_PROPERTY_SHEARX = 1 << 5,
	SP4_PROPERTY_SHEARY = 1 << 6,
	SP4_PROPERTY_RGB = 1 << 7,
	SP4_PROPERTY_ALPHA = 1 << 8,
	SP4_PROPERTY_RGB2 = 1 << 9,
	SP4_PROPERTY_ATTACHMENT = 1 << 10,
	SP4_PROPERTY_DEFORM = 1 << 11,
	SP4_PROPERTY_EVENT = 1 << 12,
	SP4_PROPERTY_DRAWORDER = 1 << 13,
	SP4_PROPERTY_IKCONSTRAINT = 1 << 14,
	SP4_PROPERTY_TRANSFORMCONSTRAINT = 1 << 15,
	SP4_PROPERTY_PATHCONSTRAINT_POSITION = 1 << 16,
	SP4_PROPERTY_PATHCONSTRAINT_SPACING = 1 << 17,
	SP4_PROPERTY_PATHCONSTRAINT_MIX = 1 << 18
} sp4Property;

#define SP4_MAX_PROPERTY_IDS 3

typedef struct _sp4TimelineVtable {
	void (*apply)(sp4Timeline *self, struct sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
				  int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction);

	void (*dispose)(sp4Timeline *self);

	void
	(*setBezier)(sp4Timeline *self, int bezier, int frame, float value, float time1, float value1, float cx1, float cy1,
				 float cx2, float cy2, float time2, float value2);
} _sp4TimelineVtable;

struct sp4Timeline {
	_sp4TimelineVtable vtable;
	sp4PropertyId propertyIds[SP4_MAX_PROPERTY_IDS];
	int propertyIdsCount;
	sp4FloatArray *frames;
	int frameCount;
	int frameEntries;
	sp4TimelineType type;
};

SP4_API void sp4Timeline_dispose(sp4Timeline *self);

SP4_API void
sp4Timeline_apply(sp4Timeline *self, struct sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
				 int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction);

SP4_API void
sp4Timeline_setBezier(sp4Timeline *self, int bezier, int frame, float value, float time1, float value1, float cx1,
					 float cy1, float cx2, float cy2, float time2, float value2);

SP4_API float sp4Timeline_getDuration(const sp4Timeline *self);

/**/

typedef struct sp4CurveTimeline {
	sp4Timeline super;
	sp4FloatArray *curves; /* type, x, y, ... */
} sp4CurveTimeline;

SP4_API void sp4CurveTimeline_setLinear(sp4CurveTimeline *self, int frameIndex);

SP4_API void sp4CurveTimeline_setStepped(sp4CurveTimeline *self, int frameIndex);

/* Sets the control handle positions for an interpolation bezier curve used to transition from this keyframe to the next.
 * cx1 and cx2 are from 0 to 1, representing the percent of time between the two keyframes. cy1 and cy2 are the percent of
 * the difference between the keyframe's values. */
SP4_API void sp4CurveTimeline_setCurve(sp4CurveTimeline *self, int frameIndex, float cx1, float cy1, float cx2, float cy2);

SP4_API float sp4CurveTimeline_getCurvePercent(const sp4CurveTimeline *self, int frameIndex, float percent);

typedef struct sp4CurveTimeline sp4CurveTimeline1;

SP4_API void sp4CurveTimeline1_setFrame(sp4CurveTimeline1 *self, int frame, float time, float value);

SP4_API float sp4CurveTimeline1_getCurveValue(sp4CurveTimeline1 *self, float time);

typedef struct sp4CurveTimeline sp4CurveTimeline2;

SP4_API void sp4CurveTimeline2_setFrame(sp4CurveTimeline1 *self, int frame, float time, float value1, float value2);

/**/

typedef struct sp4RotateTimeline {
	sp4CurveTimeline1 super;
	int boneIndex;
} sp4RotateTimeline;

SP4_API sp4RotateTimeline *sp4RotateTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4RotateTimeline_setFrame(sp4RotateTimeline *self, int frameIndex, float time, float angle);

/**/

typedef struct sp4TranslateTimeline {
	sp4CurveTimeline2 super;
	int boneIndex;
} sp4TranslateTimeline;

SP4_API sp4TranslateTimeline *sp4TranslateTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4TranslateTimeline_setFrame(sp4TranslateTimeline *self, int frameIndex, float time, float x, float y);

/**/

typedef struct sp4TranslateXTimeline {
	sp4CurveTimeline1 super;
	int boneIndex;
} sp4TranslateXTimeline;

SP4_API sp4TranslateXTimeline *sp4TranslateXTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4TranslateXTimeline_setFrame(sp4TranslateXTimeline *self, int frame, float time, float x);

/**/

typedef struct sp4TranslateYTimeline {
	sp4CurveTimeline1 super;
	int boneIndex;
} sp4TranslateYTimeline;

SP4_API sp4TranslateYTimeline *sp4TranslateYTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4TranslateYTimeline_setFrame(sp4TranslateYTimeline *self, int frame, float time, float y);

/**/

typedef struct sp4ScaleTimeline {
	sp4CurveTimeline2 super;
	int boneIndex;
} sp4ScaleTimeline;

SP4_API sp4ScaleTimeline *sp4ScaleTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4ScaleTimeline_setFrame(sp4ScaleTimeline *self, int frameIndex, float time, float x, float y);

/**/

typedef struct sp4ScaleXTimeline {
	sp4CurveTimeline1 super;
	int boneIndex;
} sp4ScaleXTimeline;

SP4_API sp4ScaleXTimeline *sp4ScaleXTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4ScaleXTimeline_setFrame(sp4ScaleXTimeline *self, int frame, float time, float x);

/**/

typedef struct sp4ScaleYTimeline {
	sp4CurveTimeline1 super;
	int boneIndex;
} sp4ScaleYTimeline;

SP4_API sp4ScaleYTimeline *sp4ScaleYTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4ScaleYTimeline_setFrame(sp4ScaleYTimeline *self, int frame, float time, float y);

/**/

typedef struct sp4ShearTimeline {
	sp4CurveTimeline2 super;
	int boneIndex;
} sp4ShearTimeline;

SP4_API sp4ShearTimeline *sp4ShearTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4ShearTimeline_setFrame(sp4ShearTimeline *self, int frameIndex, float time, float x, float y);

/**/

typedef struct sp4ShearXTimeline {
	sp4CurveTimeline1 super;
	int boneIndex;
} sp4ShearXTimeline;

SP4_API sp4ShearXTimeline *sp4ShearXTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4ShearXTimeline_setFrame(sp4ShearXTimeline *self, int frame, float time, float x);

/**/

typedef struct sp4ShearYTimeline {
	sp4CurveTimeline1 super;
	int boneIndex;
} sp4ShearYTimeline;

SP4_API sp4ShearYTimeline *sp4ShearYTimeline_create(int frameCount, int bezierCount, int boneIndex);

SP4_API void sp4ShearYTimeline_setFrame(sp4ShearYTimeline *self, int frame, float time, float x);

/**/

typedef struct sp4RGBATimeline {
	sp4CurveTimeline2 super;
	int slotIndex;
} sp4RGBATimeline;

SP4_API sp4RGBATimeline *sp4RGBATimeline_create(int framesCount, int bezierCount, int slotIndex);

SP4_API void
sp4RGBATimeline_setFrame(sp4RGBATimeline *self, int frameIndex, float time, float r, float g, float b, float a);

/**/

typedef struct sp4RGBTimeline {
	sp4CurveTimeline2 super;
	int slotIndex;
} sp4RGBTimeline;

SP4_API sp4RGBTimeline *sp4RGBTimeline_create(int framesCount, int bezierCount, int slotIndex);

SP4_API void sp4RGBTimeline_setFrame(sp4RGBTimeline *self, int frameIndex, float time, float r, float g, float b);

/**/

typedef struct sp4AlphaTimeline {
	sp4CurveTimeline1 super;
	int slotIndex;
} sp4AlphaTimeline;

SP4_API sp4AlphaTimeline *sp4AlphaTimeline_create(int frameCount, int bezierCount, int slotIndex);

SP4_API void sp4AlphaTimeline_setFrame(sp4AlphaTimeline *self, int frame, float time, float x);

/**/

typedef struct sp4RGBA2Timeline {
	sp4CurveTimeline super;
	int slotIndex;
} sp4RGBA2Timeline;

SP4_API sp4RGBA2Timeline *sp4RGBA2Timeline_create(int framesCount, int bezierCount, int slotIndex);

SP4_API void
sp4RGBA2Timeline_setFrame(sp4RGBA2Timeline *self, int frameIndex, float time, float r, float g, float b, float a,
						 float r2, float g2, float b2);

/**/

typedef struct sp4RGB2Timeline {
	sp4CurveTimeline super;
	int slotIndex;
} sp4RGB2Timeline;

SP4_API sp4RGB2Timeline *sp4RGB2Timeline_create(int framesCount, int bezierCount, int slotIndex);

SP4_API void
sp4RGB2Timeline_setFrame(sp4RGB2Timeline *self, int frameIndex, float time, float r, float g, float b, float r2, float g2,
						float b2);

/**/

typedef struct sp4AttachmentTimeline {
	sp4Timeline super;
	int slotIndex;
	const char **const attachmentNames;
} sp4AttachmentTimeline;

SP4_API sp4AttachmentTimeline *sp4AttachmentTimeline_create(int framesCount, int SlotIndex);

/* @param attachmentName May be 0. */
SP4_API void
sp4AttachmentTimeline_setFrame(sp4AttachmentTimeline *self, int frameIndex, float time, const char *attachmentName);

/**/

typedef struct sp4DeformTimeline {
	sp4CurveTimeline super;
	int const frameVerticesCount;
	const float **const frameVertices;
	int slotIndex;
	sp4Attachment *attachment;
} sp4DeformTimeline;

SP4_API sp4DeformTimeline *
sp4DeformTimeline_create(int framesCount, int frameVerticesCount, int bezierCount, int slotIndex,
						sp4VertexAttachment *attachment);

SP4_API void sp4DeformTimeline_setFrame(sp4DeformTimeline *self, int frameIndex, float time, float *vertices);

/**/

typedef struct sp4EventTimeline {
	sp4Timeline super;
	sp4Event **const events;
} sp4EventTimeline;

SP4_API sp4EventTimeline *sp4EventTimeline_create(int framesCount);

SP4_API void sp4EventTimeline_setFrame(sp4EventTimeline *self, int frameIndex, sp4Event *event);

/**/

typedef struct sp4DrawOrderTimeline {
	sp4Timeline super;
	const int **const drawOrders;
	int const slotsCount;
} sp4DrawOrderTimeline;

SP4_API sp4DrawOrderTimeline *sp4DrawOrderTimeline_create(int framesCount, int slotsCount);

SP4_API void sp4DrawOrderTimeline_setFrame(sp4DrawOrderTimeline *self, int frameIndex, float time, const int *drawOrder);

/**/

typedef struct sp4IkConstraintTimeline {
	sp4CurveTimeline super;
	int ikConstraintIndex;
} sp4IkConstraintTimeline;

SP4_API sp4IkConstraintTimeline *
sp4IkConstraintTimeline_create(int framesCount, int bezierCount, int transformConstraintIndex);

SP4_API void
sp4IkConstraintTimeline_setFrame(sp4IkConstraintTimeline *self, int frameIndex, float time, float mix, float softness,
								int bendDirection, int /*boolean*/ compress, int /**boolean**/ stretch);

/**/

typedef struct sp4TransformConstraintTimeline {
	sp4CurveTimeline super;
	int transformConstraintIndex;
} sp4TransformConstraintTimeline;

SP4_API sp4TransformConstraintTimeline *
sp4TransformConstraintTimeline_create(int framesCount, int bezierCount, int transformConstraintIndex);

SP4_API void
sp4TransformConstraintTimeline_setFrame(sp4TransformConstraintTimeline *self, int frameIndex, float time, float mixRotate,
									   float mixX, float mixY, float mixScaleX, float mixScaleY, float mixShearY);

/**/

typedef struct sp4PathConstraintPositionTimeline {
	sp4CurveTimeline super;
	int pathConstraintIndex;
} sp4PathConstraintPositionTimeline;

SP4_API sp4PathConstraintPositionTimeline *
sp4PathConstraintPositionTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex);

SP4_API void
sp4PathConstraintPositionTimeline_setFrame(sp4PathConstraintPositionTimeline *self, int frameIndex, float time,
										  float value);

/**/

typedef struct sp4PathConstraintSpacingTimeline {
	sp4CurveTimeline super;
	int pathConstraintIndex;
} sp4PathConstraintSpacingTimeline;

SP4_API sp4PathConstraintSpacingTimeline *
sp4PathConstraintSpacingTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex);

SP4_API void sp4PathConstraintSpacingTimeline_setFrame(sp4PathConstraintSpacingTimeline *self, int frameIndex, float time,
													 float value);

/**/

typedef struct sp4PathConstraintMixTimeline {
	sp4CurveTimeline super;
	int pathConstraintIndex;
} sp4PathConstraintMixTimeline;

SP4_API sp4PathConstraintMixTimeline *
sp4PathConstraintMixTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex);

SP4_API void
sp4PathConstraintMixTimeline_setFrame(sp4PathConstraintMixTimeline *self, int frameIndex, float time, float mixRotate,
									 float mixX, float mixY);

/**/

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_ANIMATION_H_ */
