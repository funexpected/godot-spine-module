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

#include <v4/spine/Animation.h>
#include <v4/spine/Array.h>
#include <v4/spine/AtlasAttachmentLoader.h>
#include <v4/spine/SkeletonBinary.h>
#include <v4/spine/extension.h>
#include <stdio.h>

typedef struct {
	const unsigned char *cursor;
	const unsigned char *end;
} _dataInput;

typedef struct {
	const char *parent;
	const char *skin;
	int slotIndex;
	sp4MeshAttachment *mesh;
	int inheritDeform;
} _sp4LinkedMesh;

typedef struct {
	sp4SkeletonBinary super;
	int ownsLoader;

	int linkedMeshCount;
	int linkedMeshCapacity;
	_sp4LinkedMesh *linkedMeshes;
} _sp4SkeletonBinary;

sp4SkeletonBinary *sp4SkeletonBinary_createWithLoader(sp4AttachmentLoader *attachmentLoader) {
	sp4SkeletonBinary *self = SUPER(NEW(_sp4SkeletonBinary));
	self->scale = 1;
	self->attachmentLoader = attachmentLoader;
	return self;
}

sp4SkeletonBinary *sp4SkeletonBinary_create(sp4Atlas *atlas) {
	sp4AtlasAttachmentLoader *attachmentLoader = sp4AtlasAttachmentLoader_create(atlas);
	sp4SkeletonBinary *self = sp4SkeletonBinary_createWithLoader(SUPER(attachmentLoader));
	SUB_CAST(_sp4SkeletonBinary, self)->ownsLoader = 1;
	return self;
}

void sp4SkeletonBinary_dispose(sp4SkeletonBinary *self) {
	_sp4SkeletonBinary *internal = SUB_CAST(_sp4SkeletonBinary, self);
	if (internal->ownsLoader) sp4AttachmentLoader_dispose(self->attachmentLoader);
	FREE(internal->linkedMeshes);
	FREE(self->error);
	FREE(self);
}

void _sp4SkeletonBinary_setError(sp4SkeletonBinary *self, const char *value1, const char *value2) {
	char message[256];
	int length;
	FREE(self->error);
	strcpy(message, value1);
	length = (int) strlen(value1);
	if (value2) strncat(message + length, value2, 255 - length);
	MALLOC_STR(self->error, message);
}

static unsigned char readByte(_dataInput *input) {
	return *input->cursor++;
}

static signed char readSByte(_dataInput *input) {
	return (signed char) readByte(input);
}

static int readBoolean(_dataInput *input) {
	return readByte(input) != 0;
}

static int readInt(_dataInput *input) {
	int result = readByte(input);
	result <<= 8;
	result |= readByte(input);
	result <<= 8;
	result |= readByte(input);
	result <<= 8;
	result |= readByte(input);
	return result;
}

static int readVarint(_dataInput *input, int /*bool*/ optimizePositive) {
	unsigned char b = readByte(input);
	int value = b & 0x7F;
	if (b & 0x80) {
		b = readByte(input);
		value |= (b & 0x7F) << 7;
		if (b & 0x80) {
			b = readByte(input);
			value |= (b & 0x7F) << 14;
			if (b & 0x80) {
				b = readByte(input);
				value |= (b & 0x7F) << 21;
				if (b & 0x80) value |= (readByte(input) & 0x7F) << 28;
			}
		}
	}
	if (!optimizePositive) value = (((unsigned int) value >> 1) ^ -(value & 1));
	return value;
}

float readFloat_4(_dataInput *input) {
	union {
		int intValue;
		float floatValue;
	} intToFloat;
	intToFloat.intValue = readInt(input);
	return intToFloat.floatValue;
}

char *readString_4(_dataInput *input) {
	int length = readVarint(input, 1);
	char *string;
	if (length == 0) return NULL;
	string = MALLOC(char, length);
	memcpy(string, input->cursor, length - 1);
	input->cursor += length - 1;
	string[length - 1] = '\0';
	return string;
}

static char *readStringRef(_dataInput *input, sp4SkeletonData *skeletonData) {
	int index = readVarint(input, 1);
	return index == 0 ? 0 : skeletonData->strings[index - 1];
}

static void readColor(_dataInput *input, float *r, float *g, float *b, float *a) {
	*r = readByte(input) / 255.0f;
	*g = readByte(input) / 255.0f;
	*b = readByte(input) / 255.0f;
	*a = readByte(input) / 255.0f;
}

#define ATTACHMENT_REGION 0
#define ATTACHMENT_BOUNDING_BOX 1
#define ATTACHMENT_MESH 2
#define ATTACHMENT_LINKED_MESH 3
#define ATTACHMENT_PATH 4

#define BLEND_MODE_NORMAL 0
#define BLEND_MODE_ADDITIVE 1
#define BLEND_MODE_MULTIPLY 2
#define BLEND_MODE_SCREEN 3

#define BONE_ROTATE 0
#define BONE_TRANSLATE 1
#define BONE_TRANSLATEX 2
#define BONE_TRANSLATEY 3
#define BONE_SCALE 4
#define BONE_SCALEX 5
#define BONE_SCALEY 6
#define BONE_SHEAR 7
#define BONE_SHEARX 8
#define BONE_SHEARY 9

#define SLOT_ATTACHMENT 0
#define SLOT_RGBA 1
#define SLOT_RGB 2
#define SLOT_RGBA2 3
#define SLOT_RGB2 4
#define SLOT_ALPHA 5

#define PATH_POSITION 0
#define PATH_SPACING 1
#define PATH_MIX 2

#define CURVE_LINEAR 0
#define CURVE_STEPPED 1
#define CURVE_BEZIER 2

#define PATH_POSITION_FIXED 0
#define PATH_POSITION_PERCENT 1

#define PATH_SPACING_LENGTH 0
#define PATH_SPACING_FIXED 1
#define PATH_SPACING_PERCENT 2

#define PATH_ROTATE_TANGENT 0
#define PATH_ROTATE_CHAIN 1
#define PATH_ROTATE_CHAIN_SCALE 2

static void
setBezier(_dataInput *input, sp4Timeline *timeline, int bezier, int frame, int value, float time1, float time2,
		  float value1, float value2, float scale) {
	float cx1 = readFloat_4(input);
	float cy1 = readFloat_4(input);
	float cx2 = readFloat_4(input);
	float cy2 = readFloat_4(input);
	sp4Timeline_setBezier(timeline, bezier, frame, value, time1, value1, cx1, cy1 * scale, cx2, cy2 * scale, time2,
						 value2);
}

