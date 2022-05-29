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

#include <v4/spine/Slot.h>
#include <v4/spine/extension.h>

typedef struct {
	sp4Slot super;
	float attachmentTime;
} _sp4Slot;

sp4Slot *sp4Slot_create(sp4SlotData *data, sp4Bone *bone) {
	sp4Slot *self = SUPER(NEW(_sp4Slot));
	CONST_CAST(sp4SlotData *, self->data) = data;
	CONST_CAST(sp4Bone *, self->bone) = bone;
	sp4Color_setFromFloats(&self->color, 1, 1, 1, 1);
	self->darkColor = data->darkColor == 0 ? 0 : sp4Color_create();
	sp4Slot_setToSetupPose(self);
	return self;
}

void sp4Slot_dispose(sp4Slot *self) {
	FREE(self->deform);
	FREE(self->darkColor);
	FREE(self);
}

static int isVertexAttachment(sp4Attachment *attachment) {
	if (attachment == NULL) return 0;
	switch (attachment->type) {
		case SP4_ATTACHMENT_BOUNDING_BOX:
		case SP4_ATTACHMENT_CLIPPING:
		case SP4_ATTACHMENT_MESH:
		case SP4_ATTACHMENT_PATH:
			return -1;
		default:
			return 0;
	}
}

void sp4Slot_setAttachment(sp4Slot *self, sp4Attachment *attachment) {
	if (attachment == self->attachment) return;

	if (!isVertexAttachment(attachment) ||
		!isVertexAttachment(self->attachment) || (SUB_CAST(sp4VertexAttachment, attachment)->deformAttachment != SUB_CAST(sp4VertexAttachment, self->attachment)->deformAttachment)) {
		self->deformCount = 0;
	}

	CONST_CAST(sp4Attachment *, self->attachment) = attachment;
	SUB_CAST(_sp4Slot, self)->attachmentTime = self->bone->skeleton->time;
}

void sp4Slot_setAttachmentTime(sp4Slot *self, float time) {
	SUB_CAST(_sp4Slot, self)->attachmentTime = self->bone->skeleton->time - time;
}

float sp4Slot_getAttachmentTime(const sp4Slot *self) {
	return self->bone->skeleton->time - SUB_CAST(_sp4Slot, self)->attachmentTime;
}

void sp4Slot_setToSetupPose(sp4Slot *self) {
	sp4Color_setFromColor(&self->color, &self->data->color);
	if (self->darkColor) sp4Color_setFromColor(self->darkColor, self->data->darkColor);

	if (!self->data->attachmentName)
		sp4Slot_setAttachment(self, 0);
	else {
		sp4Attachment *attachment = sp4Skeleton_getAttachmentForSlotIndex(
				self->bone->skeleton, self->data->index, self->data->attachmentName);
		CONST_CAST(sp4Attachment *, self->attachment) = 0;
		sp4Slot_setAttachment(self, attachment);
	}
}
