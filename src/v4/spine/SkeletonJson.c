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

#include "Json.h"
#include <v4/spine/Array.h>
#include <v4/spine/AtlasAttachmentLoader.h>
#include <v4/spine/SkeletonJson.h>
#include <v4/spine/extension.h>
#include <stdio.h>

typedef struct {
	const char *parent;
	const char *skin;
	int slotIndex;
	sp4MeshAttachment *mesh;
	int inheritDeform;
} _sp4LinkedMesh;

typedef struct {
	sp4SkeletonJson super;
	int ownsLoader;

	int linkedMeshCount;
	int linkedMeshCapacity;
	_sp4LinkedMesh *linkedMeshes;
} _sp4SkeletonJson;

sp4SkeletonJson *sp4SkeletonJson_createWithLoader(sp4AttachmentLoader *attachmentLoader) {
	sp4SkeletonJson *self = SUPER(NEW(_sp4SkeletonJson));
	self->scale = 1;
	self->attachmentLoader = attachmentLoader;
	return self;
}

sp4SkeletonJson *sp4SkeletonJson_create(sp4Atlas *atlas) {
	sp4AtlasAttachmentLoader *attachmentLoader = sp4AtlasAttachmentLoader_create(atlas);
	sp4SkeletonJson *self = sp4SkeletonJson_createWithLoader(SUPER(attachmentLoader));
	SUB_CAST(_sp4SkeletonJson, self)->ownsLoader = 1;
	return self;
}

void sp4SkeletonJson_dispose(sp4SkeletonJson *self) {
	_sp4SkeletonJson *internal = SUB_CAST(_sp4SkeletonJson, self);
	if (internal->ownsLoader) sp4AttachmentLoader_dispose(self->attachmentLoader);
	FREE(internal->linkedMeshes);
	FREE(self->error);
	FREE(self);
}

void _sp4SkeletonJson_setError(sp4SkeletonJson *self, Json *root, const char *value1, const char *value2) {
	char message[256];
	int length;
	FREE(self->error);
	strcpy(message, value1);
	length = (int) strlen(value1);
	if (value2) strncat(message + length, value2, 255 - length);
	MALLOC_STR(self->error, message);
	if (root) Json_dispose_4(root);
}

static float toColor(const char *value, int index) {
	char digits[3];
	char *error;
	int color;

	if ((size_t) index >= strlen(value) / 2) return -1;
	value += index * 2;

	digits[0] = *value;
	digits[1] = *(value + 1);
	digits[2] = '\0';
	color = (int) strtoul(digits, &error, 16);
	if (*error != 0) return -1;
	return color / (float) 255;
}

static void toColor2(sp4Color *color, const char *value, int /*bool*/ hasAlpha) {
	color->r = toColor(value, 0);
	color->g = toColor(value, 1);
	color->b = toColor(value, 2);
	if (hasAlpha) color->a = toColor(value, 3);
}

static void
setBezier(sp4CurveTimeline *timeline, int frame, int value, int bezier, float time1, float value1, float cx1, float cy1,
		  float cx2, float cy2, float time2, float value2) {
	sp4Timeline_setBezier(SUPER(timeline), bezier, frame, value, time1, value1, cx1, cy1, cx2, cy2, time2, value2);
}

static int readCurve(Json *curve, sp4CurveTimeline *timeline, int bezier, int frame, int value, float time1, float time2,
					 float value1, float value2, float scale) {
	float cx1, cy1, cx2, cy2;
	if (curve->type == Json_String && strcmp(curve->valueString, "stepped") == 0) {
		if (value != 0) sp4CurveTimeline_setStepped(timeline, frame);
		return bezier;
	}
	curve = Json_getItemAtIndex_4(curve, value << 2);
	cx1 = curve->valueFloat;
	curve = curve->next;
	cy1 = curve->valueFloat * scale;
	curve = curve->next;
	cx2 = curve->valueFloat;
	curve = curve->next;
	cy2 = curve->valueFloat * scale;
	setBezier(timeline, frame, value, bezier, time1, value1, cx1, cy1, cx2, cy2, time2, value2);
	return bezier + 1;
}

static sp4Timeline *readTimeline(Json *keyMap, sp4CurveTimeline1 *timeline, float defaultValue, float scale) {
	float time = Json_getFloat_4(keyMap, "time", 0);
	float value = Json_getFloat_4(keyMap, "value", defaultValue) * scale;
	int frame, bezier = 0;
	for (frame = 0;; ++frame) {
		Json *nextMap, *curve;
		float time2, value2;
		sp4CurveTimeline1_setFrame(timeline, frame, time, value);
		nextMap = keyMap->next;
		if (nextMap == NULL) break;
		time2 = Json_getFloat_4(nextMap, "time", 0);
		value2 = Json_getFloat_4(nextMap, "value", defaultValue) * scale;
		curve = Json_getItem_4(keyMap, "curve");
		if (curve != NULL) bezier = readCurve(curve, timeline, bezier, frame, 0, time, time2, value, value2, scale);
		time = time2;
		value = value2;
		keyMap = nextMap;
	}
	/* timeline.shrink(); // BOZO */
	return SUPER(timeline);
}

static sp4Timeline *
readTimeline2(Json *keyMap, sp4CurveTimeline2 *timeline, const char *name1, const char *name2, float defaultValue,
			  float scale) {
	float time = Json_getFloat_4(keyMap, "time", 0);
	float value1 = Json_getFloat_4(keyMap, name1, defaultValue) * scale;
	float value2 = Json_getFloat_4(keyMap, name2, defaultValue) * scale;
	int frame, bezier = 0;
	for (frame = 0;; ++frame) {
		Json *nextMap, *curve;
		float time2, nvalue1, nvalue2;
		sp4CurveTimeline2_setFrame(timeline, frame, time, value1, value2);
		nextMap = keyMap->next;
		if (nextMap == NULL) break;
		time2 = Json_getFloat_4(nextMap, "time", 0);
		nvalue1 = Json_getFloat_4(nextMap, name1, defaultValue) * scale;
		nvalue2 = Json_getFloat_4(nextMap, name2, defaultValue) * scale;
		curve = Json_getItem_4(keyMap, "curve");
		if (curve != NULL) {
			bezier = readCurve(curve, timeline, bezier, frame, 0, time, time2, value1, nvalue1, scale);
			bezier = readCurve(curve, timeline, bezier, frame, 1, time, time2, value2, nvalue2, scale);
		}
		time = time2;
		value1 = nvalue1;
		value2 = nvalue2;
		keyMap = nextMap;
	}
	/* timeline.shrink(); // BOZO */
	return SUPER(timeline);
}

