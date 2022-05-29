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

#include <v4/spine/Skin.h>
#include <v4/spine/extension.h>
#include <stdio.h>

_SP4_ARRAY_IMPLEMENT_TYPE(sp4BoneDataArray, sp4BoneData *)

_SP4_ARRAY_IMPLEMENT_TYPE(sp4IkConstraintDataArray, sp4IkConstraintData *)

_SP4_ARRAY_IMPLEMENT_TYPE(sp4TransformConstraintDataArray, sp4TransformConstraintData *)

_SP4_ARRAY_IMPLEMENT_TYPE(sp4PathConstraintDataArray, sp4PathConstraintData *)

_4_Entry *_4_Entry_create(int slotIndex, const char *name, sp4Attachment *attachment) {
	_4_Entry *self = NEW(_4_Entry);
	self->slotIndex = slotIndex;
	MALLOC_STR(self->name, name);
	self->attachment = attachment;
	return self;
}

void _4_Entry_dispose(_4_Entry *self) {
	sp4Attachment_dispose(self->attachment);
	FREE(self->name);
	FREE(self);
}

static _SkinHashTableEntry *_SkinHashTableEntry_create(_4_Entry *entry) {
	_SkinHashTableEntry *self = NEW(_SkinHashTableEntry);
	self->entry = entry;
	return self;
}

static void _SkinHashTableEntry_dispose(_SkinHashTableEntry *self) {
	FREE(self);
}

/**/

sp4Skin *sp4Skin_create(const char *name) {
	sp4Skin *self = SUPER(NEW(_sp4Skin));
	MALLOC_STR(self->name, name);
	self->bones = sp4BoneDataArray_create(4);
	self->ikConstraints = sp4IkConstraintDataArray_create(4);
	self->transformConstraints = sp4TransformConstraintDataArray_create(4);
	self->pathConstraints = sp4PathConstraintDataArray_create(4);
	return self;
}

void sp4Skin_dispose(sp4Skin *self) {
	_4_Entry *entry = SUB_CAST(_sp4Skin, self)->entries;

	while (entry) {
		_4_Entry *nextEntry = entry->next;
		_4_Entry_dispose(entry);
		entry = nextEntry;
	}

	{
		_SkinHashTableEntry **currentHashtableEntry = SUB_CAST(_sp4Skin, self)->entriesHashTable;
		int i;

		for (i = 0; i < SKIN_ENTRIES_HASH_TABLE_SIZE; ++i, ++currentHashtableEntry) {
			_SkinHashTableEntry *hashtableEntry = *currentHashtableEntry;

			while (hashtableEntry) {
				_SkinHashTableEntry *nextEntry = hashtableEntry->next;
				_SkinHashTableEntry_dispose(hashtableEntry);
				hashtableEntry = nextEntry;
			}
		}
	}

	sp4BoneDataArray_dispose(self->bones);
	sp4IkConstraintDataArray_dispose(self->ikConstraints);
	sp4TransformConstraintDataArray_dispose(self->transformConstraints);
	sp4PathConstraintDataArray_dispose(self->pathConstraints);
	FREE(self->name);
	FREE(self);
}

void sp4Skin_setAttachment(sp4Skin *self, int slotIndex, const char *name, sp4Attachment *attachment) {
	_SkinHashTableEntry *existingEntry = 0;
	_SkinHashTableEntry *hashEntry = SUB_CAST(_sp4Skin, self)->entriesHashTable[(unsigned int) slotIndex % SKIN_ENTRIES_HASH_TABLE_SIZE];
	while (hashEntry) {
		if (hashEntry->entry->slotIndex == slotIndex && strcmp(hashEntry->entry->name, name) == 0) {
			existingEntry = hashEntry;
			break;
		}
		hashEntry = hashEntry->next;
	}

	if (attachment) attachment->refCount++;

	if (existingEntry) {
		if (hashEntry->entry->attachment) sp4Attachment_dispose(hashEntry->entry->attachment);
		hashEntry->entry->attachment = attachment;
	} else {
		_4_Entry *newEntry = _4_Entry_create(slotIndex, name, attachment);
		newEntry->next = SUB_CAST(_sp4Skin, self)->entries;
		SUB_CAST(_sp4Skin, self)->entries = newEntry;
		{
			unsigned int hashTableIndex = (unsigned int) slotIndex % SKIN_ENTRIES_HASH_TABLE_SIZE;
			_SkinHashTableEntry **hashTable = SUB_CAST(_sp4Skin, self)->entriesHashTable;

			_SkinHashTableEntry *newHashEntry = _SkinHashTableEntry_create(newEntry);
			newHashEntry->next = hashTable[hashTableIndex];
			SUB_CAST(_sp4Skin, self)->entriesHashTable[hashTableIndex] = newHashEntry;
		}
	}
}

sp4Attachment *sp4Skin_getAttachment(const sp4Skin *self, int slotIndex, const char *name) {
	const _SkinHashTableEntry *hashEntry = SUB_CAST(_sp4Skin, self)->entriesHashTable[(unsigned int) slotIndex % SKIN_ENTRIES_HASH_TABLE_SIZE];
	while (hashEntry) {
		if (hashEntry->entry->slotIndex == slotIndex && strcmp(hashEntry->entry->name, name) == 0)
			return hashEntry->entry->attachment;
		hashEntry = hashEntry->next;
	}
	return 0;
}

const char *sp4Skin_getAttachmentName(const sp4Skin *self, int slotIndex, int attachmentIndex) {
	const _4_Entry *entry = SUB_CAST(_sp4Skin, self)->entries;
	int i = 0;
	while (entry) {
		if (entry->slotIndex == slotIndex) {
			if (i == attachmentIndex) return entry->name;
			i++;
		}
		entry = entry->next;
	}
	return 0;
}