static sp4Timeline *readTimeline(_dataInput *input, sp4CurveTimeline1 *timeline, float scale) {
	int frame, bezier, frameLast;
	float time2, value2;
	float time = readFloat_4(input);
	float value = readFloat_4(input) * scale;
	for (frame = 0, bezier = 0, frameLast = timeline->super.frameCount - 1;; frame++) {
		sp4CurveTimeline1_setFrame(timeline, frame, time, value);
		if (frame == frameLast) break;
		time2 = readFloat_4(input);
		value2 = readFloat_4(input) * scale;
		switch (readSByte(input)) {
			case CURVE_STEPPED:
				sp4CurveTimeline_setStepped(timeline, frame);
				break;
			case CURVE_BEZIER:
				setBezier(input, SUPER(timeline), bezier++, frame, 0, time, time2, value, value2, scale);
		}
		time = time2;
		value = value2;
	}
	return SUPER(timeline);
}

static sp4Timeline *readTimeline2(_dataInput *input, sp4CurveTimeline2 *timeline, float scale) {
	int frame, bezier, frameLast;
	float time2, nvalue1, nvalue2;
	float time = readFloat_4(input);
	float value1 = readFloat_4(input) * scale;
	float value2 = readFloat_4(input) * scale;
	for (frame = 0, bezier = 0, frameLast = timeline->super.frameCount - 1;; frame++) {
		sp4CurveTimeline2_setFrame(timeline, frame, time, value1, value2);
		if (frame == frameLast) break;
		time2 = readFloat_4(input);
		nvalue1 = readFloat_4(input) * scale;
		nvalue2 = readFloat_4(input) * scale;
		switch (readSByte(input)) {
			case CURVE_STEPPED:
				sp4CurveTimeline_setStepped(timeline, frame);
				break;
			case CURVE_BEZIER:
				setBezier(input, SUPER(timeline), bezier++, frame, 0, time, time2, value1, nvalue1, scale);
				setBezier(input, SUPER(timeline), bezier++, frame, 1, time, time2, value2, nvalue2, scale);
		}
		time = time2;
		value1 = nvalue1;
		value2 = nvalue2;
	}
	return SUPER(timeline);
}

static void _sp4SkeletonBinary_addLinkedMesh(sp4SkeletonBinary *self, sp4MeshAttachment *mesh,
											const char *skin, int slotIndex, const char *parent, int inheritDeform) {
	_sp4LinkedMesh *linkedMesh;
	_sp4SkeletonBinary *internal = SUB_CAST(_sp4SkeletonBinary, self);

	if (internal->linkedMeshCount == internal->linkedMeshCapacity) {
		_sp4LinkedMesh *linkedMeshes;
		internal->linkedMeshCapacity *= 2;
		if (internal->linkedMeshCapacity < 8) internal->linkedMeshCapacity = 8;
		/* TODO Why not realloc? */
		linkedMeshes = MALLOC(_sp4LinkedMesh, internal->linkedMeshCapacity);
		memcpy(linkedMeshes, internal->linkedMeshes, sizeof(_sp4LinkedMesh) * internal->linkedMeshCount);
		FREE(internal->linkedMeshes);
		internal->linkedMeshes = linkedMeshes;
	}

	linkedMesh = internal->linkedMeshes + internal->linkedMeshCount++;
	linkedMesh->mesh = mesh;
	linkedMesh->skin = skin;
	linkedMesh->slotIndex = slotIndex;
	linkedMesh->parent = parent;
	linkedMesh->inheritDeform = inheritDeform;
}

