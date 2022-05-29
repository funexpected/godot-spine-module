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

#include <v4/spine/PathAttachment.h>
#include <v4/spine/extension.h>

void _sp4PathAttachment_dispose(sp4Attachment *attachment) {
	sp4PathAttachment *self = SUB_CAST(sp4PathAttachment, attachment);

	_sp4VertexAttachment_deinit(SUPER(self));

	FREE(self->lengths);
	FREE(self);
}

sp4Attachment *_sp4PathAttachment_copy(sp4Attachment *attachment) {
	sp4PathAttachment *copy = sp4PathAttachment_create(attachment->name);
	sp4PathAttachment *self = SUB_CAST(sp4PathAttachment, attachment);
	sp4VertexAttachment_copyTo(SUPER(self), SUPER(copy));
	copy->lengthsLength = self->lengthsLength;
	copy->lengths = MALLOC(float, self->lengthsLength);
	memcpy(copy->lengths, self->lengths, self->lengthsLength * sizeof(float));
	copy->closed = self->closed;
	copy->constantSpeed = self->constantSpeed;
	return SUPER(SUPER(copy));
}

sp4PathAttachment *sp4PathAttachment_create(const char *name) {
	sp4PathAttachment *self = NEW(sp4PathAttachment);
	_sp4VertexAttachment_init(SUPER(self));
	_sp4Attachment_init(SUPER(SUPER(self)), name, SP4_ATTACHMENT_PATH, _sp4PathAttachment_dispose, _sp4PathAttachment_copy);
	return self;
}
