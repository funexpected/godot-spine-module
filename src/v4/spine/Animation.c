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

#include <limits.h>
#include <v4/spine/Animation.h>
#include <v4/spine/IkConstraint.h>
#include <v4/spine/extension.h>

_SP4_ARRAY_IMPLEMENT_TYPE(sp4PropertyIdArray, sp4PropertyId)

_SP4_ARRAY_IMPLEMENT_TYPE(sp4TimelineArray, sp4Timeline *)

sp4Animation *sp4Animation_create(const char *name, sp4TimelineArray *timelines, float duration) {
	int i, n;
	sp4Animation *self = NEW(sp4Animation);
	MALLOC_STR(self->name, name);
	self->timelines = timelines != NULL ? timelines : sp4TimelineArray_create(1);
	timelines = self->timelines;
	self->timelineIds = sp4PropertyIdArray_create(16);
	for (i = 0, n = timelines->size; i < n; i++) {
		sp4PropertyIdArray_addAllValues(self->timelineIds, timelines->items[i]->propertyIds, 0,
									   timelines->items[i]->propertyIdsCount);
	}
	self->duration = duration;
	return self;
}

void sp4Animation_dispose(sp4Animation *self) {
	int i;
	for (i = 0; i < self->timelines->size; ++i)
		sp4Timeline_dispose(self->timelines->items[i]);
	sp4TimelineArray_dispose(self->timelines);
	sp4PropertyIdArray_dispose(self->timelineIds);
	FREE(self->name);
	FREE(self);
}

int /*bool*/ sp4Animation_hasTimeline(sp4Animation *self, sp4PropertyId *ids, int idsCount) {
	int i, n, ii;
	for (i = 0, n = self->timelineIds->size; i < n; i++) {
		for (ii = 0; ii < idsCount; ii++) {
			if (self->timelineIds->items[i] == ids[ii]) return 1;
		}
	}
	return 0;
}

void sp4Animation_apply(const sp4Animation *self, sp4Skeleton *skeleton, float lastTime, float time, int loop, sp4Event **events,
					   int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	int i, n = self->timelines->size;

	if (loop && self->duration) {
		time = FMOD(time, self->duration);
		if (lastTime > 0) lastTime = FMOD(lastTime, self->duration);
	}

	for (i = 0; i < n; ++i)
		sp4Timeline_apply(self->timelines->items[i], skeleton, lastTime, time, events, eventsCount, alpha, blend,
						 direction);
}

static int search(sp4FloatArray
						  *values,
				  float time) {
	int i, n;
	float *items = values->items;
	for (
			i = 1, n = values->size;
			i < n;
			i++)
		if (items[i] > time) return i - 1;
	return values->size - 1;
}

static int search2(sp4FloatArray
						   *values,
				   float time,
				   int step) {
	int i, n;
	float *items = values->items;
	for (
			i = step, n = values->size;
			i < n;
			i += step)
		if (items[i] > time) return i -
									step;
	return values->size -
		   step;
}

/**/

void _sp4Timeline_init(sp4Timeline *self,
					  int frameCount,
					  int frameEntries,
					  sp4PropertyId *propertyIds,
					  int propertyIdsCount,
					  sp4TimelineType type,
					  void (*dispose)(sp4Timeline *self),
					  void (*apply)(sp4Timeline *self, sp4Skeleton *skeleton, float lastTime, float time,
									sp4Event **firedEvents,
									int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction),
					  void (*setBezier)(sp4Timeline *self, int bezier, int frame, float value, float time1, float value1,
										float cx1, float cy1,
										float cx2, float cy2, float time2, float value2)) {
	int i;
	self->frames = sp4FloatArray_create(frameCount * frameEntries);
	self->frames->size = frameCount * frameEntries;
	self->frameCount = frameCount;
	self->frameEntries = frameEntries;

	for (i = 0; i < propertyIdsCount; i++)
		self->propertyIds[i] = propertyIds[i];
	self->propertyIdsCount = propertyIdsCount;

	self->type = type;

	self->vtable.dispose = dispose;
	self->vtable.apply = apply;
	self->vtable.setBezier = setBezier;
}

void sp4Timeline_dispose(sp4Timeline *self) {
	self->vtable.dispose(self);
	sp4FloatArray_dispose(self->frames);
	FREE(self);
}

void sp4Timeline_apply(sp4Timeline *self, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
					  int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	self->vtable.apply(self, skeleton, lastTime, time, firedEvents, eventsCount, alpha, blend, direction);
}

void sp4Timeline_setBezier(sp4Timeline *self, int bezier, int frame, float value, float time1, float value1, float cx1,
						  float cy1, float cx2, float cy2, float time2, float value2) {
	if (self->vtable.setBezier)
		self->vtable.setBezier(self, bezier, frame, value, time1, value1, cx1, cy1, cx2, cy2, time2, value2);
}

float sp4Timeline_getDuration(const sp4Timeline *self) {
	return self->frames->items[self->frames->size - self->frameEntries];
}

/**/

#define CURVE_LINEAR 0
#define CURVE_STEPPED 1
#define CURVE_BEZIER 2
#define BEZIER_SIZE 18

void _sp4CurveTimeline_init(sp4CurveTimeline *self,
						   int frameCount,
						   int frameEntries,
						   int bezierCount,
						   sp4PropertyId *propertyIds,
						   int propertyIdsCount,
						   sp4TimelineType type,
						   void (*dispose)(sp4Timeline *self),
						   void (*apply)(sp4Timeline *self, sp4Skeleton *skeleton, float lastTime, float time,
										 sp4Event **firedEvents,
										 int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction),
						   void (*setBezier)(sp4Timeline *self, int bezier, int frame, float value, float time1,
											 float value1, float cx1, float cy1,
											 float cx2, float cy2, float time2, float value2)) {
	_sp4Timeline_init(SUPER(self), frameCount, frameEntries, propertyIds, propertyIdsCount, type, dispose, apply,
					 setBezier);
	self->curves = sp4FloatArray_create(frameCount + bezierCount * BEZIER_SIZE);
	self->curves->size = frameCount + bezierCount * BEZIER_SIZE;
	self->curves->items[frameCount - 1] = CURVE_STEPPED;
}

void _sp4CurveTimeline_dispose(sp4Timeline *self) {
	sp4FloatArray_dispose(SUB_CAST(sp4CurveTimeline, self)->curves);
}

void _sp4CurveTimeline_setBezier(sp4Timeline *timeline, int bezier, int frame, float value, float time1, float value1,
								float cx1, float cy1, float cx2, float cy2, float time2, float value2) {
	sp4CurveTimeline *self = SUB_CAST(sp4CurveTimeline, timeline);
	float tmpx, tmpy, dddx, dddy, ddx, ddy, dx, dy, x, y;
	int i = self->super.frameCount + bezier * BEZIER_SIZE, n;
	float *curves = self->curves->items;
	if (value == 0) curves[frame] = CURVE_BEZIER + i;
	tmpx = (time1 - cx1 * 2 + cx2) * 0.03;
	tmpy = (value1 - cy1 * 2 + cy2) * 0.03;
	dddx = ((cx1 - cx2) * 3 - time1 + time2) * 0.006;
	dddy = ((cy1 - cy2) * 3 - value1 + value2) * 0.006;
	ddx = tmpx * 2 + dddx;
	ddy = tmpy * 2 + dddy;
	dx = (cx1 - time1) * 0.3 + tmpx + dddx * 0.16666667;
	dy = (cy1 - value1) * 0.3 + tmpy + dddy * 0.16666667;
	x = time1 + dx, y = value1 + dy;
	for (n = i + BEZIER_SIZE; i < n; i += 2) {
		curves[i] = x;
		curves[i + 1] = y;
		dx += ddx;
		dy += ddy;
		ddx += dddx;
		ddy += dddy;
		x += dx;
		y += dy;
	}
}

float _sp4CurveTimeline_getBezierValue(sp4CurveTimeline *self, float time, int frameIndex, int valueOffset, int i) {
	float *curves = self->curves->items;
	float *frames = SUPER(self)->frames->items;
	float x, y;
	int n;
	if (curves[i] > time) {
		x = frames[frameIndex];
		y = frames[frameIndex + valueOffset];
		return y + (time - x) / (curves[i] - x) * (curves[i + 1] - y);
	}
	n = i + BEZIER_SIZE;
	for (i += 2; i < n; i += 2) {
		if (curves[i] >= time) {
			x = curves[i - 2];
			y = curves[i - 1];
			return y + (time - x) / (curves[i] - x) * (curves[i + 1] - y);
		}
	}
	frameIndex += self->super.frameEntries;
	x = curves[n - 2];
	y = curves[n - 1];
	return y + (time - x) / (frames[frameIndex] - x) * (frames[frameIndex + valueOffset] - y);
}

void sp4CurveTimeline_setLinear(sp4CurveTimeline *self, int frame) {
	self->curves->items[frame] = CURVE_LINEAR;
}

void sp4CurveTimeline_setStepped(sp4CurveTimeline *self, int frame) {
	self->curves->items[frame] = CURVE_STEPPED;
}

#define CURVE1_ENTRIES 2
#define CURVE1_VALUE 1

void sp4CurveTimeline1_setFrame(sp4CurveTimeline1 *self, int frame, float time, float value) {
	float *frames = self->super.frames->items;
	frame <<= 1;
	frames[frame] = time;
	frames[frame + CURVE1_VALUE] = value;
}

float sp4CurveTimeline1_getCurveValue(sp4CurveTimeline1 *self, float time) {
	float *frames = self->super.frames->items;
	float *curves = self->curves->items;
	int i = self->super.frames->size - 2;
	int ii, curveType;
	for (ii = 2; ii <= i; ii += 2) {
		if (frames[ii] > time) {
			i = ii - 2;
			break;
		}
	}

	curveType = (int) curves[i >> 1];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i], value = frames[i + CURVE1_VALUE];
			return value + (time - before) / (frames[i + CURVE1_ENTRIES] - before) *
								   (frames[i + CURVE1_ENTRIES + CURVE1_VALUE] - value);
		}
		case CURVE_STEPPED:
			return frames[i + CURVE1_VALUE];
	}
	return _sp4CurveTimeline_getBezierValue(self, time, i, CURVE1_VALUE, curveType - CURVE_BEZIER);
}

#define CURVE2_ENTRIES 3
#define CURVE2_VALUE1 1
#define CURVE2_VALUE2 2

SP4_API void sp4CurveTimeline2_setFrame(sp4CurveTimeline1 *self, int frame, float time, float value1, float value2) {
	float *frames = self->super.frames->items;
	frame *= CURVE2_ENTRIES;
	frames[frame] = time;
	frames[frame + CURVE2_VALUE1] = value1;
	frames[frame + CURVE2_VALUE2] = value2;
}

/**/