void sp4Skin_attachAll(const sp4Skin *self, sp4Skeleton *skeleton, const sp4Skin *oldSkin) {
	const _4_Entry *entry = SUB_CAST(_sp4Skin, oldSkin)->entries;
	while (entry) {
		sp4Slot *slot = skeleton->slots[entry->slotIndex];
		if (slot->attachment == entry->attachment) {
			sp4Attachment *attachment = sp4Skin_getAttachment(self, entry->slotIndex, entry->name);
			if (attachment) sp4Slot_setAttachment(slot, attachment);
		}
		entry = entry->next;
	}
}

void sp4Skin_addSkin(sp4Skin *self, const sp4Skin *other) {
	int i = 0;
	sp4SkinEntry *entry;

	for (i = 0; i < other->bones->size; i++) {
		if (!sp4BoneDataArray_contains(self->bones, other->bones->items[i]))
			sp4BoneDataArray_add(self->bones, other->bones->items[i]);
	}

	for (i = 0; i < other->ikConstraints->size; i++) {
		if (!sp4IkConstraintDataArray_contains(self->ikConstraints, other->ikConstraints->items[i]))
			sp4IkConstraintDataArray_add(self->ikConstraints, other->ikConstraints->items[i]);
	}

	for (i = 0; i < other->transformConstraints->size; i++) {
		if (!sp4TransformConstraintDataArray_contains(self->transformConstraints, other->transformConstraints->items[i]))
			sp4TransformConstraintDataArray_add(self->transformConstraints, other->transformConstraints->items[i]);
	}

	for (i = 0; i < other->pathConstraints->size; i++) {
		if (!sp4PathConstraintDataArray_contains(self->pathConstraints, other->pathConstraints->items[i]))
			sp4PathConstraintDataArray_add(self->pathConstraints, other->pathConstraints->items[i]);
	}

	entry = sp4Skin_getAttachments(other);
	while (entry) {
		sp4Skin_setAttachment(self, entry->slotIndex, entry->name, entry->attachment);
		entry = entry->next;
	}
}

void sp4Skin_copySkin(sp4Skin *self, const sp4Skin *other) {
	int i = 0;
	sp4SkinEntry *entry;

	for (i = 0; i < other->bones->size; i++) {
		if (!sp4BoneDataArray_contains(self->bones, other->bones->items[i]))
			sp4BoneDataArray_add(self->bones, other->bones->items[i]);
	}

	for (i = 0; i < other->ikConstraints->size; i++) {
		if (!sp4IkConstraintDataArray_contains(self->ikConstraints, other->ikConstraints->items[i]))
			sp4IkConstraintDataArray_add(self->ikConstraints, other->ikConstraints->items[i]);
	}

	for (i = 0; i < other->transformConstraints->size; i++) {
		if (!sp4TransformConstraintDataArray_contains(self->transformConstraints, other->transformConstraints->items[i]))
			sp4TransformConstraintDataArray_add(self->transformConstraints, other->transformConstraints->items[i]);
	}

	for (i = 0; i < other->pathConstraints->size; i++) {
		if (!sp4PathConstraintDataArray_contains(self->pathConstraints, other->pathConstraints->items[i]))
			sp4PathConstraintDataArray_add(self->pathConstraints, other->pathConstraints->items[i]);
	}

	entry = sp4Skin_getAttachments(other);
	while (entry) {
		if (entry->attachment->type == SP4_ATTACHMENT_MESH) {
			sp4MeshAttachment *attachment = sp4MeshAttachment_newLinkedMesh(
					SUB_CAST(sp4MeshAttachment, entry->attachment));
			sp4Skin_setAttachment(self, entry->slotIndex, entry->name, SUPER(SUPER(attachment)));
		} else {
			sp4Attachment *attachment = entry->attachment ? sp4Attachment_copy(entry->attachment) : 0;
			sp4Skin_setAttachment(self, entry->slotIndex, entry->name, attachment);
		}
		entry = entry->next;
	}
}

sp4SkinEntry *sp4Skin_getAttachments(const sp4Skin *self) {
	return SUB_CAST(_sp4Skin, self)->entries;
}

void sp4Skin_clear(sp4Skin *self) {
	_4_Entry *entry = SUB_CAST(_sp4Skin, self)->entries;

	while (entry) {
		_4_Entry *nextEntry = entry->next;
		_4_Entry_dispose(entry);
		entry = nextEntry;
	}

	SUB_CAST(_sp4Skin, self)->entries = 0;

	{
		_SkinHashTableEntry **currentHashtableEntry = SUB_CAST(_sp4Skin, self)->entriesHashTable;
		int i;

		for (i = 0; i < SKIN_ENTRIES_HASH_TABLE_SIZE; ++i, ++currentHashtableEntry) {
			_SkinHashTableEntry *hashtableEntry = *currentHashtableEntry;

			while (hashtableEntry) {
				_SkinHashTableEntry *nextEntry = hashtableEntry->next;
				_SkinHashTableEntry_dispose(hashtableEntry);
				hashtableEntry = nextEntry;
			}

			SUB_CAST(_sp4Skin, self)->entriesHashTable[i] = 0;
		}
	}

	sp4BoneDataArray_clear(self->bones);
	sp4IkConstraintDataArray_clear(self->ikConstraints);
	sp4TransformConstraintDataArray_clear(self->transformConstraints);
	sp4PathConstraintDataArray_clear(self->pathConstraints);
}