static sp4Animation *_sp4SkeletonBinary_readAnimation(sp4SkeletonBinary *self, const char *name,
													_dataInput *input, sp4SkeletonData *skeletonData) {
	sp4TimelineArray *timelines = sp4TimelineArray_create(18);
	float duration = 0;
	int i, n, ii, nn, iii, nnn;
	int frame, bezier;
	int drawOrderCount, eventCount;
	sp4Animation *animation;
	float scale = self->scale;

	int numTimelines = readVarint(input, 1);
	UNUSED(numTimelines);

	/* Slot timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int slotIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			unsigned char timelineType = readByte(input);
			int frameCount = readVarint(input, 1);
			int frameLast = frameCount - 1;
			switch (timelineType) {
				case SLOT_ATTACHMENT: {
					sp4AttachmentTimeline *timeline = sp4AttachmentTimeline_create(frameCount, slotIndex);
					for (frame = 0; frame < frameCount; ++frame) {
						float time = readFloat_4(input);
						const char *attachmentName = readStringRef(input, skeletonData);
						sp4AttachmentTimeline_setFrame(timeline, frame, time, attachmentName);
					}
					sp4TimelineArray_add(timelines, SUPER(timeline));
					break;
				}
				case SLOT_RGBA: {
					int bezierCount = readVarint(input, 1);
					sp4RGBATimeline *timeline = sp4RGBATimeline_create(frameCount, bezierCount, slotIndex);

					float time = readFloat_4(input);
					float r = readByte(input) / 255.0;
					float g = readByte(input) / 255.0;
					float b = readByte(input) / 255.0;
					float a = readByte(input) / 255.0;

					for (frame = 0, bezier = 0;; frame++) {
						float time2, r2, g2, b2, a2;
						sp4RGBATimeline_setFrame(timeline, frame, time, r, g, b, a);
						if (frame == frameLast) break;

						time2 = readFloat_4(input);
						r2 = readByte(input) / 255.0;
						g2 = readByte(input) / 255.0;
						b2 = readByte(input) / 255.0;
						a2 = readByte(input) / 255.0;

						switch (readSByte(input)) {
							case CURVE_STEPPED:
								sp4CurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, r, r2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, g, g2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, b, b2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 3, time, time2, a, a2, 1);
						}
						time = time2;
						r = r2;
						g = g2;
						b = b2;
						a = a2;
					}
					sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				case SLOT_RGB: {
					int bezierCount = readVarint(input, 1);
					sp4RGBTimeline *timeline = sp4RGBTimeline_create(frameCount, bezierCount, slotIndex);

					float time = readFloat_4(input);
					float r = readByte(input) / 255.0;
					float g = readByte(input) / 255.0;
					float b = readByte(input) / 255.0;

					for (frame = 0, bezier = 0;; frame++) {
						float time2, r2, g2, b2;
						sp4RGBTimeline_setFrame(timeline, frame, time, r, g, b);
						if (frame == frameLast) break;

						time2 = readFloat_4(input);
						r2 = readByte(input) / 255.0;
						g2 = readByte(input) / 255.0;
						b2 = readByte(input) / 255.0;

						switch (readSByte(input)) {
							case CURVE_STEPPED:
								sp4CurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, r, r2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, g, g2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, b, b2, 1);
						}
						time = time2;
						r = r2;
						g = g2;
						b = b2;
					}
					sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				case SLOT_RGBA2: {
					int bezierCount = readVarint(input, 1);
					sp4RGBA2Timeline *timeline = sp4RGBA2Timeline_create(frameCount, bezierCount, slotIndex);

					float time = readFloat_4(input);
					float r = readByte(input) / 255.0;
					float g = readByte(input) / 255.0;
					float b = readByte(input) / 255.0;
					float a = readByte(input) / 255.0;
					float r2 = readByte(input) / 255.0;
					float g2 = readByte(input) / 255.0;
					float b2 = readByte(input) / 255.0;

					for (frame = 0, bezier = 0;; frame++) {
						float time2, nr, ng, nb, na, nr2, ng2, nb2;
						sp4RGBA2Timeline_setFrame(timeline, frame, time, r, g, b, a, r2, g2, b2);
						if (frame == frameLast) break;
						time2 = readFloat_4(input);
						nr = readByte(input) / 255.0;
						ng = readByte(input) / 255.0;
						nb = readByte(input) / 255.0;
						na = readByte(input) / 255.0;
						nr2 = readByte(input) / 255.0;
						ng2 = readByte(input) / 255.0;
						nb2 = readByte(input) / 255.0;

						switch (readSByte(input)) {
							case CURVE_STEPPED:
								sp4CurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, r, nr, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, g, ng, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, b, nb, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 3, time, time2, a, na, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 4, time, time2, r2, nr2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 5, time, time2, g2, ng2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 6, time, time2, b2, nb2, 1);
						}
						time = time2;
						r = nr;
						g = ng;
						b = nb;
						a = na;
						r2 = nr2;
						g2 = ng2;
						b2 = nb2;
					}
					sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				case SLOT_RGB2: {
					int bezierCount = readVarint(input, 1);
					sp4RGB2Timeline *timeline = sp4RGB2Timeline_create(frameCount, bezierCount, slotIndex);

					float time = readFloat_4(input);
					float r = readByte(input) / 255.0;
					float g = readByte(input) / 255.0;
					float b = readByte(input) / 255.0;
					float r2 = readByte(input) / 255.0;
					float g2 = readByte(input) / 255.0;
					float b2 = readByte(input) / 255.0;

					for (frame = 0, bezier = 0;; frame++) {
						float time2, nr, ng, nb, nr2, ng2, nb2;
						sp4RGB2Timeline_setFrame(timeline, frame, time, r, g, b, r2, g2, b2);
						if (frame == frameLast) break;
						time2 = readFloat_4(input);
						nr = readByte(input) / 255.0;
						ng = readByte(input) / 255.0;
						nb = readByte(input) / 255.0;
						nr2 = readByte(input) / 255.0;
						ng2 = readByte(input) / 255.0;
						nb2 = readByte(input) / 255.0;

						switch (readSByte(input)) {
							case CURVE_STEPPED:
								sp4CurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, r, nr, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, g, ng, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, b, nb, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 3, time, time2, r2, nr2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 4, time, time2, g2, ng2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 5, time, time2, b2, nb2, 1);
						}
						time = time2;
						r = nr;
						g = ng;
						b = nb;
						r2 = nr2;
						g2 = ng2;
						b2 = nb2;
					}
					sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				case SLOT_ALPHA: {
					int bezierCount = readVarint(input, 1);
					sp4AlphaTimeline *timeline = sp4AlphaTimeline_create(frameCount, bezierCount, slotIndex);
					float time = readFloat_4(input);
					float a = readByte(input) / 255.0;
					for (frame = 0, bezier = 0;; frame++) {
						float time2, a2;
						sp4AlphaTimeline_setFrame(timeline, frame, time, a);
						if (frame == frameLast) break;
						time2 = readFloat_4(input);
						a2 = readByte(input) / 255;
						switch (readSByte(input)) {
							case CURVE_STEPPED:
								sp4CurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, a, a2, 1);
						}
						time = time2;
						a = a2;
					}
					sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
					break;
				}
				default: {
					return NULL;
				}
			}
		}
	}

	/* Bone timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int boneIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			unsigned char timelineType = readByte(input);
			int frameCount = readVarint(input, 1);
			int bezierCount = readVarint(input, 1);
			sp4Timeline *timeline = NULL;
			switch (timelineType) {
				case BONE_ROTATE:
					timeline = readTimeline(input, SUPER(sp4RotateTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				case BONE_TRANSLATE:
					timeline = readTimeline2(input,
											 SUPER(sp4TranslateTimeline_create(frameCount, bezierCount, boneIndex)),
											 scale);
					break;
				case BONE_TRANSLATEX:
					timeline = readTimeline(input,
											SUPER(sp4TranslateXTimeline_create(frameCount, bezierCount, boneIndex)),
											scale);
					break;
				case BONE_TRANSLATEY:
					timeline = readTimeline(input,
											SUPER(sp4TranslateYTimeline_create(frameCount, bezierCount, boneIndex)),
											scale);
					break;
				case BONE_SCALE:
					timeline = readTimeline2(input, SUPER(sp4ScaleTimeline_create(frameCount, bezierCount, boneIndex)),
											 1);
					break;
				case BONE_SCALEX:
					timeline = readTimeline(input, SUPER(sp4ScaleXTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				case BONE_SCALEY:
					timeline = readTimeline(input, SUPER(sp4ScaleYTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				case BONE_SHEAR:
					timeline = readTimeline2(input, SUPER(sp4ShearTimeline_create(frameCount, bezierCount, boneIndex)),
											 1);
					break;
				case BONE_SHEARX:
					timeline = readTimeline(input, SUPER(sp4ShearXTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				case BONE_SHEARY:
					timeline = readTimeline(input, SUPER(sp4ShearYTimeline_create(frameCount, bezierCount, boneIndex)),
											1);
					break;
				default: {
					for (iii = 0; iii < timelines->size; ++iii)
						sp4Timeline_dispose(timelines->items[iii]);
					sp4TimelineArray_dispose(timelines);
					_sp4SkeletonBinary_setError(self, "Invalid timeline type for a bone: ",
											   skeletonData->bones[boneIndex]->name);
					return NULL;
				}
			}
			sp4TimelineArray_add(timelines, timeline);
		}
	}

	/* IK constraint timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int index = readVarint(input, 1);
		int frameCount = readVarint(input, 1);
		int frameLast = frameCount - 1;
		int bezierCount = readVarint(input, 1);
		sp4IkConstraintTimeline *timeline = sp4IkConstraintTimeline_create(frameCount, bezierCount, index);
		float time = readFloat_4(input);
		float mix = readFloat_4(input);
		float softness = readFloat_4(input) * scale;
		for (frame = 0, bezier = 0;; frame++) {
			float time2, mix2, softness2;
			int bendDirection = readSByte(input);
			int compress = readBoolean(input);
			int stretch = readBoolean(input);
			sp4IkConstraintTimeline_setFrame(timeline, frame, time, mix, softness, bendDirection, compress, stretch);
			if (frame == frameLast) break;
			time2 = readFloat_4(input);
			mix2 = readFloat_4(input);
			softness2 = readFloat_4(input) * scale;
			switch (readSByte(input)) {
				case CURVE_STEPPED:
					sp4CurveTimeline_setStepped(SUPER(timeline), frame);
					break;
				case CURVE_BEZIER:
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, mix, mix2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, softness, softness2,
							  scale);
			}
			time = time2;
			mix = mix2;
			softness = softness2;
		}
		sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
	}

	/* Transform constraint timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int index = readVarint(input, 1);
		int frameCount = readVarint(input, 1);
		int frameLast = frameCount - 1;
		int bezierCount = readVarint(input, 1);
		sp4TransformConstraintTimeline *timeline = sp4TransformConstraintTimeline_create(frameCount, bezierCount, index);
		float time = readFloat_4(input);
		float mixRotate = readFloat_4(input);
		float mixX = readFloat_4(input);
		float mixY = readFloat_4(input);
		float mixScaleX = readFloat_4(input);
		float mixScaleY = readFloat_4(input);
		float mixShearY = readFloat_4(input);
		for (frame = 0, bezier = 0;; frame++) {
			float time2, mixRotate2, mixX2, mixY2, mixScaleX2, mixScaleY2, mixShearY2;
			sp4TransformConstraintTimeline_setFrame(timeline, frame, time, mixRotate, mixX, mixY, mixScaleX, mixScaleY,
												   mixShearY);
			if (frame == frameLast) break;
			time2 = readFloat_4(input);
			mixRotate2 = readFloat_4(input);
			mixX2 = readFloat_4(input);
			mixY2 = readFloat_4(input);
			mixScaleX2 = readFloat_4(input);
			mixScaleY2 = readFloat_4(input);
			mixShearY2 = readFloat_4(input);
			switch (readSByte(input)) {
				case CURVE_STEPPED:
					sp4CurveTimeline_setStepped(SUPER(timeline), frame);
					break;
				case CURVE_BEZIER:
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, mixRotate, mixRotate2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, mixX, mixX2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, mixY, mixY2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 3, time, time2, mixScaleX, mixScaleX2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 4, time, time2, mixScaleY, mixScaleY2, 1);
					setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 5, time, time2, mixShearY, mixShearY2, 1);
			}
			time = time2;
			mixRotate = mixRotate2;
			mixX = mixX2;
			mixY = mixY2;
			mixScaleX = mixScaleX2;
			mixScaleY = mixScaleY2;
			mixShearY = mixShearY2;
		}
		sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
	}

	/* Path constraint timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int index = readVarint(input, 1);
		sp4PathConstraintData *data = skeletonData->pathConstraints[index];
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			int type = readSByte(input);
			int frameCount = readVarint(input, 1);
			int bezierCount = readVarint(input, 1);
			switch (type) {
				case PATH_POSITION: {
					sp4TimelineArray_add(timelines, readTimeline(input, SUPER(sp4PathConstraintPositionTimeline_create(frameCount, bezierCount, index)),
																data->positionMode == SP4_POSITION_MODE_FIXED ? scale
																											 : 1));
					break;
				}
				case PATH_SPACING: {
					sp4TimelineArray_add(timelines, readTimeline(input,
																SUPER(sp4PathConstraintSpacingTimeline_create(frameCount,
																											 bezierCount,
																											 index)),
																data->spacingMode == SP4_SPACING_MODE_LENGTH ||
																				data->spacingMode == SP4_SPACING_MODE_FIXED
																		? scale
																		: 1));
					break;
				}
				case PATH_MIX: {
					float time, mixRotate, mixX, mixY;
					int frameLast;
					sp4PathConstraintMixTimeline *timeline = sp4PathConstraintMixTimeline_create(frameCount, bezierCount,
																							   index);
					time = readFloat_4(input);
					mixRotate = readFloat_4(input);
					mixX = readFloat_4(input);
					mixY = readFloat_4(input);
					for (frame = 0, bezier = 0, frameLast = timeline->super.super.frameCount - 1;; frame++) {
						float time2, mixRotate2, mixX2, mixY2;
						sp4PathConstraintMixTimeline_setFrame(timeline, frame, time, mixRotate, mixX, mixY);
						if (frame == frameLast) break;
						time2 = readFloat_4(input);
						mixRotate2 = readFloat_4(input);
						mixX2 = readFloat_4(input);
						mixY2 = readFloat_4(input);
						switch (readSByte(input)) {
							case CURVE_STEPPED:
								sp4CurveTimeline_setStepped(SUPER(timeline), frame);
								break;
							case CURVE_BEZIER:
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, mixRotate,
										  mixRotate2, 1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 1, time, time2, mixX, mixX2,
										  1);
								setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 2, time, time2, mixY, mixY2,
										  1);
						}
						time = time2;
						mixRotate = mixRotate2;
						mixX = mixX2;
						mixY = mixY2;
					}
					sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
				}
			}
		}
	}

	/* Deform timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		sp4Skin *skin = skeletonData->skins[readVarint(input, 1)];
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			int slotIndex = readVarint(input, 1);
			for (iii = 0, nnn = readVarint(input, 1); iii < nnn; ++iii) {
				float *tempDeform;
				sp4DeformTimeline *timeline;
				int weighted, deformLength;
				const char *attachmentName = readStringRef(input, skeletonData);
				int frameCount, frameLast, bezierCount;
				float time, time2;

				sp4VertexAttachment *attachment = SUB_CAST(sp4VertexAttachment,
														  sp4Skin_getAttachment(skin, slotIndex, attachmentName));
				if (!attachment) {
					for (i = 0; i < timelines->size; ++i)
						sp4Timeline_dispose(timelines->items[i]);
					sp4TimelineArray_dispose(timelines);
					_sp4SkeletonBinary_setError(self, "Attachment not found: ", attachmentName);
					return NULL;
				}

				weighted = attachment->bones != 0;
				deformLength = weighted ? attachment->verticesCount / 3 * 2 : attachment->verticesCount;
				tempDeform = MALLOC(float, deformLength);

				frameCount = readVarint(input, 1);
				frameLast = frameCount - 1;
				bezierCount = readVarint(input, 1);
				timeline = sp4DeformTimeline_create(frameCount, deformLength, bezierCount, slotIndex, attachment);

				time = readFloat_4(input);
				for (frame = 0, bezier = 0;; ++frame) {
					float *deform;
					int end = readVarint(input, 1);
					if (!end) {
						if (weighted) {
							deform = tempDeform;
							memset(deform, 0, sizeof(float) * deformLength);
						} else
							deform = attachment->vertices;
					} else {
						int v, start = readVarint(input, 1);
						deform = tempDeform;
						memset(deform, 0, sizeof(float) * start);
						end += start;
						if (self->scale == 1) {
							for (v = start; v < end; ++v)
								deform[v] = readFloat_4(input);
						} else {
							for (v = start; v < end; ++v)
								deform[v] = readFloat_4(input) * self->scale;
						}
						memset(deform + v, 0, sizeof(float) * (deformLength - v));
						if (!weighted) {
							float *vertices = attachment->vertices;
							for (v = 0; v < deformLength; ++v)
								deform[v] += vertices[v];
						}
					}
					sp4DeformTimeline_setFrame(timeline, frame, time, deform);
					if (frame == frameLast) break;
					time2 = readFloat_4(input);
					switch (readSByte(input)) {
						case CURVE_STEPPED:
							sp4CurveTimeline_setStepped(SUPER(timeline), frame);
							break;
						case CURVE_BEZIER:
							setBezier(input, SUPER(SUPER(timeline)), bezier++, frame, 0, time, time2, 0, 1, 1);
					}
					time = time2;
				}
				FREE(tempDeform);

				sp4TimelineArray_add(timelines, (sp4Timeline *) timeline);
			}
		}
	}

	/* Draw order timeline. */
	drawOrderCount = readVarint(input, 1);
	if (drawOrderCount) {
		sp4DrawOrderTimeline *timeline = sp4DrawOrderTimeline_create(drawOrderCount, skeletonData->slotsCount);
		for (i = 0; i < drawOrderCount; ++i) {
			float time = readFloat_4(input);
			int offsetCount = readVarint(input, 1);
			int *drawOrder = MALLOC(int, skeletonData->slotsCount);
			int *unchanged = MALLOC(int, skeletonData->slotsCount - offsetCount);
			int originalIndex = 0, unchangedIndex = 0;
			memset(drawOrder, -1, sizeof(int) * skeletonData->slotsCount);
			for (ii = 0; ii < offsetCount; ++ii) {
				int slotIndex = readVarint(input, 1);
				/* Collect unchanged items. */
				while (originalIndex != slotIndex)
					unchanged[unchangedIndex++] = originalIndex++;
				/* Set changed items. */
				drawOrder[originalIndex + readVarint(input, 1)] = originalIndex;
				++originalIndex;
			}
			/* Collect remaining unchanged items. */
			while (originalIndex < skeletonData->slotsCount)
				unchanged[unchangedIndex++] = originalIndex++;
			/* Fill in unchanged items. */
			for (ii = skeletonData->slotsCount - 1; ii >= 0; ii--)
				if (drawOrder[ii] == -1) drawOrder[ii] = unchanged[--unchangedIndex];
			FREE(unchanged);
			/* TODO Avoid copying of drawOrder inside */
			sp4DrawOrderTimeline_setFrame(timeline, i, time, drawOrder);
			FREE(drawOrder);
		}
		sp4TimelineArray_add(timelines, (sp4Timeline *) timeline);
	}

	/* Event timeline. */
	eventCount = readVarint(input, 1);
	if (eventCount) {
		sp4EventTimeline *timeline = sp4EventTimeline_create(eventCount);
		for (i = 0; i < eventCount; ++i) {
			float time = readFloat_4(input);
			sp4EventData *eventData = skeletonData->events[readVarint(input, 1)];
			sp4Event *event = sp4Event_create(time, eventData);
			event->intValue = readVarint(input, 0);
			event->floatValue = readFloat_4(input);
			if (readBoolean(input))
				event->stringValue = readString_4(input);
			else
				MALLOC_STR(event->stringValue, eventData->stringValue);
			if (eventData->audioPath) {
				event->volume = readFloat_4(input);
				event->balance = readFloat_4(input);
			}
			sp4EventTimeline_setFrame(timeline, i, event);
		}
		sp4TimelineArray_add(timelines, (sp4Timeline *) timeline);
	}

	duration = 0;
	for (i = 0, n = timelines->size; i < n; i++) {
		duration = MAX(duration, sp4Timeline_getDuration(timelines->items[i]));
	}
	animation = sp4Animation_create(name, timelines, duration);
	return animation;
}