void _sp4RotateTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
							 int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	sp4Bone *bone;
	float r;
	sp4RotateTimeline *self = SUB_CAST(sp4RotateTimeline, timeline);
	float *frames = self->super.super.frames->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->rotation = bone->data->rotation;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->rotation += (bone->data->rotation - bone->rotation) * alpha;
			default: {
			}
		}
		return;
	}

	r = sp4CurveTimeline1_getCurveValue(SUPER(self), time);
	switch (blend) {
		case SP4_MIX_BLEND_SETUP:
			bone->rotation = bone->data->rotation + r * alpha;
			break;
		case SP4_MIX_BLEND_FIRST:
		case SP4_MIX_BLEND_REPLACE:
			r += bone->data->rotation - bone->rotation;
		case SP4_MIX_BLEND_ADD:
			bone->rotation += r * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4RotateTimeline *sp4RotateTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4RotateTimeline *timeline = NEW(sp4RotateTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_ROTATE << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE1_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_ROTATE,
						  _sp4CurveTimeline_dispose, _sp4RotateTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4RotateTimeline_setFrame(sp4RotateTimeline *self, int frame, float time, float degrees) {
	sp4CurveTimeline1_setFrame(SUPER(self), frame, time, degrees);
}

/**/

void _sp4TranslateTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
								sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
								sp4MixDirection direction) {
	sp4Bone *bone;
	float x, y, t;
	int i, curveType;

	sp4TranslateTimeline *self = SUB_CAST(sp4TranslateTimeline, timeline);
	float *frames = self->super.super.frames->items;
	float *curves = self->super.curves->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->x = bone->data->x;
				bone->y = bone->data->y;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->x += (bone->data->x - bone->x) * alpha;
				bone->y += (bone->data->y - bone->y) * alpha;
			default: {
			}
		}
		return;
	}

	i = search2(self->super.super.frames, time, CURVE2_ENTRIES);
	curveType = (int) curves[i / CURVE2_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			x = frames[i + CURVE2_VALUE1];
			y = frames[i + CURVE2_VALUE2];
			t = (time - before) / (frames[i + CURVE2_ENTRIES] - before);
			x += (frames[i + CURVE2_ENTRIES + CURVE2_VALUE1] - x) * t;
			y += (frames[i + CURVE2_ENTRIES + CURVE2_VALUE2] - y) * t;
			break;
		}
		case CURVE_STEPPED: {
			x = frames[i + CURVE2_VALUE1];
			y = frames[i + CURVE2_VALUE2];
			break;
		}
		default: {
			x = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, CURVE2_VALUE1, curveType - CURVE_BEZIER);
			y = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, CURVE2_VALUE2,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
		}
	}

	switch (blend) {
		case SP4_MIX_BLEND_SETUP:
			bone->x = bone->data->x + x * alpha;
			bone->y = bone->data->y + y * alpha;
			break;
		case SP4_MIX_BLEND_FIRST:
		case SP4_MIX_BLEND_REPLACE:
			bone->x += (bone->data->x + x - bone->x) * alpha;
			bone->y += (bone->data->y + y - bone->y) * alpha;
			break;
		case SP4_MIX_BLEND_ADD:
			bone->x += x * alpha;
			bone->y += y * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4TranslateTimeline *sp4TranslateTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4TranslateTimeline *timeline = NEW(sp4TranslateTimeline);
	sp4PropertyId ids[2];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_X << 32) | boneIndex;
	ids[1] = ((sp4PropertyId) SP4_PROPERTY_Y << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE2_ENTRIES, bezierCount, ids, 2, SP4_TIMELINE_TRANSLATE,
						  _sp4CurveTimeline_dispose, _sp4TranslateTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4TranslateTimeline_setFrame(sp4TranslateTimeline *self, int frame, float time, float x, float y) {
	sp4CurveTimeline2_setFrame(SUPER(self), frame, time, x, y);
}

/**/

void _sp4TranslateXTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
								 sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
								 sp4MixDirection direction) {
	sp4Bone *bone;
	float x;

	sp4TranslateXTimeline *self = SUB_CAST(sp4TranslateXTimeline, timeline);
	float *frames = self->super.super.frames->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->x = bone->data->x;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->x += (bone->data->x - bone->x) * alpha;
			default: {
			}
		}
		return;
	}

	x = sp4CurveTimeline1_getCurveValue(SUPER(self), time);
	switch (blend) {
		case SP4_MIX_BLEND_SETUP:
			bone->x = bone->data->x + x * alpha;
			break;
		case SP4_MIX_BLEND_FIRST:
		case SP4_MIX_BLEND_REPLACE:
			bone->x += (bone->data->x + x - bone->x) * alpha;
			break;
		case SP4_MIX_BLEND_ADD:
			bone->x += x * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4TranslateXTimeline *sp4TranslateXTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4TranslateXTimeline *timeline = NEW(sp4TranslateXTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_X << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE1_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_TRANSLATEX,
						  _sp4CurveTimeline_dispose, _sp4TranslateXTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4TranslateXTimeline_setFrame(sp4TranslateXTimeline *self, int frame, float time, float x) {
	sp4CurveTimeline1_setFrame(SUPER(self), frame, time, x);
}

/**/

void _sp4TranslateYTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
								 sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
								 sp4MixDirection direction) {
	sp4Bone *bone;
	float y;

	sp4TranslateYTimeline *self = SUB_CAST(sp4TranslateYTimeline, timeline);
	float *frames = self->super.super.frames->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->y = bone->data->y;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->y += (bone->data->y - bone->y) * alpha;
			default: {
			}
		}
		return;
	}

	y = sp4CurveTimeline1_getCurveValue(SUPER(self), time);
	switch (blend) {
		case SP4_MIX_BLEND_SETUP:
			bone->y = bone->data->y + y * alpha;
			break;
		case SP4_MIX_BLEND_FIRST:
		case SP4_MIX_BLEND_REPLACE:
			bone->y += (bone->data->y + y - bone->y) * alpha;
			break;
		case SP4_MIX_BLEND_ADD:
			bone->y += y * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4TranslateYTimeline *sp4TranslateYTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4TranslateYTimeline *timeline = NEW(sp4TranslateYTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_Y << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE1_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_TRANSLATEY,
						  _sp4CurveTimeline_dispose, _sp4TranslateYTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4TranslateYTimeline_setFrame(sp4TranslateYTimeline *self, int frame, float time, float y) {
	sp4CurveTimeline1_setFrame(SUPER(self), frame, time, y);
}

/**/

void _sp4ScaleTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
							int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	sp4Bone *bone;
	int i, curveType;
	float x, y, t;

	sp4ScaleTimeline *self = SUB_CAST(sp4ScaleTimeline, timeline);
	float *frames = self->super.super.frames->items;
	float *curves = self->super.curves->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;
	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->scaleX = bone->data->scaleX;
				bone->scaleY = bone->data->scaleY;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->scaleX += (bone->data->scaleX - bone->scaleX) * alpha;
				bone->scaleY += (bone->data->scaleY - bone->scaleY) * alpha;
			default: {
			}
		}
		return;
	}

	i = search2(self->super.super.frames, time, CURVE2_ENTRIES);
	curveType = (int) curves[i / CURVE2_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			x = frames[i + CURVE2_VALUE1];
			y = frames[i + CURVE2_VALUE2];
			t = (time - before) / (frames[i + CURVE2_ENTRIES] - before);
			x += (frames[i + CURVE2_ENTRIES + CURVE2_VALUE1] - x) * t;
			y += (frames[i + CURVE2_ENTRIES + CURVE2_VALUE2] - y) * t;
			break;
		}
		case CURVE_STEPPED: {
			x = frames[i + CURVE2_VALUE1];
			y = frames[i + CURVE2_VALUE2];
			break;
		}
		default: {
			x = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, CURVE2_VALUE1, curveType - CURVE_BEZIER);
			y = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, CURVE2_VALUE2,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
		}
	}
	x *= bone->data->scaleX;
	y *= bone->data->scaleY;

	if (alpha == 1) {
		if (blend == SP4_MIX_BLEND_ADD) {
			bone->scaleX += x - bone->data->scaleX;
			bone->scaleY += y - bone->data->scaleY;
		} else {
			bone->scaleX = x;
			bone->scaleY = y;
		}
	} else {
		float bx, by;
		if (direction == SP4_MIX_DIRECTION_OUT) {
			switch (blend) {
				case SP4_MIX_BLEND_SETUP:
					bx = bone->data->scaleX;
					by = bone->data->scaleY;
					bone->scaleX = bx + (ABS(x) * SIGNUM(bx) - bx) * alpha;
					bone->scaleY = by + (ABS(y) * SIGNUM(by) - by) * alpha;
					break;
				case SP4_MIX_BLEND_FIRST:
				case SP4_MIX_BLEND_REPLACE:
					bx = bone->scaleX;
					by = bone->scaleY;
					bone->scaleX = bx + (ABS(x) * SIGNUM(bx) - bx) * alpha;
					bone->scaleY = by + (ABS(y) * SIGNUM(by) - by) * alpha;
					break;
				case SP4_MIX_BLEND_ADD:
					bone->scaleX += (x - bone->data->scaleX) * alpha;
					bone->scaleY += (y - bone->data->scaleY) * alpha;
			}
		} else {
			switch (blend) {
				case SP4_MIX_BLEND_SETUP:
					bx = ABS(bone->data->scaleX) * SIGNUM(x);
					by = ABS(bone->data->scaleY) * SIGNUM(y);
					bone->scaleX = bx + (x - bx) * alpha;
					bone->scaleY = by + (y - by) * alpha;
					break;
				case SP4_MIX_BLEND_FIRST:
				case SP4_MIX_BLEND_REPLACE:
					bx = ABS(bone->scaleX) * SIGNUM(x);
					by = ABS(bone->scaleY) * SIGNUM(y);
					bone->scaleX = bx + (x - bx) * alpha;
					bone->scaleY = by + (y - by) * alpha;
					break;
				case SP4_MIX_BLEND_ADD:
					bone->scaleX += (x - bone->data->scaleX) * alpha;
					bone->scaleY += (y - bone->data->scaleY) * alpha;
			}
		}
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
}

sp4ScaleTimeline *sp4ScaleTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4ScaleTimeline *timeline = NEW(sp4ScaleTimeline);
	sp4PropertyId ids[2];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_SCALEX << 32) | boneIndex;
	ids[1] = ((sp4PropertyId) SP4_PROPERTY_SCALEY << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE2_ENTRIES, bezierCount, ids, 2, SP4_TIMELINE_SCALE,
						  _sp4CurveTimeline_dispose, _sp4ScaleTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4ScaleTimeline_setFrame(sp4ScaleTimeline *self, int frame, float time, float x, float y) {
	sp4CurveTimeline2_setFrame(SUPER(self), frame, time, x, y);
}

/**/

void _sp4ScaleXTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
							 sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
							 sp4MixDirection direction) {
	sp4Bone *bone;
	float x;

	sp4ScaleXTimeline *self = SUB_CAST(sp4ScaleXTimeline, timeline);
	float *frames = self->super.super.frames->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->scaleX = bone->data->scaleX;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->scaleX += (bone->data->scaleX - bone->scaleX) * alpha;
			default: {
			}
		}
		return;
	}

	x = sp4CurveTimeline1_getCurveValue(SUPER(self), time) * bone->data->scaleX;
	if (alpha == 1) {
		if (blend == SP4_MIX_BLEND_ADD)
			bone->scaleX += x - bone->data->scaleX;
		else
			bone->scaleX = x;
	} else {
		/* Mixing out uses sign of setup or current pose, else use sign of key. */
		float bx;
		if (direction == SP4_MIX_DIRECTION_OUT) {
			switch (blend) {
				case SP4_MIX_BLEND_SETUP:
					bx = bone->data->scaleX;
					bone->scaleX = bx + (ABS(x) * SIGNUM(bx) - bx) * alpha;
					break;
				case SP4_MIX_BLEND_FIRST:
				case SP4_MIX_BLEND_REPLACE:
					bx = bone->scaleX;
					bone->scaleX = bx + (ABS(x) * SIGNUM(bx) - bx) * alpha;
					break;
				case SP4_MIX_BLEND_ADD:
					bone->scaleX += (x - bone->data->scaleX) * alpha;
			}
		} else {
			switch (blend) {
				case SP4_MIX_BLEND_SETUP:
					bx = ABS(bone->data->scaleX) * SIGNUM(x);
					bone->scaleX = bx + (x - bx) * alpha;
					break;
				case SP4_MIX_BLEND_FIRST:
				case SP4_MIX_BLEND_REPLACE:
					bx = ABS(bone->scaleX) * SIGNUM(x);
					bone->scaleX = bx + (x - bx) * alpha;
					break;
				case SP4_MIX_BLEND_ADD:
					bone->scaleX += (x - bone->data->scaleX) * alpha;
			}
		}
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
}

