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

/*
 Implementation notes:

 - An OOP style is used where each "class" is made up of a struct and a number of functions prefixed with the struct name.

 - struct fields that are const are readonly. Either they are set in a create function and can never be changed, or they can only
 be changed by calling a function.

 - Inheritance is done using a struct field named "super" as the first field, allowing the struct to be cast to its "super class".
 This works because a pointer to a struct is guaranteed to be a pointer to the first struct field.

 - Classes intended for inheritance provide init/deinit functions which subclasses must call in their create/dispose functions.

 - Polymorphism is done by a base class providing function pointers in its init function. The public API delegates to these
 function pointers.

 - Subclasses do not provide a dispose function, instead the base class' dispose function should be used, which will delegate to
 a dispose function pointer.

 - Classes not designed for inheritance cannot be extended because they may use an internal subclass to hide private data and don't
 expose function pointers.

 - The public API hides implementation details, such as init/deinit functions. An internal API is exposed by extension.h to allow
 classes to be extended. Internal functions begin with underscore (_).

 - OOP in C tends to lose type safety. Macros for casting are provided in extension.h to give context for why a cast is being done.

 */

#ifndef SPINE_V4_EXTENSION_H_
#define SPINE_V4_EXTENSION_H_

#include <v4/spine/dll.h>

/* Required for sprintf and consorts on MSVC */
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

/* All allocation uses these. */
#define MALLOC(TYPE, COUNT) ((TYPE*)_sp4Malloc(sizeof(TYPE) * (COUNT), __FILE__, __LINE__))
#define CALLOC(TYPE, COUNT) ((TYPE*)_sp4Calloc(COUNT, sizeof(TYPE), __FILE__, __LINE__))
#define REALLOC(PTR, TYPE, COUNT) ((TYPE*)_sp4Realloc(PTR, sizeof(TYPE) * (COUNT)))
#define NEW(TYPE) CALLOC(TYPE,1)

/* Gets the direct super class. Type safe. */
#define SUPER(VALUE) (&VALUE->super)

/* Cast to a super class. Not type safe, use with care. Prefer SUPER() where possible. */
#define SUPER_CAST(TYPE, VALUE) ((TYPE*)VALUE)

/* Cast to a sub class. Not type safe, use with care. */
#define SUB_CAST(TYPE, VALUE) ((TYPE*)VALUE)

/* Casts away const. Can be used as an lvalue. Not type safe, use with care. */
#define CONST_CAST(TYPE, VALUE) (*(TYPE*)&VALUE)

/* Gets the vtable for the specified type. Not type safe, use with care. */
#define VTABLE(TYPE, VALUE) ((_##TYPE##Vtable*)((TYPE*)VALUE)->vtable)

/* Frees memory. Can be used on const types. */
#define FREE(VALUE) _sp4Free((void*)VALUE)

/* Allocates a new char[], assigns it to TO, and copies FROM to it. Can be used on const types. */
#define MALLOC_STR(TO, FROM) strcpy(CONST_CAST(char*, TO) = (char*)MALLOC(char, strlen(FROM) + 1), FROM)

#define PI 3.1415926535897932385f
#define PI2 (PI * 2)
#define DEG_RAD (PI / 180)
#define RAD_DEG (180 / PI)

#define ABS(A) ((A) < 0? -(A): (A))
#define SIGNUM(A) ((A) < 0? -1.0f: (A) > 0 ? 1.0f : 0.0f)

#ifdef __STDC_VERSION__
#define FMOD(A,B) fmodf(A, B)
#define ATAN2(A,B) atan2f(A, B)
#define SIN(A) sinf(A)
#define COS(A) cosf(A)
#define SQRT(A) sqrtf(A)
#define ACOS(A) acosf(A)
#define POW(A,B) pow(A, B)
#define ISNAN(A) (int)isnan(A)
#else
#define FMOD(A, B) (float)fmod(A, B)
#define ATAN2(A, B) (float)atan2(A, B)
#define COS(A) (float)cos(A)
#define SIN(A) (float)sin(A)
#define SQRT(A) (float)sqrt(A)
#define ACOS(A) (float)acos(A)
#define POW(A, B) (float)pow(A, B)
#define ISNAN(A) (int)isnan(A)
#endif