static float *_readFloatArray(_dataInput *input, int n, float scale) {
	float *array = MALLOC(float, n);
	int i;
	if (scale == 1)
		for (i = 0; i < n; ++i)
			array[i] = readFloat_4(input);
	else
		for (i = 0; i < n; ++i)
			array[i] = readFloat_4(input) * scale;
	return array;
}

static short *_readShortArray(_dataInput *input, int *length) {
	int n = readVarint(input, 1);
	short *array = MALLOC(short, n);
	int i;
	*length = n;
	for (i = 0; i < n; ++i) {
		array[i] = readByte(input) << 8;
		array[i] |= readByte(input);
	}
	return array;
}

static void _readVertices(sp4SkeletonBinary *self, _dataInput *input, sp4VertexAttachment *attachment,
						  int vertexCount) {
	int i, ii;
	int verticesLength = vertexCount << 1;
	sp4FloatArray *weights = sp4FloatArray_create(8);
	sp4IntArray *bones = sp4IntArray_create(8);

	attachment->worldVerticesLength = verticesLength;

	if (!readBoolean(input)) {
		attachment->verticesCount = verticesLength;
		attachment->vertices = _readFloatArray(input, verticesLength, self->scale);
		attachment->bonesCount = 0;
		attachment->bones = 0;
		sp4FloatArray_dispose(weights);
		sp4IntArray_dispose(bones);
		return;
	}

	sp4FloatArray_ensureCapacity(weights, verticesLength * 3 * 3);
	sp4IntArray_ensureCapacity(bones, verticesLength * 3);

	for (i = 0; i < vertexCount; ++i) {
		int boneCount = readVarint(input, 1);
		sp4IntArray_add(bones, boneCount);
		for (ii = 0; ii < boneCount; ++ii) {
			sp4IntArray_add(bones, readVarint(input, 1));
			sp4FloatArray_add(weights, readFloat_4(input) * self->scale);
			sp4FloatArray_add(weights, readFloat_4(input) * self->scale);
			sp4FloatArray_add(weights, readFloat_4(input));
		}
	}

	attachment->verticesCount = weights->size;
	attachment->vertices = weights->items;
	FREE(weights);

	attachment->bonesCount = bones->size;
	attachment->bones = bones->items;
	FREE(bones);
}