static void _sp4SkeletonJson_addLinkedMesh(sp4SkeletonJson *self, sp4MeshAttachment *mesh, const char *skin, int slotIndex,
										  const char *parent, int inheritDeform) {
	_sp4LinkedMesh *linkedMesh;
	_sp4SkeletonJson *internal = SUB_CAST(_sp4SkeletonJson, self);

	if (internal->linkedMeshCount == internal->linkedMeshCapacity) {
		_sp4LinkedMesh *linkedMeshes;
		internal->linkedMeshCapacity *= 2;
		if (internal->linkedMeshCapacity < 8) internal->linkedMeshCapacity = 8;
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

static void cleanUpTimelines(sp4TimelineArray *timelines) {
	int i, n;
	for (i = 0, n = timelines->size; i < n; ++i)
		sp4Timeline_dispose(timelines->items[i]);
	sp4TimelineArray_dispose(timelines);
}

static int findSlotIndex(sp4SkeletonJson *json, const sp4SkeletonData *skeletonData, const char *slotName, sp4TimelineArray *timelines) {
	sp4SlotData *slot = sp4SkeletonData_findSlot(skeletonData, slotName);
	if (slot) return slot->index;
	cleanUpTimelines(timelines);
	_sp4SkeletonJson_setError(json, NULL, "Slot not found: ", slotName);
	return -1;
}

int findIkConstraintIndex(sp4SkeletonJson *json, const sp4SkeletonData *skeletonData, const sp4IkConstraintData *constraint, sp4TimelineArray *timelines) {
	if (constraint) {
		int i;
		for (i = 0; i < skeletonData->ikConstraintsCount; ++i)
			if (skeletonData->ikConstraints[i] == constraint) return i;
	}
	cleanUpTimelines(timelines);
	_sp4SkeletonJson_setError(json, NULL, "IK constraint not found: ", constraint->name);
	return -1;
}

int findTransformConstraintIndex(sp4SkeletonJson *json, const sp4SkeletonData *skeletonData, const sp4TransformConstraintData *constraint, sp4TimelineArray *timelines) {
	if (constraint) {
		int i;
		for (i = 0; i < skeletonData->transformConstraintsCount; ++i)
			if (skeletonData->transformConstraints[i] == constraint) return i;
	}
	cleanUpTimelines(timelines);
	_sp4SkeletonJson_setError(json, NULL, "Transform constraint not found: ", constraint->name);
	return -1;
}

int findPathConstraintIndex(sp4SkeletonJson *json, const sp4SkeletonData *skeletonData, const sp4PathConstraintData *constraint, sp4TimelineArray *timelines) {
	if (constraint) {
		int i;
		for (i = 0; i < skeletonData->pathConstraintsCount; ++i)
			if (skeletonData->pathConstraints[i] == constraint) return i;
	}
	cleanUpTimelines(timelines);
	_sp4SkeletonJson_setError(json, NULL, "Path constraint not found: ", constraint->name);
	return -1;
}

static sp4Animation *_sp4SkeletonJson_readAnimation(sp4SkeletonJson *self, Json *root, sp4SkeletonData *skeletonData) {
	sp4TimelineArray *timelines = sp4TimelineArray_create(8);

	float scale = self->scale, duration;
	Json *bones = Json_getItem_4(root, "bones");
	Json *slots = Json_getItem_4(root, "slots");
	Json *ik = Json_getItem_4(root, "ik");
	Json *transform = Json_getItem_4(root, "transform");
	Json *paths = Json_getItem_4(root, "path");
	Json *deformJson = Json_getItem_4(root, "deform");
	Json *drawOrderJson = Json_getItem_4(root, "drawOrder");
	Json *events = Json_getItem_4(root, "events");
	Json *boneMap, *slotMap, *constraintMap, *keyMap, *nextMap, *curve, *timelineMap;
	int frame, bezier, i, n;
	sp4Color color, color2, newColor, newColor2;

	/* Slot timelines. */
	for (slotMap = slots ? slots->child : 0; slotMap; slotMap = slotMap->next) {
		int slotIndex = findSlotIndex(self, skeletonData, slotMap->name, timelines);
		if (slotIndex == -1) return NULL;

		for (timelineMap = slotMap->child; timelineMap; timelineMap = timelineMap->next) {
			int frames = timelineMap->size;
			if (strcmp(timelineMap->name, "attachment") == 0) {
				sp4AttachmentTimeline *timeline = sp4AttachmentTimeline_create(frames, slotIndex);
				for (keyMap = timelineMap->child, frame = 0; keyMap; keyMap = keyMap->next, ++frame) {
					sp4AttachmentTimeline_setFrame(timeline, frame, Json_getFloat_4(keyMap, "time", 0),
												  Json_getItem_4(keyMap, "name")->valueString);
				}
				sp4TimelineArray_add(timelines, SUPER(timeline));

			} else if (strcmp(timelineMap->name, "rgba") == 0) {
				float time;
				sp4RGBATimeline *timeline = sp4RGBATimeline_create(frames, frames << 2, slotIndex);
				keyMap = timelineMap->child;
				time = Json_getFloat_4(keyMap, "time", 0);
				toColor2(&color, Json_getString_4(keyMap, "color", 0), 1);

				for (frame = 0, bezier = 0;; ++frame) {
					float time2;
					sp4RGBATimeline_setFrame(timeline, frame, time, color.r, color.g, color.b, color.a);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = Json_getFloat_4(nextMap, "time", 0);
					toColor2(&newColor, Json_getString_4(nextMap, "color", 0), 1);
					curve = Json_getItem_4(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, color.r, newColor.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, color.g, newColor.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, color.b, newColor.b,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 3, time, time2, color.a, newColor.a,
										   1);
					}
					time = time2;
					color = newColor;
					keyMap = nextMap;
				}
				sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
			} else if (strcmp(timelineMap->name, "rgb") == 0) {
				float time;
				sp4RGBTimeline *timeline = sp4RGBTimeline_create(frames, frames * 3, slotIndex);
				keyMap = timelineMap->child;
				time = Json_getFloat_4(keyMap, "time", 0);
				toColor2(&color, Json_getString_4(keyMap, "color", 0), 1);

				for (frame = 0, bezier = 0;; ++frame) {
					float time2;
					sp4RGBTimeline_setFrame(timeline, frame, time, color.r, color.g, color.b);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = Json_getFloat_4(nextMap, "time", 0);
					toColor2(&newColor, Json_getString_4(nextMap, "color", 0), 1);
					curve = Json_getItem_4(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, color.r, newColor.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, color.g, newColor.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, color.b, newColor.b,
										   1);
					}
					time = time2;
					color = newColor;
					keyMap = nextMap;
				}
				sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
			} else if (strcmp(timelineMap->name, "alpha") == 0) {
				sp4TimelineArray_add(timelines, readTimeline(timelineMap->child,
															SUPER(sp4AlphaTimeline_create(frames,
																						 frames, slotIndex)),
															0, 1));
			} else if (strcmp(timelineMap->name, "rgba2") == 0) {
				float time;
				sp4RGBA2Timeline *timeline = sp4RGBA2Timeline_create(frames, frames * 7, slotIndex);
				keyMap = timelineMap->child;
				time = Json_getFloat_4(keyMap, "time", 0);
				toColor2(&color, Json_getString_4(keyMap, "light", 0), 1);
				toColor2(&color2, Json_getString_4(keyMap, "dark", 0), 0);

				for (frame = 0, bezier = 0;; ++frame) {
					float time2;
					sp4RGBA2Timeline_setFrame(timeline, frame, time, color.r, color.g, color.b, color.a, color2.g,
											 color2.g, color2.b);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = Json_getFloat_4(nextMap, "time", 0);
					toColor2(&newColor, Json_getString_4(nextMap, "light", 0), 1);
					toColor2(&newColor2, Json_getString_4(nextMap, "dark", 0), 0);
					curve = Json_getItem_4(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, color.r, newColor.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, color.g, newColor.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, color.b, newColor.b,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 3, time, time2, color.a, newColor.a,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 4, time, time2, color2.r, newColor2.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 5, time, time2, color2.g, newColor2.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 6, time, time2, color2.b, newColor2.b,
										   1);
					}
					time = time2;
					color = newColor;
					color2 = newColor2;
					keyMap = nextMap;
				}
				sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
			} else if (strcmp(timelineMap->name, "rgb2") == 0) {
				float time;
				sp4RGBA2Timeline *timeline = sp4RGBA2Timeline_create(frames, frames * 6, slotIndex);
				keyMap = timelineMap->child;
				time = Json_getFloat_4(keyMap, "time", 0);
				toColor2(&color, Json_getString_4(keyMap, "light", 0), 0);
				toColor2(&color2, Json_getString_4(keyMap, "dark", 0), 0);

				for (frame = 0, bezier = 0;; ++frame) {
					float time2;
					sp4RGBA2Timeline_setFrame(timeline, frame, time, color.r, color.g, color.b, color.a, color2.r,
											 color2.g, color2.b);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = Json_getFloat_4(nextMap, "time", 0);
					toColor2(&newColor, Json_getString_4(nextMap, "light", 0), 0);
					toColor2(&newColor2, Json_getString_4(nextMap, "dark", 0), 0);
					curve = Json_getItem_4(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, color.r, newColor.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, color.g, newColor.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, color.b, newColor.b,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 3, time, time2, color2.r, newColor2.r,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 4, time, time2, color2.g, newColor2.g,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 5, time, time2, color2.b, newColor2.b,
										   1);
					}
					time = time2;
					color = newColor;
					color2 = newColor2;
					keyMap = nextMap;
				}
				sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
			} else {
				cleanUpTimelines(timelines);
				_sp4SkeletonJson_setError(self, NULL, "Invalid timeline type for a slot: ", timelineMap->name);
				return NULL;
			}
		}
	}

	/* Bone timelines. */
	for (boneMap = bones ? bones->child : 0; boneMap; boneMap = boneMap->next) {
		int boneIndex = -1;
		for (i = 0; i < skeletonData->bonesCount; ++i) {
			if (strcmp(skeletonData->bones[i]->name, boneMap->name) == 0) {
				boneIndex = i;
				break;
			}
		}
		if (boneIndex == -1) {
			cleanUpTimelines(timelines);
			_sp4SkeletonJson_setError(self, NULL, "Bone not found: ", boneMap->name);
			return NULL;
		}

		for (timelineMap = boneMap->child; timelineMap; timelineMap = timelineMap->next) {
			int frames = timelineMap->size;
			if (frames == 0) continue;

			if (strcmp(timelineMap->name, "rotate") == 0) {
				sp4TimelineArray_add(timelines, readTimeline(timelineMap->child,
															SUPER(sp4RotateTimeline_create(frames,
																						  frames,
																						  boneIndex)),
															0, 1));
			} else if (strcmp(timelineMap->name, "translate") == 0) {
				sp4TranslateTimeline *timeline = sp4TranslateTimeline_create(frames, frames << 1,
																		   boneIndex);
				sp4TimelineArray_add(timelines, readTimeline2(timelineMap->child, SUPER(timeline), "x", "y", 0, scale));
			} else if (strcmp(timelineMap->name, "translatex") == 0) {
				sp4TranslateXTimeline *timeline = sp4TranslateXTimeline_create(frames, frames,
																			 boneIndex);
				sp4TimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 0, scale));
			} else if (strcmp(timelineMap->name, "translatey") == 0) {
				sp4TranslateYTimeline *timeline = sp4TranslateYTimeline_create(frames, frames,
																			 boneIndex);
				sp4TimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 0, scale));
			} else if (strcmp(timelineMap->name, "scale") == 0) {
				sp4ScaleTimeline *timeline = sp4ScaleTimeline_create(frames, frames << 1,
																   boneIndex);
				sp4TimelineArray_add(timelines, readTimeline2(timelineMap->child, SUPER(timeline), "x", "y", 1, 1));
			} else if (strcmp(timelineMap->name, "scalex") == 0) {
				sp4ScaleXTimeline *timeline = sp4ScaleXTimeline_create(frames, frames, boneIndex);
				sp4TimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 1, 1));
			} else if (strcmp(timelineMap->name, "scaley") == 0) {
				sp4ScaleYTimeline *timeline = sp4ScaleYTimeline_create(frames, frames, boneIndex);
				sp4TimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 1, 1));
			} else if (strcmp(timelineMap->name, "shear") == 0) {
				sp4ShearTimeline *timeline = sp4ShearTimeline_create(frames, frames << 1,
																   boneIndex);
				sp4TimelineArray_add(timelines, readTimeline2(timelineMap->child, SUPER(timeline), "x", "y", 0, 1));
			} else if (strcmp(timelineMap->name, "shearx") == 0) {
				sp4ShearXTimeline *timeline = sp4ShearXTimeline_create(frames, frames, boneIndex);
				sp4TimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 0, 1));
			} else if (strcmp(timelineMap->name, "sheary") == 0) {
				sp4ShearYTimeline *timeline = sp4ShearYTimeline_create(frames, frames, boneIndex);
				sp4TimelineArray_add(timelines, readTimeline(timelineMap->child, SUPER(timeline), 0, 1));
			} else {
				cleanUpTimelines(timelines);
				_sp4SkeletonJson_setError(self, NULL, "Invalid timeline type for a bone: ", timelineMap->name);
				return NULL;
			}
		}
	}

	/* IK constraint timelines. */
	for (constraintMap = ik ? ik->child : 0; constraintMap; constraintMap = constraintMap->next) {
		sp4IkConstraintData *constraint;
		sp4IkConstraintTimeline *timeline;
		int constraintIndex;
		float time, mix, softness;
		keyMap = constraintMap->child;
		if (keyMap == NULL) continue;

		constraint = sp4SkeletonData_findIkConstraint(skeletonData, constraintMap->name);
		constraintIndex = findIkConstraintIndex(self, skeletonData, constraint, timelines);
		if (constraintIndex == -1) return NULL;
		timeline = sp4IkConstraintTimeline_create(constraintMap->size, constraintMap->size << 1, constraintIndex);

		time = Json_getFloat_4(keyMap, "time", 0);
		mix = Json_getFloat_4(keyMap, "mix", 1);
		softness = Json_getFloat_4(keyMap, "softness", 0) * scale;

		for (frame = 0, bezier = 0;; ++frame) {
			float time2, mix2, softness2;
			int bendDirection = Json_getInt_4(keyMap, "bendPositive", 1) ? 1 : -1;
			sp4IkConstraintTimeline_setFrame(timeline, frame, time, mix, softness, bendDirection,
											Json_getInt_4(keyMap, "compress", 0) ? 1 : 0,
											Json_getInt_4(keyMap, "stretch", 0) ? 1 : 0);
			nextMap = keyMap->next;
			if (!nextMap) {
				/* timeline.shrink(); // BOZO */
				break;
			}

			time2 = Json_getFloat_4(nextMap, "time", 0);
			mix2 = Json_getFloat_4(nextMap, "mix", 1);
			softness2 = Json_getFloat_4(nextMap, "softness", 0) * scale;
			curve = Json_getItem_4(keyMap, "curve");
			if (curve) {
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, mix, mix2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, softness, softness2, scale);
			}

			time = time2;
			mix = mix2;
			softness = softness2;
			keyMap = nextMap;
		}

		sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
	}

	/* Transform constraint timelines. */
	for (constraintMap = transform ? transform->child : 0; constraintMap; constraintMap = constraintMap->next) {
		sp4TransformConstraintData *constraint;
		sp4TransformConstraintTimeline *timeline;
		int constraintIndex;
		float time, mixRotate, mixShearY, mixX, mixY, mixScaleX, mixScaleY;
		keyMap = constraintMap->child;
		if (keyMap == NULL) continue;

		constraint = sp4SkeletonData_findTransformConstraint(skeletonData, constraintMap->name);
		constraintIndex = findTransformConstraintIndex(self, skeletonData, constraint, timelines);
		if (constraintIndex == -1) return NULL;
		timeline = sp4TransformConstraintTimeline_create(constraintMap->size, constraintMap->size * 6, constraintIndex);

		time = Json_getFloat_4(keyMap, "time", 0);
		mixRotate = Json_getFloat_4(keyMap, "mixRotate", 1);
		mixShearY = Json_getFloat_4(keyMap, "mixShearY", 1);
		mixX = Json_getFloat_4(keyMap, "mixX", 1);
		mixY = Json_getFloat_4(keyMap, "mixY", mixX);
		mixScaleX = Json_getFloat_4(keyMap, "mixScaleX", 1);
		mixScaleY = Json_getFloat_4(keyMap, "mixScaleY", mixScaleX);

		for (frame = 0, bezier = 0;; ++frame) {
			float time2, mixRotate2, mixShearY2, mixX2, mixY2, mixScaleX2, mixScaleY2;
			sp4TransformConstraintTimeline_setFrame(timeline, frame, time, mixRotate, mixX, mixY, mixScaleX, mixScaleY,
												   mixShearY);
			nextMap = keyMap->next;
			if (!nextMap) {
				/* timeline.shrink(); // BOZO */
				break;
			}

			time2 = Json_getFloat_4(nextMap, "time", 0);
			mixRotate2 = Json_getFloat_4(nextMap, "mixRotate", 1);
			mixShearY2 = Json_getFloat_4(nextMap, "mixShearY", 1);
			mixX2 = Json_getFloat_4(nextMap, "mixX", 1);
			mixY2 = Json_getFloat_4(nextMap, "mixY", mixX2);
			mixScaleX2 = Json_getFloat_4(nextMap, "mixScaleX", 1);
			mixScaleY2 = Json_getFloat_4(nextMap, "mixScaleY", mixScaleX2);
			curve = Json_getItem_4(keyMap, "curve");
			if (curve) {
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, mixRotate, mixRotate2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, mixX, mixX2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, mixY, mixY2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 3, time, time2, mixScaleX, mixScaleX2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 4, time, time2, mixScaleY, mixScaleY2, 1);
				bezier = readCurve(curve, SUPER(timeline), bezier, frame, 5, time, time2, mixShearY, mixShearY2, 1);
			}

			time = time2;
			mixRotate = mixRotate2;
			mixX = mixX2;
			mixY = mixY2;
			mixScaleX = mixScaleX2;
			mixScaleY = mixScaleY2;
			mixScaleX = mixScaleX2;
			keyMap = nextMap;
		}

		sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
	}

	/** Path constraint timelines. */
	for (constraintMap = paths ? paths->child : 0; constraintMap; constraintMap = constraintMap->next) {
		sp4PathConstraintData *constraint = sp4SkeletonData_findPathConstraint(skeletonData, constraintMap->name);
		int constraintIndex = findPathConstraintIndex(self, skeletonData, constraint, timelines);
		if (constraintIndex == -1) return NULL;
		for (timelineMap = constraintMap->child; timelineMap; timelineMap = timelineMap->next) {
			const char *timelineName;
			int frames;
			keyMap = timelineMap->child;
			if (keyMap == NULL) continue;
			frames = timelineMap->size;
			timelineName = timelineMap->name;
			if (strcmp(timelineName, "position") == 0) {
				sp4PathConstraintPositionTimeline *timeline = sp4PathConstraintPositionTimeline_create(frames,
																									 frames,
																									 constraintIndex);
				sp4TimelineArray_add(timelines, readTimeline(keyMap, SUPER(timeline), 0,
															constraint->positionMode == SP4_POSITION_MODE_FIXED ? scale : 1));
			} else if (strcmp(timelineName, "spacing") == 0) {
				sp4CurveTimeline1 *timeline = SUPER(
						sp4PathConstraintSpacingTimeline_create(frames, frames, constraintIndex));
				sp4TimelineArray_add(timelines, readTimeline(keyMap, timeline, 0,
															constraint->spacingMode == SP4_SPACING_MODE_LENGTH ||
																			constraint->spacingMode == SP4_SPACING_MODE_FIXED
																	? scale
																	: 1));
			} else if (strcmp(timelineName, "mix") == 0) {
				sp4PathConstraintMixTimeline *timeline = sp4PathConstraintMixTimeline_create(frames,
																						   frames * 3,
																						   constraintIndex);
				float time = Json_getFloat_4(keyMap, "time", 0);
				float mixRotate = Json_getFloat_4(keyMap, "mixRotate", 1);
				float mixX = Json_getFloat_4(keyMap, "mixX", 1);
				float mixY = Json_getFloat_4(keyMap, "mixY", mixX);
				for (frame = 0, bezier = 0;; ++frame) {
					float time2, mixRotate2, mixX2, mixY2;
					sp4PathConstraintMixTimeline_setFrame(timeline, frame, time, mixRotate, mixX, mixY);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}

					time2 = Json_getFloat_4(nextMap, "time", 0);
					mixRotate2 = Json_getFloat_4(nextMap, "mixRotate", 1);
					mixX2 = Json_getFloat_4(nextMap, "mixX", 1);
					mixY2 = Json_getFloat_4(nextMap, "mixY", mixX2);
					curve = Json_getItem_4(keyMap, "curve");
					if (curve != NULL) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, mixRotate, mixRotate2,
										   1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 1, time, time2, mixX, mixX2, 1);
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 2, time, time2, mixY, mixY2, 1);
					}
					time = time2;
					mixRotate = mixRotate2;
					mixX = mixX2;
					mixY = mixY2;
					keyMap = nextMap;
				}
				sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
			}
		}
	}

	/* Deform timelines. */
	for (constraintMap = deformJson ? deformJson->child : 0; constraintMap; constraintMap = constraintMap->next) {
		sp4Skin *skin = sp4SkeletonData_findSkin(skeletonData, constraintMap->name);
		for (slotMap = constraintMap->child; slotMap; slotMap = slotMap->next) {
			int slotIndex = findSlotIndex(self, skeletonData, slotMap->name, timelines);
			if (slotIndex == -1) return NULL;

			for (timelineMap = slotMap->child; timelineMap; timelineMap = timelineMap->next) {
				float *tempDeform;
				sp4VertexAttachment *attachment;
				int weighted, deformLength;
				sp4DeformTimeline *timeline;
				float time;
				keyMap = timelineMap->child;
				if (keyMap == NULL) continue;

				attachment = SUB_CAST(sp4VertexAttachment, sp4Skin_getAttachment(skin, slotIndex, timelineMap->name));
				if (!attachment) {
					cleanUpTimelines(timelines);
					_sp4SkeletonJson_setError(self, 0, "Attachment not found: ", timelineMap->name);
					return NULL;
				}
				weighted = attachment->bones != 0;
				deformLength = weighted ? attachment->verticesCount / 3 * 2 : attachment->verticesCount;
				tempDeform = MALLOC(float, deformLength);
				timeline = sp4DeformTimeline_create(timelineMap->size, deformLength, timelineMap->size, slotIndex,
												   attachment);

				time = Json_getFloat_4(keyMap, "time", 0);
				for (frame = 0, bezier = 0;; ++frame) {
					Json *vertices = Json_getItem_4(keyMap, "vertices");
					float *deform;
					float time2;

					if (!vertices) {
						if (weighted) {
							deform = tempDeform;
							memset(deform, 0, sizeof(float) * deformLength);
						} else
							deform = attachment->vertices;
					} else {
						int v, start = Json_getInt_4(keyMap, "offset", 0);
						Json *vertex;
						deform = tempDeform;
						memset(deform, 0, sizeof(float) * start);
						if (self->scale == 1) {
							for (vertex = vertices->child, v = start; vertex; vertex = vertex->next, ++v)
								deform[v] = vertex->valueFloat;
						} else {
							for (vertex = vertices->child, v = start; vertex; vertex = vertex->next, ++v)
								deform[v] = vertex->valueFloat * self->scale;
						}
						memset(deform + v, 0, sizeof(float) * (deformLength - v));
						if (!weighted) {
							float *verticesValues = attachment->vertices;
							for (v = 0; v < deformLength; ++v)
								deform[v] += verticesValues[v];
						}
					}
					sp4DeformTimeline_setFrame(timeline, frame, time, deform);
					nextMap = keyMap->next;
					if (!nextMap) {
						/* timeline.shrink(); // BOZO */
						break;
					}
					time2 = Json_getFloat_4(nextMap, "time", 0);
					curve = Json_getItem_4(keyMap, "curve");
					if (curve) {
						bezier = readCurve(curve, SUPER(timeline), bezier, frame, 0, time, time2, 0, 1, 1);
					}
					time = time2;
					keyMap = nextMap;
				}
				FREE(tempDeform);

				sp4TimelineArray_add(timelines, SUPER(SUPER(timeline)));
			}
		}
	}

	/* Draw order timeline. */
	if (drawOrderJson) {
		sp4DrawOrderTimeline *timeline = sp4DrawOrderTimeline_create(drawOrderJson->size, skeletonData->slotsCount);
		for (keyMap = drawOrderJson->child, frame = 0; keyMap; keyMap = keyMap->next, ++frame) {
			int ii;
			int *drawOrder = 0;
			Json *offsets = Json_getItem_4(keyMap, "offsets");
			if (offsets) {
				Json *offsetMap;
				int *unchanged = MALLOC(int, skeletonData->slotsCount - offsets->size);
				int originalIndex = 0, unchangedIndex = 0;

				drawOrder = MALLOC(int, skeletonData->slotsCount);
				for (ii = skeletonData->slotsCount - 1; ii >= 0; --ii)
					drawOrder[ii] = -1;

				for (offsetMap = offsets->child; offsetMap; offsetMap = offsetMap->next) {
					int slotIndex = findSlotIndex(self, skeletonData, Json_getString_4(offsetMap, "slot", 0), timelines);
					if (slotIndex == -1) return NULL;

					/* Collect unchanged items. */
					while (originalIndex != slotIndex)
						unchanged[unchangedIndex++] = originalIndex++;
					/* Set changed items. */
					drawOrder[originalIndex + Json_getInt_4(offsetMap, "offset", 0)] = originalIndex;
					originalIndex++;
				}
				/* Collect remaining unchanged items. */
				while (originalIndex < skeletonData->slotsCount)
					unchanged[unchangedIndex++] = originalIndex++;
				/* Fill in unchanged items. */
				for (ii = skeletonData->slotsCount - 1; ii >= 0; ii--)
					if (drawOrder[ii] == -1) drawOrder[ii] = unchanged[--unchangedIndex];
				FREE(unchanged);
			}
			sp4DrawOrderTimeline_setFrame(timeline, frame, Json_getFloat_4(keyMap, "time", 0), drawOrder);
			FREE(drawOrder);
		}

		sp4TimelineArray_add(timelines, SUPER(timeline));
	}

	/* Event timeline. */
	if (events) {
		sp4EventTimeline *timeline = sp4EventTimeline_create(events->size);
		for (keyMap = events->child, frame = 0; keyMap; keyMap = keyMap->next, ++frame) {
			sp4Event *event;
			const char *stringValue;
			sp4EventData *eventData = sp4SkeletonData_findEvent(skeletonData, Json_getString_4(keyMap, "name", 0));
			if (!eventData) {
				cleanUpTimelines(timelines);
				_sp4SkeletonJson_setError(self, 0, "Event not found: ", Json_getString_4(keyMap, "name", 0));
				return NULL;
			}
			event = sp4Event_create(Json_getFloat_4(keyMap, "time", 0), eventData);
			event->intValue = Json_getInt_4(keyMap, "int", eventData->intValue);
			event->floatValue = Json_getFloat_4(keyMap, "float", eventData->floatValue);
			stringValue = Json_getString_4(keyMap, "string", eventData->stringValue);
			if (stringValue) MALLOC_STR(event->stringValue, stringValue);
			if (eventData->audioPath) {
				event->volume = Json_getFloat_4(keyMap, "volume", 1);
				event->balance = Json_getFloat_4(keyMap, "volume", 0);
			}
			sp4EventTimeline_setFrame(timeline, frame, event);
		}
		sp4TimelineArray_add(timelines, SUPER(timeline));
	}

	duration = 0;
	for (i = 0, n = timelines->size; i < n; ++i)
		duration = MAX(duration, sp4Timeline_getDuration(timelines->items[i]));
	return sp4Animation_create(root->name, timelines, duration);
}