#define SIN_DEG(A) SIN((A) * DEG_RAD)
#define COS_DEG(A) COS((A) * DEG_RAD)
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#define UNUSED(x) (void)(x)

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <v4/spine/Skeleton.h>
#include <v4/spine/Animation.h>
#include <v4/spine/Atlas.h>
#include <v4/spine/AttachmentLoader.h>
#include <v4/spine/VertexAttachment.h>
#include <v4/spine/RegionAttachment.h>
#include <v4/spine/MeshAttachment.h>
#include <v4/spine/BoundingBoxAttachment.h>
#include <v4/spine/ClippingAttachment.h>
#include <v4/spine/PathAttachment.h>
#include <v4/spine/PointAttachment.h>
#include <v4/spine/AnimationState.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Functions that must be implemented:
 */

void _sp4AtlasPage_createTexture(sp4AtlasPage *self, const char *path);

void _sp4AtlasPage_disposeTexture(sp4AtlasPage *self);

char *_sp4Util_readFile(const char *path, int *length);

/*
 * Internal API available for extension:
 */

void *_sp4Malloc(size_t size, const char *file, int line);

void *_sp4Calloc(size_t num, size_t size, const char *file, int line);

void *_sp4Realloc(void *ptr, size_t size);

void _sp4Free(void *ptr);

float _sp4Random();

SP4_API void _sp4SetMalloc(void *(*_malloc)(size_t size));

SP4_API void _sp4SetDebugMalloc(void *(*_malloc)(size_t size, const char *file, int line));

SP4_API void _sp4SetRealloc(void *(*_realloc)(void *ptr, size_t size));

SP4_API void _sp4SetFree(void (*_free)(void *ptr));

SP4_API void _sp4SetRandom(float (*_random)());

char *_sp4ReadFile(const char *path, int *length);


/*
 * Math utilities
 */
float _sp4Math_random(float min, float max);

float _sp4Math_randomTriangular(float min, float max);

float _sp4Math_randomTriangularWith(float min, float max, float mode);

float _sp4Math_interpolate(float (*apply)(float a), float start, float end, float a);

float _sp4Math_pow2_apply(float a);

float _sp4Math_pow2out_apply(float a);

/**/

typedef union _sp4EventQueueItem {
	int type;
	sp4TrackEntry *entry;
	sp4Event *event;
} _sp4EventQueueItem;

typedef struct _sp4AnimationState _sp4AnimationState;

typedef struct _sp4EventQueue {
	_sp4AnimationState *state;
	_sp4EventQueueItem *objects;
	int objectsCount;
	int objectsCapacity;
	int /*boolean*/ drainDisabled;
} _sp4EventQueue;

struct _sp4AnimationState {
	sp4AnimationState super;

	int eventsCount;
	sp4Event **events;

	_sp4EventQueue *queue;

	sp4PropertyId *propertyIDs;
	int propertyIDsCount;
	int propertyIDsCapacity;

	int /*boolean*/ animationsChanged;
};


/**/

/* configureAttachment and disposeAttachment may be 0. */
void _sp4AttachmentLoader_init(sp4AttachmentLoader *self,
							  void (*dispose)(sp4AttachmentLoader *self),
							  sp4Attachment *(*createAttachment)(sp4AttachmentLoader *self, sp4Skin *skin,
																sp4AttachmentType type, const char *name,
																const char *path),
							  void (*configureAttachment)(sp4AttachmentLoader *self, sp4Attachment *),
							  void (*disposeAttachment)(sp4AttachmentLoader *self, sp4Attachment *)
);

void _sp4AttachmentLoader_deinit(sp4AttachmentLoader *self);

/* Can only be called from createAttachment. */
void _sp4AttachmentLoader_setError(sp4AttachmentLoader *self, const char *error1, const char *error2);

void _sp4AttachmentLoader_setUnknownTypeError(sp4AttachmentLoader *self, sp4AttachmentType type);

/**/

void _sp4Attachment_init(sp4Attachment *self, const char *name, sp4AttachmentType type,
						void (*dispose)(sp4Attachment *self), sp4Attachment *(*copy)(sp4Attachment *self));

void _sp4Attachment_deinit(sp4Attachment *self);

void _sp4VertexAttachment_init(sp4VertexAttachment *self);

void _sp4VertexAttachment_deinit(sp4VertexAttachment *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_EXTENSION_H_ */
