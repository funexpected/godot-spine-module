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

#include <v4/spine/Attachment.h>
#include <v4/spine/Slot.h>
#include <v4/spine/extension.h>

typedef struct _sp4AttachmentVtable {
	void (*dispose)(sp4Attachment *self);

	sp4Attachment *(*copy)(sp4Attachment *self);
} _sp4AttachmentVtable;

void _sp4Attachment_init(sp4Attachment *self, const char *name, sp4AttachmentType type, /**/
						void (*dispose)(sp4Attachment *self), sp4Attachment *(*copy)(sp4Attachment *self)) {

	CONST_CAST(_sp4AttachmentVtable *, self->vtable) = NEW(_sp4AttachmentVtable);
	VTABLE(sp4Attachment, self)->dispose = dispose;
	VTABLE(sp4Attachment, self)->copy = copy;

	MALLOC_STR(self->name, name);
	CONST_CAST(sp4AttachmentType, self->type) = type;
}

void _sp4Attachment_deinit(sp4Attachment *self) {
	if (self->attachmentLoader) sp4AttachmentLoader_disposeAttachment(self->attachmentLoader, self);
	FREE(self->vtable);
	FREE(self->name);
}

sp4Attachment *sp4Attachment_copy(sp4Attachment *self) {
	return VTABLE(sp4Attachment, self)->copy(self);
}

void sp4Attachment_dispose(sp4Attachment *self) {
	self->refCount--;
	if (self->refCount <= 0)
		VTABLE(sp4Attachment, self)->dispose(self);
}