sp4ScaleXTimeline *sp4ScaleXTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4ScaleXTimeline *timeline = NEW(sp4ScaleXTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_SCALEX << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE1_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_SCALEX,
						  _sp4CurveTimeline_dispose, _sp4ScaleXTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4ScaleXTimeline_setFrame(sp4ScaleXTimeline *self, int frame, float time, float y) {
	sp4CurveTimeline1_setFrame(SUPER(self), frame, time, y);
}

/**/

void _sp4ScaleYTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
							 sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
							 sp4MixDirection direction) {
	sp4Bone *bone;
	float y;

	sp4ScaleYTimeline *self = SUB_CAST(sp4ScaleYTimeline, timeline);
	float *frames = self->super.super.frames->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->scaleY = bone->data->scaleY;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->scaleY += (bone->data->scaleY - bone->scaleY) * alpha;
			default: {
			}
		}
		return;
	}

	y = sp4CurveTimeline1_getCurveValue(SUPER(self), time) * bone->data->scaleY;
	if (alpha == 1) {
		if (blend == SP4_MIX_BLEND_ADD)
			bone->scaleY += y - bone->data->scaleY;
		else
			bone->scaleY = y;
	} else {
		/* Mixing out uses sign of setup or current pose, else use sign of key. */
		float by = 0;
		if (direction == SP4_MIX_DIRECTION_OUT) {
			switch (blend) {
				case SP4_MIX_BLEND_SETUP:
					by = bone->data->scaleY;
					bone->scaleY = by + (ABS(y) * SIGNUM(by) - by) * alpha;
					break;
				case SP4_MIX_BLEND_FIRST:
				case SP4_MIX_BLEND_REPLACE:
					by = bone->scaleY;
					bone->scaleY = by + (ABS(y) * SIGNUM(by) - by) * alpha;
					break;
				case SP4_MIX_BLEND_ADD:
					bone->scaleY += (y - bone->data->scaleY) * alpha;
			}
		} else {
			switch (blend) {
				case SP4_MIX_BLEND_SETUP:
					by = ABS(bone->data->scaleY) * SIGNUM(y);
					bone->scaleY = by + (y - by) * alpha;
					break;
				case SP4_MIX_BLEND_FIRST:
				case SP4_MIX_BLEND_REPLACE:
					by = ABS(bone->scaleY) * SIGNUM(y);
					bone->scaleY = by + (y - by) * alpha;
					break;
				case SP4_MIX_BLEND_ADD:
					bone->scaleY += (y - bone->data->scaleY) * alpha;
			}
		}
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
}

sp4ScaleYTimeline *sp4ScaleYTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4ScaleYTimeline *timeline = NEW(sp4ScaleYTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_SCALEY << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE1_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_SCALEY,
						  _sp4CurveTimeline_dispose, _sp4ScaleYTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4ScaleYTimeline_setFrame(sp4ScaleYTimeline *self, int frame, float time, float y) {
	sp4CurveTimeline1_setFrame(SUPER(self), frame, time, y);
}

/**/

void _sp4ShearTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
							int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	sp4Bone *bone;
	float x, y, t;
	int i, curveType;

	sp4ShearTimeline *self = SUB_CAST(sp4ShearTimeline, timeline);
	float *frames = SUPER(self)->super.frames->items;
	float *curves = SUPER(self)->curves->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;
	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->shearX = bone->data->shearX;
				bone->shearY = bone->data->shearY;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->shearX += (bone->data->shearX - bone->shearX) * alpha;
				bone->shearY += (bone->data->shearY - bone->shearY) * alpha;
			default: {
			}
		}
		return;
	}

	i = search2(self->super.super.frames, time, CURVE2_ENTRIES);
	curveType = (int) curves[i / CURVE2_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			x = frames[i + CURVE2_VALUE1];
			y = frames[i + CURVE2_VALUE2];
			t = (time - before) / (frames[i + CURVE2_ENTRIES] - before);
			x += (frames[i + CURVE2_ENTRIES + CURVE2_VALUE1] - x) * t;
			y += (frames[i + CURVE2_ENTRIES + CURVE2_VALUE2] - y) * t;
			break;
		}
		case CURVE_STEPPED: {
			x = frames[i + CURVE2_VALUE1];
			y = frames[i + CURVE2_VALUE2];
			break;
		}
		default: {
			x = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, CURVE2_VALUE1, curveType - CURVE_BEZIER);
			y = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, CURVE2_VALUE2,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
		}
	}

	switch (blend) {
		case SP4_MIX_BLEND_SETUP:
			bone->shearX = bone->data->shearX + x * alpha;
			bone->shearY = bone->data->shearY + y * alpha;
			break;
		case SP4_MIX_BLEND_FIRST:
		case SP4_MIX_BLEND_REPLACE:
			bone->shearX += (bone->data->shearX + x - bone->shearX) * alpha;
			bone->shearY += (bone->data->shearY + y - bone->shearY) * alpha;
			break;
		case SP4_MIX_BLEND_ADD:
			bone->shearX += x * alpha;
			bone->shearY += y * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4ShearTimeline *sp4ShearTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4ShearTimeline *timeline = NEW(sp4ShearTimeline);
	sp4PropertyId ids[2];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_SHEARX << 32) | boneIndex;
	ids[1] = ((sp4PropertyId) SP4_PROPERTY_SHEARY << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE2_ENTRIES, bezierCount, ids, 2, SP4_TIMELINE_SHEAR,
						  _sp4CurveTimeline_dispose, _sp4ShearTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4ShearTimeline_setFrame(sp4ShearTimeline *self, int frame, float time, float x, float y) {
	sp4CurveTimeline2_setFrame(SUPER(self), frame, time, x, y);
}

/**/

void _sp4ShearXTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
							 sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
							 sp4MixDirection direction) {
	sp4Bone *bone;
	float x;

	sp4ShearXTimeline *self = SUB_CAST(sp4ShearXTimeline, timeline);
	float *frames = self->super.super.frames->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->shearX = bone->data->shearX;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->shearX += (bone->data->shearX - bone->shearX) * alpha;
			default: {
			}
		}
		return;
	}

	x = sp4CurveTimeline1_getCurveValue(SUPER(self), time);
	switch (blend) {
		case SP4_MIX_BLEND_SETUP:
			bone->shearX = bone->data->shearX + x * alpha;
			break;
		case SP4_MIX_BLEND_FIRST:
		case SP4_MIX_BLEND_REPLACE:
			bone->shearX += (bone->data->shearX + x - bone->shearX) * alpha;
			break;
		case SP4_MIX_BLEND_ADD:
			bone->shearX += x * alpha;
	}
	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4ShearXTimeline *sp4ShearXTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4ShearXTimeline *timeline = NEW(sp4ShearXTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_SHEARX << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE1_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_SHEARX,
						  _sp4CurveTimeline_dispose, _sp4ShearXTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4ShearXTimeline_setFrame(sp4ShearXTimeline *self, int frame, float time, float x) {
	sp4CurveTimeline1_setFrame(SUPER(self), frame, time, x);
}

/**/

void _sp4ShearYTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
							 sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
							 sp4MixDirection direction) {
	sp4Bone *bone;
	float y;

	sp4ShearYTimeline *self = SUB_CAST(sp4ShearYTimeline, timeline);
	float *frames = self->super.super.frames->items;

	bone = skeleton->bones[self->boneIndex];
	if (!bone->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				bone->shearY = bone->data->shearY;
				return;
			case SP4_MIX_BLEND_FIRST:
				bone->shearY += (bone->data->shearY - bone->shearY) * alpha;
			default: {
			}
		}
		return;
	}

	y = sp4CurveTimeline1_getCurveValue(SUPER(self), time);
	switch (blend) {
		case SP4_MIX_BLEND_SETUP:
			bone->shearY = bone->data->shearY + y * alpha;
			break;
		case SP4_MIX_BLEND_FIRST:
		case SP4_MIX_BLEND_REPLACE:
			bone->shearY += (bone->data->shearY + y - bone->shearY) * alpha;
			break;
		case SP4_MIX_BLEND_ADD:
			bone->shearY += y * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4ShearYTimeline *sp4ShearYTimeline_create(int frameCount, int bezierCount, int boneIndex) {
	sp4ShearYTimeline *timeline = NEW(sp4ShearYTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_SHEARY << 32) | boneIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE1_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_SHEARY,
						  _sp4CurveTimeline_dispose, _sp4ShearYTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->boneIndex = boneIndex;
	return timeline;
}

void sp4ShearYTimeline_setFrame(sp4ShearYTimeline *self, int frame, float time, float y) {
	sp4CurveTimeline1_setFrame(SUPER(self), frame, time, y);
}

/**/

static const int RGBA_ENTRIES = 5, COLOR_R = 1, COLOR_G = 2, COLOR_B = 3, COLOR_A = 4;

void _sp4RGBATimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
						   int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	sp4Slot *slot;
	int i, curveType;
	float r, g, b, a, t;
	sp4Color *color;
	sp4Color *setup;
	sp4RGBATimeline *self = (sp4RGBATimeline *) timeline;
	float *frames = self->super.super.frames->items;
	float *curves = self->super.curves->items;

	slot = skeleton->slots[self->slotIndex];
	if (!slot->bone->active) return;

	if (time < frames[0]) {
		color = &slot->color;
		setup = &slot->data->color;
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				sp4Color_setFromColor(color, setup);
				return;
			case SP4_MIX_BLEND_FIRST:
				sp4Color_addFloats(color, (setup->r - color->r) * alpha, (setup->g - color->g) * alpha,
								  (setup->b - color->b) * alpha,
								  (setup->a - color->a) * alpha);
			default: {
			}
		}
		return;
	}

	i = search2(self->super.super.frames, time, RGBA_ENTRIES);
	curveType = (int) curves[i / RGBA_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			r = frames[i + COLOR_R];
			g = frames[i + COLOR_G];
			b = frames[i + COLOR_B];
			a = frames[i + COLOR_A];
			t = (time - before) / (frames[i + RGBA_ENTRIES] - before);
			r += (frames[i + RGBA_ENTRIES + COLOR_R] - r) * t;
			g += (frames[i + RGBA_ENTRIES + COLOR_G] - g) * t;
			b += (frames[i + RGBA_ENTRIES + COLOR_B] - b) * t;
			a += (frames[i + RGBA_ENTRIES + COLOR_A] - a) * t;
			break;
		}
		case CURVE_STEPPED: {
			r = frames[i + COLOR_R];
			g = frames[i + COLOR_G];
			b = frames[i + COLOR_B];
			a = frames[i + COLOR_A];
			break;
		}
		default: {
			r = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_R, curveType - CURVE_BEZIER);
			g = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_G,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
			b = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_B,
												curveType + BEZIER_SIZE * 2 - CURVE_BEZIER);
			a = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_A,
												curveType + BEZIER_SIZE * 3 - CURVE_BEZIER);
		}
	}
	color = &slot->color;
	if (alpha == 1)
		sp4Color_setFromFloats(color, r, g, b, a);
	else {
		if (blend == SP4_MIX_BLEND_SETUP) sp4Color_setFromColor(color, &slot->data->color);
		sp4Color_addFloats(color, (r - color->r) * alpha, (g - color->g) * alpha, (b - color->b) * alpha,
						  (a - color->a) * alpha);
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4RGBATimeline *sp4RGBATimeline_create(int framesCount, int bezierCount, int slotIndex) {
	sp4RGBATimeline *timeline = NEW(sp4RGBATimeline);
	sp4PropertyId ids[2];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_RGB << 32) | slotIndex;
	ids[1] = ((sp4PropertyId) SP4_PROPERTY_ALPHA << 32) | slotIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, RGBA_ENTRIES, bezierCount, ids, 2, SP4_TIMELINE_RGBA,
						  _sp4CurveTimeline_dispose, _sp4RGBATimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->slotIndex = slotIndex;
	return timeline;
}