sp4Attachment *sp4SkeletonBinary_readAttachment(sp4SkeletonBinary *self, _dataInput *input,
											  sp4Skin *skin, int slotIndex, const char *attachmentName,
											  sp4SkeletonData *skeletonData, int /*bool*/ nonessential) {
	int i;
	sp4AttachmentType type;
	const char *name = readStringRef(input, skeletonData);
	if (!name) name = attachmentName;

	type = (sp4AttachmentType) readByte(input);

	switch (type) {
		case SP4_ATTACHMENT_REGION: {
			const char *path = readStringRef(input, skeletonData);
			sp4Attachment *attachment;
			sp4RegionAttachment *region;
			if (!path) MALLOC_STR(path, name);
			else {
				const char *tmp = 0;
				MALLOC_STR(tmp, path);
				path = tmp;
			}
			attachment = sp4AttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, path);
			region = SUB_CAST(sp4RegionAttachment, attachment);
			region->path = path;
			region->rotation = readFloat_4(input);
			region->x = readFloat_4(input) * self->scale;
			region->y = readFloat_4(input) * self->scale;
			region->scaleX = readFloat_4(input);
			region->scaleY = readFloat_4(input);
			region->width = readFloat_4(input) * self->scale;
			region->height = readFloat_4(input) * self->scale;
			readColor(input, &region->color.r, &region->color.g, &region->color.b, &region->color.a);
			sp4RegionAttachment_updateOffset(region);
			sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP4_ATTACHMENT_BOUNDING_BOX: {
			int vertexCount = readVarint(input, 1);
			sp4Attachment *attachment = sp4AttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, 0);
			_readVertices(self, input, SUB_CAST(sp4VertexAttachment, attachment), vertexCount);
			if (nonessential) {
				sp4BoundingBoxAttachment *bbox = SUB_CAST(sp4BoundingBoxAttachment, attachment);
				readColor(input, &bbox->color.r, &bbox->color.g, &bbox->color.b, &bbox->color.a);
			}
			sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP4_ATTACHMENT_MESH: {
			int vertexCount;
			sp4Attachment *attachment;
			sp4MeshAttachment *mesh;
			const char *path = readStringRef(input, skeletonData);
			if (!path) MALLOC_STR(path, name);
			else {
				const char *tmp = 0;
				MALLOC_STR(tmp, path);
				path = tmp;
			}
			attachment = sp4AttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, path);
			mesh = SUB_CAST(sp4MeshAttachment, attachment);
			mesh->path = path;
			readColor(input, &mesh->color.r, &mesh->color.g, &mesh->color.b, &mesh->color.a);
			vertexCount = readVarint(input, 1);
			mesh->regionUVs = _readFloatArray(input, vertexCount << 1, 1);
			mesh->triangles = (unsigned short *) _readShortArray(input, &mesh->trianglesCount);
			_readVertices(self, input, SUPER(mesh), vertexCount);
			sp4MeshAttachment_updateUVs(mesh);
			mesh->hullLength = readVarint(input, 1) << 1;
			if (nonessential) {
				mesh->edges = (int *) _readShortArray(input, &mesh->edgesCount);
				mesh->width = readFloat_4(input) * self->scale;
				mesh->height = readFloat_4(input) * self->scale;
			} else {
				mesh->edges = 0;
				mesh->width = 0;
				mesh->height = 0;
			}
			sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP4_ATTACHMENT_LINKED_MESH: {
			const char *skinName;
			const char *parent;
			sp4Attachment *attachment;
			sp4MeshAttachment *mesh;
			int inheritDeform;
			const char *path = readStringRef(input, skeletonData);
			if (!path) MALLOC_STR(path, name);
			else {
				const char *tmp = 0;
				MALLOC_STR(tmp, path);
				path = tmp;
			}
			attachment = sp4AttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, path);
			mesh = SUB_CAST(sp4MeshAttachment, attachment);
			mesh->path = path;
			readColor(input, &mesh->color.r, &mesh->color.g, &mesh->color.b, &mesh->color.a);
			skinName = readStringRef(input, skeletonData);
			parent = readStringRef(input, skeletonData);
			inheritDeform = readBoolean(input);
			if (nonessential) {
				mesh->width = readFloat_4(input) * self->scale;
				mesh->height = readFloat_4(input) * self->scale;
			}
			_sp4SkeletonBinary_addLinkedMesh(self, mesh, skinName, slotIndex, parent, inheritDeform);
			return attachment;
		}
		case SP4_ATTACHMENT_PATH: {
			sp4Attachment *attachment = sp4AttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, 0);
			sp4PathAttachment *path = SUB_CAST(sp4PathAttachment, attachment);
			int vertexCount = 0;
			path->closed = readBoolean(input);
			path->constantSpeed = readBoolean(input);
			vertexCount = readVarint(input, 1);
			_readVertices(self, input, SUPER(path), vertexCount);
			path->lengthsLength = vertexCount / 3;
			path->lengths = MALLOC(float, path->lengthsLength);
			for (i = 0; i < path->lengthsLength; ++i) {
				path->lengths[i] = readFloat_4(input) * self->scale;
			}
			if (nonessential) {
				readColor(input, &path->color.r, &path->color.g, &path->color.b, &path->color.a);
			}
			sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP4_ATTACHMENT_POINT: {
			sp4Attachment *attachment = sp4AttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, 0);
			sp4PointAttachment *point = SUB_CAST(sp4PointAttachment, attachment);
			point->rotation = readFloat_4(input);
			point->x = readFloat_4(input) * self->scale;
			point->y = readFloat_4(input) * self->scale;

			if (nonessential) {
				readColor(input, &point->color.r, &point->color.g, &point->color.b, &point->color.a);
			}
			sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
		case SP4_ATTACHMENT_CLIPPING: {
			int endSlotIndex = readVarint(input, 1);
			int vertexCount = readVarint(input, 1);
			sp4Attachment *attachment = sp4AttachmentLoader_createAttachment(self->attachmentLoader, skin, type, name, 0);
			sp4ClippingAttachment *clip = SUB_CAST(sp4ClippingAttachment, attachment);
			_readVertices(self, input, SUB_CAST(sp4VertexAttachment, attachment), vertexCount);
			if (nonessential) {
				readColor(input, &clip->color.r, &clip->color.g, &clip->color.b, &clip->color.a);
			}
			clip->endSlot = skeletonData->slots[endSlotIndex];
			sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
			return attachment;
		}
	}

	return NULL;
}

