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

#ifndef SPINE_V4_ATTACHMENTLOADER_H_
#define SPINE_V4_ATTACHMENTLOADER_H_

#include <v4/spine/dll.h>
#include <v4/spine/Attachment.h>
#include <v4/spine/Skin.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4AttachmentLoader {
	const char *error1;
	const char *error2;

	const void *const vtable;
} sp4AttachmentLoader;

SP4_API void sp4AttachmentLoader_dispose(sp4AttachmentLoader *self);

/* Called to create each attachment. Returns 0 to not load an attachment. If 0 is returned and _sp4AttachmentLoader_setError was
 * called, an error occurred. */
SP4_API sp4Attachment *
sp4AttachmentLoader_createAttachment(sp4AttachmentLoader *self, sp4Skin *skin, sp4AttachmentType type, const char *name,
									const char *path);
/* Called after the attachment has been fully configured. */
SP4_API void sp4AttachmentLoader_configureAttachment(sp4AttachmentLoader *self, sp4Attachment *attachment);
/* Called just before the attachment is disposed. This can release allocations made in sp4AttachmentLoader_configureAttachment. */
SP4_API void sp4AttachmentLoader_disposeAttachment(sp4AttachmentLoader *self, sp4Attachment *attachment);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_ATTACHMENTLOADER_H_ */
