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

#ifndef SPINE_V4_SKIN_H_
#define SPINE_V4_SKIN_H_

#include <v4/spine/dll.h>
#include <v4/spine/Attachment.h>
#include <v4/spine/IkConstraintData.h>
#include <v4/spine/TransformConstraintData.h>
#include <v4/spine/PathConstraintData.h>
#include <v4/spine/Array.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Size of hashtable used in skin structure for fast attachment lookup. */
#define SKIN_ENTRIES_HASH_TABLE_SIZE 100

struct sp4Skeleton;

_SP4_ARRAY_DECLARE_TYPE(sp4BoneDataArray, sp4BoneData*)

_SP4_ARRAY_DECLARE_TYPE(sp4IkConstraintDataArray, sp4IkConstraintData*)

_SP4_ARRAY_DECLARE_TYPE(sp4TransformConstraintDataArray, sp4TransformConstraintData*)

_SP4_ARRAY_DECLARE_TYPE(sp4PathConstraintDataArray, sp4PathConstraintData*)

typedef struct sp4Skin {
	const char *const name;

	sp4BoneDataArray *bones;
	sp4IkConstraintDataArray *ikConstraints;
	sp4TransformConstraintDataArray *transformConstraints;
	sp4PathConstraintDataArray *pathConstraints;
} sp4Skin;

/* Private structs, needed by Skeleton */
typedef struct _4_Entry _4_Entry;
typedef struct _4_Entry sp4SkinEntry;
struct _4_Entry {
	int slotIndex;
	const char *name;
	sp4Attachment *attachment;
	_4_Entry *next;
};

typedef struct _SkinHashTableEntry _SkinHashTableEntry;
struct _SkinHashTableEntry {
	_4_Entry *entry;
	_SkinHashTableEntry *next;
};

typedef struct {
	sp4Skin super;
	_4_Entry *entries; /* entries list stored for getting attachment name by attachment index */
	_SkinHashTableEntry *entriesHashTable[SKIN_ENTRIES_HASH_TABLE_SIZE]; /* hashtable for fast attachment lookup */
} _sp4Skin;

SP4_API sp4Skin *sp4Skin_create(const char *name);

SP4_API void sp4Skin_dispose(sp4Skin *self);

/* The Skin owns the attachment. */
SP4_API void sp4Skin_setAttachment(sp4Skin *self, int slotIndex, const char *name, sp4Attachment *attachment);
/* Returns 0 if the attachment was not found. */
SP4_API sp4Attachment *sp4Skin_getAttachment(const sp4Skin *self, int slotIndex, const char *name);

/* Returns 0 if the slot or attachment was not found. */
SP4_API const char *sp4Skin_getAttachmentName(const sp4Skin *self, int slotIndex, int attachmentIndex);

/** Attach each attachment in this skin if the corresponding attachment in oldSkin is currently attached. */
SP4_API void sp4Skin_attachAll(const sp4Skin *self, struct sp4Skeleton *skeleton, const sp4Skin *oldsp4Skin);

/** Adds all attachments, bones, and constraints from the specified skin to this skin. */
SP4_API void sp4Skin_addSkin(sp4Skin *self, const sp4Skin *other);

/** Adds all attachments, bones, and constraints from the specified skin to this skin. Attachments are deep copied. */
SP4_API void sp4Skin_copySkin(sp4Skin *self, const sp4Skin *other);

/** Returns all attachments in this skin. */
SP4_API sp4SkinEntry *sp4Skin_getAttachments(const sp4Skin *self);

/** Clears all attachments, bones, and constraints. */
SP4_API void sp4Skin_clear(sp4Skin *self);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_SKIN_H_ */
