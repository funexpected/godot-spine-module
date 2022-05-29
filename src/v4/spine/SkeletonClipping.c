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

#include <v4/spine/SkeletonClipping.h>
#include <v4/spine/extension.h>

sp4SkeletonClipping *sp4SkeletonClipping_create() {
	sp4SkeletonClipping *clipping = CALLOC(sp4SkeletonClipping, 1);

	clipping->triangulator = sp4Triangulator_create();
	clipping->clippingPolygon = sp4FloatArray_create(128);
	clipping->clipOutput = sp4FloatArray_create(128);
	clipping->clippedVertices = sp4FloatArray_create(128);
	clipping->clippedUVs = sp4FloatArray_create(128);
	clipping->clippedTriangles = sp4UnsignedShortArray_create(128);
	clipping->scratch = sp4FloatArray_create(128);

	return clipping;
}

void sp4SkeletonClipping_dispose(sp4SkeletonClipping *self) {
	sp4Triangulator_dispose(self->triangulator);
	sp4FloatArray_dispose(self->clippingPolygon);
	sp4FloatArray_dispose(self->clipOutput);
	sp4FloatArray_dispose(self->clippedVertices);
	sp4FloatArray_dispose(self->clippedUVs);
	sp4UnsignedShortArray_dispose(self->clippedTriangles);
	sp4FloatArray_dispose(self->scratch);
	FREE(self);
}

static void _makeClockwise(sp4FloatArray *polygon) {
	int i, n, lastX;
	float *vertices = polygon->items;
	int verticeslength = polygon->size;

	float area =
				  vertices[verticeslength - 2] * vertices[1] - vertices[0] * vertices[verticeslength - 1],
		  p1x, p1y, p2x, p2y;
	for (i = 0, n = verticeslength - 3; i < n; i += 2) {
		p1x = vertices[i];
		p1y = vertices[i + 1];
		p2x = vertices[i + 2];
		p2y = vertices[i + 3];
		area += p1x * p2y - p2x * p1y;
	}
	if (area < 0) return;

	for (i = 0, lastX = verticeslength - 2, n = verticeslength >> 1; i < n; i += 2) {
		float x = vertices[i], y = vertices[i + 1];
		int other = lastX - i;
		vertices[i] = vertices[other];
		vertices[i + 1] = vertices[other + 1];
		vertices[other] = x;
		vertices[other + 1] = y;
	}
}

int sp4SkeletonClipping_clipStart(sp4SkeletonClipping *self, sp4Slot *slot, sp4ClippingAttachment *clip) {
	int i, n;
	float *vertices;
	if (self->clipAttachment) return 0;
	self->clipAttachment = clip;

	n = clip->super.worldVerticesLength;
	vertices = sp4FloatArray_setSize(self->clippingPolygon, n)->items;
	sp4VertexAttachment_computeWorldVertices(SUPER(clip), slot, 0, n, vertices, 0, 2);
	_makeClockwise(self->clippingPolygon);
	self->clippingPolygons = sp4Triangulator_decompose(self->triangulator, self->clippingPolygon,
													  sp4Triangulator_triangulate(self->triangulator,
																				 self->clippingPolygon));
	for (i = 0, n = self->clippingPolygons->size; i < n; i++) {
		sp4FloatArray *polygon = self->clippingPolygons->items[i];
		_makeClockwise(polygon);
		sp4FloatArray_add(polygon, polygon->items[0]);
		sp4FloatArray_add(polygon, polygon->items[1]);
	}
	return self->clippingPolygons->size;
}

void sp4SkeletonClipping_clipEnd(sp4SkeletonClipping *self, sp4Slot *slot) {
	if (self->clipAttachment != 0 && self->clipAttachment->endSlot == slot->data) sp4SkeletonClipping_clipEnd2(self);
}

void sp4SkeletonClipping_clipEnd2(sp4SkeletonClipping *self) {
	if (!self->clipAttachment) return;
	self->clipAttachment = 0;
	self->clippingPolygons = 0;
	sp4FloatArray_clear(self->clippedVertices);
	sp4FloatArray_clear(self->clippedUVs);
	sp4UnsignedShortArray_clear(self->clippedTriangles);
	sp4FloatArray_clear(self->clippingPolygon);
}

int /*boolean*/ sp4SkeletonClipping_isClipping(sp4SkeletonClipping *self) {
	return self->clipAttachment != 0;
}

