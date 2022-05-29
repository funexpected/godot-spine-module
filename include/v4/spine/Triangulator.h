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

#ifndef SPINE_V4_TRIANGULATOR_H
#define SPINE_V4_TRIANGULATOR_H

#include <v4/spine/dll.h>
#include <v4/spine/Array.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4Triangulator {
	sp4ArrayFloatArray *convexPolygons;
	sp4ArrayShortArray *convexPolygonsIndices;

	sp4ShortArray *indicesArray;
	sp4IntArray *isConcaveArray;
	sp4ShortArray *triangles;

	sp4ArrayFloatArray *polygonPool;
	sp4ArrayShortArray *polygonIndicesPool;
} sp4Triangulator;

SP4_API sp4Triangulator *sp4Triangulator_create();

SP4_API sp4ShortArray *sp4Triangulator_triangulate(sp4Triangulator *self, sp4FloatArray *verticesArray);

SP4_API sp4ArrayFloatArray *
sp4Triangulator_decompose(sp4Triangulator *self, sp4FloatArray *verticesArray, sp4ShortArray *triangles);

SP4_API void sp4Triangulator_dispose(sp4Triangulator *self);


#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_TRIANGULATOR_H_ */
