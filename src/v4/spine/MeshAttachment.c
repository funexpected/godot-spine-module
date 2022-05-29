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

#include <v4/spine/MeshAttachment.h>
#include <v4/spine/extension.h>
#include <stdio.h>

void _sp4MeshAttachment_dispose(sp4Attachment *attachment) {
	sp4MeshAttachment *self = SUB_CAST(sp4MeshAttachment, attachment);
	FREE(self->path);
	FREE(self->uvs);
	if (!self->parentMesh) {
		_sp4VertexAttachment_deinit(SUPER(self));
		FREE(self->regionUVs);
		FREE(self->triangles);
		FREE(self->edges);
	} else
		_sp4Attachment_deinit(attachment);
	FREE(self);
}

sp4Attachment *_sp4MeshAttachment_copy(sp4Attachment *attachment) {
	sp4MeshAttachment *copy;
	sp4MeshAttachment *self = SUB_CAST(sp4MeshAttachment, attachment);
	if (self->parentMesh)
		return SUPER(SUPER(sp4MeshAttachment_newLinkedMesh(self)));
	copy = sp4MeshAttachment_create(attachment->name);
	copy->rendererObject = self->rendererObject;
	copy->regionU = self->regionU;
	copy->regionV = self->regionV;
	copy->regionU2 = self->regionU2;
	copy->regionV2 = self->regionV2;
	copy->regionDegrees = self->regionDegrees;
	copy->regionOffsetX = self->regionOffsetX;
	copy->regionOffsetY = self->regionOffsetY;
	copy->regionWidth = self->regionWidth;
	copy->regionHeight = self->regionHeight;
	copy->regionOriginalWidth = self->regionOriginalWidth;
	copy->regionOriginalHeight = self->regionOriginalHeight;
	MALLOC_STR(copy->path, self->path);
	sp4Color_setFromColor(&copy->color, &self->color);

	sp4VertexAttachment_copyTo(SUPER(self), SUPER(copy));
	copy->regionUVs = MALLOC(float, SUPER(self)->worldVerticesLength);
	memcpy(copy->regionUVs, self->regionUVs, SUPER(self)->worldVerticesLength * sizeof(float));
	copy->uvs = MALLOC(float, SUPER(self)->worldVerticesLength);
	memcpy(copy->uvs, self->uvs, SUPER(self)->worldVerticesLength * sizeof(float));
	copy->trianglesCount = self->trianglesCount;
	copy->triangles = MALLOC(unsigned short, self->trianglesCount);
	memcpy(copy->triangles, self->triangles, self->trianglesCount * sizeof(short));
	copy->hullLength = self->hullLength;
	if (self->edgesCount > 0) {
		copy->edgesCount = self->edgesCount;
		copy->edges = MALLOC(int, self->edgesCount);
		memcpy(copy->edges, self->edges, self->edgesCount * sizeof(int));
	}
	copy->width = self->width;
	copy->height = self->height;

	return SUPER(SUPER(copy));
}

sp4MeshAttachment *sp4MeshAttachment_newLinkedMesh(sp4MeshAttachment *self) {
	sp4MeshAttachment *copy = sp4MeshAttachment_create(self->super.super.name);

	copy->rendererObject = self->rendererObject;
	copy->regionU = self->regionU;
	copy->regionV = self->regionV;
	copy->regionU2 = self->regionU2;
	copy->regionV2 = self->regionV2;
	copy->regionDegrees = self->regionDegrees;
	copy->regionOffsetX = self->regionOffsetX;
	copy->regionOffsetY = self->regionOffsetY;
	copy->regionWidth = self->regionWidth;
	copy->regionHeight = self->regionHeight;
	copy->regionOriginalWidth = self->regionOriginalWidth;
	copy->regionOriginalHeight = self->regionOriginalHeight;
	MALLOC_STR(copy->path, self->path);
	sp4Color_setFromColor(&copy->color, &self->color);
	copy->super.deformAttachment = self->super.deformAttachment;
	sp4MeshAttachment_setParentMesh(copy, self->parentMesh ? self->parentMesh : self);
	sp4MeshAttachment_updateUVs(copy);
	return copy;
}