void sp4RGBATimeline_setFrame(sp4RGBATimeline *self, int frame, float time, float r, float g, float b, float a) {
	float *frames = self->super.super.frames->items;
	frame *= RGBA_ENTRIES;
	frames[frame] = time;
	frames[frame + COLOR_R] = r;
	frames[frame + COLOR_G] = g;
	frames[frame + COLOR_B] = b;
	frames[frame + COLOR_A] = a;
}

/**/

#define RGB_ENTRIES 4

void _sp4RGBTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
						  int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	sp4Slot *slot;
	int i, curveType;
	float r, g, b, t;
	sp4Color *color;
	sp4Color *setup;
	sp4RGBTimeline *self = (sp4RGBTimeline *) timeline;
	float *frames = self->super.super.frames->items;
	float *curves = self->super.curves->items;

	slot = skeleton->slots[self->slotIndex];
	if (!slot->bone->active) return;

	if (time < frames[0]) {
		color = &slot->color;
		setup = &slot->data->color;
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				sp4Color_setFromColor(color, setup);
				return;
			case SP4_MIX_BLEND_FIRST:
				sp4Color_addFloats(color, (setup->r - color->r) * alpha, (setup->g - color->g) * alpha,
								  (setup->b - color->b) * alpha,
								  (setup->a - color->a) * alpha);
			default: {
			}
		}
		return;
	}

	i = search2(self->super.super.frames, time, RGB_ENTRIES);
	curveType = (int) curves[i / RGB_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			r = frames[i + COLOR_R];
			g = frames[i + COLOR_G];
			b = frames[i + COLOR_B];
			t = (time - before) / (frames[i + RGB_ENTRIES] - before);
			r += (frames[i + RGB_ENTRIES + COLOR_R] - r) * t;
			g += (frames[i + RGB_ENTRIES + COLOR_G] - g) * t;
			b += (frames[i + RGB_ENTRIES + COLOR_B] - b) * t;
			break;
		}
		case CURVE_STEPPED: {
			r = frames[i + COLOR_R];
			g = frames[i + COLOR_G];
			b = frames[i + COLOR_B];
			break;
		}
		default: {
			r = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_R, curveType - CURVE_BEZIER);
			g = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_G,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
			b = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_B,
												curveType + BEZIER_SIZE * 2 - CURVE_BEZIER);
		}
	}
	color = &slot->color;
	if (alpha == 1) {
		color->r = r;
		color->g = g;
		color->b = b;
	} else {
		if (blend == SP4_MIX_BLEND_SETUP) {
			color->r = slot->data->color.r;
			color->g = slot->data->color.g;
			color->b = slot->data->color.b;
		}
		color->r += (r - color->r) * alpha;
		color->g += (g - color->g) * alpha;
		color->b += (b - color->b) * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4RGBTimeline *sp4RGBTimeline_create(int framesCount, int bezierCount, int slotIndex) {
	sp4RGBTimeline *timeline = NEW(sp4RGBTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_RGB << 32) | slotIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, RGB_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_RGB,
						  _sp4CurveTimeline_dispose, _sp4RGBTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->slotIndex = slotIndex;
	return timeline;
}

void sp4RGBTimeline_setFrame(sp4RGBTimeline *self, int frame, float time, float r, float g, float b) {
	float *frames = self->super.super.frames->items;
	frame *= RGB_ENTRIES;
	frames[frame] = time;
	frames[frame + COLOR_R] = r;
	frames[frame + COLOR_G] = g;
	frames[frame + COLOR_B] = b;
}

/**/

void _sp4AlphaTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
							sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
							sp4MixDirection direction) {
	sp4Slot *slot;
	float a;
	sp4Color *color;
	sp4Color *setup;
	sp4AlphaTimeline *self = (sp4AlphaTimeline *) timeline;
	float *frames = self->super.super.frames->items;

	slot = skeleton->slots[self->slotIndex];
	if (!slot->bone->active) return;

	if (time < frames[0]) { /* Time is before first frame-> */
		color = &slot->color;
		setup = &slot->data->color;
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				color->a = setup->a;
				return;
			case SP4_MIX_BLEND_FIRST:
				color->a += (setup->a - color->a) * alpha;
			default: {
			}
		}
		return;
	}

	a = sp4CurveTimeline1_getCurveValue(SUPER(self), time);
	if (alpha == 1)
		slot->color.a = a;
	else {
		if (blend == SP4_MIX_BLEND_SETUP) slot->color.a = slot->data->color.a;
		slot->color.a += (a - slot->color.a) * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4AlphaTimeline *sp4AlphaTimeline_create(int frameCount, int bezierCount, int slotIndex) {
	sp4AlphaTimeline *timeline = NEW(sp4AlphaTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_ALPHA << 32) | slotIndex;
	_sp4CurveTimeline_init(SUPER(timeline), frameCount, CURVE1_ENTRIES, bezierCount, ids, 1, SP4_TIMELINE_ALPHA,
						  _sp4CurveTimeline_dispose, _sp4AlphaTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->slotIndex = slotIndex;
	return timeline;
}

void sp4AlphaTimeline_setFrame(sp4AlphaTimeline *self, int frame, float time, float alpha) {
	sp4CurveTimeline1_setFrame(SUPER(self), frame, time, alpha);
}

/**/

static const int RGBA2_ENTRIES = 8, COLOR_R2 = 5, COLOR_G2 = 6, COLOR_B2 = 7;

void _sp4RGBA2Timeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
							int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	sp4Slot *slot;
	int i, curveType;
	float r, g, b, a, r2, g2, b2, t;
	sp4Color *light, *setupLight;
	sp4Color *dark, *setupDark;
	sp4RGBA2Timeline *self = (sp4RGBA2Timeline *) timeline;
	float *frames = self->super.super.frames->items;
	float *curves = self->super.curves->items;

	slot = skeleton->slots[self->slotIndex];
	if (!slot->bone->active) return;

	if (time < frames[0]) {
		light = &slot->color;
		dark = slot->darkColor;
		setupLight = &slot->data->color;
		setupDark = slot->data->darkColor;
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				sp4Color_setFromColor(light, setupLight);
				sp4Color_setFromFloats3(dark, setupDark->r, setupDark->g, setupDark->b);
				return;
			case SP4_MIX_BLEND_FIRST:
				sp4Color_addFloats(light, (setupLight->r - light->r) * alpha, (setupLight->g - light->g) * alpha,
								  (setupLight->b - light->b) * alpha,
								  (setupLight->a - light->a) * alpha);
				dark->r += (setupDark->r - dark->r) * alpha;
				dark->g += (setupDark->g - dark->g) * alpha;
				dark->b += (setupDark->b - dark->b) * alpha;
			default: {
			}
		}
		return;
	}

	r = 0, g = 0, b = 0, a = 0, r2 = 0, g2 = 0, b2 = 0;
	i = search2(self->super.super.frames, time, RGBA2_ENTRIES);
	curveType = (int) curves[i / RGBA2_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			r = frames[i + COLOR_R];
			g = frames[i + COLOR_G];
			b = frames[i + COLOR_B];
			a = frames[i + COLOR_A];
			r2 = frames[i + COLOR_R2];
			g2 = frames[i + COLOR_G2];
			b2 = frames[i + COLOR_B2];
			t = (time - before) / (frames[i + RGBA2_ENTRIES] - before);
			r += (frames[i + RGBA2_ENTRIES + COLOR_R] - r) * t;
			g += (frames[i + RGBA2_ENTRIES + COLOR_G] - g) * t;
			b += (frames[i + RGBA2_ENTRIES + COLOR_B] - b) * t;
			a += (frames[i + RGBA2_ENTRIES + COLOR_A] - a) * t;
			r2 += (frames[i + RGBA2_ENTRIES + COLOR_R2] - r2) * t;
			g2 += (frames[i + RGBA2_ENTRIES + COLOR_G2] - g2) * t;
			b2 += (frames[i + RGBA2_ENTRIES + COLOR_B2] - b2) * t;
			break;
		}
		case CURVE_STEPPED: {
			r = frames[i + COLOR_R];
			g = frames[i + COLOR_G];
			b = frames[i + COLOR_B];
			a = frames[i + COLOR_A];
			r2 = frames[i + COLOR_R2];
			g2 = frames[i + COLOR_G2];
			b2 = frames[i + COLOR_B2];
			break;
		}
		default: {
			r = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_R, curveType - CURVE_BEZIER);
			g = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_G,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
			b = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_B,
												curveType + BEZIER_SIZE * 2 - CURVE_BEZIER);
			a = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_A,
												curveType + BEZIER_SIZE * 3 - CURVE_BEZIER);
			r2 = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_R2,
												 curveType + BEZIER_SIZE * 4 - CURVE_BEZIER);
			g2 = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_G2,
												 curveType + BEZIER_SIZE * 5 - CURVE_BEZIER);
			b2 = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_B2,
												 curveType + BEZIER_SIZE * 6 - CURVE_BEZIER);
		}
	}

	light = &slot->color, dark = slot->darkColor;
	if (alpha == 1) {
		sp4Color_setFromFloats(light, r, g, b, a);
		sp4Color_setFromFloats3(dark, r2, g2, b2);
	} else {
		if (blend == SP4_MIX_BLEND_SETUP) {
			sp4Color_setFromColor(light, &slot->data->color);
			sp4Color_setFromColor(dark, slot->data->darkColor);
		}
		sp4Color_addFloats(light, (r - light->r) * alpha, (g - light->g) * alpha, (b - light->b) * alpha,
						  (a - light->a) * alpha);
		dark->r += (r2 - dark->r) * alpha;
		dark->g += (g2 - dark->g) * alpha;
		dark->b += (b2 - dark->b) * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4RGBA2Timeline *sp4RGBA2Timeline_create(int framesCount, int bezierCount, int slotIndex) {
	sp4RGBA2Timeline *timeline = NEW(sp4RGBA2Timeline);
	sp4PropertyId ids[3];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_RGB << 32) | slotIndex;
	ids[1] = ((sp4PropertyId) SP4_PROPERTY_ALPHA << 32) | slotIndex;
	ids[2] = ((sp4PropertyId) SP4_PROPERTY_RGB2 << 32) | slotIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, RGBA2_ENTRIES, bezierCount, ids, 3, SP4_TIMELINE_RGBA2,
						  _sp4CurveTimeline_dispose, _sp4RGBA2Timeline_apply, _sp4CurveTimeline_setBezier);
	timeline->slotIndex = slotIndex;
	return timeline;
}

void sp4RGBA2Timeline_setFrame(sp4RGBA2Timeline *self, int frame, float time, float r, float g, float b, float a, float r2,
							  float g2, float b2) {
	float *frames = self->super.super.frames->items;
	frame *= RGBA2_ENTRIES;
	frames[frame] = time;
	frames[frame + COLOR_R] = r;
	frames[frame + COLOR_G] = g;
	frames[frame + COLOR_B] = b;
	frames[frame + COLOR_A] = a;
	frames[frame + COLOR_R2] = r2;
	frames[frame + COLOR_G2] = g2;
	frames[frame + COLOR_B2] = b2;
}

/**/

static const int RGB2_ENTRIES = 7, COLOR2_R2 = 5, COLOR2_G2 = 6, COLOR2_B2 = 7;

