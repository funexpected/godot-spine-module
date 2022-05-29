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

#ifndef SPINE_V4_SKELETONBINARY_H_
#define SPINE_V4_SKELETONBINARY_H_

#include <v4/spine/dll.h>
#include <v4/spine/Attachment.h>
#include <v4/spine/AttachmentLoader.h>
#include <v4/spine/SkeletonData.h>
#include <v4/spine/Atlas.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sp4AtlasAttachmentLoader;

typedef struct sp4SkeletonBinary {
	float scale;
	sp4AttachmentLoader *attachmentLoader;
	const char *const error;
} sp4SkeletonBinary;

SP4_API sp4SkeletonBinary *sp4SkeletonBinary_createWithLoader(sp4AttachmentLoader *attachmentLoader);

SP4_API sp4SkeletonBinary *sp4SkeletonBinary_create(sp4Atlas *atlas);

SP4_API void sp4SkeletonBinary_dispose(sp4SkeletonBinary *self);

SP4_API sp4SkeletonData *
sp4SkeletonBinary_readSkeletonData(sp4SkeletonBinary *self, const unsigned char *binary, const int length);

SP4_API sp4SkeletonData *sp4SkeletonBinary_readSkeletonDataFile(sp4SkeletonBinary *self, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_SKELETONBINARY_H_ */
