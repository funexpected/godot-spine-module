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

#include <v4/spine/AttachmentLoader.h>
#include <v4/spine/extension.h>
#include <stdio.h>

typedef struct _sp4AttachmentLoaderVtable {
	sp4Attachment *(*createAttachment)(sp4AttachmentLoader *self, sp4Skin *skin, sp4AttachmentType type, const char *name,
									  const char *path);

	void (*configureAttachment)(sp4AttachmentLoader *self, sp4Attachment *);

	void (*disposeAttachment)(sp4AttachmentLoader *self, sp4Attachment *);

	void (*dispose)(sp4AttachmentLoader *self);
} _sp4AttachmentLoaderVtable;

void _sp4AttachmentLoader_init(sp4AttachmentLoader *self,
							  void (*dispose)(sp4AttachmentLoader *self),
							  sp4Attachment *(*createAttachment)(sp4AttachmentLoader *self, sp4Skin *skin,
																sp4AttachmentType type, const char *name,
																const char *path),
							  void (*configureAttachment)(sp4AttachmentLoader *self, sp4Attachment *),
							  void (*disposeAttachment)(sp4AttachmentLoader *self, sp4Attachment *)) {
	CONST_CAST(_sp4AttachmentLoaderVtable *, self->vtable) = NEW(_sp4AttachmentLoaderVtable);
	VTABLE(sp4AttachmentLoader, self)->dispose = dispose;
	VTABLE(sp4AttachmentLoader, self)->createAttachment = createAttachment;
	VTABLE(sp4AttachmentLoader, self)->configureAttachment = configureAttachment;
	VTABLE(sp4AttachmentLoader, self)->disposeAttachment = disposeAttachment;
}

void _sp4AttachmentLoader_deinit(sp4AttachmentLoader *self) {
	FREE(self->vtable);
	FREE(self->error1);
	FREE(self->error2);
}

void sp4AttachmentLoader_dispose(sp4AttachmentLoader *self) {
	VTABLE(sp4AttachmentLoader, self)->dispose(self);
	FREE(self);
}

sp4Attachment *
sp4AttachmentLoader_createAttachment(sp4AttachmentLoader *self, sp4Skin *skin, sp4AttachmentType type, const char *name,
									const char *path) {
	FREE(self->error1);
	FREE(self->error2);
	self->error1 = 0;
	self->error2 = 0;
	return VTABLE(sp4AttachmentLoader, self)->createAttachment(self, skin, type, name, path);
}

void sp4AttachmentLoader_configureAttachment(sp4AttachmentLoader *self, sp4Attachment *attachment) {
	if (!VTABLE(sp4AttachmentLoader, self)->configureAttachment) return;
	VTABLE(sp4AttachmentLoader, self)->configureAttachment(self, attachment);
}

void sp4AttachmentLoader_disposeAttachment(sp4AttachmentLoader *self, sp4Attachment *attachment) {
	if (!VTABLE(sp4AttachmentLoader, self)->disposeAttachment) return;
	VTABLE(sp4AttachmentLoader, self)->disposeAttachment(self, attachment);
}

void _sp4AttachmentLoader_setError(sp4AttachmentLoader *self, const char *error1, const char *error2) {
	FREE(self->error1);
	FREE(self->error2);
	MALLOC_STR(self->error1, error1);
	MALLOC_STR(self->error2, error2);
}

void _sp4AttachmentLoader_setUnknownTypeError(sp4AttachmentLoader *self, sp4AttachmentType type) {
	char buffer[16];
	sprintf(buffer, "%d", type);
	_sp4AttachmentLoader_setError(self, "Unknown attachment type: ", buffer);
}
