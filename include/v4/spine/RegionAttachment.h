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

#ifndef SPINE_V4_REGIONATTACHMENT_H_
#define SPINE_V4_REGIONATTACHMENT_H_

#include <v4/spine/dll.h>
#include <v4/spine/Attachment.h>
#include <v4/spine/Atlas.h>
#include <v4/spine/Slot.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4RegionAttachment {
	sp4Attachment super;
	const char *path;
	float x, y, scaleX, scaleY, rotation, width, height;
	sp4Color color;

	void *rendererObject;
	int regionOffsetX, regionOffsetY; /* Pixels stripped from the bottom left, unrotated. */
	int regionWidth, regionHeight; /* Unrotated, stripped pixel size. */
	int regionOriginalWidth, regionOriginalHeight; /* Unrotated, unstripped pixel size. */

	float offset[8];
	float uvs[8];
} sp4RegionAttachment;

SP4_API sp4RegionAttachment *sp4RegionAttachment_create(const char *name);

SP4_API void sp4RegionAttachment_setUVs(sp4RegionAttachment *self, float u, float v, float u2, float v2, float degrees);

SP4_API void sp4RegionAttachment_updateOffset(sp4RegionAttachment *self);

SP4_API void sp4RegionAttachment_computeWorldVertices(sp4RegionAttachment *self, sp4Bone *bone, float *vertices, int offset,
													int stride);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_REGIONATTACHMENT_H_ */
