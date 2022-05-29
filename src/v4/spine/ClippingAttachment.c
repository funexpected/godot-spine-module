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

#include <v4/spine/ClippingAttachment.h>
#include <v4/spine/extension.h>

void _sp4ClippingAttachment_dispose(sp4Attachment *attachment) {
	sp4ClippingAttachment *self = SUB_CAST(sp4ClippingAttachment, attachment);

	_sp4VertexAttachment_deinit(SUPER(self));

	FREE(self);
}

sp4Attachment *_sp4ClippingAttachment_copy(sp4Attachment *attachment) {
	sp4ClippingAttachment *copy = sp4ClippingAttachment_create(attachment->name);
	sp4ClippingAttachment *self = SUB_CAST(sp4ClippingAttachment, attachment);
	sp4VertexAttachment_copyTo(SUPER(self), SUPER(copy));
	copy->endSlot = self->endSlot;
	return SUPER(SUPER(copy));
}

sp4ClippingAttachment *sp4ClippingAttachment_create(const char *name) {
	sp4ClippingAttachment *self = NEW(sp4ClippingAttachment);
	_sp4VertexAttachment_init(SUPER(self));
	_sp4Attachment_init(SUPER(SUPER(self)), name, SP4_ATTACHMENT_CLIPPING, _sp4ClippingAttachment_dispose,
					   _sp4ClippingAttachment_copy);
	self->endSlot = 0;
	return self;
}
