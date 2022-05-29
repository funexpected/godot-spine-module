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

#ifndef SPINE_V4_ATLAS_H_
#define SPINE_V4_ATLAS_H_

#include <v4/spine/dll.h>
#include <v4/spine/Array.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4Atlas sp4Atlas;

typedef enum {
	SP4_ATLAS_UNKNOWN_FORMAT,
	SP4_ATLAS_ALPHA,
	SP4_ATLAS_INTENSITY,
	SP4_ATLAS_LUMINANCE_ALPHA,
	SP4_ATLAS_RGB565,
	SP4_ATLAS_RGBA4444,
	SP4_ATLAS_RGB888,
	SP4_ATLAS_RGBA8888
} sp4AtlasFormat;

typedef enum {
	SP4_ATLAS_UNKNOWN_FILTER,
	SP4_ATLAS_NEAREST,
	SP4_ATLAS_LINEAR,
	SP4_ATLAS_MIPMAP,
	SP4_ATLAS_MIPMAP_NEAREST_NEAREST,
	SP4_ATLAS_MIPMAP_LINEAR_NEAREST,
	SP4_ATLAS_MIPMAP_NEAREST_LINEAR,
	SP4_ATLAS_MIPMAP_LINEAR_LINEAR
} sp4AtlasFilter;

typedef enum {
	SP4_ATLAS_MIRROREDREPEAT,
	SP4_ATLAS_CLAMPTOEDGE,
	SP4_ATLAS_REPEAT
} sp4AtlasWrap;

typedef struct sp4AtlasPage sp4AtlasPage;
struct sp4AtlasPage {
	const sp4Atlas *atlas;
	const char *name;
	sp4AtlasFormat format;
	sp4AtlasFilter minFilter, magFilter;
	sp4AtlasWrap uWrap, vWrap;

	void *rendererObject;
	int width, height;
	int /*boolean*/ pma;

	sp4AtlasPage *next;
};

SP4_API sp4AtlasPage *sp4AtlasPage_create(sp4Atlas *atlas, const char *name);

SP4_API void sp4AtlasPage_dispose(sp4AtlasPage *self);

/**/
typedef struct sp4KeyValue {
	char *name;
	float values[5];
} sp4KeyValue;
_SP4_ARRAY_DECLARE_TYPE(sp4KeyValueArray, sp4KeyValue)

/**/
typedef struct sp4AtlasRegion sp4AtlasRegion;
struct sp4AtlasRegion {
	const char *name;
	int x, y, width, height;
	float u, v, u2, v2;
	int offsetX, offsetY;
	int originalWidth, originalHeight;
	int index;
	int degrees;
	int *splits;
	int *pads;
	sp4KeyValueArray *keyValues;

	sp4AtlasPage *page;

	sp4AtlasRegion *next;
};

SP4_API sp4AtlasRegion *sp4AtlasRegion_create();

SP4_API void sp4AtlasRegion_dispose(sp4AtlasRegion *self);

/**/

struct sp4Atlas {
	sp4AtlasPage *pages;
	sp4AtlasRegion *regions;

	void *rendererObject;
};

/* Image files referenced in the atlas file will be prefixed with dir. */
SP4_API sp4Atlas *sp4Atlas_create(const char *data, int length, const char *dir, void *rendererObject);
/* Image files referenced in the atlas file will be prefixed with the directory containing the atlas file. */
SP4_API sp4Atlas *sp4Atlas_createFromFile(const char *path, void *rendererObject);

SP4_API void sp4Atlas_dispose(sp4Atlas *atlas);

/* Returns 0 if the region was not found. */
SP4_API sp4AtlasRegion *sp4Atlas_findRegion(const sp4Atlas *self, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_ATLAS_H_ */
