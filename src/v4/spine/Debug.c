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
#include <v4/spine/Debug.h>

#include <stdio.h>

static const char *_sp4TimelineTypeNames[] = {
		"Attachment",
		"Alpha",
		"PathConstraintPosition",
		"PathConstraintSpace",
		"Rotate",
		"ScaleX",
		"ScaleY",
		"ShearX",
		"ShearY",
		"TranslateX",
		"TranslateY",
		"Scale",
		"Shear",
		"Translate",
		"Deform",
		"IkConstraint",
		"PathConstraintMix",
		"Rgb2",
		"Rgba2",
		"Rgba",
		"Rgb",
		"TransformConstraint",
		"DrawOrder",
		"Event"};

void sp4Debug_printSkeletonData(sp4SkeletonData *skeletonData) {
	int i, n;
	sp4Debug_printBoneDatas(skeletonData->bones, skeletonData->bonesCount);

	for (i = 0, n = skeletonData->animationsCount; i < n; i++) {
		sp4Debug_printAnimation(skeletonData->animations[i]);
	}
}

void _sp4Debug_printTimelineBase(sp4Timeline *timeline) {
	printf("   Timeline %s:\n", _sp4TimelineTypeNames[timeline->type]);
	printf("      frame count: %i\n", timeline->frameCount);
	printf("      frame entries: %i\n", timeline->frameEntries);
	printf("      frames: ");
	sp4Debug_printFloats(timeline->frames->items, timeline->frames->size);
	printf("\n");
}

void _sp4Debug_printCurveTimeline(sp4CurveTimeline *timeline) {
	_sp4Debug_printTimelineBase(&timeline->super);
	printf("      curves: ");
	sp4Debug_printFloats(timeline->curves->items, timeline->curves->size);
	printf("\n");
}

void sp4Debug_printTimeline(sp4Timeline *timeline) {
	switch (timeline->type) {
		case SP4_TIMELINE_ATTACHMENT: {
			sp4AttachmentTimeline *t = (sp4AttachmentTimeline *) timeline;
			_sp4Debug_printTimelineBase(&t->super);
			break;
		}
		case SP4_TIMELINE_ALPHA: {
			sp4AlphaTimeline *t = (sp4AlphaTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_PATHCONSTRAINTPOSITION: {
			sp4PathConstraintPositionTimeline *t = (sp4PathConstraintPositionTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_PATHCONSTRAINTSPACING: {
			sp4PathConstraintMixTimeline *t = (sp4PathConstraintMixTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_ROTATE: {
			sp4RotateTimeline *t = (sp4RotateTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_SCALEX: {
			sp4ScaleXTimeline *t = (sp4ScaleXTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_SCALEY: {
			sp4ScaleYTimeline *t = (sp4ScaleYTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_SHEARX: {
			sp4ShearXTimeline *t = (sp4ShearXTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_SHEARY: {
			sp4ShearYTimeline *t = (sp4ShearYTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_TRANSLATEX: {
			sp4TranslateXTimeline *t = (sp4TranslateXTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_TRANSLATEY: {
			sp4TranslateYTimeline *t = (sp4TranslateYTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_SCALE: {
			sp4ScaleTimeline *t = (sp4ScaleTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_SHEAR: {
			sp4ShearTimeline *t = (sp4ShearTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_TRANSLATE: {
			sp4TranslateTimeline *t = (sp4TranslateTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_DEFORM: {
			sp4DeformTimeline *t = (sp4DeformTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_IKCONSTRAINT: {
			sp4IkConstraintTimeline *t = (sp4IkConstraintTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_PATHCONSTRAINTMIX: {
			sp4PathConstraintMixTimeline *t = (sp4PathConstraintMixTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_RGB2: {
			sp4RGB2Timeline *t = (sp4RGB2Timeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_RGBA2: {
			sp4RGBA2Timeline *t = (sp4RGBA2Timeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_RGBA: {
			sp4RGBATimeline *t = (sp4RGBATimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_RGB: {
			sp4RGBTimeline *t = (sp4RGBTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_TRANSFORMCONSTRAINT: {
			sp4TransformConstraintTimeline *t = (sp4TransformConstraintTimeline *) timeline;
			_sp4Debug_printCurveTimeline(&t->super);
			break;
		}
		case SP4_TIMELINE_DRAWORDER: {
			sp4DrawOrderTimeline *t = (sp4DrawOrderTimeline *) timeline;
			_sp4Debug_printTimelineBase(&t->super);
			break;
		}
		case SP4_TIMELINE_EVENT: {
			sp4EventTimeline *t = (sp4EventTimeline *) timeline;
			_sp4Debug_printTimelineBase(&t->super);
			break;
		}
	}
}

void sp4Debug_printAnimation(sp4Animation *animation) {
	int i, n;
	printf("Animation %s: %i timelines\n", animation->name, animation->timelines->size);

	for (i = 0, n = animation->timelines->size; i < n; i++) {
		sp4Debug_printTimeline(animation->timelines->items[i]);
	}
}

void sp4Debug_printBoneDatas(sp4BoneData **boneDatas, int numBoneDatas) {
	int i;
	for (i = 0; i < numBoneDatas; i++) {
		sp4Debug_printBoneData(boneDatas[i]);
	}
}

void sp4Debug_printBoneData(sp4BoneData *boneData) {
	printf("Bone data %s: %f, %f, %f, %f, %f, %f %f\n", boneData->name, boneData->rotation, boneData->scaleX,
		   boneData->scaleY, boneData->x, boneData->y, boneData->shearX, boneData->shearY);
}

void sp4Debug_printSkeleton(sp4Skeleton *skeleton) {
	sp4Debug_printBones(skeleton->bones, skeleton->bonesCount);
}

void sp4Debug_printBones(sp4Bone **bones, int numBones) {
	int i;
	for (i = 0; i < numBones; i++) {
		sp4Debug_printBone(bones[i]);
	}
}

void sp4Debug_printBone(sp4Bone *bone) {
	printf("Bone %s: %f, %f, %f, %f, %f, %f\n", bone->data->name, bone->a, bone->b, bone->c, bone->d, bone->worldX,
		   bone->worldY);
}

void sp4Debug_printFloats(float *values, int numFloats) {
	int i;
	printf("(%i) [", numFloats);
	for (i = 0; i < numFloats; i++) {
		printf("%f, ", values[i]);
	}
	printf("]");
}