sp4MeshAttachment *sp4MeshAttachment_create(const char *name) {
	sp4MeshAttachment *self = NEW(sp4MeshAttachment);
	_sp4VertexAttachment_init(SUPER(self));
	sp4Color_setFromFloats(&self->color, 1, 1, 1, 1);
	_sp4Attachment_init(SUPER(SUPER(self)), name, SP4_ATTACHMENT_MESH, _sp4MeshAttachment_dispose, _sp4MeshAttachment_copy);
	return self;
}

void sp4MeshAttachment_updateUVs(sp4MeshAttachment *self) {
	int i, n;
	float *uvs;
	float u, v, width, height;
	int verticesLength = SUPER(self)->worldVerticesLength;
	FREE(self->uvs);
	uvs = self->uvs = MALLOC(float, verticesLength);
	n = verticesLength;
	u = self->regionU;
	v = self->regionV;

	switch (self->regionDegrees) {
		case 90: {
			float textureWidth = self->regionHeight / (self->regionU2 - self->regionU);
			float textureHeight = self->regionWidth / (self->regionV2 - self->regionV);
			u -= (self->regionOriginalHeight - self->regionOffsetY - self->regionHeight) / textureWidth;
			v -= (self->regionOriginalWidth - self->regionOffsetX - self->regionWidth) / textureHeight;
			width = self->regionOriginalHeight / textureWidth;
			height = self->regionOriginalWidth / textureHeight;
			for (i = 0; i < n; i += 2) {
				uvs[i] = u + self->regionUVs[i + 1] * width;
				uvs[i + 1] = v + (1 - self->regionUVs[i]) * height;
			}
			return;
		}
		case 180: {
			float textureWidth = self->regionWidth / (self->regionU2 - self->regionU);
			float textureHeight = self->regionHeight / (self->regionV2 - self->regionV);
			u -= (self->regionOriginalWidth - self->regionOffsetX - self->regionWidth) / textureWidth;
			v -= self->regionOffsetY / textureHeight;
			width = self->regionOriginalWidth / textureWidth;
			height = self->regionOriginalHeight / textureHeight;
			for (i = 0; i < n; i += 2) {
				uvs[i] = u + (1 - self->regionUVs[i]) * width;
				uvs[i + 1] = v + (1 - self->regionUVs[i + 1]) * height;
			}
			return;
		}
		case 270: {
			float textureHeight = self->regionHeight / (self->regionV2 - self->regionV);
			float textureWidth = self->regionWidth / (self->regionU2 - self->regionU);
			u -= self->regionOffsetY / textureWidth;
			v -= self->regionOffsetX / textureHeight;
			width = self->regionOriginalHeight / textureWidth;
			height = self->regionOriginalWidth / textureHeight;
			for (i = 0; i < n; i += 2) {
				uvs[i] = u + (1 - self->regionUVs[i + 1]) * width;
				uvs[i + 1] = v + self->regionUVs[i] * height;
			}
			return;
		}
		default: {
			float textureWidth = self->regionWidth / (self->regionU2 - self->regionU);
			float textureHeight = self->regionHeight / (self->regionV2 - self->regionV);
			u -= self->regionOffsetX / textureWidth;
			v -= (self->regionOriginalHeight - self->regionOffsetY - self->regionHeight) / textureHeight;
			width = self->regionOriginalWidth / textureWidth;
			height = self->regionOriginalHeight / textureHeight;
			for (i = 0; i < n; i += 2) {
				uvs[i] = u + self->regionUVs[i] * width;
				uvs[i + 1] = v + self->regionUVs[i + 1] * height;
			}
		}
	}
}

void sp4MeshAttachment_setParentMesh(sp4MeshAttachment *self, sp4MeshAttachment *parentMesh) {
	CONST_CAST(sp4MeshAttachment *, self->parentMesh) = parentMesh;
	if (parentMesh) {
		self->super.bones = parentMesh->super.bones;
		self->super.bonesCount = parentMesh->super.bonesCount;

		self->super.vertices = parentMesh->super.vertices;
		self->super.verticesCount = parentMesh->super.verticesCount;

		self->regionUVs = parentMesh->regionUVs;

		self->triangles = parentMesh->triangles;
		self->trianglesCount = parentMesh->trianglesCount;

		self->hullLength = parentMesh->hullLength;

		self->super.worldVerticesLength = parentMesh->super.worldVerticesLength;

		self->edges = parentMesh->edges;
		self->edgesCount = parentMesh->edgesCount;

		self->width = parentMesh->width;
		self->height = parentMesh->height;
	}
}