void _sp4RGB2Timeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
						   int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	sp4Slot *slot;
	int i, curveType;
	float r, g, b, r2, g2, b2, t;
	sp4Color *light, *setupLight;
	sp4Color *dark, *setupDark;
	sp4RGB2Timeline *self = (sp4RGB2Timeline *) timeline;
	float *frames = self->super.super.frames->items;
	float *curves = self->super.curves->items;

	slot = skeleton->slots[self->slotIndex];
	if (!slot->bone->active) return;

	if (time < frames[0]) {
		light = &slot->color;
		dark = slot->darkColor;
		setupLight = &slot->data->color;
		setupDark = slot->data->darkColor;
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				sp4Color_setFromColor3(light, setupLight);
				sp4Color_setFromColor3(dark, setupDark);
				return;
			case SP4_MIX_BLEND_FIRST:
				sp4Color_addFloats3(light, (setupLight->r - light->r) * alpha, (setupLight->g - light->g) * alpha,
								   (setupLight->b - light->b) * alpha);
				dark->r += (setupDark->r - dark->r) * alpha;
				dark->g += (setupDark->g - dark->g) * alpha;
				dark->b += (setupDark->b - dark->b) * alpha;
			default: {
			}
		}
		return;
	}

	r = 0, g = 0, b = 0, r2 = 0, g2 = 0, b2 = 0;
	i = search2(self->super.super.frames, time, RGB2_ENTRIES);
	curveType = (int) curves[i / RGB2_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			r = frames[i + COLOR_R];
			g = frames[i + COLOR_G];
			b = frames[i + COLOR_B];
			r2 = frames[i + COLOR2_R2];
			g2 = frames[i + COLOR2_G2];
			b2 = frames[i + COLOR2_B2];
			t = (time - before) / (frames[i + RGB2_ENTRIES] - before);
			r += (frames[i + RGB2_ENTRIES + COLOR_R] - r) * t;
			g += (frames[i + RGB2_ENTRIES + COLOR_G] - g) * t;
			b += (frames[i + RGB2_ENTRIES + COLOR_B] - b) * t;
			r2 += (frames[i + RGB2_ENTRIES + COLOR2_R2] - r2) * t;
			g2 += (frames[i + RGB2_ENTRIES + COLOR2_G2] - g2) * t;
			b2 += (frames[i + RGB2_ENTRIES + COLOR2_B2] - b2) * t;
			break;
		}
		case CURVE_STEPPED: {
			r = frames[i + COLOR_R];
			g = frames[i + COLOR_G];
			b = frames[i + COLOR_B];
			r2 = frames[i + COLOR2_R2];
			g2 = frames[i + COLOR2_G2];
			b2 = frames[i + COLOR2_B2];
			break;
		}
		default: {
			r = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_R, curveType - CURVE_BEZIER);
			g = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_G,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
			b = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR_B,
												curveType + BEZIER_SIZE * 2 - CURVE_BEZIER);
			r2 = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR2_R2,
												 curveType + BEZIER_SIZE * 3 - CURVE_BEZIER);
			g2 = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR2_G2,
												 curveType + BEZIER_SIZE * 4 - CURVE_BEZIER);
			b2 = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, COLOR2_B2,
												 curveType + BEZIER_SIZE * 5 - CURVE_BEZIER);
		}
	}

	light = &slot->color, dark = slot->darkColor;
	if (alpha == 1) {
		sp4Color_setFromFloats3(light, r, g, b);
		sp4Color_setFromFloats3(dark, r2, g2, b2);
	} else {
		if (blend == SP4_MIX_BLEND_SETUP) {
			sp4Color_setFromColor3(light, &slot->data->color);

			sp4Color_setFromColor3(dark, slot->data->darkColor);
		}
		sp4Color_addFloats3(light, (r - light->r) * alpha, (g - light->g) * alpha, (b - light->b) * alpha);
		dark->r += (r2 - dark->r) * alpha;
		dark->g += (g2 - dark->g) * alpha;
		dark->b += (b2 - dark->b) * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4RGB2Timeline *sp4RGB2Timeline_create(int framesCount, int bezierCount, int slotIndex) {
	sp4RGB2Timeline *timeline = NEW(sp4RGB2Timeline);
	sp4PropertyId ids[2];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_RGB << 32) | slotIndex;
	ids[1] = ((sp4PropertyId) SP4_PROPERTY_RGB2 << 32) | slotIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, RGB2_ENTRIES, bezierCount, ids, 2, SP4_TIMELINE_RGB2,
						  _sp4CurveTimeline_dispose, _sp4RGB2Timeline_apply, _sp4CurveTimeline_setBezier);
	timeline->slotIndex = slotIndex;
	return timeline;
}

void sp4RGB2Timeline_setFrame(sp4RGB2Timeline *self, int frame, float time, float r, float g, float b, float r2, float g2,
							 float b2) {
	float *frames = self->super.super.frames->items;
	frame *= RGB2_ENTRIES;
	frames[frame] = time;
	frames[frame + COLOR_R] = r;
	frames[frame + COLOR_G] = g;
	frames[frame + COLOR_B] = b;
	frames[frame + COLOR2_R2] = r2;
	frames[frame + COLOR2_G2] = g2;
	frames[frame + COLOR2_B2] = b2;
}

/**/

static void
_sp4SetAttachment(sp4AttachmentTimeline *timeline, sp4Skeleton *skeleton, sp4Slot *slot, const char *attachmentName) {
	sp4Slot_setAttachment(slot, attachmentName == NULL ? NULL : sp4Skeleton_getAttachmentForSlotIndex(skeleton, timeline->slotIndex, attachmentName));
}

void _sp4AttachmentTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
								 sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
								 sp4MixDirection direction) {
	const char *attachmentName;
	sp4AttachmentTimeline *self = (sp4AttachmentTimeline *) timeline;
	float *frames = self->super.frames->items;
	sp4Slot *slot = skeleton->slots[self->slotIndex];
	if (!slot->bone->active) return;

	if (direction == SP4_MIX_DIRECTION_OUT) {
		if (blend == SP4_MIX_BLEND_SETUP) {
			_sp4SetAttachment(self, skeleton, slot, slot->data->attachmentName);
		}
		return;
	}

	if (time < frames[0]) {
		if (blend == SP4_MIX_BLEND_SETUP || blend == SP4_MIX_BLEND_FIRST) {
			_sp4SetAttachment(self, skeleton, slot, slot->data->attachmentName);
		}
		return;
	}

	if (time < frames[0]) {
		if (blend == SP4_MIX_BLEND_SETUP || blend == SP4_MIX_BLEND_FIRST)
			_sp4SetAttachment(self, skeleton, slot, slot->data->attachmentName);
		return;
	}

	attachmentName = self->attachmentNames[search(self->super.frames, time)];
	_sp4SetAttachment(self, skeleton, slot, attachmentName);

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(alpha);
}

void _sp4AttachmentTimeline_dispose(sp4Timeline *timeline) {
	sp4AttachmentTimeline *self = SUB_CAST(sp4AttachmentTimeline, timeline);
	int i;
	for (i = 0; i < self->super.frames->size; ++i)
		FREE(self->attachmentNames[i]);
	FREE(self->attachmentNames);
}

sp4AttachmentTimeline *sp4AttachmentTimeline_create(int framesCount, int slotIndex) {
	sp4AttachmentTimeline *self = NEW(sp4AttachmentTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_ATTACHMENT << 32) | slotIndex;
	_sp4Timeline_init(SUPER(self), framesCount, 1, ids, 1, SP4_TIMELINE_ATTACHMENT, _sp4AttachmentTimeline_dispose,
					 _sp4AttachmentTimeline_apply, 0);
	CONST_CAST(char **, self->attachmentNames) = CALLOC(char *, framesCount);
	self->slotIndex = slotIndex;
	return self;
}

void sp4AttachmentTimeline_setFrame(sp4AttachmentTimeline *self, int frame, float time, const char *attachmentName) {
	self->super.frames->items[frame] = time;

	FREE(self->attachmentNames[frame]);
	if (attachmentName)
		MALLOC_STR(self->attachmentNames[frame], attachmentName);
	else
		self->attachmentNames[frame] = 0;
}

/**/

void _sp4DeformTimeline_setBezier(sp4Timeline *timeline, int bezier, int frame, float value, float time1, float value1,
								 float cx1, float cy1,
								 float cx2, float cy2, float time2, float value2) {
	sp4DeformTimeline *self = SUB_CAST(sp4DeformTimeline, timeline);
	int n, i = self->super.super.frameCount + bezier * BEZIER_SIZE;
	float *curves = self->super.curves->items;
	float tmpx = (time1 - cx1 * 2 + cx2) * 0.03, tmpy = cy2 * 0.03 - cy1 * 0.06;
	float dddx = ((cx1 - cx2) * 3 - time1 + time2) * 0.006, dddy = (cy1 - cy2 + 0.33333333) * 0.018;
	float ddx = tmpx * 2 + dddx, ddy = tmpy * 2 + dddy;
	float dx = (cx1 - time1) * 0.3 + tmpx + dddx * 0.16666667, dy = cy1 * 0.3 + tmpy + dddy * 0.16666667;
	float x = time1 + dx, y = dy;
	if (value == 0) curves[frame] = CURVE_BEZIER + i;
	for (n = i + BEZIER_SIZE; i < n; i += 2) {
		curves[i] = x;
		curves[i + 1] = y;
		dx += ddx;
		dy += ddy;
		ddx += dddx;
		ddy += dddy;
		x += dx;
		y += dy;
	}

	UNUSED(value1);
	UNUSED(value2);
}

float _sp4DeformTimeline_getCurvePercent(sp4DeformTimeline *self, float time, int frame) {
	float *curves = self->super.curves->items;
	float *frames = self->super.super.frames->items;
	int n, i = (int) curves[frame];
	int frameEntries = self->super.super.frameEntries;
	float x, y;
	switch (i) {
		case CURVE_LINEAR: {
			x = frames[frame];
			return (time - x) / (frames[frame + frameEntries] - x);
		}
		case CURVE_STEPPED: {
			return 0;
		}
		default: {
		}
	}
	i -= CURVE_BEZIER;
	if (curves[i] > time) {
		x = frames[frame];
		return curves[i + 1] * (time - x) / (curves[i] - x);
	}
	n = i + BEZIER_SIZE;
	for (i += 2; i < n; i += 2) {
		if (curves[i] >= time) {
			x = curves[i - 2], y = curves[i - 1];
			return y + (time - x) / (curves[i] - x) * (curves[i + 1] - y);
		}
	}
	x = curves[n - 2], y = curves[n - 1];
	return y + (1 - y) * (time - x) / (frames[frame + frameEntries] - x);
}