sp4Skin *sp4SkeletonBinary_readSkin(sp4SkeletonBinary *self, _dataInput *input, int /*bool*/ defaultSkin,
								  sp4SkeletonData *skeletonData, int /*bool*/ nonessential) {
	sp4Skin *skin;
	int i, n, ii, nn, slotCount;

	if (defaultSkin) {
		slotCount = readVarint(input, 1);
		if (slotCount == 0) return NULL;
		skin = sp4Skin_create("default");
	} else {
		skin = sp4Skin_create(readStringRef(input, skeletonData));
		for (i = 0, n = readVarint(input, 1); i < n; i++)
			sp4BoneDataArray_add(skin->bones, skeletonData->bones[readVarint(input, 1)]);

		for (i = 0, n = readVarint(input, 1); i < n; i++)
			sp4IkConstraintDataArray_add(skin->ikConstraints, skeletonData->ikConstraints[readVarint(input, 1)]);

		for (i = 0, n = readVarint(input, 1); i < n; i++)
			sp4TransformConstraintDataArray_add(skin->transformConstraints,
											   skeletonData->transformConstraints[readVarint(input, 1)]);

		for (i = 0, n = readVarint(input, 1); i < n; i++)
			sp4PathConstraintDataArray_add(skin->pathConstraints, skeletonData->pathConstraints[readVarint(input, 1)]);

		slotCount = readVarint(input, 1);
	}

	for (i = 0; i < slotCount; ++i) {
		int slotIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			const char *name = readStringRef(input, skeletonData);
			sp4Attachment *attachment = sp4SkeletonBinary_readAttachment(self, input, skin, slotIndex, name, skeletonData,
																	   nonessential);
			if (attachment) sp4Skin_setAttachment(skin, slotIndex, name, attachment);
		}
	}
	return skin;
}

