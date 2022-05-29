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

#ifndef SPINE_V4_SKELETONBOUNDS_H_
#define SPINE_V4_SKELETONBOUNDS_H_

#include <v4/spine/dll.h>
#include <v4/spine/BoundingBoxAttachment.h>
#include <v4/spine/Skeleton.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sp4Polygon {
	float *const vertices;
	int count;
	int capacity;
} sp4Polygon;

SP4_API sp4Polygon *sp4Polygon_create(int capacity);

SP4_API void sp4Polygon_dispose(sp4Polygon *self);

SP4_API int/*bool*/sp4Polygon_containsPoint(sp4Polygon *polygon, float x, float y);

SP4_API int/*bool*/sp4Polygon_intersectsSegment(sp4Polygon *polygon, float x1, float y1, float x2, float y2);

/**/

typedef struct sp4SkeletonBounds {
	int count;
	sp4BoundingBoxAttachment **boundingBoxes;
	sp4Polygon **polygons;

	float minX, minY, maxX, maxY;
} sp4SkeletonBounds;

SP4_API sp4SkeletonBounds *sp4SkeletonBounds_create();

SP4_API void sp4SkeletonBounds_dispose(sp4SkeletonBounds *self);

SP4_API void sp4SkeletonBounds_update(sp4SkeletonBounds *self, sp4Skeleton *skeleton, int/*bool*/updateAabb);

/** Returns true if the axis aligned bounding box contains the point. */
SP4_API int/*bool*/sp4SkeletonBounds_aabbContainsPoint(sp4SkeletonBounds *self, float x, float y);

/** Returns true if the axis aligned bounding box intersects the line segment. */
SP4_API int/*bool*/
sp4SkeletonBounds_aabbIntersectsSegment(sp4SkeletonBounds *self, float x1, float y1, float x2, float y2);

/** Returns true if the axis aligned bounding box intersects the axis aligned bounding box of the specified bounds. */
SP4_API int/*bool*/sp4SkeletonBounds_aabbIntersectsSkeleton(sp4SkeletonBounds *self, sp4SkeletonBounds *bounds);

/** Returns the first bounding box attachment that contains the point, or null. When doing many checks, it is usually more
 * efficient to only call this method if sp4SkeletonBounds_aabbContainsPoint returns true. */
SP4_API sp4BoundingBoxAttachment *sp4SkeletonBounds_containsPoint(sp4SkeletonBounds *self, float x, float y);

/** Returns the first bounding box attachment that contains the line segment, or null. When doing many checks, it is usually
 * more efficient to only call this method if sp4SkeletonBounds_aabbIntersectsSegment returns true. */
SP4_API sp4BoundingBoxAttachment *
sp4SkeletonBounds_intersectsSegment(sp4SkeletonBounds *self, float x1, float y1, float x2, float y2);

/** Returns the polygon for the specified bounding box, or null. */
SP4_API sp4Polygon *sp4SkeletonBounds_getPolygon(sp4SkeletonBounds *self, sp4BoundingBoxAttachment *boundingBox);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_V4_SKELETONBOUNDS_H_ */