void _sp4DeformTimeline_apply(
		sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
		int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	int frame, i, vertexCount;
	float percent;
	const float *prevVertices;
	const float *nextVertices;
	float *frames;
	int framesCount;
	const float **frameVertices;
	float *deformArray;
	sp4DeformTimeline *self = (sp4DeformTimeline *) timeline;

	sp4Slot *slot = skeleton->slots[self->slotIndex];
	if (!slot->bone->active) return;

	if (!slot->attachment) return;
	switch (slot->attachment->type) {
		case SP4_ATTACHMENT_BOUNDING_BOX:
		case SP4_ATTACHMENT_CLIPPING:
		case SP4_ATTACHMENT_MESH:
		case SP4_ATTACHMENT_PATH: {
			sp4VertexAttachment *vertexAttachment = SUB_CAST(sp4VertexAttachment, slot->attachment);
			if (vertexAttachment->deformAttachment != SUB_CAST(sp4VertexAttachment, self->attachment)) return;
			break;
		}
		default:
			return;
	}

	frames = self->super.super.frames->items;
	framesCount = self->super.super.frames->size;
	vertexCount = self->frameVerticesCount;
	if (slot->deformCount < vertexCount) {
		if (slot->deformCapacity < vertexCount) {
			FREE(slot->deform);
			slot->deform = MALLOC(float, vertexCount);
			slot->deformCapacity = vertexCount;
		}
	}
	if (slot->deformCount == 0) blend = SP4_MIX_BLEND_SETUP;

	frameVertices = self->frameVertices;
	deformArray = slot->deform;

	if (time < frames[0]) { /* Time is before first frame. */
		sp4VertexAttachment *vertexAttachment = SUB_CAST(sp4VertexAttachment, slot->attachment);
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				slot->deformCount = 0;
				return;
			case SP4_MIX_BLEND_FIRST:
				if (alpha == 1) {
					slot->deformCount = 0;
					return;
				}
				slot->deformCount = vertexCount;
				if (!vertexAttachment->bones) {
					float *setupVertices = vertexAttachment->vertices;
					for (i = 0; i < vertexCount; i++) {
						deformArray[i] += (setupVertices[i] - deformArray[i]) * alpha;
					}
				} else {
					alpha = 1 - alpha;
					for (i = 0; i < vertexCount; i++) {
						deformArray[i] *= alpha;
					}
				}
			case SP4_MIX_BLEND_REPLACE:
			case SP4_MIX_BLEND_ADD:; /* to appease compiler */
		}
		return;
	}

	slot->deformCount = vertexCount;
	if (time >= frames[framesCount - 1]) { /* Time is after last frame. */
		const float *lastVertices = self->frameVertices[framesCount - 1];
		if (alpha == 1) {
			if (blend == SP4_MIX_BLEND_ADD) {
				sp4VertexAttachment *vertexAttachment = SUB_CAST(sp4VertexAttachment, slot->attachment);
				if (!vertexAttachment->bones) {
					/* Unweighted vertex positions, with alpha. */
					float *setupVertices = vertexAttachment->vertices;
					for (i = 0; i < vertexCount; i++) {
						deformArray[i] += lastVertices[i] - setupVertices[i];
					}
				} else {
					/* Weighted deform offsets, with alpha. */
					for (i = 0; i < vertexCount; i++)
						deformArray[i] += lastVertices[i];
				}
			} else {
				/* Vertex positions or deform offsets, no alpha. */
				memcpy(deformArray, lastVertices, vertexCount * sizeof(float));
			}
		} else {
			sp4VertexAttachment *vertexAttachment;
			switch (blend) {
				case SP4_MIX_BLEND_SETUP:
					vertexAttachment = SUB_CAST(sp4VertexAttachment, slot->attachment);
					if (!vertexAttachment->bones) {
						/* Unweighted vertex positions, with alpha. */
						float *setupVertices = vertexAttachment->vertices;
						for (i = 0; i < vertexCount; i++) {
							float setup = setupVertices[i];
							deformArray[i] = setup + (lastVertices[i] - setup) * alpha;
						}
					} else {
						/* Weighted deform offsets, with alpha. */
						for (i = 0; i < vertexCount; i++)
							deformArray[i] = lastVertices[i] * alpha;
					}
					break;
				case SP4_MIX_BLEND_FIRST:
				case SP4_MIX_BLEND_REPLACE:
					/* Vertex positions or deform offsets, with alpha. */
					for (i = 0; i < vertexCount; i++)
						deformArray[i] += (lastVertices[i] - deformArray[i]) * alpha;
				case SP4_MIX_BLEND_ADD:
					vertexAttachment = SUB_CAST(sp4VertexAttachment, slot->attachment);
					if (!vertexAttachment->bones) {
						/* Unweighted vertex positions, with alpha. */
						float *setupVertices = vertexAttachment->vertices;
						for (i = 0; i < vertexCount; i++) {
							deformArray[i] += (lastVertices[i] - setupVertices[i]) * alpha;
						}
					} else {
						for (i = 0; i < vertexCount; i++)
							deformArray[i] += lastVertices[i] * alpha;
					}
			}
		}
		return;
	}

	/* Interpolate between the previous frame and the current frame. */
	frame = search(self->super.super.frames, time);
	percent = _sp4DeformTimeline_getCurvePercent(self, time, frame);
	prevVertices = frameVertices[frame];
	nextVertices = frameVertices[frame + 1];

	if (alpha == 1) {
		if (blend == SP4_MIX_BLEND_ADD) {
			sp4VertexAttachment *vertexAttachment = SUB_CAST(sp4VertexAttachment, slot->attachment);
			if (!vertexAttachment->bones) {
				float *setupVertices = vertexAttachment->vertices;
				for (i = 0; i < vertexCount; i++) {
					float prev = prevVertices[i];
					deformArray[i] += prev + (nextVertices[i] - prev) * percent - setupVertices[i];
				}
			} else {
				for (i = 0; i < vertexCount; i++) {
					float prev = prevVertices[i];
					deformArray[i] += prev + (nextVertices[i] - prev) * percent;
				}
			}
		} else {
			for (i = 0; i < vertexCount; i++) {
				float prev = prevVertices[i];
				deformArray[i] = prev + (nextVertices[i] - prev) * percent;
			}
		}
	} else {
		sp4VertexAttachment *vertexAttachment;
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				vertexAttachment = SUB_CAST(sp4VertexAttachment, slot->attachment);
				if (!vertexAttachment->bones) {
					float *setupVertices = vertexAttachment->vertices;
					for (i = 0; i < vertexCount; i++) {
						float prev = prevVertices[i], setup = setupVertices[i];
						deformArray[i] = setup + (prev + (nextVertices[i] - prev) * percent - setup) * alpha;
					}
				} else {
					for (i = 0; i < vertexCount; i++) {
						float prev = prevVertices[i];
						deformArray[i] = (prev + (nextVertices[i] - prev) * percent) * alpha;
					}
				}
				break;
			case SP4_MIX_BLEND_FIRST:
			case SP4_MIX_BLEND_REPLACE:
				for (i = 0; i < vertexCount; i++) {
					float prev = prevVertices[i];
					deformArray[i] += (prev + (nextVertices[i] - prev) * percent - deformArray[i]) * alpha;
				}
				break;
			case SP4_MIX_BLEND_ADD:
				vertexAttachment = SUB_CAST(sp4VertexAttachment, slot->attachment);
				if (!vertexAttachment->bones) {
					float *setupVertices = vertexAttachment->vertices;
					for (i = 0; i < vertexCount; i++) {
						float prev = prevVertices[i];
						deformArray[i] += (prev + (nextVertices[i] - prev) * percent - setupVertices[i]) * alpha;
					}
				} else {
					for (i = 0; i < vertexCount; i++) {
						float prev = prevVertices[i];
						deformArray[i] += (prev + (nextVertices[i] - prev) * percent) * alpha;
					}
				}
		}
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

void _sp4DeformTimeline_dispose(sp4Timeline *timeline) {
	sp4DeformTimeline *self = SUB_CAST(sp4DeformTimeline, timeline);
	int i;
	for (i = 0; i < self->super.super.frames->size; ++i)
		FREE(self->frameVertices[i]);
	FREE(self->frameVertices);
	_sp4CurveTimeline_dispose(timeline);
}

sp4DeformTimeline *sp4DeformTimeline_create(int framesCount, int frameVerticesCount, int bezierCount, int slotIndex,
										  sp4VertexAttachment *attachment) {
	sp4DeformTimeline *self = NEW(sp4DeformTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_DEFORM << 32) | ((slotIndex << 16 | attachment->id) & 0xffffffff);
	_sp4CurveTimeline_init(SUPER(self), framesCount, 1, bezierCount, ids, 1, SP4_TIMELINE_DEFORM,
						  _sp4DeformTimeline_dispose, _sp4DeformTimeline_apply, _sp4DeformTimeline_setBezier);
	CONST_CAST(float **, self->frameVertices) = CALLOC(float *, framesCount);
	CONST_CAST(int, self->frameVerticesCount) = frameVerticesCount;
	self->slotIndex = slotIndex;
	self->attachment = SUPER(attachment);
	return self;
}

void sp4DeformTimeline_setFrame(sp4DeformTimeline *self, int frame, float time, float *vertices) {
	self->super.super.frames->items[frame] = time;

	FREE(self->frameVertices[frame]);
	if (!vertices)
		self->frameVertices[frame] = 0;
	else {
		self->frameVertices[frame] = MALLOC(float, self->frameVerticesCount);
		memcpy(CONST_CAST(float *, self->frameVertices[frame]), vertices, self->frameVerticesCount * sizeof(float));
	}
}

/**/

/** Fires events for frames > lastTime and <= time. */
void _sp4EventTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time, sp4Event **firedEvents,
							int *eventsCount, float alpha, sp4MixBlend blend, sp4MixDirection direction) {
	sp4EventTimeline *self = (sp4EventTimeline *) timeline;
	float *frames = self->super.frames->items;
	int framesCount = self->super.frames->size;
	int i;
	if (!firedEvents) return;

	if (lastTime > time) { /* Fire events after last time for looped animations. */
		_sp4EventTimeline_apply(timeline, skeleton, lastTime, (float) INT_MAX, firedEvents, eventsCount, alpha, blend,
							   direction);
		lastTime = -1;
	} else if (lastTime >= frames[framesCount - 1]) {
		/* Last time is after last i. */
		return;
	}

	if (time < frames[0]) return; /* Time is before first i. */

	if (lastTime < frames[0])
		i = 0;
	else {
		float frameTime;
		i = search(self->super.frames, lastTime) + 1;
		frameTime = frames[i];
		while (i > 0) { /* Fire multiple events with the same i. */
			if (frames[i - 1] != frameTime) break;
			i--;
		}
	}
	for (; i < framesCount && time >= frames[i]; ++i) {
		firedEvents[*eventsCount] = self->events[i];
		(*eventsCount)++;
	}
	UNUSED(direction);
}

void _sp4EventTimeline_dispose(sp4Timeline *timeline) {
	sp4EventTimeline *self = SUB_CAST(sp4EventTimeline, timeline);
	int i;

	for (i = 0; i < self->super.frames->size; ++i)
		sp4Event_dispose(self->events[i]);
	FREE(self->events);
}

sp4EventTimeline *sp4EventTimeline_create(int framesCount) {
	sp4EventTimeline *self = NEW(sp4EventTimeline);
	sp4PropertyId ids[1];
	ids[0] = (sp4PropertyId) SP4_PROPERTY_EVENT << 32;
	_sp4Timeline_init(SUPER(self), framesCount, 1, ids, 1, SP4_TIMELINE_EVENT, _sp4EventTimeline_dispose,
					 _sp4EventTimeline_apply, 0);
	CONST_CAST(sp4Event **, self->events) = CALLOC(sp4Event *, framesCount);
	return self;
}

void sp4EventTimeline_setFrame(sp4EventTimeline *self, int frame, sp4Event *event) {
	self->super.frames->items[frame] = event->time;

	FREE(self->events[frame]);
	self->events[frame] = event;
}

/**/

