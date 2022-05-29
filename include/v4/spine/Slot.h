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

#ifndef SPINE_V4_SLOT_H_
#define SPINE_V4_SLOT_H_

#include <v4/spine/dll.h>
#include <v4/spine/Bone.h>
#include <v4/spine/Attachment.h>
#include <v4/spine/SlotData.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4Slot {
	sp4SlotData *const data;
	sp4Bone *const bone;
	sp4Color color;
	sp4Color *darkColor;
	sp4Attachment *attachment;
	int attachmentState;

	int deformCapacity;
	int deformCount;
	float *deform;
} sp4Slot;

SP4_API sp4Slot *sp4Slot_create(sp4SlotData *data, sp4Bone *bone);

SP4_API void sp4Slot_dispose(sp4Slot *self);

/* @param attachment May be 0 to clear the attachment for the slot. */
SP4_API void sp4Slot_setAttachment(sp4Slot *self, sp4Attachment *attachment);

SP4_API void sp4Slot_setAttachmentTime(sp4Slot *self, float time);

SP4_API float sp4Slot_getAttachmentTime(const sp4Slot *self);

SP4_API void sp4Slot_setToSetupPose(sp4Slot *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_SLOT_H_ */