sp4SkeletonData *sp4SkeletonBinary_readSkeletonDataFile(sp4SkeletonBinary *self, const char *path) {
	int length;
	sp4SkeletonData *skeletonData;
	const char *binary = _sp4Util_readFile(path, &length);
	if (length == 0 || !binary) {
		_sp4SkeletonBinary_setError(self, "Unable to read skeleton file: ", path);
		return NULL;
	}
	skeletonData = sp4SkeletonBinary_readSkeletonData(self, (unsigned char *) binary, length);
	FREE(binary);
	return skeletonData;
}

sp4SkeletonData *sp4SkeletonBinary_readSkeletonData(sp4SkeletonBinary *self, const unsigned char *binary,
												  const int length) {
	int i, n, ii, nonessential;
	char buffer[32];
	int lowHash, highHash;
	sp4SkeletonData *skeletonData;
	_sp4SkeletonBinary *internal = SUB_CAST(_sp4SkeletonBinary, self);

	_dataInput *input = NEW(_dataInput);
	input->cursor = binary;
	input->end = binary + length;

	FREE(self->error);
	CONST_CAST(char *, self->error) = 0;
	internal->linkedMeshCount = 0;

	skeletonData = sp4SkeletonData_create();
	lowHash = readInt(input);
	highHash = readInt(input);
	sprintf(buffer, "%x%x", highHash, lowHash);
	buffer[31] = 0;
	MALLOC_STR(skeletonData->hash, buffer);

	skeletonData->version = readString_4(input);
	if (!strlen(skeletonData->version)) {
		FREE(skeletonData->version);
		skeletonData->version = 0;
	}

	skeletonData->x = readFloat_4(input);
	skeletonData->y = readFloat_4(input);
	skeletonData->width = readFloat_4(input);
	skeletonData->height = readFloat_4(input);

	nonessential = readBoolean(input);

	if (nonessential) {
		skeletonData->fps = readFloat_4(input);
		skeletonData->imagesPath = readString_4(input);
		if (!strlen(skeletonData->imagesPath)) {
			FREE(skeletonData->imagesPath);
			skeletonData->imagesPath = 0;
		}
		skeletonData->audioPath = readString_4(input);
		if (!strlen(skeletonData->audioPath)) {
			FREE(skeletonData->audioPath);
			skeletonData->audioPath = 0;
		}
	}

	skeletonData->stringsCount = n = readVarint(input, 1);
	skeletonData->strings = MALLOC(char *, skeletonData->stringsCount);
	for (i = 0; i < n; i++) {
		skeletonData->strings[i] = readString_4(input);
	}

	/* Bones. */
	skeletonData->bonesCount = readVarint(input, 1);
	skeletonData->bones = MALLOC(sp4BoneData *, skeletonData->bonesCount);
	for (i = 0; i < skeletonData->bonesCount; ++i) {
		sp4BoneData *data;
		int mode;
		const char *name = readString_4(input);
		sp4BoneData *parent = i == 0 ? 0 : skeletonData->bones[readVarint(input, 1)];
		/* TODO Avoid copying of name */
		data = sp4BoneData_create(i, name, parent);
		FREE(name);
		data->rotation = readFloat_4(input);
		data->x = readFloat_4(input) * self->scale;
		data->y = readFloat_4(input) * self->scale;
		data->scaleX = readFloat_4(input);
		data->scaleY = readFloat_4(input);
		data->shearX = readFloat_4(input);
		data->shearY = readFloat_4(input);
		data->length = readFloat_4(input) * self->scale;
		mode = readVarint(input, 1);
		switch (mode) {
			case 0:
				data->transformMode = SP4_TRANSFORMMODE_NORMAL;
				break;
			case 1:
				data->transformMode = SP4_TRANSFORMMODE_ONLYTRANSLATION;
				break;
			case 2:
				data->transformMode = SP4_TRANSFORMMODE_NOROTATIONORREFLECTION;
				break;
			case 3:
				data->transformMode = SP4_TRANSFORMMODE_NOSCALE;
				break;
			case 4:
				data->transformMode = SP4_TRANSFORMMODE_NOSCALEORREFLECTION;
				break;
		}
		data->skinRequired = readBoolean(input);
		if (nonessential) {
			readColor(input, &data->color.r, &data->color.g, &data->color.b, &data->color.a);
		}
		skeletonData->bones[i] = data;
	}

	/* Slots. */
	skeletonData->slotsCount = readVarint(input, 1);
	skeletonData->slots = MALLOC(sp4SlotData *, skeletonData->slotsCount);
	for (i = 0; i < skeletonData->slotsCount; ++i) {
		int r, g, b, a;
		const char *attachmentName;
		const char *slotName = readString_4(input);
		sp4BoneData *boneData = skeletonData->bones[readVarint(input, 1)];
		/* TODO Avoid copying of slotName */
		sp4SlotData *slotData = sp4SlotData_create(i, slotName, boneData);
		FREE(slotName);
		readColor(input, &slotData->color.r, &slotData->color.g, &slotData->color.b, &slotData->color.a);
		a = readByte(input);
		r = readByte(input);
		g = readByte(input);
		b = readByte(input);
		if (!(r == 0xff && g == 0xff && b == 0xff && a == 0xff)) {
			slotData->darkColor = sp4Color_create();
			sp4Color_setFromFloats(slotData->darkColor, r / 255.0f, g / 255.0f, b / 255.0f, 1);
		}
		attachmentName = readStringRef(input, skeletonData);
		if (attachmentName) MALLOC_STR(slotData->attachmentName, attachmentName);
		else
			slotData->attachmentName = 0;
		slotData->blendMode = (sp4BlendMode) readVarint(input, 1);
		skeletonData->slots[i] = slotData;
	}

	/* IK constraints. */
	skeletonData->ikConstraintsCount = readVarint(input, 1);
	skeletonData->ikConstraints = MALLOC(sp4IkConstraintData *, skeletonData->ikConstraintsCount);
	for (i = 0; i < skeletonData->ikConstraintsCount; ++i) {
		const char *name = readString_4(input);
		/* TODO Avoid copying of name */
		sp4IkConstraintData *data = sp4IkConstraintData_create(name);
		data->order = readVarint(input, 1);
		data->skinRequired = readBoolean(input);
		FREE(name);
		data->bonesCount = readVarint(input, 1);
		data->bones = MALLOC(sp4BoneData *, data->bonesCount);
		for (ii = 0; ii < data->bonesCount; ++ii)
			data->bones[ii] = skeletonData->bones[readVarint(input, 1)];
		data->target = skeletonData->bones[readVarint(input, 1)];
		data->mix = readFloat_4(input);
		data->softness = readFloat_4(input);
		data->bendDirection = readSByte(input);
		data->compress = readBoolean(input);
		data->stretch = readBoolean(input);
		data->uniform = readBoolean(input);
		skeletonData->ikConstraints[i] = data;
	}

	/* Transform constraints. */
	skeletonData->transformConstraintsCount = readVarint(input, 1);
	skeletonData->transformConstraints = MALLOC(
			sp4TransformConstraintData *, skeletonData->transformConstraintsCount);
	for (i = 0; i < skeletonData->transformConstraintsCount; ++i) {
		const char *name = readString_4(input);
		/* TODO Avoid copying of name */
		sp4TransformConstraintData *data = sp4TransformConstraintData_create(name);
		data->order = readVarint(input, 1);
		data->skinRequired = readBoolean(input);
		FREE(name);
		data->bonesCount = readVarint(input, 1);
		CONST_CAST(sp4BoneData **, data->bones) = MALLOC(sp4BoneData *, data->bonesCount);
		for (ii = 0; ii < data->bonesCount; ++ii)
			data->bones[ii] = skeletonData->bones[readVarint(input, 1)];
		data->target = skeletonData->bones[readVarint(input, 1)];
		data->local = readBoolean(input);
		data->relative = readBoolean(input);
		data->offsetRotation = readFloat_4(input);
		data->offsetX = readFloat_4(input) * self->scale;
		data->offsetY = readFloat_4(input) * self->scale;
		data->offsetScaleX = readFloat_4(input);
		data->offsetScaleY = readFloat_4(input);
		data->offsetShearY = readFloat_4(input);
		data->mixRotate = readFloat_4(input);
		data->mixX = readFloat_4(input);
		data->mixY = readFloat_4(input);
		data->mixScaleX = readFloat_4(input);
		data->mixScaleY = readFloat_4(input);
		data->mixShearY = readFloat_4(input);
		skeletonData->transformConstraints[i] = data;
	}

	/* Path constraints */
	skeletonData->pathConstraintsCount = readVarint(input, 1);
	skeletonData->pathConstraints = MALLOC(sp4PathConstraintData *, skeletonData->pathConstraintsCount);
	for (i = 0; i < skeletonData->pathConstraintsCount; ++i) {
		const char *name = readString_4(input);
		/* TODO Avoid copying of name */
		sp4PathConstraintData *data = sp4PathConstraintData_create(name);
		data->order = readVarint(input, 1);
		data->skinRequired = readBoolean(input);
		FREE(name);
		data->bonesCount = readVarint(input, 1);
		CONST_CAST(sp4BoneData **, data->bones) = MALLOC(sp4BoneData *, data->bonesCount);
		for (ii = 0; ii < data->bonesCount; ++ii)
			data->bones[ii] = skeletonData->bones[readVarint(input, 1)];
		data->target = skeletonData->slots[readVarint(input, 1)];
		data->positionMode = (sp4PositionMode) readVarint(input, 1);
		data->spacingMode = (sp4SpacingMode) readVarint(input, 1);
		data->rotateMode = (sp4RotateMode) readVarint(input, 1);
		data->offsetRotation = readFloat_4(input);
		data->position = readFloat_4(input);
		if (data->positionMode == SP4_POSITION_MODE_FIXED) data->position *= self->scale;
		data->spacing = readFloat_4(input);
		if (data->spacingMode == SP4_SPACING_MODE_LENGTH || data->spacingMode == SP4_SPACING_MODE_FIXED)
			data->spacing *= self->scale;
		data->mixRotate = readFloat_4(input);
		data->mixX = readFloat_4(input);
		data->mixY = readFloat_4(input);
		skeletonData->pathConstraints[i] = data;
	}

	/* Default skin. */
	skeletonData->defaultSkin = sp4SkeletonBinary_readSkin(self, input, -1, skeletonData, nonessential);
	skeletonData->skinsCount = readVarint(input, 1);

	if (skeletonData->defaultSkin)
		++skeletonData->skinsCount;

	skeletonData->skins = MALLOC(sp4Skin *, skeletonData->skinsCount);

	if (skeletonData->defaultSkin)
		skeletonData->skins[0] = skeletonData->defaultSkin;

	/* Skins. */
	for (i = skeletonData->defaultSkin ? 1 : 0; i < skeletonData->skinsCount; ++i) {
		skeletonData->skins[i] = sp4SkeletonBinary_readSkin(self, input, 0, skeletonData, nonessential);
	}

	/* Linked meshes. */
	for (i = 0; i < internal->linkedMeshCount; ++i) {
		_sp4LinkedMesh *linkedMesh = internal->linkedMeshes + i;
		sp4Skin *skin = !linkedMesh->skin ? skeletonData->defaultSkin : sp4SkeletonData_findSkin(skeletonData, linkedMesh->skin);
		sp4Attachment *parent;
		if (!skin) {
			FREE(input);
			sp4SkeletonData_dispose(skeletonData);
			_sp4SkeletonBinary_setError(self, "Skin not found: ", linkedMesh->skin);
			return NULL;
		}
		parent = sp4Skin_getAttachment(skin, linkedMesh->slotIndex, linkedMesh->parent);
		if (!parent) {
			FREE(input);
			sp4SkeletonData_dispose(skeletonData);
			_sp4SkeletonBinary_setError(self, "Parent mesh not found: ", linkedMesh->parent);
			return NULL;
		}
		linkedMesh->mesh->super.deformAttachment = linkedMesh->inheritDeform ? SUB_CAST(sp4VertexAttachment, parent)
																			 : SUB_CAST(sp4VertexAttachment,
																						linkedMesh->mesh);
		sp4MeshAttachment_setParentMesh(linkedMesh->mesh, SUB_CAST(sp4MeshAttachment, parent));
		sp4MeshAttachment_updateUVs(linkedMesh->mesh);
		sp4AttachmentLoader_configureAttachment(self->attachmentLoader, SUPER(SUPER(linkedMesh->mesh)));
	}

	/* Events. */
	skeletonData->eventsCount = readVarint(input, 1);
	skeletonData->events = MALLOC(sp4EventData *, skeletonData->eventsCount);
	for (i = 0; i < skeletonData->eventsCount; ++i) {
		const char *name = readStringRef(input, skeletonData);
		sp4EventData *eventData = sp4EventData_create(name);
		eventData->intValue = readVarint(input, 0);
		eventData->floatValue = readFloat_4(input);
		eventData->stringValue = readString_4(input);
		eventData->audioPath = readString_4(input);
		if (eventData->audioPath) {
			eventData->volume = readFloat_4(input);
			eventData->balance = readFloat_4(input);
		}
		skeletonData->events[i] = eventData;
	}

	/* Animations. */
	skeletonData->animationsCount = readVarint(input, 1);
	skeletonData->animations = MALLOC(sp4Animation *, skeletonData->animationsCount);
	for (i = 0; i < skeletonData->animationsCount; ++i) {
		const char *name = readString_4(input);
		sp4Animation *animation = _sp4SkeletonBinary_readAnimation(self, name, input, skeletonData);
		FREE(name);
		if (!animation) {
			FREE(input);
			sp4SkeletonData_dispose(skeletonData);
			return NULL;
		}
		skeletonData->animations[i] = animation;
	}

	FREE(input);
	return skeletonData;
}
