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

#ifndef SPINE_V4_VERTEXEFFECT_H_
#define SPINE_V4_VERTEXEFFECT_H_

#include <v4/spine/dll.h>
#include <v4/spine/Skeleton.h>
#include <v4/spine/Color.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sp4VertexEffect;

typedef void (*sp4VertexEffectBegin)(struct sp4VertexEffect *self, sp4Skeleton *skeleton);

typedef void (*sp4VertexEffectTransform)(struct sp4VertexEffect *self, float *x, float *y, float *u, float *v,
										sp4Color *light, sp4Color *dark);

typedef void (*sp4VertexEffectEnd)(struct sp4VertexEffect *self);

typedef struct sp4VertexEffect {
	sp4VertexEffectBegin begin;
	sp4VertexEffectTransform transform;
	sp4VertexEffectEnd end;
} sp4VertexEffect;

typedef struct sp4JitterVertexEffect {
	sp4VertexEffect super;
	float jitterX;
	float jitterY;
} sp4JitterVertexEffect;

typedef struct sp4SwirlVertexEffect {
	sp4VertexEffect super;
	float centerX;
	float centerY;
	float radius;
	float angle;
	float worldX;
	float worldY;
} sp4SwirlVertexEffect;

SP4_API sp4JitterVertexEffect *sp4JitterVertexEffect_create(float jitterX, float jitterY);

SP4_API void sp4JitterVertexEffect_dispose(sp4JitterVertexEffect *effect);

SP4_API sp4SwirlVertexEffect *sp4SwirlVertexEffect_create(float radius);

SP4_API void sp4SwirlVertexEffect_dispose(sp4SwirlVertexEffect *effect);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_VERTEX_EFFECT_H_ */