int /*boolean*/
_4_clip(sp4SkeletonClipping *self, float x1, float y1, float x2, float y2, float x3, float y3, sp4FloatArray *clippingArea,
	  sp4FloatArray *output) {
	int i;
	sp4FloatArray *originalOutput = output;
	int clipped = 0;
	float *clippingVertices;
	int clippingVerticesLast;

	sp4FloatArray *input = 0;
	if (clippingArea->size % 4 >= 2) {
		input = output;
		output = self->scratch;
	} else
		input = self->scratch;

	sp4FloatArray_clear(input);
	sp4FloatArray_add(input, x1);
	sp4FloatArray_add(input, y1);
	sp4FloatArray_add(input, x2);
	sp4FloatArray_add(input, y2);
	sp4FloatArray_add(input, x3);
	sp4FloatArray_add(input, y3);
	sp4FloatArray_add(input, x1);
	sp4FloatArray_add(input, y1);
	sp4FloatArray_clear(output);

	clippingVertices = clippingArea->items;
	clippingVerticesLast = clippingArea->size - 4;
	for (i = 0;; i += 2) {
		int ii;
		sp4FloatArray *temp;
		float edgeX = clippingVertices[i], edgeY = clippingVertices[i + 1];
		float edgeX2 = clippingVertices[i + 2], edgeY2 = clippingVertices[i + 3];
		float deltaX = edgeX - edgeX2, deltaY = edgeY - edgeY2;

		float *inputVertices = input->items;
		int inputVerticesLength = input->size - 2, outputStart = output->size;
		for (ii = 0; ii < inputVerticesLength; ii += 2) {
			float inputX = inputVertices[ii], inputY = inputVertices[ii + 1];
			float inputX2 = inputVertices[ii + 2], inputY2 = inputVertices[ii + 3];
			int side2 = deltaX * (inputY2 - edgeY2) - deltaY * (inputX2 - edgeX2) > 0;
			if (deltaX * (inputY - edgeY2) - deltaY * (inputX - edgeX2) > 0) {
				float c0, c2;
				float s, ua;
				if (side2) {
					sp4FloatArray_add(output, inputX2);
					sp4FloatArray_add(output, inputY2);
					continue;
				}
				c0 = inputY2 - inputY, c2 = inputX2 - inputX;
				s = c0 * (edgeX2 - edgeX) - c2 * (edgeY2 - edgeY);
				if (ABS(s) > 0.000001f) {
					ua = (c2 * (edgeY - inputY) - c0 * (edgeX - inputX)) / s;
					sp4FloatArray_add(output, edgeX + (edgeX2 - edgeX) * ua);
					sp4FloatArray_add(output, edgeY + (edgeY2 - edgeY) * ua);
				} else {
					sp4FloatArray_add(output, edgeX);
					sp4FloatArray_add(output, edgeY);
				}
			} else if (side2) {
				float c0 = inputY2 - inputY, c2 = inputX2 - inputX;
				float s = c0 * (edgeX2 - edgeX) - c2 * (edgeY2 - edgeY);
				if (ABS(s) > 0.000001f) {
					float ua = (c2 * (edgeY - inputY) - c0 * (edgeX - inputX)) / s;
					sp4FloatArray_add(output, edgeX + (edgeX2 - edgeX) * ua);
					sp4FloatArray_add(output, edgeY + (edgeY2 - edgeY) * ua);
				} else {
					sp4FloatArray_add(output, edgeX);
					sp4FloatArray_add(output, edgeY);
				}
				sp4FloatArray_add(output, inputX2);
				sp4FloatArray_add(output, inputY2);
			}
			clipped = 1;
		}

		if (outputStart == output->size) {
			sp4FloatArray_clear(originalOutput);
			return 1;
		}

		sp4FloatArray_add(output, output->items[0]);
		sp4FloatArray_add(output, output->items[1]);

		if (i == clippingVerticesLast) break;
		temp = output;
		output = input;
		sp4FloatArray_clear(output);
		input = temp;
	}

	if (originalOutput != output) {
		sp4FloatArray_clear(originalOutput);
		sp4FloatArray_addAllValues(originalOutput, output->items, 0, output->size - 2);
	} else
		sp4FloatArray_setSize(originalOutput, originalOutput->size - 2);

	return clipped;
}

