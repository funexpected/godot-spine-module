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

#ifndef SPINE_V4_SKELETONCLIPPING_H
#define SPINE_V4_SKELETONCLIPPING_H

#include <v4/spine/dll.h>
#include <v4/spine/Array.h>
#include <v4/spine/ClippingAttachment.h>
#include <v4/spine/Slot.h>
#include <v4/spine/Triangulator.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4SkeletonClipping {
	sp4Triangulator *triangulator;
	sp4FloatArray *clippingPolygon;
	sp4FloatArray *clipOutput;
	sp4FloatArray *clippedVertices;
	sp4FloatArray *clippedUVs;
	sp4UnsignedShortArray *clippedTriangles;
	sp4FloatArray *scratch;
	sp4ClippingAttachment *clipAttachment;
	sp4ArrayFloatArray *clippingPolygons;
} sp4SkeletonClipping;

SP4_API sp4SkeletonClipping *sp4SkeletonClipping_create();

SP4_API int sp4SkeletonClipping_clipStart(sp4SkeletonClipping *self, sp4Slot *slot, sp4ClippingAttachment *clip);

SP4_API void sp4SkeletonClipping_clipEnd(sp4SkeletonClipping *self, sp4Slot *slot);

SP4_API void sp4SkeletonClipping_clipEnd2(sp4SkeletonClipping *self);

SP4_API int /*boolean*/ sp4SkeletonClipping_isClipping(sp4SkeletonClipping *self);

SP4_API void sp4SkeletonClipping_clipTriangles(sp4SkeletonClipping *self, float *vertices, int verticesLength,
											 unsigned short *triangles, int trianglesLength, float *uvs, int stride);

SP4_API void sp4SkeletonClipping_dispose(sp4SkeletonClipping *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_SKELETONCLIPPING_H */