void _sp4DrawOrderTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
								sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
								sp4MixDirection direction) {
	int i;
	const int *drawOrderToSetupIndex;
	sp4DrawOrderTimeline *self = (sp4DrawOrderTimeline *) timeline;
	float *frames = self->super.frames->items;

	if (direction == SP4_MIX_DIRECTION_OUT) {
		if (blend == SP4_MIX_BLEND_SETUP)
			memcpy(skeleton->drawOrder, skeleton->slots, self->slotsCount * sizeof(sp4Slot *));
		return;
	}

	if (time < frames[0]) {
		if (blend == SP4_MIX_BLEND_SETUP || blend == SP4_MIX_BLEND_FIRST)
			memcpy(skeleton->drawOrder, skeleton->slots, self->slotsCount * sizeof(sp4Slot *));
		return;
	}

	drawOrderToSetupIndex = self->drawOrders[search(self->super.frames, time)];
	if (!drawOrderToSetupIndex)
		memcpy(skeleton->drawOrder, skeleton->slots, self->slotsCount * sizeof(sp4Slot *));
	else {
		for (i = 0; i < self->slotsCount; ++i)
			skeleton->drawOrder[i] = skeleton->slots[drawOrderToSetupIndex[i]];
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(alpha);
}

void _sp4DrawOrderTimeline_dispose(sp4Timeline *timeline) {
	sp4DrawOrderTimeline *self = SUB_CAST(sp4DrawOrderTimeline, timeline);
	int i;

	for (i = 0; i < self->super.frames->size; ++i)
		FREE(self->drawOrders[i]);
	FREE(self->drawOrders);
}

sp4DrawOrderTimeline *sp4DrawOrderTimeline_create(int framesCount, int slotsCount) {
	sp4DrawOrderTimeline *self = NEW(sp4DrawOrderTimeline);
	sp4PropertyId ids[1];
	ids[0] = (sp4PropertyId) SP4_PROPERTY_DRAWORDER << 32;
	_sp4Timeline_init(SUPER(self), framesCount, 1, ids, 1, SP4_TIMELINE_DRAWORDER, _sp4DrawOrderTimeline_dispose,
					 _sp4DrawOrderTimeline_apply, 0);

	CONST_CAST(int **, self->drawOrders) = CALLOC(int *, framesCount);
	CONST_CAST(int, self->slotsCount) = slotsCount;

	return self;
}

void sp4DrawOrderTimeline_setFrame(sp4DrawOrderTimeline *self, int frame, float time, const int *drawOrder) {
	self->super.frames->items[frame] = time;

	FREE(self->drawOrders[frame]);
	if (!drawOrder)
		self->drawOrders[frame] = 0;
	else {
		self->drawOrders[frame] = MALLOC(int, self->slotsCount);
		memcpy(CONST_CAST(int *, self->drawOrders[frame]), drawOrder, self->slotsCount * sizeof(int));
	}
}

/**/

static const int IKCONSTRAINT_ENTRIES = 6;
static const int IKCONSTRAINT_MIX = 1, IKCONSTRAINT_SOFTNESS = 2, IKCONSTRAINT_BEND_DIRECTION = 3, IKCONSTRAINT_COMPRESS = 4, IKCONSTRAINT_STRETCH = 5;

void _sp4IkConstraintTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
								   sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
								   sp4MixDirection direction) {
	int i, curveType;
	float mix, softness, t;
	sp4IkConstraint *constraint;
	sp4IkConstraintTimeline *self = (sp4IkConstraintTimeline *) timeline;
	float *frames = self->super.super.frames->items;
	float *curves = self->super.curves->items;

	constraint = skeleton->ikConstraints[self->ikConstraintIndex];
	if (!constraint->active) return;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				constraint->mix = constraint->data->mix;
				constraint->softness = constraint->data->softness;
				constraint->bendDirection = constraint->data->bendDirection;
				constraint->compress = constraint->data->compress;
				constraint->stretch = constraint->data->stretch;
				return;
			case SP4_MIX_BLEND_FIRST:
				constraint->mix += (constraint->data->mix - constraint->mix) * alpha;
				constraint->softness += (constraint->data->softness - constraint->softness) * alpha;
				constraint->bendDirection = constraint->data->bendDirection;
				constraint->compress = constraint->data->compress;
				constraint->stretch = constraint->data->stretch;
				return;
			default:
				return;
		}
	}

	i = search2(self->super.super.frames, time, IKCONSTRAINT_ENTRIES);
	curveType = (int) curves[i / IKCONSTRAINT_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			mix = frames[i + IKCONSTRAINT_MIX];
			softness = frames[i + IKCONSTRAINT_SOFTNESS];
			t = (time - before) / (frames[i + IKCONSTRAINT_ENTRIES] - before);
			mix += (frames[i + IKCONSTRAINT_ENTRIES + IKCONSTRAINT_MIX] - mix) * t;
			softness += (frames[i + IKCONSTRAINT_ENTRIES + IKCONSTRAINT_SOFTNESS] - softness) * t;
			break;
		}
		case CURVE_STEPPED: {
			mix = frames[i + IKCONSTRAINT_MIX];
			softness = frames[i + IKCONSTRAINT_SOFTNESS];
			break;
		}
		default: {
			mix = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, IKCONSTRAINT_MIX, curveType - CURVE_BEZIER);
			softness = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, IKCONSTRAINT_SOFTNESS,
													   curveType + BEZIER_SIZE - CURVE_BEZIER);
		}
	}

	if (blend == SP4_MIX_BLEND_SETUP) {
		constraint->mix = constraint->data->mix + (mix - constraint->data->mix) * alpha;
		constraint->softness = constraint->data->softness + (softness - constraint->data->softness) * alpha;

		if (direction == SP4_MIX_DIRECTION_OUT) {
			constraint->bendDirection = constraint->data->bendDirection;
			constraint->compress = constraint->data->compress;
			constraint->stretch = constraint->data->stretch;
		} else {
			constraint->bendDirection = frames[i + IKCONSTRAINT_BEND_DIRECTION];
			constraint->compress = frames[i + IKCONSTRAINT_COMPRESS] != 0;
			constraint->stretch = frames[i + IKCONSTRAINT_STRETCH] != 0;
		}
	} else {
		constraint->mix += (mix - constraint->mix) * alpha;
		constraint->softness += (softness - constraint->softness) * alpha;
		if (direction == SP4_MIX_DIRECTION_IN) {
			constraint->bendDirection = frames[i + IKCONSTRAINT_BEND_DIRECTION];
			constraint->compress = frames[i + IKCONSTRAINT_COMPRESS] != 0;
			constraint->stretch = frames[i + IKCONSTRAINT_STRETCH] != 0;
		}
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
}

sp4IkConstraintTimeline *sp4IkConstraintTimeline_create(int framesCount, int bezierCount, int ikConstraintIndex) {
	sp4IkConstraintTimeline *timeline = NEW(sp4IkConstraintTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_IKCONSTRAINT << 32) | ikConstraintIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, IKCONSTRAINT_ENTRIES, bezierCount, ids, 1,
						  SP4_TIMELINE_IKCONSTRAINT, _sp4CurveTimeline_dispose, _sp4IkConstraintTimeline_apply,
						  _sp4CurveTimeline_setBezier);
	timeline->ikConstraintIndex = ikConstraintIndex;
	return timeline;
}

void sp4IkConstraintTimeline_setFrame(sp4IkConstraintTimeline *self, int frame, float time, float mix, float softness,
									 int bendDirection, int /*boolean*/ compress, int /*boolean*/ stretch) {
	float *frames = self->super.super.frames->items;
	frame *= IKCONSTRAINT_ENTRIES;
	frames[frame] = time;
	frames[frame + IKCONSTRAINT_MIX] = mix;
	frames[frame + IKCONSTRAINT_SOFTNESS] = softness;
	frames[frame + IKCONSTRAINT_BEND_DIRECTION] = (float) bendDirection;
	frames[frame + IKCONSTRAINT_COMPRESS] = compress ? 1 : 0;
	frames[frame + IKCONSTRAINT_STRETCH] = stretch ? 1 : 0;
}

/**/
static const int TRANSFORMCONSTRAINT_ENTRIES = 7;
static const int TRANSFORMCONSTRAINT_ROTATE = 1;
static const int TRANSFORMCONSTRAINT_X = 2;
static const int TRANSFORMCONSTRAINT_Y = 3;
static const int TRANSFORMCONSTRAINT_SCALEX = 4;
static const int TRANSFORMCONSTRAINT_SCALEY = 5;
static const int TRANSFORMCONSTRAINT_SHEARY = 6;

void _sp4TransformConstraintTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
										  sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
										  sp4MixDirection direction) {
	int i, curveType;
	float rotate, x, y, scaleX, scaleY, shearY, t;
	sp4TransformConstraint *constraint;
	sp4TransformConstraintTimeline *self = (sp4TransformConstraintTimeline *) timeline;
	float *frames;
	float *curves;
	sp4TransformConstraintData *data;

	constraint = skeleton->transformConstraints[self->transformConstraintIndex];
	if (!constraint->active) return;

	frames = self->super.super.frames->items;
	curves = self->super.curves->items;

	data = constraint->data;
	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				constraint->mixRotate = data->mixRotate;
				constraint->mixX = data->mixX;
				constraint->mixY = data->mixY;
				constraint->mixScaleX = data->mixScaleX;
				constraint->mixScaleY = data->mixScaleY;
				constraint->mixShearY = data->mixShearY;
				return;
			case SP4_MIX_BLEND_FIRST:
				constraint->mixRotate += (data->mixRotate - constraint->mixRotate) * alpha;
				constraint->mixX += (data->mixX - constraint->mixX) * alpha;
				constraint->mixY += (data->mixY - constraint->mixY) * alpha;
				constraint->mixScaleX += (data->mixScaleX - constraint->mixScaleX) * alpha;
				constraint->mixScaleY += (data->mixScaleY - constraint->mixScaleY) * alpha;
				constraint->mixShearY += (data->mixShearY - constraint->mixShearY) * alpha;
				return;
			default:
				return;
		}
	}

	i = search2(self->super.super.frames, time, TRANSFORMCONSTRAINT_ENTRIES);
	curveType = (int) curves[i / TRANSFORMCONSTRAINT_ENTRIES];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			rotate = frames[i + TRANSFORMCONSTRAINT_ROTATE];
			x = frames[i + TRANSFORMCONSTRAINT_X];
			y = frames[i + TRANSFORMCONSTRAINT_Y];
			scaleX = frames[i + TRANSFORMCONSTRAINT_SCALEX];
			scaleY = frames[i + TRANSFORMCONSTRAINT_SCALEY];
			shearY = frames[i + TRANSFORMCONSTRAINT_SHEARY];
			t = (time - before) / (frames[i + TRANSFORMCONSTRAINT_ENTRIES] - before);
			rotate += (frames[i + TRANSFORMCONSTRAINT_ENTRIES + TRANSFORMCONSTRAINT_ROTATE] - rotate) * t;
			x += (frames[i + TRANSFORMCONSTRAINT_ENTRIES + TRANSFORMCONSTRAINT_X] - x) * t;
			y += (frames[i + TRANSFORMCONSTRAINT_ENTRIES + TRANSFORMCONSTRAINT_Y] - y) * t;
			scaleX += (frames[i + TRANSFORMCONSTRAINT_ENTRIES + TRANSFORMCONSTRAINT_SCALEX] - scaleX) * t;
			scaleY += (frames[i + TRANSFORMCONSTRAINT_ENTRIES + TRANSFORMCONSTRAINT_SCALEY] - scaleY) * t;
			shearY += (frames[i + TRANSFORMCONSTRAINT_ENTRIES + TRANSFORMCONSTRAINT_SHEARY] - shearY) * t;
			break;
		}
		case CURVE_STEPPED: {
			rotate = frames[i + TRANSFORMCONSTRAINT_ROTATE];
			x = frames[i + TRANSFORMCONSTRAINT_X];
			y = frames[i + TRANSFORMCONSTRAINT_Y];
			scaleX = frames[i + TRANSFORMCONSTRAINT_SCALEX];
			scaleY = frames[i + TRANSFORMCONSTRAINT_SCALEY];
			shearY = frames[i + TRANSFORMCONSTRAINT_SHEARY];
			break;
		}
		default: {
			rotate = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, TRANSFORMCONSTRAINT_ROTATE,
													 curveType - CURVE_BEZIER);
			x = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, TRANSFORMCONSTRAINT_X,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
			y = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, TRANSFORMCONSTRAINT_Y,
												curveType + BEZIER_SIZE * 2 - CURVE_BEZIER);
			scaleX = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, TRANSFORMCONSTRAINT_SCALEX,
													 curveType + BEZIER_SIZE * 3 - CURVE_BEZIER);
			scaleY = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, TRANSFORMCONSTRAINT_SCALEY,
													 curveType + BEZIER_SIZE * 4 - CURVE_BEZIER);
			shearY = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, TRANSFORMCONSTRAINT_SHEARY,
													 curveType + BEZIER_SIZE * 5 - CURVE_BEZIER);
		}
	}

	if (blend == SP4_MIX_BLEND_SETUP) {
		constraint->mixRotate = data->mixRotate + (rotate - data->mixRotate) * alpha;
		constraint->mixX = data->mixX + (x - data->mixX) * alpha;
		constraint->mixY = data->mixY + (y - data->mixY) * alpha;
		constraint->mixScaleX = data->mixScaleX + (scaleX - data->mixScaleX) * alpha;
		constraint->mixScaleY = data->mixScaleY + (scaleY - data->mixScaleY) * alpha;
		constraint->mixShearY = data->mixShearY + (shearY - data->mixShearY) * alpha;
	} else {
		constraint->mixRotate += (rotate - constraint->mixRotate) * alpha;
		constraint->mixX += (x - constraint->mixX) * alpha;
		constraint->mixY += (y - constraint->mixY) * alpha;
		constraint->mixScaleX += (scaleX - constraint->mixScaleX) * alpha;
		constraint->mixScaleY += (scaleY - constraint->mixScaleY) * alpha;
		constraint->mixShearY += (shearY - constraint->mixShearY) * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4TransformConstraintTimeline *
sp4TransformConstraintTimeline_create(int framesCount, int bezierCount, int transformConstraintIndex) {
	sp4TransformConstraintTimeline *timeline = NEW(sp4TransformConstraintTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_TRANSFORMCONSTRAINT << 32) | transformConstraintIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, TRANSFORMCONSTRAINT_ENTRIES, bezierCount, ids, 1,
						  SP4_TIMELINE_TRANSFORMCONSTRAINT, _sp4CurveTimeline_dispose,
						  _sp4TransformConstraintTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->transformConstraintIndex = transformConstraintIndex;
	return timeline;
}

void sp4TransformConstraintTimeline_setFrame(sp4TransformConstraintTimeline *self, int frame, float time, float mixRotate,
											float mixX, float mixY, float mixScaleX, float mixScaleY, float mixShearY) {
	float *frames = self->super.super.frames->items;
	frame *= TRANSFORMCONSTRAINT_ENTRIES;
	frames[frame] = time;
	frames[frame + TRANSFORMCONSTRAINT_ROTATE] = mixRotate;
	frames[frame + TRANSFORMCONSTRAINT_X] = mixX;
	frames[frame + TRANSFORMCONSTRAINT_X] = mixY;
	frames[frame + TRANSFORMCONSTRAINT_SCALEX] = mixScaleX;
	frames[frame + TRANSFORMCONSTRAINT_SCALEY] = mixScaleY;
	frames[frame + TRANSFORMCONSTRAINT_SHEARY] = mixShearY;
}

/**/
static const int PATHCONSTRAINTPOSITION_ENTRIES = 2;
static const int PATHCONSTRAINTPOSITION_VALUE = 1;

void _sp4PathConstraintPositionTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
											 sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
											 sp4MixDirection direction) {
	float position;
	sp4PathConstraint *constraint;
	sp4PathConstraintPositionTimeline *self = (sp4PathConstraintPositionTimeline *) timeline;
	float *frames;

	constraint = skeleton->pathConstraints[self->pathConstraintIndex];
	if (!constraint->active) return;

	frames = self->super.super.frames->items;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				constraint->position = constraint->data->position;
				return;
			case SP4_MIX_BLEND_FIRST:
				constraint->position += (constraint->data->position - constraint->position) * alpha;
				return;
			default:
				return;
		}
	}

	position = sp4CurveTimeline1_getCurveValue(SUPER(self), time);

	if (blend == SP4_MIX_BLEND_SETUP)
		constraint->position = constraint->data->position + (position - constraint->data->position) * alpha;
	else
		constraint->position += (position - constraint->position) * alpha;

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4PathConstraintPositionTimeline *
sp4PathConstraintPositionTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex) {
	sp4PathConstraintPositionTimeline *timeline = NEW(sp4PathConstraintPositionTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_PATHCONSTRAINT_POSITION << 32) | pathConstraintIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, PATHCONSTRAINTPOSITION_ENTRIES, bezierCount, ids, 1,
						  SP4_TIMELINE_PATHCONSTRAINTPOSITION, _sp4CurveTimeline_dispose,
						  _sp4PathConstraintPositionTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->pathConstraintIndex = pathConstraintIndex;
	return timeline;
}