static void
_readVertices(sp4SkeletonJson *self, Json *attachmentMap, sp4VertexAttachment *attachment, int verticesLength) {
	Json *entry;
	float *vertices;
	int i, n, nn, entrySize;
	sp4FloatArray *weights;
	sp4IntArray *bones;

	attachment->worldVerticesLength = verticesLength;

	entry = Json_getItem_4(attachmentMap, "vertices");
	entrySize = entry->size;
	vertices = MALLOC(float, entrySize);
	for (entry = entry->child, i = 0; entry; entry = entry->next, ++i)
		vertices[i] = entry->valueFloat;

	if (verticesLength == entrySize) {
		if (self->scale != 1)
			for (i = 0; i < entrySize; ++i)
				vertices[i] *= self->scale;
		attachment->verticesCount = verticesLength;
		attachment->vertices = vertices;

		attachment->bonesCount = 0;
		attachment->bones = 0;
		return;
	}

	weights = sp4FloatArray_create(verticesLength * 3 * 3);
	bones = sp4IntArray_create(verticesLength * 3);

	for (i = 0, n = entrySize; i < n;) {
		int boneCount = (int) vertices[i++];
		sp4IntArray_add(bones, boneCount);
		for (nn = i + boneCount * 4; i < nn; i += 4) {
			sp4IntArray_add(bones, (int) vertices[i]);
			sp4FloatArray_add(weights, vertices[i + 1] * self->scale);
			sp4FloatArray_add(weights, vertices[i + 2] * self->scale);
			sp4FloatArray_add(weights, vertices[i + 3]);
		}
	}

	attachment->verticesCount = weights->size;
	attachment->vertices = weights->items;
	FREE(weights);
	attachment->bonesCount = bones->size;
	attachment->bones = bones->items;
	FREE(bones);

	FREE(vertices);
}

