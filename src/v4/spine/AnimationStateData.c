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

#include <v4/spine/AnimationStateData.h>
#include <v4/spine/extension.h>

typedef struct _ToEntry _ToEntry;
struct _ToEntry {
	sp4Animation *animation;
	float duration;
	_ToEntry *next;
};

_ToEntry *_4_ToEntry_create(sp4Animation *to, float duration) {
	_ToEntry *self = NEW(_ToEntry);
	self->animation = to;
	self->duration = duration;
	return self;
}

void _4_ToEntry_dispose(_ToEntry *self) {
	FREE(self);
}

/**/

typedef struct _FromEntry _FromEntry;
struct _FromEntry {
	sp4Animation *animation;
	_ToEntry *toEntries;
	_FromEntry *next;
};

_FromEntry *_4_FromEntry_create(sp4Animation *from) {
	_FromEntry *self = NEW(_FromEntry);
	self->animation = from;
	return self;
}

void _4_FromEntry_dispose(_FromEntry *self) {
	FREE(self);
}

/**/

sp4AnimationStateData *sp4AnimationStateData_create(sp4SkeletonData *skeletonData) {
	sp4AnimationStateData *self = NEW(sp4AnimationStateData);
	CONST_CAST(sp4SkeletonData *, self->skeletonData) = skeletonData;
	return self;
}

void sp4AnimationStateData_dispose(sp4AnimationStateData *self) {
	_ToEntry *toEntry;
	_ToEntry *nextToEntry;
	_FromEntry *nextFromEntry;

	_FromEntry *fromEntry = (_FromEntry *) self->entries;
	while (fromEntry) {
		toEntry = fromEntry->toEntries;
		while (toEntry) {
			nextToEntry = toEntry->next;
			_4_ToEntry_dispose(toEntry);
			toEntry = nextToEntry;
		}
		nextFromEntry = fromEntry->next;
		_4_FromEntry_dispose(fromEntry);
		fromEntry = nextFromEntry;
	}

	FREE(self);
}

void sp4AnimationStateData_setMixByName(sp4AnimationStateData *self, const char *fromName, const char *toName,
									   float duration) {
	sp4Animation *to;
	sp4Animation *from = sp4SkeletonData_findAnimation(self->skeletonData, fromName);
	if (!from) return;
	to = sp4SkeletonData_findAnimation(self->skeletonData, toName);
	if (!to) return;
	sp4AnimationStateData_setMix(self, from, to, duration);
}

void sp4AnimationStateData_setMix(sp4AnimationStateData *self, sp4Animation *from, sp4Animation *to, float duration) {
	/* Find existing FromEntry. */
	_ToEntry *toEntry;
	_FromEntry *fromEntry = (_FromEntry *) self->entries;
	while (fromEntry) {
		if (fromEntry->animation == from) {
			/* Find existing ToEntry. */
			toEntry = fromEntry->toEntries;
			while (toEntry) {
				if (toEntry->animation == to) {
					toEntry->duration = duration;
					return;
				}
				toEntry = toEntry->next;
			}
			break; /* Add new ToEntry to the existing FromEntry. */
		}
		fromEntry = fromEntry->next;
	}
	if (!fromEntry) {
		fromEntry = _4_FromEntry_create(from);
		fromEntry->next = (_FromEntry *) self->entries;
		CONST_CAST(_FromEntry *, self->entries) = fromEntry;
	}
	toEntry = _4_ToEntry_create(to, duration);
	toEntry->next = fromEntry->toEntries;
	fromEntry->toEntries = toEntry;
}

float sp4AnimationStateData_getMix(sp4AnimationStateData *self, sp4Animation *from, sp4Animation *to) {
	_FromEntry *fromEntry = (_FromEntry *) self->entries;
	while (fromEntry) {
		if (fromEntry->animation == from) {
			_ToEntry *toEntry = fromEntry->toEntries;
			while (toEntry) {
				if (toEntry->animation == to) return toEntry->duration;
				toEntry = toEntry->next;
			}
		}
		fromEntry = fromEntry->next;
	}
	return self->defaultMix;
}