void sp4SkeletonClipping_clipTriangles(sp4SkeletonClipping *self, float *vertices, int verticesLength,
									  unsigned short *triangles, int trianglesLength, float *uvs, int stride) {
	int i;
	sp4FloatArray *clipOutput = self->clipOutput;
	sp4FloatArray *clippedVertices = self->clippedVertices;
	sp4FloatArray *clippedUVs = self->clippedUVs;
	sp4UnsignedShortArray *clippedTriangles = self->clippedTriangles;
	sp4FloatArray **polygons = self->clippingPolygons->items;
	int polygonsCount = self->clippingPolygons->size;

	short index = 0;
	sp4FloatArray_clear(clippedVertices);
	sp4FloatArray_clear(clippedUVs);
	sp4UnsignedShortArray_clear(clippedTriangles);
	i = 0;
continue_outer:
	for (; i < trianglesLength; i += 3) {
		int p;
		int vertexOffset = triangles[i] * stride;
		float x2, y2, u2, v2, x3, y3, u3, v3;
		float x1 = vertices[vertexOffset], y1 = vertices[vertexOffset + 1];
		float u1 = uvs[vertexOffset], v1 = uvs[vertexOffset + 1];

		vertexOffset = triangles[i + 1] * stride;
		x2 = vertices[vertexOffset];
		y2 = vertices[vertexOffset + 1];
		u2 = uvs[vertexOffset];
		v2 = uvs[vertexOffset + 1];

		vertexOffset = triangles[i + 2] * stride;
		x3 = vertices[vertexOffset];
		y3 = vertices[vertexOffset + 1];
		u3 = uvs[vertexOffset];
		v3 = uvs[vertexOffset + 1];

		for (p = 0; p < polygonsCount; p++) {
			int s = clippedVertices->size;
			if (_4_clip(self, x1, y1, x2, y2, x3, y3, polygons[p], clipOutput)) {
				int ii;
				float d0, d1, d2, d4, d;
				unsigned short *clippedTrianglesItems;
				int clipOutputCount;
				float *clipOutputItems;
				float *clippedVerticesItems;
				float *clippedUVsItems;

				int clipOutputLength = clipOutput->size;
				if (clipOutputLength == 0) continue;
				d0 = y2 - y3;
				d1 = x3 - x2;
				d2 = x1 - x3;
				d4 = y3 - y1;
				d = 1 / (d0 * d2 + d1 * (y1 - y3));

				clipOutputCount = clipOutputLength >> 1;
				clipOutputItems = clipOutput->items;
				clippedVerticesItems = sp4FloatArray_setSize(clippedVertices, s + (clipOutputCount << 1))->items;
				clippedUVsItems = sp4FloatArray_setSize(clippedUVs, s + (clipOutputCount << 1))->items;
				for (ii = 0; ii < clipOutputLength; ii += 2) {
					float c0, c1, a, b, c;
					float x = clipOutputItems[ii], y = clipOutputItems[ii + 1];
					clippedVerticesItems[s] = x;
					clippedVerticesItems[s + 1] = y;
					c0 = x - x3;
					c1 = y - y3;
					a = (d0 * c0 + d1 * c1) * d;
					b = (d4 * c0 + d2 * c1) * d;
					c = 1 - a - b;
					clippedUVsItems[s] = u1 * a + u2 * b + u3 * c;
					clippedUVsItems[s + 1] = v1 * a + v2 * b + v3 * c;
					s += 2;
				}

				s = clippedTriangles->size;
				clippedTrianglesItems = sp4UnsignedShortArray_setSize(clippedTriangles,
																	 s + 3 * (clipOutputCount - 2))
												->items;
				clipOutputCount--;
				for (ii = 1; ii < clipOutputCount; ii++) {
					clippedTrianglesItems[s] = index;
					clippedTrianglesItems[s + 1] = (unsigned short) (index + ii);
					clippedTrianglesItems[s + 2] = (unsigned short) (index + ii + 1);
					s += 3;
				}
				index += clipOutputCount + 1;

			} else {
				unsigned short *clippedTrianglesItems;
				float *clippedVerticesItems = sp4FloatArray_setSize(clippedVertices, s + (3 << 1))->items;
				float *clippedUVsItems = sp4FloatArray_setSize(clippedUVs, s + (3 << 1))->items;
				clippedVerticesItems[s] = x1;
				clippedVerticesItems[s + 1] = y1;
				clippedVerticesItems[s + 2] = x2;
				clippedVerticesItems[s + 3] = y2;
				clippedVerticesItems[s + 4] = x3;
				clippedVerticesItems[s + 5] = y3;

				clippedUVsItems[s] = u1;
				clippedUVsItems[s + 1] = v1;
				clippedUVsItems[s + 2] = u2;
				clippedUVsItems[s + 3] = v2;
				clippedUVsItems[s + 4] = u3;
				clippedUVsItems[s + 5] = v3;

				s = clippedTriangles->size;
				clippedTrianglesItems = sp4UnsignedShortArray_setSize(clippedTriangles, s + 3)->items;
				clippedTrianglesItems[s] = index;
				clippedTrianglesItems[s + 1] = (unsigned short) (index + 1);
				clippedTrianglesItems[s + 2] = (unsigned short) (index + 2);
				index += 3;
				i += 3;
				goto continue_outer;
			}
		}
	}
	UNUSED(verticesLength);
}
