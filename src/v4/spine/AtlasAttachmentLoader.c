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

#include <v4/spine/AtlasAttachmentLoader.h>
#include <v4/spine/extension.h>

sp4Attachment *_sp4AtlasAttachmentLoader_createAttachment(sp4AttachmentLoader *loader, sp4Skin *skin, sp4AttachmentType type,
														const char *name, const char *path) {
	sp4AtlasAttachmentLoader *self = SUB_CAST(sp4AtlasAttachmentLoader, loader);
	switch (type) {
		case SP4_ATTACHMENT_REGION: {
			sp4RegionAttachment *attachment;
			sp4AtlasRegion *region = sp4Atlas_findRegion(self->atlas, path);
			if (!region) {
				_sp4AttachmentLoader_setError(loader, "Region not found: ", path);
				return 0;
			}
			attachment = sp4RegionAttachment_create(name);
			attachment->rendererObject = region;
			sp4RegionAttachment_setUVs(attachment, region->u, region->v, region->u2, region->v2, region->degrees);
			attachment->regionOffsetX = region->offsetX;
			attachment->regionOffsetY = region->offsetY;
			attachment->regionWidth = region->width;
			attachment->regionHeight = region->height;
			attachment->regionOriginalWidth = region->originalWidth;
			attachment->regionOriginalHeight = region->originalHeight;
			return SUPER(attachment);
		}
		case SP4_ATTACHMENT_MESH:
		case SP4_ATTACHMENT_LINKED_MESH: {
			sp4MeshAttachment *attachment;
			sp4AtlasRegion *region = sp4Atlas_findRegion(self->atlas, path);
			if (!region) {
				_sp4AttachmentLoader_setError(loader, "Region not found: ", path);
				return 0;
			}
			attachment = sp4MeshAttachment_create(name);
			attachment->rendererObject = region;
			attachment->regionU = region->u;
			attachment->regionV = region->v;
			attachment->regionU2 = region->u2;
			attachment->regionV2 = region->v2;
			attachment->regionDegrees = region->degrees;
			attachment->regionOffsetX = region->offsetX;
			attachment->regionOffsetY = region->offsetY;
			attachment->regionWidth = region->width;
			attachment->regionHeight = region->height;
			attachment->regionOriginalWidth = region->originalWidth;
			attachment->regionOriginalHeight = region->originalHeight;
			return SUPER(SUPER(attachment));
		}
		case SP4_ATTACHMENT_BOUNDING_BOX:
			return SUPER(SUPER(sp4BoundingBoxAttachment_create(name)));
		case SP4_ATTACHMENT_PATH:
			return SUPER(SUPER(sp4PathAttachment_create(name)));
		case SP4_ATTACHMENT_POINT:
			return SUPER(sp4PointAttachment_create(name));
		case SP4_ATTACHMENT_CLIPPING:
			return SUPER(SUPER(sp4ClippingAttachment_create(name)));
		default:
			_sp4AttachmentLoader_setUnknownTypeError(loader, type);
			return 0;
	}

	UNUSED(skin);
}

sp4AtlasAttachmentLoader *sp4AtlasAttachmentLoader_create(sp4Atlas *atlas) {
	sp4AtlasAttachmentLoader *self = NEW(sp4AtlasAttachmentLoader);
	_sp4AttachmentLoader_init(SUPER(self), _sp4AttachmentLoader_deinit, _sp4AtlasAttachmentLoader_createAttachment, 0, 0);
	self->atlas = atlas;
	return self;
}