void sp4PathConstraintPositionTimeline_setFrame(sp4PathConstraintPositionTimeline *self, int frame, float time, float value) {
	float *frames = self->super.super.frames->items;
	frame *= PATHCONSTRAINTPOSITION_ENTRIES;
	frames[frame] = time;
	frames[frame + PATHCONSTRAINTPOSITION_VALUE] = value;
}

/**/
static const int PATHCONSTRAINTSPACING_ENTRIES = 2;
static const int PATHCONSTRAINTSPACING_VALUE = 1;

void _sp4PathConstraintSpacingTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
											sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
											sp4MixDirection direction) {
	float spacing;
	sp4PathConstraint *constraint;
	sp4PathConstraintSpacingTimeline *self = (sp4PathConstraintSpacingTimeline *) timeline;
	float *frames;

	constraint = skeleton->pathConstraints[self->pathConstraintIndex];
	if (!constraint->active) return;

	frames = self->super.super.frames->items;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				constraint->spacing = constraint->data->spacing;
				return;
			case SP4_MIX_BLEND_FIRST:
				constraint->spacing += (constraint->data->spacing - constraint->spacing) * alpha;
				return;
			default:
				return;
		}
	}

	spacing = sp4CurveTimeline1_getCurveValue(SUPER(self), time);

	if (blend == SP4_MIX_BLEND_SETUP)
		constraint->spacing = constraint->data->spacing + (spacing - constraint->data->spacing) * alpha;
	else
		constraint->spacing += (spacing - constraint->spacing) * alpha;

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4PathConstraintSpacingTimeline *
sp4PathConstraintSpacingTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex) {
	sp4PathConstraintSpacingTimeline *timeline = NEW(sp4PathConstraintSpacingTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_PATHCONSTRAINT_SPACING << 32) | pathConstraintIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, PATHCONSTRAINTSPACING_ENTRIES, bezierCount, ids, 1,
						  SP4_TIMELINE_PATHCONSTRAINTSPACING, _sp4CurveTimeline_dispose,
						  _sp4PathConstraintSpacingTimeline_apply, _sp4CurveTimeline_setBezier);
	timeline->pathConstraintIndex = pathConstraintIndex;
	return timeline;
}

void sp4PathConstraintSpacingTimeline_setFrame(sp4PathConstraintSpacingTimeline *self, int frame, float time, float value) {
	float *frames = self->super.super.frames->items;
	frame *= PATHCONSTRAINTSPACING_ENTRIES;
	frames[frame] = time;
	frames[frame + PATHCONSTRAINTSPACING_VALUE] = value;
}

/**/

static const int PATHCONSTRAINTMIX_ENTRIES = 4;
static const int PATHCONSTRAINTMIX_ROTATE = 1;
static const int PATHCONSTRAINTMIX_X = 2;
static const int PATHCONSTRAINTMIX_Y = 3;

void _sp4PathConstraintMixTimeline_apply(sp4Timeline *timeline, sp4Skeleton *skeleton, float lastTime, float time,
										sp4Event **firedEvents, int *eventsCount, float alpha, sp4MixBlend blend,
										sp4MixDirection direction) {
	int i, curveType;
	float rotate, x, y, t;
	sp4PathConstraint *constraint;
	sp4PathConstraintMixTimeline *self = (sp4PathConstraintMixTimeline *) timeline;
	float *frames;
	float *curves;

	constraint = skeleton->pathConstraints[self->pathConstraintIndex];
	if (!constraint->active) return;

	frames = self->super.super.frames->items;
	curves = self->super.curves->items;

	if (time < frames[0]) {
		switch (blend) {
			case SP4_MIX_BLEND_SETUP:
				constraint->mixRotate = constraint->data->mixRotate;
				constraint->mixX = constraint->data->mixX;
				constraint->mixY = constraint->data->mixY;
				return;
			case SP4_MIX_BLEND_FIRST:
				constraint->mixRotate += (constraint->data->mixRotate - constraint->mixRotate) * alpha;
				constraint->mixX += (constraint->data->mixX - constraint->mixX) * alpha;
				constraint->mixY += (constraint->data->mixY - constraint->mixY) * alpha;
			default: {
			}
		}
		return;
	}

	i = search2(self->super.super.frames, time, PATHCONSTRAINTMIX_ENTRIES);
	curveType = (int) curves[i >> 2];
	switch (curveType) {
		case CURVE_LINEAR: {
			float before = frames[i];
			rotate = frames[i + PATHCONSTRAINTMIX_ROTATE];
			x = frames[i + PATHCONSTRAINTMIX_X];
			y = frames[i + PATHCONSTRAINTMIX_Y];
			t = (time - before) / (frames[i + PATHCONSTRAINTMIX_ENTRIES] - before);
			rotate += (frames[i + PATHCONSTRAINTMIX_ENTRIES + PATHCONSTRAINTMIX_ROTATE] - rotate) * t;
			x += (frames[i + PATHCONSTRAINTMIX_ENTRIES + PATHCONSTRAINTMIX_X] - x) * t;
			y += (frames[i + PATHCONSTRAINTMIX_ENTRIES + PATHCONSTRAINTMIX_Y] - y) * t;
			break;
		}
		case CURVE_STEPPED: {
			rotate = frames[i + PATHCONSTRAINTMIX_ROTATE];
			x = frames[i + PATHCONSTRAINTMIX_X];
			y = frames[i + PATHCONSTRAINTMIX_Y];
			break;
		}
		default: {
			rotate = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, PATHCONSTRAINTMIX_ROTATE,
													 curveType - CURVE_BEZIER);
			x = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, PATHCONSTRAINTMIX_X,
												curveType + BEZIER_SIZE - CURVE_BEZIER);
			y = _sp4CurveTimeline_getBezierValue(SUPER(self), time, i, PATHCONSTRAINTMIX_Y,
												curveType + BEZIER_SIZE * 2 - CURVE_BEZIER);
		}
	}

	if (blend == SP4_MIX_BLEND_SETUP) {
		sp4PathConstraintData *data = constraint->data;
		constraint->mixRotate = data->mixRotate + (rotate - data->mixRotate) * alpha;
		constraint->mixX = data->mixX + (x - data->mixX) * alpha;
		constraint->mixY = data->mixY + (y - data->mixY) * alpha;
	} else {
		constraint->mixRotate += (rotate - constraint->mixRotate) * alpha;
		constraint->mixX += (x - constraint->mixX) * alpha;
		constraint->mixY += (y - constraint->mixY) * alpha;
	}

	UNUSED(lastTime);
	UNUSED(firedEvents);
	UNUSED(eventsCount);
	UNUSED(direction);
}

sp4PathConstraintMixTimeline *
sp4PathConstraintMixTimeline_create(int framesCount, int bezierCount, int pathConstraintIndex) {
	sp4PathConstraintMixTimeline *timeline = NEW(sp4PathConstraintMixTimeline);
	sp4PropertyId ids[1];
	ids[0] = ((sp4PropertyId) SP4_PROPERTY_PATHCONSTRAINT_MIX << 32) | pathConstraintIndex;
	_sp4CurveTimeline_init(SUPER(timeline), framesCount, PATHCONSTRAINTMIX_ENTRIES, bezierCount, ids, 1,
						  SP4_TIMELINE_PATHCONSTRAINTMIX, _sp4CurveTimeline_dispose, _sp4PathConstraintMixTimeline_apply,
						  _sp4CurveTimeline_setBezier);
	timeline->pathConstraintIndex = pathConstraintIndex;
	return timeline;
}

void sp4PathConstraintMixTimeline_setFrame(sp4PathConstraintMixTimeline *self, int frame, float time, float mixRotate,
										  float mixX, float mixY) {
	float *frames = self->super.super.frames->items;
	frame *= PATHCONSTRAINTMIX_ENTRIES;
	frames[frame] = time;
	frames[frame + PATHCONSTRAINTMIX_ROTATE] = mixRotate;
	frames[frame + PATHCONSTRAINTMIX_X] = mixX;
	frames[frame + PATHCONSTRAINTMIX_Y] = mixY;
}