sp4SkeletonData *sp4SkeletonJson_readSkeletonDataFile(sp4SkeletonJson *self, const char *path) {
	int length;
	sp4SkeletonData *skeletonData;
	const char *json = _sp4Util_readFile(path, &length);
	if (length == 0 || !json) {
		_sp4SkeletonJson_setError(self, 0, "Unable to read skeleton file: ", path);
		return NULL;
	}
	skeletonData = sp4SkeletonJson_readSkeletonData(self, json);
	FREE(json);
	return skeletonData;
}

sp4SkeletonData *sp4SkeletonJson_readSkeletonData(sp4SkeletonJson *self, const char *json) {
	int i, ii;
	sp4SkeletonData *skeletonData;
	Json *root, *skeleton, *bones, *boneMap, *ik, *transform, *pathJson, *slots, *skins, *animations, *events;
	_sp4SkeletonJson *internal = SUB_CAST(_sp4SkeletonJson, self);

	FREE(self->error);
	CONST_CAST(char *, self->error) = 0;
	internal->linkedMeshCount = 0;

	root = Json_create_4(json);
	if (!root) {
		_sp4SkeletonJson_setError(self, 0, "Invalid skeleton JSON: ", Json_getError_4());
		return NULL;
	}

	skeletonData = sp4SkeletonData_create();

	skeleton = Json_getItem_4(root, "skeleton");
	if (skeleton) {
		MALLOC_STR(skeletonData->hash, Json_getString_4(skeleton, "hash", 0));
		MALLOC_STR(skeletonData->version, Json_getString_4(skeleton, "spine", 0));
		skeletonData->x = Json_getFloat_4(skeleton, "x", 0);
		skeletonData->y = Json_getFloat_4(skeleton, "y", 0);
		skeletonData->width = Json_getFloat_4(skeleton, "width", 0);
		skeletonData->height = Json_getFloat_4(skeleton, "height", 0);
		skeletonData->fps = Json_getFloat_4(skeleton, "fps", 30);
		skeletonData->imagesPath = Json_getString_4(skeleton, "images", 0);
		if (skeletonData->imagesPath) {
			char *tmp = NULL;
			MALLOC_STR(tmp, skeletonData->imagesPath);
			skeletonData->imagesPath = tmp;
		}
		skeletonData->audioPath = Json_getString_4(skeleton, "audio", 0);
		if (skeletonData->audioPath) {
			char *tmp = NULL;
			MALLOC_STR(tmp, skeletonData->audioPath);
			skeletonData->audioPath = tmp;
		}
	}

	/* Bones. */
	bones = Json_getItem_4(root, "bones");
	skeletonData->bones = MALLOC(sp4BoneData *, bones->size);
	for (boneMap = bones->child, i = 0; boneMap; boneMap = boneMap->next, ++i) {
		sp4BoneData *data;
		const char *transformMode;
		const char *color;

		sp4BoneData *parent = 0;
		const char *parentName = Json_getString_4(boneMap, "parent", 0);
		if (parentName) {
			parent = sp4SkeletonData_findBone(skeletonData, parentName);
			if (!parent) {
				sp4SkeletonData_dispose(skeletonData);
				_sp4SkeletonJson_setError(self, root, "Parent bone not found: ", parentName);
				return NULL;
			}
		}

		data = sp4BoneData_create(skeletonData->bonesCount, Json_getString_4(boneMap, "name", 0), parent);
		data->length = Json_getFloat_4(boneMap, "length", 0) * self->scale;
		data->x = Json_getFloat_4(boneMap, "x", 0) * self->scale;
		data->y = Json_getFloat_4(boneMap, "y", 0) * self->scale;
		data->rotation = Json_getFloat_4(boneMap, "rotation", 0);
		data->scaleX = Json_getFloat_4(boneMap, "scaleX", 1);
		data->scaleY = Json_getFloat_4(boneMap, "scaleY", 1);
		data->shearX = Json_getFloat_4(boneMap, "shearX", 0);
		data->shearY = Json_getFloat_4(boneMap, "shearY", 0);
		transformMode = Json_getString_4(boneMap, "transform", "normal");
		data->transformMode = SP4_TRANSFORMMODE_NORMAL;
		if (strcmp(transformMode, "normal") == 0) data->transformMode = SP4_TRANSFORMMODE_NORMAL;
		else if (strcmp(transformMode, "onlyTranslation") == 0)
			data->transformMode = SP4_TRANSFORMMODE_ONLYTRANSLATION;
		else if (strcmp(transformMode, "noRotationOrReflection") == 0)
			data->transformMode = SP4_TRANSFORMMODE_NOROTATIONORREFLECTION;
		else if (strcmp(transformMode, "noScale") == 0)
			data->transformMode = SP4_TRANSFORMMODE_NOSCALE;
		else if (strcmp(transformMode, "noScaleOrReflection") == 0)
			data->transformMode = SP4_TRANSFORMMODE_NOSCALEORREFLECTION;
		data->skinRequired = Json_getInt_4(boneMap, "skin", 0) ? 1 : 0;

		color = Json_getString_4(boneMap, "color", 0);
		if (color) toColor2(&data->color, color, -1);

		skeletonData->bones[i] = data;
		skeletonData->bonesCount++;
	}

	/* Slots. */
	slots = Json_getItem_4(root, "slots");
	if (slots) {
		Json *slotMap;
		skeletonData->slotsCount = slots->size;
		skeletonData->slots = MALLOC(sp4SlotData *, slots->size);
		for (slotMap = slots->child, i = 0; slotMap; slotMap = slotMap->next, ++i) {
			sp4SlotData *data;
			const char *color;
			const char *dark;
			Json *item;

			const char *boneName = Json_getString_4(slotMap, "bone", 0);
			sp4BoneData *boneData = sp4SkeletonData_findBone(skeletonData, boneName);
			if (!boneData) {
				sp4SkeletonData_dispose(skeletonData);
				_sp4SkeletonJson_setError(self, root, "Slot bone not found: ", boneName);
				return NULL;
			}

			data = sp4SlotData_create(i, Json_getString_4(slotMap, "name", 0), boneData);

			color = Json_getString_4(slotMap, "color", 0);
			if (color) {
				sp4Color_setFromFloats(&data->color,
									  toColor(color, 0),
									  toColor(color, 1),
									  toColor(color, 2),
									  toColor(color, 3));
			}

			dark = Json_getString_4(slotMap, "dark", 0);
			if (dark) {
				data->darkColor = sp4Color_create();
				sp4Color_setFromFloats(data->darkColor,
									  toColor(dark, 0),
									  toColor(dark, 1),
									  toColor(dark, 2),
									  toColor(dark, 3));
			}

			item = Json_getItem_4(slotMap, "attachment");
			if (item) sp4SlotData_setAttachmentName(data, item->valueString);

			item = Json_getItem_4(slotMap, "blend");
			if (item) {
				if (strcmp(item->valueString, "additive") == 0)
					data->blendMode = SP4_BLEND_MODE_ADDITIVE;
				else if (strcmp(item->valueString, "multiply") == 0)
					data->blendMode = SP4_BLEND_MODE_MULTIPLY;
				else if (strcmp(item->valueString, "screen") == 0)
					data->blendMode = SP4_BLEND_MODE_SCREEN;
			}

			skeletonData->slots[i] = data;
		}
	}

	/* IK constraints. */
	ik = Json_getItem_4(root, "ik");
	if (ik) {
		Json *constraintMap;
		skeletonData->ikConstraintsCount = ik->size;
		skeletonData->ikConstraints = MALLOC(sp4IkConstraintData *, ik->size);
		for (constraintMap = ik->child, i = 0; constraintMap; constraintMap = constraintMap->next, ++i) {
			const char *targetName;

			sp4IkConstraintData *data = sp4IkConstraintData_create(Json_getString_4(constraintMap, "name", 0));
			data->order = Json_getInt_4(constraintMap, "order", 0);
			data->skinRequired = Json_getInt_4(constraintMap, "skin", 0) ? 1 : 0;

			boneMap = Json_getItem_4(constraintMap, "bones");
			data->bonesCount = boneMap->size;
			data->bones = MALLOC(sp4BoneData *, boneMap->size);
			for (boneMap = boneMap->child, ii = 0; boneMap; boneMap = boneMap->next, ++ii) {
				data->bones[ii] = sp4SkeletonData_findBone(skeletonData, boneMap->valueString);
				if (!data->bones[ii]) {
					sp4SkeletonData_dispose(skeletonData);
					_sp4SkeletonJson_setError(self, root, "IK bone not found: ", boneMap->valueString);
					return NULL;
				}
			}

			targetName = Json_getString_4(constraintMap, "target", 0);
			data->target = sp4SkeletonData_findBone(skeletonData, targetName);
			if (!data->target) {
				sp4SkeletonData_dispose(skeletonData);
				_sp4SkeletonJson_setError(self, root, "Target bone not found: ", targetName);
				return NULL;
			}

			data->bendDirection = Json_getInt_4(constraintMap, "bendPositive", 1) ? 1 : -1;
			data->compress = Json_getInt_4(constraintMap, "compress", 0) ? 1 : 0;
			data->stretch = Json_getInt_4(constraintMap, "stretch", 0) ? 1 : 0;
			data->uniform = Json_getInt_4(constraintMap, "uniform", 0) ? 1 : 0;
			data->mix = Json_getFloat_4(constraintMap, "mix", 1);
			data->softness = Json_getFloat_4(constraintMap, "softness", 0) * self->scale;

			skeletonData->ikConstraints[i] = data;
		}
	}

	/* Transform constraints. */
	transform = Json_getItem_4(root, "transform");
	if (transform) {
		Json *constraintMap;
		skeletonData->transformConstraintsCount = transform->size;
		skeletonData->transformConstraints = MALLOC(sp4TransformConstraintData *, transform->size);
		for (constraintMap = transform->child, i = 0; constraintMap; constraintMap = constraintMap->next, ++i) {
			const char *name;

			sp4TransformConstraintData *data = sp4TransformConstraintData_create(
					Json_getString_4(constraintMap, "name", 0));
			data->order = Json_getInt_4(constraintMap, "order", 0);
			data->skinRequired = Json_getInt_4(constraintMap, "skin", 0) ? 1 : 0;

			boneMap = Json_getItem_4(constraintMap, "bones");
			data->bonesCount = boneMap->size;
			CONST_CAST(sp4BoneData **, data->bones) = MALLOC(sp4BoneData *, boneMap->size);
			for (boneMap = boneMap->child, ii = 0; boneMap; boneMap = boneMap->next, ++ii) {
				data->bones[ii] = sp4SkeletonData_findBone(skeletonData, boneMap->valueString);
				if (!data->bones[ii]) {
					sp4SkeletonData_dispose(skeletonData);
					_sp4SkeletonJson_setError(self, root, "Transform bone not found: ", boneMap->valueString);
					return NULL;
				}
			}

			name = Json_getString_4(constraintMap, "target", 0);
			data->target = sp4SkeletonData_findBone(skeletonData, name);
			if (!data->target) {
				sp4SkeletonData_dispose(skeletonData);
				_sp4SkeletonJson_setError(self, root, "Target bone not found: ", name);
				return NULL;
			}

			data->local = Json_getInt_4(constraintMap, "local", 0);
			data->relative = Json_getInt_4(constraintMap, "relative", 0);
			data->offsetRotation = Json_getFloat_4(constraintMap, "rotation", 0);
			data->offsetX = Json_getFloat_4(constraintMap, "x", 0) * self->scale;
			data->offsetY = Json_getFloat_4(constraintMap, "y", 0) * self->scale;
			data->offsetScaleX = Json_getFloat_4(constraintMap, "scaleX", 0);
			data->offsetScaleY = Json_getFloat_4(constraintMap, "scaleY", 0);
			data->offsetShearY = Json_getFloat_4(constraintMap, "shearY", 0);

			data->mixRotate = Json_getFloat_4(constraintMap, "mixRotate", 1);
			data->mixX = Json_getFloat_4(constraintMap, "mixX", 1);
			data->mixY = Json_getFloat_4(constraintMap, "mixY", data->mixX);
			data->mixScaleX = Json_getFloat_4(constraintMap, "mixScaleX", 1);
			data->mixScaleY = Json_getFloat_4(constraintMap, "mixScaleY", data->mixScaleX);
			data->mixShearY = Json_getFloat_4(constraintMap, "mixShearY", 1);

			skeletonData->transformConstraints[i] = data;
		}
	}

	/* Path constraints */
	pathJson = Json_getItem_4(root, "path");
	if (pathJson) {
		Json *constraintMap;
		skeletonData->pathConstraintsCount = pathJson->size;
		skeletonData->pathConstraints = MALLOC(sp4PathConstraintData *, pathJson->size);
		for (constraintMap = pathJson->child, i = 0; constraintMap; constraintMap = constraintMap->next, ++i) {
			const char *name;
			const char *item;

			sp4PathConstraintData *data = sp4PathConstraintData_create(Json_getString_4(constraintMap, "name", 0));
			data->order = Json_getInt_4(constraintMap, "order", 0);
			data->skinRequired = Json_getInt_4(constraintMap, "skin", 0) ? 1 : 0;

			boneMap = Json_getItem_4(constraintMap, "bones");
			data->bonesCount = boneMap->size;
			CONST_CAST(sp4BoneData **, data->bones) = MALLOC(sp4BoneData *, boneMap->size);
			for (boneMap = boneMap->child, ii = 0; boneMap; boneMap = boneMap->next, ++ii) {
				data->bones[ii] = sp4SkeletonData_findBone(skeletonData, boneMap->valueString);
				if (!data->bones[ii]) {
					sp4SkeletonData_dispose(skeletonData);
					_sp4SkeletonJson_setError(self, root, "Path bone not found: ", boneMap->valueString);
					return NULL;
				}
			}

			name = Json_getString_4(constraintMap, "target", 0);
			data->target = sp4SkeletonData_findSlot(skeletonData, name);
			if (!data->target) {
				sp4SkeletonData_dispose(skeletonData);
				_sp4SkeletonJson_setError(self, root, "Target slot not found: ", name);
				return NULL;
			}

			item = Json_getString_4(constraintMap, "positionMode", "percent");
			if (strcmp(item, "fixed") == 0) data->positionMode = SP4_POSITION_MODE_FIXED;
			else if (strcmp(item, "percent") == 0)
				data->positionMode = SP4_POSITION_MODE_PERCENT;

			item = Json_getString_4(constraintMap, "spacingMode", "length");
			if (strcmp(item, "length") == 0) data->spacingMode = SP4_SPACING_MODE_LENGTH;
			else if (strcmp(item, "fixed") == 0)
				data->spacingMode = SP4_SPACING_MODE_FIXED;
			else if (strcmp(item, "percent") == 0)
				data->spacingMode = SP4_SPACING_MODE_PERCENT;

			item = Json_getString_4(constraintMap, "rotateMode", "tangent");
			if (strcmp(item, "tangent") == 0) data->rotateMode = SP4_ROTATE_MODE_TANGENT;
			else if (strcmp(item, "chain") == 0)
				data->rotateMode = SP4_ROTATE_MODE_CHAIN;
			else if (strcmp(item, "chainScale") == 0)
				data->rotateMode = SP4_ROTATE_MODE_CHAIN_SCALE;

			data->offsetRotation = Json_getFloat_4(constraintMap, "rotation", 0);
			data->position = Json_getFloat_4(constraintMap, "position", 0);
			if (data->positionMode == SP4_POSITION_MODE_FIXED) data->position *= self->scale;
			data->spacing = Json_getFloat_4(constraintMap, "spacing", 0);
			if (data->spacingMode == SP4_SPACING_MODE_LENGTH || data->spacingMode == SP4_SPACING_MODE_FIXED)
				data->spacing *= self->scale;
			data->mixRotate = Json_getFloat_4(constraintMap, "mixRotate", 1);
			data->mixX = Json_getFloat_4(constraintMap, "mixX", 1);
			data->mixY = Json_getFloat_4(constraintMap, "mixY", data->mixX);

			skeletonData->pathConstraints[i] = data;
		}
	}

	/* Skins. */
	skins = Json_getItem_4(root, "skins");
	if (skins) {
		Json *skinMap;
		skeletonData->skins = MALLOC(sp4Skin *, skins->size);
		for (skinMap = skins->child, i = 0; skinMap; skinMap = skinMap->next, ++i) {
			Json *attachmentsMap;
			Json *curves;
			Json *skinPart;
			sp4Skin *skin = sp4Skin_create(Json_getString_4(skinMap, "name", ""));

			skinPart = Json_getItem_4(skinMap, "bones");
			if (skinPart) {
				for (skinPart = skinPart->child; skinPart; skinPart = skinPart->next) {
					sp4BoneData *bone = sp4SkeletonData_findBone(skeletonData, skinPart->valueString);
					if (!bone) {
						sp4SkeletonData_dispose(skeletonData);
						_sp4SkeletonJson_setError(self, root, "Skin bone constraint not found: ", skinPart->valueString);
						return NULL;
					}
					sp4BoneDataArray_add(skin->bones, bone);
				}
			}

			skinPart = Json_getItem_4(skinMap, "ik");
			if (skinPart) {
				for (skinPart = skinPart->child; skinPart; skinPart = skinPart->next) {
					sp4IkConstraintData *constraint = sp4SkeletonData_findIkConstraint(skeletonData,
																					 skinPart->valueString);
					if (!constraint) {
						sp4SkeletonData_dispose(skeletonData);
						_sp4SkeletonJson_setError(self, root, "Skin IK constraint not found: ", skinPart->valueString);
						return NULL;
					}
					sp4IkConstraintDataArray_add(skin->ikConstraints, constraint);
				}
			}

			skinPart = Json_getItem_4(skinMap, "path");
			if (skinPart) {
				for (skinPart = skinPart->child; skinPart; skinPart = skinPart->next) {
					sp4PathConstraintData *constraint = sp4SkeletonData_findPathConstraint(skeletonData,
																						 skinPart->valueString);
					if (!constraint) {
						sp4SkeletonData_dispose(skeletonData);
						_sp4SkeletonJson_setError(self, root, "Skin path constraint not found: ", skinPart->valueString);
						return NULL;
					}
					sp4PathConstraintDataArray_add(skin->pathConstraints, constraint);
				}
			}

			skinPart = Json_getItem_4(skinMap, "transform");
			if (skinPart) {
				for (skinPart = skinPart->child; skinPart; skinPart = skinPart->next) {
					sp4TransformConstraintData *constraint = sp4SkeletonData_findTransformConstraint(skeletonData,
																								   skinPart->valueString);
					if (!constraint) {
						sp4SkeletonData_dispose(skeletonData);
						_sp4SkeletonJson_setError(self, root, "Skin transform constraint not found: ",
												 skinPart->valueString);
						return NULL;
					}
					sp4TransformConstraintDataArray_add(skin->transformConstraints, constraint);
				}
			}

			skeletonData->skins[skeletonData->skinsCount++] = skin;
			if (strcmp(skin->name, "default") == 0) skeletonData->defaultSkin = skin;

			for (attachmentsMap = Json_getItem_4(skinMap,
											   "attachments")
										  ->child;
				 attachmentsMap; attachmentsMap = attachmentsMap->next) {
				sp4SlotData *slot = sp4SkeletonData_findSlot(skeletonData, attachmentsMap->name);
				Json *attachmentMap;

				for (attachmentMap = attachmentsMap->child; attachmentMap; attachmentMap = attachmentMap->next) {
					sp4Attachment *attachment;
					const char *skinAttachmentName = attachmentMap->name;
					const char *attachmentName = Json_getString_4(attachmentMap, "name", skinAttachmentName);
					const char *path = Json_getString_4(attachmentMap, "path", attachmentName);
					const char *color;
					Json *entry;

					const char *typeString = Json_getString_4(attachmentMap, "type", "region");
					sp4AttachmentType type;
					if (strcmp(typeString, "region") == 0) type = SP4_ATTACHMENT_REGION;
					else if (strcmp(typeString, "mesh") == 0)
						type = SP4_ATTACHMENT_MESH;
					else if (strcmp(typeString, "linkedmesh") == 0)
						type = SP4_ATTACHMENT_LINKED_MESH;
					else if (strcmp(typeString, "boundingbox") == 0)
						type = SP4_ATTACHMENT_BOUNDING_BOX;
					else if (strcmp(typeString, "path") == 0)
						type = SP4_ATTACHMENT_PATH;
					else if (strcmp(typeString, "clipping") == 0)
						type = SP4_ATTACHMENT_CLIPPING;
					else if (strcmp(typeString, "point") == 0)
						type = SP4_ATTACHMENT_POINT;
					else {
						sp4SkeletonData_dispose(skeletonData);
						_sp4SkeletonJson_setError(self, root, "Unknown attachment type: ", typeString);
						return NULL;
					}

					attachment = sp4AttachmentLoader_createAttachment(self->attachmentLoader, skin, type, attachmentName,
																	 path);
					if (!attachment) {
						if (self->attachmentLoader->error1) {
							sp4SkeletonData_dispose(skeletonData);
							_sp4SkeletonJson_setError(self, root, self->attachmentLoader->error1,
													 self->attachmentLoader->error2);
							return NULL;
						}
						continue;
					}

					switch (attachment->type) {
						case SP4_ATTACHMENT_REGION: {
							sp4RegionAttachment *region = SUB_CAST(sp4RegionAttachment, attachment);
							if (path) MALLOC_STR(region->path, path);
							region->x = Json_getFloat_4(attachmentMap, "x", 0) * self->scale;
							region->y = Json_getFloat_4(attachmentMap, "y", 0) * self->scale;
							region->scaleX = Json_getFloat_4(attachmentMap, "scaleX", 1);
							region->scaleY = Json_getFloat_4(attachmentMap, "scaleY", 1);
							region->rotation = Json_getFloat_4(attachmentMap, "rotation", 0);
							region->width = Json_getFloat_4(attachmentMap, "width", 32) * self->scale;
							region->height = Json_getFloat_4(attachmentMap, "height", 32) * self->scale;

							color = Json_getString_4(attachmentMap, "color", 0);
							if (color) {
								sp4Color_setFromFloats(&region->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}

							sp4RegionAttachment_updateOffset(region);

							sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
							break;
						}
						case SP4_ATTACHMENT_MESH:
						case SP4_ATTACHMENT_LINKED_MESH: {
							sp4MeshAttachment *mesh = SUB_CAST(sp4MeshAttachment, attachment);

							MALLOC_STR(mesh->path, path);

							color = Json_getString_4(attachmentMap, "color", 0);
							if (color) {
								sp4Color_setFromFloats(&mesh->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}

							mesh->width = Json_getFloat_4(attachmentMap, "width", 32) * self->scale;
							mesh->height = Json_getFloat_4(attachmentMap, "height", 32) * self->scale;

							entry = Json_getItem_4(attachmentMap, "parent");
							if (!entry) {
								int verticesLength;
								entry = Json_getItem_4(attachmentMap, "triangles");
								mesh->trianglesCount = entry->size;
								mesh->triangles = MALLOC(unsigned short, entry->size);
								for (entry = entry->child, ii = 0; entry; entry = entry->next, ++ii)
									mesh->triangles[ii] = (unsigned short) entry->valueInt;

								entry = Json_getItem_4(attachmentMap, "uvs");
								verticesLength = entry->size;
								mesh->regionUVs = MALLOC(float, verticesLength);
								for (entry = entry->child, ii = 0; entry; entry = entry->next, ++ii)
									mesh->regionUVs[ii] = entry->valueFloat;

								_readVertices(self, attachmentMap, SUPER(mesh), verticesLength);

								sp4MeshAttachment_updateUVs(mesh);

								mesh->hullLength = Json_getInt_4(attachmentMap, "hull", 0);

								entry = Json_getItem_4(attachmentMap, "edges");
								if (entry) {
									mesh->edgesCount = entry->size;
									mesh->edges = MALLOC(int, entry->size);
									for (entry = entry->child, ii = 0; entry; entry = entry->next, ++ii)
										mesh->edges[ii] = entry->valueInt;
								}

								sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
							} else {
								int inheritDeform = Json_getInt_4(attachmentMap, "deform", 1);
								_sp4SkeletonJson_addLinkedMesh(self, SUB_CAST(sp4MeshAttachment, attachment),
															  Json_getString_4(attachmentMap, "skin", 0), slot->index,
															  entry->valueString, inheritDeform);
							}
							break;
						}
						case SP4_ATTACHMENT_BOUNDING_BOX: {
							sp4BoundingBoxAttachment *box = SUB_CAST(sp4BoundingBoxAttachment, attachment);
							int vertexCount = Json_getInt_4(attachmentMap, "vertexCount", 0) << 1;
							_readVertices(self, attachmentMap, SUPER(box), vertexCount);
							box->super.verticesCount = vertexCount;
							color = Json_getString_4(attachmentMap, "color", 0);
							if (color) {
								sp4Color_setFromFloats(&box->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}
							sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
							break;
						}
						case SP4_ATTACHMENT_PATH: {
							sp4PathAttachment *pathAttachment = SUB_CAST(sp4PathAttachment, attachment);
							int vertexCount = 0;
							pathAttachment->closed = Json_getInt_4(attachmentMap, "closed", 0);
							pathAttachment->constantSpeed = Json_getInt_4(attachmentMap, "constantSpeed", 1);
							vertexCount = Json_getInt_4(attachmentMap, "vertexCount", 0);
							_readVertices(self, attachmentMap, SUPER(pathAttachment), vertexCount << 1);

							pathAttachment->lengthsLength = vertexCount / 3;
							pathAttachment->lengths = MALLOC(float, pathAttachment->lengthsLength);

							curves = Json_getItem_4(attachmentMap, "lengths");
							for (curves = curves->child, ii = 0; curves; curves = curves->next, ++ii)
								pathAttachment->lengths[ii] = curves->valueFloat * self->scale;
							color = Json_getString_4(attachmentMap, "color", 0);
							if (color) {
								sp4Color_setFromFloats(&pathAttachment->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}
							break;
						}
						case SP4_ATTACHMENT_POINT: {
							sp4PointAttachment *point = SUB_CAST(sp4PointAttachment, attachment);
							point->x = Json_getFloat_4(attachmentMap, "x", 0) * self->scale;
							point->y = Json_getFloat_4(attachmentMap, "y", 0) * self->scale;
							point->rotation = Json_getFloat_4(attachmentMap, "rotation", 0);

							color = Json_getString_4(attachmentMap, "color", 0);
							if (color) {
								sp4Color_setFromFloats(&point->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}
							break;
						}
						case SP4_ATTACHMENT_CLIPPING: {
							sp4ClippingAttachment *clip = SUB_CAST(sp4ClippingAttachment, attachment);
							int vertexCount = 0;
							const char *end = Json_getString_4(attachmentMap, "end", 0);
							if (end) {
								sp4SlotData *endSlot = sp4SkeletonData_findSlot(skeletonData, end);
								clip->endSlot = endSlot;
							}
							vertexCount = Json_getInt_4(attachmentMap, "vertexCount", 0) << 1;
							_readVertices(self, attachmentMap, SUPER(clip), vertexCount);
							color = Json_getString_4(attachmentMap, "color", 0);
							if (color) {
								sp4Color_setFromFloats(&clip->color,
													  toColor(color, 0),
													  toColor(color, 1),
													  toColor(color, 2),
													  toColor(color, 3));
							}
							sp4AttachmentLoader_configureAttachment(self->attachmentLoader, attachment);
							break;
						}
					}

					sp4Skin_setAttachment(skin, slot->index, skinAttachmentName, attachment);
				}
			}
		}
	}

	/* Linked meshes. */
	for (i = 0; i < internal->linkedMeshCount; ++i) {
		sp4Attachment *parent;
		_sp4LinkedMesh *linkedMesh = internal->linkedMeshes + i;
		sp4Skin *skin = !linkedMesh->skin ? skeletonData->defaultSkin : sp4SkeletonData_findSkin(skeletonData, linkedMesh->skin);
		if (!skin) {
			sp4SkeletonData_dispose(skeletonData);
			_sp4SkeletonJson_setError(self, 0, "Skin not found: ", linkedMesh->skin);
			return NULL;
		}
		parent = sp4Skin_getAttachment(skin, linkedMesh->slotIndex, linkedMesh->parent);
		if (!parent) {
			sp4SkeletonData_dispose(skeletonData);
			_sp4SkeletonJson_setError(self, 0, "Parent mesh not found: ", linkedMesh->parent);
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
	events = Json_getItem_4(root, "events");
	if (events) {
		Json *eventMap;
		const char *stringValue;
		const char *audioPath;
		skeletonData->eventsCount = events->size;
		skeletonData->events = MALLOC(sp4EventData *, events->size);
		for (eventMap = events->child, i = 0; eventMap; eventMap = eventMap->next, ++i) {
			sp4EventData *eventData = sp4EventData_create(eventMap->name);
			eventData->intValue = Json_getInt_4(eventMap, "int", 0);
			eventData->floatValue = Json_getFloat_4(eventMap, "float", 0);
			stringValue = Json_getString_4(eventMap, "string", 0);
			if (stringValue) MALLOC_STR(eventData->stringValue, stringValue);
			audioPath = Json_getString_4(eventMap, "audio", 0);
			if (audioPath) {
				MALLOC_STR(eventData->audioPath, audioPath);
				eventData->volume = Json_getFloat_4(eventMap, "volume", 1);
				eventData->balance = Json_getFloat_4(eventMap, "balance", 0);
			}
			skeletonData->events[i] = eventData;
		}
	}

	/* Animations. */
	animations = Json_getItem_4(root, "animations");
	if (animations) {
		Json *animationMap;
		skeletonData->animations = MALLOC(sp4Animation *, animations->size);
		for (animationMap = animations->child; animationMap; animationMap = animationMap->next) {
			sp4Animation *animation = _sp4SkeletonJson_readAnimation(self, animationMap, skeletonData);
			if (!animation) {
				sp4SkeletonData_dispose(skeletonData);
				return NULL;
			}
			skeletonData->animations[skeletonData->animationsCount++] = animation;
		}
	}

	Json_dispose_4(root);
	return skeletonData;
}
