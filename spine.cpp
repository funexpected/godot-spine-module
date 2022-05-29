/******************************************************************************
 * Spine Runtimes Software License v2.5
 *
 * Copyright (c) 2013-2016, Esoteric Software
 * All rights reserved.
 *
 * You are granted a perpetual, non-exclusive, non-sublicensable, and
 * non-transferable license to use, install, execute, and perform the Spine
 * Runtimes software and derivative works solely for personal or internal
 * use. Without the written permission of Esoteric Software (see Section 2 of
 * the Spine Software License Agreement), you may not (a) modify, translate,
 * adapt, or develop new applications using the Spine Runtimes or otherwise
 * create derivative works or improvements of the Spine Runtimes or (b) remove,
 * delete, alter, or obscure any trademarks or any copyright, trademark, patent,
 * or other intellectual property or proprietary rights notices on or in the
 * Software, including any copy thereof. Redistributions in binary or source
 * form must include this license and terms.
 *
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, BUSINESS INTERRUPTION, OR LOSS OF
 * USE, DATA, OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
#ifdef MODULE_SPINE_ENABLED

#include "spine.h"
#include "core/io/resource_loader.h"
#include "scene/2d/collision_object_2d.h"
#include "scene/resources/convex_polygon_shape_2d.h"
#include <core/engine.h>

#include <v3/spine/extension.h>
#include <v3/spine/spine.h>

#include <core/method_bind_ext.gen.inc>

VARIANT_ENUM_CAST(Spine::AnimationProcessMode);
VARIANT_ENUM_CAST(Spine::DebugAttachmentMode);

VARIANT_ENUM_CAST(Spine4::AnimationProcessMode);
VARIANT_ENUM_CAST(Spine4::DebugAttachmentMode);

Spine::SpineResource::SpineResource() {

	atlas = NULL;
	data = NULL;
}

Spine::SpineResource::~SpineResource() {

	if (atlas != NULL)
		spAtlas_dispose(atlas);

	if (data != NULL)
		spSkeletonData_dispose(data);
}

Array *Spine::invalid_names = NULL;
Array Spine::get_invalid_names() {
	if (invalid_names == NULL) {
		invalid_names = memnew(Array());
	}
	return *invalid_names;
}

void Spine::spine_animation_callback(spAnimationState *p_state, spEventType p_type, spTrackEntry *p_track, spEvent *p_event) {

	((Spine *)p_state->rendererObject)->_on_animation_state_event(p_track->trackIndex, p_type, p_event, 1);
}

String Spine::build_state_hash() {
	if (!state) return "";
	PoolStringArray items;
	for (int i=0; i < state->tracksCount; i++) {
		spTrackEntry* track = state->tracks[i];
		if (!track || !track->animation) continue;

		items.push_back(track->animation->name);
		items.push_back(track->loop ?
			String::num(FMOD(track->trackTime, track->animation->duration), 3) :
			String::num(MIN(track->trackTime, track->animation->duration), 3)
		);
	}
	return items.join("::");
}

void Spine::_on_animation_state_event(int p_track, spEventType p_type, spEvent *p_event, int p_loop_count) {

	switch (p_type) {
		case SP_ANIMATION_START:
			emit_signal("animation_start", p_track);
			break;
		case SP_ANIMATION_COMPLETE:
			emit_signal("animation_complete", p_track, p_loop_count);
			break;
		case SP_ANIMATION_EVENT: {
			Dictionary event;
			event["name"] = p_event->data->name;
			event["int"] = p_event->intValue;
			event["float"] = p_event->floatValue;
			event["string"] = p_event->stringValue ? p_event->stringValue : "";
			emit_signal("animation_event", p_track, event);
		} break;
		case SP_ANIMATION_END:
			emit_signal("animation_end", p_track);
			break;
	}
}

void Spine::_spine_dispose() {

	if (playing) {
		// stop first
		stop();
	}

	if (state) {

		spAnimationStateData_dispose(state->data);
		spAnimationState_dispose(state);
	}

	if (skeleton)
		spSkeleton_dispose(skeleton);

	if (clipper)
		spSkeletonClipping_dispose(clipper);

	state = NULL;
	skeleton = NULL;
	clipper = NULL;
	res = RES();

	for (AttachmentNodes::Element *E = attachment_nodes.front(); E; E = E->next()) {

		AttachmentNode &node = E->get();
		memdelete(node.ref);
	}
	attachment_nodes.clear();

	update();
}

static Ref<Texture> spine_get_texture(spRegionAttachment *attachment) {

	if (Ref<Texture> *ref = static_cast<Ref<Texture> *>(((spAtlasRegion *)attachment->rendererObject)->page->rendererObject))
		return *ref;
	return NULL;
}

static Ref<Texture> spine_get_texture(spMeshAttachment *attachment) {

	if (Ref<Texture> *ref = static_cast<Ref<Texture> *>(((spAtlasRegion *)attachment->rendererObject)->page->rendererObject))
		return *ref;
	return NULL;
}

void Spine::_on_fx_draw() {

	if (skeleton == NULL)
		return;
	fx_batcher.reset();
	RID eci = fx_node->get_canvas_item();
	// VisualServer::get_singleton()->canvas_item_add_set_blend_mode(eci, VS::MaterialBlendMode(fx_node->get_blend_mode()));
	fx_batcher.flush();
}

void Spine::_animation_draw() {
	if (skeleton == NULL)
		return;

	spColor_setFromFloats(&skeleton->color, modulate.r, modulate.g, modulate.b, modulate.a);

	int additive = 0;
	int fx_additive = 0;
	Color color;
	float *uvs = NULL;
	int verties_count = 0;
	unsigned short *triangles = NULL;
	int triangles_count = 0;
	float r = 0, g = 0, b = 0, a = 0;

	RID ci = this->get_canvas_item();
	batcher.reset();
	// VisualServer::get_singleton()->canvas_item_add_set_blend_mode(ci, VS::MaterialBlendMode(get_blend_mode()));

	const char *fx_prefix = fx_slot_prefix.get_data();

	for (int i = 0, n = skeleton->slotsCount; i < n; i++) {

		spSlot *slot = skeleton->drawOrder[i];
		if (!slot->attachment || slot->color.a == 0) {
			spSkeletonClipping_clipEnd(clipper, slot);
			continue;
		}
		bool is_fx = false;
		Ref<Texture> texture;
		switch (slot->attachment->type) {

			case SP_ATTACHMENT_REGION: {

				spRegionAttachment *attachment = (spRegionAttachment *)slot->attachment;
				if (attachment->color.a == 0){
					spSkeletonClipping_clipEnd(clipper, slot);
					continue;
				}
				is_fx = strstr(attachment->path, fx_prefix) != NULL;
				spRegionAttachment_computeWorldVertices(attachment, slot->bone, world_verts.ptrw(), 0, 2);
				texture = spine_get_texture(attachment);
				uvs = attachment->uvs;
				verties_count = 8;
				static unsigned short quadTriangles[6] = { 0, 1, 2, 2, 3, 0 };
				triangles = quadTriangles;
				triangles_count = 6;
				r = attachment->color.r;
				g = attachment->color.g;
				b = attachment->color.b;
				a = attachment->color.a;
				break;
			}
			case SP_ATTACHMENT_MESH: {

				spMeshAttachment *attachment = (spMeshAttachment *)slot->attachment;
				if (attachment->color.a == 0){
					spSkeletonClipping_clipEnd(clipper, slot);
					continue;
				}
				is_fx = strstr(attachment->path, fx_prefix) != NULL;
				spVertexAttachment_computeWorldVertices(SUPER(attachment), slot, 0, attachment->super.worldVerticesLength, world_verts.ptrw(), 0, 2);
				texture = spine_get_texture(attachment);
				uvs = attachment->uvs;
				verties_count = ((spVertexAttachment *)attachment)->worldVerticesLength;

				triangles = attachment->triangles;
				triangles_count = attachment->trianglesCount;
				r = attachment->color.r;
				g = attachment->color.g;
				b = attachment->color.b;
				a = attachment->color.a;
				break;
			}

			case SP_ATTACHMENT_CLIPPING: {
				spClippingAttachment *attachment = (spClippingAttachment *)slot->attachment;
				spSkeletonClipping_clipStart(clipper, slot, attachment);
				continue;
			}

			default: {
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}
		}
		if (texture.is_null()){
			spSkeletonClipping_clipEnd(clipper, slot);
			continue;
		}
		/*
		if (is_fx && slot->data->blendMode != fx_additive) {

			fx_batcher.add_set_blender_mode(slot->data->additiveBlending
				? VisualServer::MATERIAL_BLEND_MODE_ADD
				: get_blend_mode()
			);
			fx_additive = slot->data->additiveBlending;
		}
		else if (slot->data->additiveBlending != additive) {

			batcher.add_set_blender_mode(slot->data->additiveBlending
				? VisualServer::MATERIAL_BLEND_MODE_ADD
				: fx_node->get_blend_mode()
			);
			additive = slot->data->additiveBlending;
		}
		 */

		color.a = skeleton->color.a * slot->color.a * a;
		color.r = skeleton->color.r * slot->color.r * r;
		color.g = skeleton->color.g * slot->color.g * g;
		color.b = skeleton->color.b * slot->color.b * b;

		if (spSkeletonClipping_isClipping(clipper)){
			spSkeletonClipping_clipTriangles(clipper, world_verts.ptrw(), verties_count, triangles, triangles_count, uvs, 2);
			if (clipper->clippedTriangles->size == 0){
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}
			batcher.add(texture, clipper->clippedVertices->items,
								 clipper->clippedUVs->items,
								 clipper->clippedVertices->size,
								 clipper->clippedTriangles->items,
								 clipper->clippedTriangles->size,
								 &color, flip_x, flip_y, (slot->data->index)*individual_textures);
		} else if (is_fx) {
			fx_batcher.add(texture, world_verts.ptr(), uvs, verties_count, triangles, triangles_count, &color, flip_x, flip_y, (slot->data->index)*individual_textures);
		} else {
			batcher.add(texture, world_verts.ptr(), uvs, verties_count, triangles, triangles_count, &color, flip_x, flip_y, (slot->data->index)*individual_textures);
		}
		spSkeletonClipping_clipEnd(clipper, slot);
	}


	spSkeletonClipping_clipEnd2(clipper);
	performance_triangles_drawn = performance_triangles_generated = batcher.triangles_count();
	batcher.flush();
	fx_node->update();

	// Slots.
	if (debug_attachment_region || debug_attachment_mesh || debug_attachment_skinned_mesh || debug_attachment_bounding_box) {

		Color color(0, 0, 1, 1);
		for (int i = 0, n = skeleton->slotsCount; i < n; i++) {

			spSlot *slot = skeleton->drawOrder[i];
			if (!slot->attachment)
				continue;
			switch (slot->attachment->type) {

				case SP_ATTACHMENT_REGION: {
					if (!debug_attachment_region)
						continue;
					spRegionAttachment *attachment = (spRegionAttachment *)slot->attachment;
					verties_count = 8;
					spRegionAttachment_computeWorldVertices(attachment, slot->bone, world_verts.ptrw(), 0, 2);
					color = Color(0, 0, 1, 1);
					triangles = NULL;
					triangles_count = 0;
					break;
				}
				case SP_ATTACHMENT_MESH: {

					if (!debug_attachment_mesh)
						continue;
					spMeshAttachment *attachment = (spMeshAttachment *)slot->attachment;
					spVertexAttachment_computeWorldVertices(SUPER(attachment), slot, 0, attachment->super.worldVerticesLength, world_verts.ptrw(), 0, 2);
					verties_count = ((spVertexAttachment *)attachment)->verticesCount;
					color = Color(0, 1, 1, 1);
					triangles = attachment->triangles;
					triangles_count = attachment->trianglesCount;
					break;
				}
				case SP_ATTACHMENT_BOUNDING_BOX: {

					if (!debug_attachment_bounding_box)
						continue;
					spBoundingBoxAttachment *attachment = (spBoundingBoxAttachment *)slot->attachment;
					spVertexAttachment_computeWorldVertices(SUPER(attachment), slot, 0, ((spVertexAttachment *)attachment)->verticesCount, world_verts.ptrw(), 0, 2);
					verties_count = ((spVertexAttachment *)attachment)->verticesCount;
					color = Color(0, 1, 0, 1);
					triangles = NULL;
					triangles_count = 0;
					break;
				}
			}

			Point2 *points = (Point2 *)world_verts.ptr();
			int points_size = verties_count / 2;

			for (int idx = 0; idx < points_size; idx++) {

				Point2 &pt = points[idx];
				if (flip_x)
					pt.x = -pt.x;
				if (!flip_y)
					pt.y = -pt.y;
			}

			if (triangles == NULL || triangles_count == 0) {

				for (int idx = 0; idx < points_size; idx++) {

					if (idx == points_size - 1)
						draw_line(points[idx], points[0], color);
					else
						draw_line(points[idx], points[idx + 1], color);
				}
			} else {

				for (int idx = 0; idx < triangles_count - 2; idx += 3) {

					int a = triangles[idx];
					int b = triangles[idx + 1];
					int c = triangles[idx + 2];

					draw_line(points[a], points[b], color);
					draw_line(points[b], points[c], color);
					draw_line(points[c], points[a], color);
				}
			}
		}
	}

	if (debug_bones) {
		// Bone lengths.
		for (int i = 0; i < skeleton->bonesCount; i++) {
			spBone *bone = skeleton->bones[i];
			float x = bone->data->length * bone->a + bone->worldX;
			float y = bone->data->length * bone->c + bone->worldY;
			draw_line(Point2(flip_x ? -bone->worldX : bone->worldX,
							  flip_y ? bone->worldY : -bone->worldY),
					Point2(flip_x ? -x : x, flip_y ? y : -y),
					Color(1, 0, 0, 1),
					2);
		}
		// Bone origins.
		for (int i = 0, n = skeleton->bonesCount; i < n; i++) {
			spBone *bone = skeleton->bones[i];
			Rect2 rt = Rect2(flip_x ? -bone->worldX - 1 : bone->worldX - 1,
					flip_y ? bone->worldY - 1 : -bone->worldY - 1,
					3,
					3);
			draw_rect(rt, (i == 0) ? Color(0, 1, 0, 1) : Color(0, 0, 1, 1));
		}
	}
}

void Spine::queue_process() {
	if (process_queued) return;
	process_queued = true;
	call_deferred("_animation_process", 0.0);
}

void Spine::_animation_process(float p_delta) {
	process_queued = false;
	performance_triangles_generated = 0;
	if (!is_inside_tree())
		return;
	if (speed_scale == 0)
		return;
	p_delta *= speed_scale;
	process_delta += p_delta;
	if (skip_frames) {
		frames_to_skip--;
		if (frames_to_skip >= 0) {
			return;
		} else {
			frames_to_skip = skip_frames;
		}
	}
    current_pos += forward ? process_delta : -process_delta;

	spAnimationState_update(state, forward ? process_delta : -process_delta);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
	process_delta = 0;

	// Calculate and draw mesh only if timelines has been changed
	String current_state_hash = build_state_hash();
	if (current_state_hash == state_hash) {
		return;
	} else {
		state_hash = current_state_hash;
	}


	for (AttachmentNodes::Element *E = attachment_nodes.front(); E; E = E->next()) {

		AttachmentNode &info = E->get();
		WeakRef *ref = info.ref;
		Object *obj = ref->get_ref();
		Node2D *node = (obj != NULL) ? Object::cast_to<Node2D>(obj) : NULL;
		if (obj == NULL || node == NULL) {

			AttachmentNodes::Element *NEXT = E->next();
			attachment_nodes.erase(E);
			E = NEXT;
			if (E == NULL)
				break;
			continue;
		}
        spSlot *slot = info.slot;
		spBone *bone = slot->bone;
		node->call("set_position", Vector2(bone->worldX + bone->skeleton->x, -bone->worldY + bone->skeleton->y) + info.ofs);
		node->call("set_scale", Vector2(spBone_getWorldScaleX(bone), spBone_getWorldScaleY(bone)) * info.scale);
		//node->call("set_rotation", Math::atan2(bone->c, bone->d) + Math::deg2rad(info.rot));
		node->call("set_rotation_degrees", spBone_getWorldRotationX(bone) + info.rot);
        Color c;
        c.a = slot->color.a;
        c.r = slot->color.r;
        c.g = slot->color.g;
        c.b = slot->color.b;
        node->call("set_modulate", c);
	}
	update();
}

void Spine::_set_process(bool p_process, bool p_force) {

	if (processing == p_process && !p_force)
		return;

	switch (animation_process_mode) {

		case ANIMATION_PROCESS_FIXED: set_physics_process(p_process && active); break;
		case ANIMATION_PROCESS_IDLE: set_process(p_process && active); break;
	}

	processing = p_process;
}

bool Spine::_set(const StringName &p_name, const Variant &p_value) {

	String name = p_name;
	if (name.begins_with("path")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size()!=3) return true;
		spPathConstraint *pc = spSkeleton_findPathConstraint(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(pc==NULL, false);
		if (params[2] == "position") {
			pc->position = p_value;
		} else if (params[2] == "tmix"){
			pc->translateMix = p_value;
		} else if (params[2] == "rmix"){
			pc->rotateMix = p_value;
		} else if (params[2] == "spacing"){
			pc->spacing = p_value;
		}
		spSkeleton_updateWorldTransform(skeleton);
	} else if (name.begins_with("bone")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spBone *bone = spSkeleton_findBone(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(bone==NULL, false);
		if (params[2] == "rotation"){
			bone->rotation = p_value;
		}
		else if (params[2] == "position"){
			Vector2 v(p_value);
			bone->x = v.x;
			bone->y = v.y;
		}
		spSkeleton_updateWorldTransform(skeleton);
	} else if (name.begins_with("slot")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spSlot *slot = spSkeleton_findSlot(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(slot==NULL, false);
		if (params[2] == "color"){
			Color c = p_value;
			slot->color.a = skeleton->color.a * c.a;
			slot->color.r = skeleton->color.r * c.r;
			slot->color.g = skeleton->color.r * c.g;
			slot->color.b = skeleton->color.b * c.b;
		}
	} else if (name == "playback/play") {

		String which = p_value;
		if (skeleton != NULL) {

			if (which == "[stop]")
				stop();
			else if (has_animation(which)) {
				reset();
				play(which, 1, loop);
			}
		} else
			current_animation = which;
	} else if (name == "playback/loop") {

		loop = p_value;
		if (skeleton != NULL && has_animation(current_animation))
			play(current_animation, 1, loop);
	} else if (name == "playback/forward") {

		forward = p_value;
	} else if (name == "playback/skin") {

		skin = p_value;
		if (skeleton != NULL)
			set_skin(skin);
	} else if (name == "debug/region")
		set_debug_attachment(DEBUG_ATTACHMENT_REGION, p_value);
	else if (name == "debug/mesh")
		set_debug_attachment(DEBUG_ATTACHMENT_MESH, p_value);
	else if (name == "debug/skinned_mesh")
		set_debug_attachment(DEBUG_ATTACHMENT_SKINNED_MESH, p_value);
	else if (name == "debug/bounding_box")
		set_debug_attachment(DEBUG_ATTACHMENT_BOUNDING_BOX, p_value);

	return true;
}

bool Spine::_get(const StringName &p_name, Variant &r_ret) const {

	String name = p_name;

	if (name == "playback/play") {

		r_ret = current_animation;
	} else if (name == "playback/loop")
		r_ret = loop;
	else if (name == "playback/forward")
		r_ret = forward;
	else if (name == "playback/skin")
		r_ret = skin;
	else if (name == "debug/region")
		r_ret = is_debug_attachment(DEBUG_ATTACHMENT_REGION);
	else if (name == "debug/mesh")
		r_ret = is_debug_attachment(DEBUG_ATTACHMENT_MESH);
	else if (name == "debug/skinned_mesh")
		r_ret = is_debug_attachment(DEBUG_ATTACHMENT_SKINNED_MESH);
	else if (name == "debug/bounding_box")
		r_ret = is_debug_attachment(DEBUG_ATTACHMENT_BOUNDING_BOX);
	else if (name.begins_with("bone")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spBone *bone = spSkeleton_findBone(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(bone==NULL, false);
		if (params[2] == "rotation")
			r_ret = bone->rotation;
		else if (params[2] == "position")
			r_ret = Vector2(bone->x, bone->y);
	} else if (name == "performance/triangles_drawn") {
		r_ret = performance_triangles_drawn;
	} else if (name == "performance/triangles_generated") {
		r_ret = performance_triangles_generated;
	}

	return true;
}

float Spine::get_animation_length(String p_animation) const {
	if (state == NULL) return 0;
	for (int i = 0; i < state->data->skeletonData->animationsCount; i++) {
		spAnimation *anim = state->data->skeletonData->animations[i];
		if (anim->name == p_animation) {
			return anim->duration;
		}
	}
	return 0;
}

void Spine::_get_property_list(List<PropertyInfo> *p_list) const {

	List<String> names;

	if (state != NULL) {

		for (int i = 0; i < state->data->skeletonData->animationsCount; i++) {

			names.push_back(state->data->skeletonData->animations[i]->name);
		}
	}
	{
		names.sort();
		names.push_front("[stop]");
		String hint;
		for (List<String>::Element *E = names.front(); E; E = E->next()) {

			if (E != names.front())
				hint += ",";
			hint += E->get();
		}

		p_list->push_back(PropertyInfo(Variant::STRING, "playback/play", PROPERTY_HINT_ENUM, hint));
		p_list->push_back(PropertyInfo(Variant::BOOL, "playback/loop", PROPERTY_HINT_NONE));
		p_list->push_back(PropertyInfo(Variant::BOOL, "playback/forward", PROPERTY_HINT_NONE));
	}

	names.clear();
	{
		if (state != NULL) {

			for (int i = 0; i < state->data->skeletonData->skinsCount; i++) {

				names.push_back(state->data->skeletonData->skins[i]->name);
			}
		}

		String hint;
		for (List<String>::Element *E = names.front(); E; E = E->next()) {

			if (E != names.front())
				hint += ",";
			hint += E->get();
		}

		p_list->push_back(PropertyInfo(Variant::STRING, "playback/skin", PROPERTY_HINT_ENUM, hint));
	}
	p_list->push_back(PropertyInfo(Variant::BOOL, "debug/region", PROPERTY_HINT_NONE));
	p_list->push_back(PropertyInfo(Variant::BOOL, "debug/mesh", PROPERTY_HINT_NONE));
	p_list->push_back(PropertyInfo(Variant::BOOL, "debug/skinned_mesh", PROPERTY_HINT_NONE));
	p_list->push_back(PropertyInfo(Variant::BOOL, "debug/bounding_box", PROPERTY_HINT_NONE));
}

void Spine::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_PREDELETE: {
			emit_signal("predelete");
		}

		case NOTIFICATION_ENTER_TREE: {

			if (!processing) {
				//make sure that a previous process state was not saved
				//only process if "processing" is set
				set_physics_process(false);
				set_process(false);
			}
		} break;
		case NOTIFICATION_READY: {

			// add fx node as child
			fx_node->connect("draw", this, "_on_fx_draw");
			fx_node->set_z_index(1);
			fx_node->set_z_as_relative(false);
			add_child(fx_node);

			if (!Engine::get_singleton()->is_editor_hint() && has_animation(autoplay)) {
				play(autoplay);
			}
		} break;
		case NOTIFICATION_PROCESS: {
			if (animation_process_mode == ANIMATION_PROCESS_FIXED)
				break;

			if (processing)
				_animation_process(get_process_delta_time());
		} break;
		case NOTIFICATION_PHYSICS_PROCESS: {

			if (animation_process_mode == ANIMATION_PROCESS_IDLE)
				break;

			if (processing)
				_animation_process(get_physics_process_delta_time());
		} break;

		case NOTIFICATION_DRAW: {

			_animation_draw();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			performance_triangles_generated = 0;
			performance_triangles_drawn = 0;
		};
		break;
	}
}

void Spine::set_resource(Ref<Spine::SpineResource> p_data) {

	if (res == p_data)
		return;

	_spine_dispose(); // cleanup

	res = p_data;
	if (res.is_null())
		return;

	ERR_FAIL_COND(!res->data);

	skeleton = spSkeleton_create(res->data);
	root_bone = skeleton->bones[0];
	clipper = spSkeletonClipping_create();

	state = spAnimationState_create(spAnimationStateData_create(skeleton->data));
	state->rendererObject = this;
	state->listener = spine_animation_callback;

	_update_verties_count();

	if (skin != "")
		set_skin(skin);
	if (current_animation != "[stop]")
		play(current_animation, 1, loop);
	else
		reset();

	_change_notify();
}

Ref<Spine::SpineResource> Spine::get_resource() {

	return res;
}

Array Spine::get_animation_names() const {

	Array names;

	if (state != NULL) {
		for (int i = 0; i < state->data->skeletonData->animationsCount; i++) {
			spAnimation *anim = state->data->skeletonData->animations[i];
			names.push_back(anim->name);
		}
	}

	return names;
}

bool Spine::has_animation(const String &p_name) {

	if (skeleton == NULL) return false;
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	return animation != NULL;
}

void Spine::mix(const String &p_from, const String &p_to, real_t p_duration) {

	ERR_FAIL_COND(state == NULL);
	spAnimationStateData_setMixByName(state->data, p_from.utf8().get_data(), p_to.utf8().get_data(), p_duration);
}

bool Spine::play(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, float p_delay) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	ERR_FAIL_COND_V(animation == NULL, false);
	spTrackEntry *entry = spAnimationState_setAnimation(state, p_track, animation, p_loop);
	entry->delay = p_delay;
	entry->timeScale = p_cunstom_scale;
	current_animation = p_name;
	if (skip_frames) {
		frames_to_skip = 0;
	}

	_set_process(true);
	playing = true;
	// update frame
	if (!is_active())
		_animation_process(0);

	return true;
}

void Spine::set_animation_state(int p_track, String p_animation, float p_pos) {
	ERR_FAIL_COND(skeleton == NULL);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_animation.utf8().get_data());
	ERR_FAIL_COND(animation == NULL);
	spTrackEntry *entry = spAnimationState_setAnimation(state, p_track, animation, false);
	if (entry == NULL) return;
	entry->trackTime = p_pos;
	queue_process();
}

bool Spine::add(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, int p_delay) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	ERR_FAIL_COND_V(animation == NULL, false);
	spTrackEntry *entry = spAnimationState_addAnimation(state, p_track, animation, p_loop, p_delay);

	_set_process(true);
	playing = true;

	return true;
}

void Spine::clear(int p_track) {

	ERR_FAIL_COND(state == NULL);
	if (p_track == -1)
		spAnimationState_clearTracks(state);
	else
		spAnimationState_clearTrack(state, p_track);
}

void Spine::stop() {

	_set_process(false);
	playing = false;
	current_animation = "[stop]";
	reset();
}

bool Spine::is_playing(int p_track) const {
	if (!playing){
		return false;
	}
	spTrackEntry *entry = spAnimationState_getCurrent(state, p_track);
	if (entry == NULL) return false;
	return entry->loop || entry->trackTime < entry->animationEnd;
}

void Spine::set_forward(bool p_forward) {

	forward = p_forward;
}

bool Spine::is_forward() const {

	return forward;
}

void Spine::set_skip_frames(int p_skip_frames) {
	skip_frames = p_skip_frames;
	frames_to_skip = 0;
}

int Spine::get_skip_frames() const {
	return skip_frames;
}

String Spine::get_current_animation(int p_track) const {

	ERR_FAIL_COND_V(state == NULL, "");
	spTrackEntry *entry = spAnimationState_getCurrent(state, p_track);
	if (entry == NULL || entry->animation == NULL)
		return "";
	return entry->animation->name;
}

void Spine::stop_all() {

	stop();

	_set_process(false); // always process when starting an animation
}

void Spine::reset() {
	if (skeleton == NULL) {
		return;
	}
	spSkeleton_setToSetupPose(skeleton);
	spAnimationState_update(state, 0);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
}

void Spine::seek(int track, float p_pos) {
	if (state == NULL) return;
	spTrackEntry *entry = spAnimationState_getCurrent(state, track);
	if (entry == NULL) return;
	entry->trackTime = p_pos;
	// if (!processing) _animation_process(0.0);
}

float Spine::tell(int track) const {
	if (state == NULL) return 0;
	spTrackEntry *entry = spAnimationState_getCurrent(state, track);
	if (entry == NULL) return 0;
	return entry->trackTime;
}

void Spine::set_active(bool p_active) {

	if (active == p_active)
		return;

	active = p_active;
	_set_process(processing, true);
}

bool Spine::is_active() const {

	return active;
}

void Spine::set_speed(float p_speed) {

	speed_scale = p_speed;
}

float Spine::get_speed() const {

	return speed_scale;
}

void Spine::set_autoplay(const String &p_name) {

	autoplay = p_name;
}

String Spine::get_autoplay() const {

	return autoplay;
}

void Spine::set_modulate(const Color &p_color) {

	modulate = p_color;
	update();
}

Color Spine::get_modulate() const {

	return modulate;
}

void Spine::set_flip_x(bool p_flip) {

	flip_x = p_flip;
	update();
}

void Spine::set_individual_textures(bool is_individual)	{
	individual_textures = is_individual;
	update();
}

bool Spine::get_individual_textures() const {
	return individual_textures;
}

void Spine::set_flip_y(bool p_flip) {

	flip_y = p_flip;
	update();
}

bool Spine::is_flip_x() const {

	return flip_x;
}

bool Spine::is_flip_y() const {

	return flip_y;
}

bool Spine::set_skin(const String &p_name) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	return spSkeleton_setSkinByName(skeleton, p_name.utf8().get_data()) ? true : false;
}

Dictionary Spine::get_skeleton() const {

	ERR_FAIL_COND_V(skeleton == NULL, Variant());
	Dictionary dict;

	dict["bonesCount"] = skeleton->bonesCount;
	dict["slotCount"] = skeleton->slotsCount;
	dict["ikConstraintsCount"] = skeleton->ikConstraintsCount;
	dict["time"] = skeleton->time;
	dict["flipX"] = skeleton->flipX;
	dict["flipY"] = skeleton->flipY;
	dict["x"] = skeleton->x;
	dict["y"] = skeleton->y;

	Array bones;
	for (int i=0; i<skeleton->bonesCount;i++){
		spBone *b = skeleton->bones[i];
		Dictionary bi;
		bi["name"] = b->data->name;
		//Variant name = Variant(String(b->parent->data->name));
		bi["parent"] = b->parent == NULL? Variant("") : Variant(b->parent->data->name);
		bones.append(bi);
	}
	dict["bones"] = bones;

	Array slots;
	spSkin* skin = skeleton->data->defaultSkin;
	for (int j=0; j<skeleton->slotsCount; j++){
		spSlot *s = skeleton->slots[j];
		Dictionary slot_dict;
		slot_dict["name"] = s->data->name;
		Array attachments;
		int k=0;
		while (true){
			const char* attachment = spSkin_getAttachmentName(skin, j, k);
			if (attachment == NULL){
				break;
			}
			attachments.append(attachment);
			k++;
		}
		slot_dict["attachments"] = attachments;
		slots.append(slot_dict);
	}
	dict["slots"] = slots;

 if (individual_textures) {
		Dictionary slot_dict;
		for (int i = 0, n = skeleton->slotsCount; i < n; i++) {
			spSlot *s = skeleton->drawOrder[i];
			slot_dict[s->data->name] = s->data->index;
		}
		dict["item_indexes"] = slot_dict;
	}

	return dict;
}

Dictionary Spine::get_attachment(const String &p_slot_name, const String &p_attachment_name) const {

	ERR_FAIL_COND_V(skeleton == NULL, Variant());
	spAttachment *attachment = spSkeleton_getAttachmentForSlotName(skeleton, p_slot_name.utf8().get_data(), p_attachment_name.utf8().get_data());
	ERR_FAIL_COND_V(attachment == NULL, Variant());

	Dictionary dict;
	dict["name"] = attachment->name;

	switch (attachment->type) {
		case SP_ATTACHMENT_REGION: {

			spRegionAttachment *info = (spRegionAttachment *)attachment;
			dict["type"] = "region";
			dict["path"] = info->path;
			dict["x"] = info->x;
			dict["y"] = info->y;
			dict["scaleX"] = info->scaleX;
			dict["scaleY"] = info->scaleY;
			dict["rotation"] = info->rotation;
			dict["width"] = info->width;
			dict["height"] = info->height;
			dict["color"] = Color(info->color.r, info->color.g, info->color.b, info->color.a);
			dict["region"] = Rect2(info->regionOffsetX, info->regionOffsetY, info->regionWidth, info->regionHeight);
			dict["region_original_size"] = Size2(info->regionOriginalWidth, info->regionOriginalHeight);

			Vector<Vector2> offset, uvs;
			for (int idx = 0; idx < 4; idx++) {
				offset.push_back(Vector2(info->offset[idx * 2], info->offset[idx * 2 + 1]));
				uvs.push_back(Vector2(info->uvs[idx * 2], info->uvs[idx * 2 + 1]));
			}
			dict["offset"] = offset;
			dict["uvs"] = uvs;

		} break;

		case SP_ATTACHMENT_BOUNDING_BOX: {

			spVertexAttachment *info = (spVertexAttachment *)attachment;
			dict["type"] = "bounding_box";

			Vector<Vector2> vertices;
			for (int idx = 0; idx < info->verticesCount / 2; idx++)
				vertices.push_back(Vector2(info->vertices[idx * 2], -info->vertices[idx * 2 + 1]));
			dict["vertices"] = vertices;
		} break;

		case SP_ATTACHMENT_MESH: {

			spMeshAttachment *info = (spMeshAttachment *)attachment;
			dict["type"] = "mesh";
			dict["path"] = info->path;
			dict["color"] = Color(info->color.r, info->color.g, info->color.b, info->color.a);
		} break;
	}
	return dict;
}

Dictionary Spine::get_bone(const String &p_bone_name) const {

	ERR_FAIL_COND_V(skeleton == NULL, Variant());
	spBone *bone = spSkeleton_findBone(skeleton, p_bone_name.utf8().get_data());
	ERR_FAIL_COND_V(bone == NULL, Variant());
	Dictionary dict;
	dict["x"] = bone->x;
	dict["y"] = bone->y;
	dict["rotation"] = bone->rotation;
	dict["rotationIK"] = 0; //bone->rotationIK;
	dict["scaleX"] = bone->scaleX;
	dict["scaleY"] = bone->scaleY;
	dict["flipX"] = 0; //bone->flipX;
	dict["flipY"] = 0; //bone->flipY;
	dict["m00"] = bone->a; //m00;
	dict["m01"] = bone->b; //m01;
	dict["m10"] = bone->c; //m10;
	dict["m11"] = bone->d; //m11;
	dict["worldX"] = bone->worldX;
	dict["worldY"] = bone->worldY;
	dict["worldRotation"] = spBone_getWorldRotationX(bone); //->worldRotation;
	dict["worldScaleX"] = spBone_getWorldScaleX(bone); //->worldScaleX;
	dict["worldScaleY"] = spBone_getWorldScaleY(bone); //->worldScaleY;
	dict["worldFlipX"] = 0; //bone->worldFlipX;
	dict["worldFlipY"] = 0; //bone->worldFlipY;

	return dict;
}

Dictionary Spine::get_slot(const String &p_slot_name) const {

	ERR_FAIL_COND_V(skeleton == NULL, Variant());
	spSlot *slot = spSkeleton_findSlot(skeleton, p_slot_name.utf8().get_data());
	ERR_FAIL_COND_V(slot == NULL, Variant());
	Dictionary dict;
	dict["color"] = Color(slot->color.r, slot->color.g, slot->color.b, slot->color.a);
	if (slot->attachment == NULL) {
		dict["attachment"] = Variant();
	} else {
		dict["attachment"] = slot->attachment->name;
	}
	return dict;
}

bool Spine::set_attachment(const String &p_slot_name, const Variant &p_attachment) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	if (p_attachment.get_type() == Variant::STRING)
		return spSkeleton_setAttachment(skeleton, p_slot_name.utf8().get_data(), ((const String)p_attachment).utf8().get_data()) != 0;
	else
		return spSkeleton_setAttachment(skeleton, p_slot_name.utf8().get_data(), NULL) != 0;
}

bool Spine::has_attachment_node(const String &p_bone_name, const Variant &p_node) {

	return false;
}

bool Spine::add_attachment_node(const String &p_bone_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	spSlot *slot = spSkeleton_findSlot(skeleton, p_bone_name.utf8().get_data());
	ERR_FAIL_COND_V(slot == NULL, false);
	Object *obj = p_node;
	ERR_FAIL_COND_V(obj == NULL, false);
	Node2D *node = Object::cast_to<Node2D>(obj);
	ERR_FAIL_COND_V(node == NULL, false);

	if (obj->has_meta("spine_meta")) {

		AttachmentNode *info = (AttachmentNode *)((uint64_t)obj->get_meta("spine_meta"));
		if (info->slot !=  slot) {
			// add to different bone, remove first
			remove_attachment_node(info->slot->data->name, p_node);
		} else {
			// add to same bone, update params
			info->ofs = p_ofs;
			info->scale = p_scale;
			info->rot = p_rot;
			return true;
		}
	}
	attachment_nodes.push_back(AttachmentNode());
	AttachmentNode &info = attachment_nodes.back()->get();
	info.E = attachment_nodes.back();
	info.slot = slot;
	info.ref = memnew(WeakRef);
	info.ref->set_obj(node);
	info.ofs = p_ofs;
	info.scale = p_scale;
	info.rot = p_rot;
	obj->set_meta("spine_meta", (uint64_t)&info);

	return true;
}

bool Spine::remove_attachment_node(const String &p_bone_name, const Variant &p_node) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	spSlot *slot = spSkeleton_findSlot(skeleton, p_bone_name.utf8().get_data());
	ERR_FAIL_COND_V(slot == NULL, false);
	Object *obj = p_node;
	ERR_FAIL_COND_V(obj == NULL, false);
	Node2D *node = Object::cast_to<Node2D>(obj);
	ERR_FAIL_COND_V(node == NULL, false);

	if (!obj->has_meta("spine_meta"))
		return false;

	AttachmentNode *info = (AttachmentNode *)((uint64_t)obj->get_meta("spine_meta"));
	ERR_FAIL_COND_V(info->slot != slot, false);
	obj->set_meta("spine_meta", Variant());
	memdelete(info->ref);
	attachment_nodes.erase(info->E);

	return false;
}

Ref<Shape2D> Spine::get_bounding_box(const String &p_slot_name, const String &p_attachment_name) {

	ERR_FAIL_COND_V(skeleton == NULL, Ref<Shape2D>());
	spAttachment *attachment = spSkeleton_getAttachmentForSlotName(skeleton, p_slot_name.utf8().get_data(), p_attachment_name.utf8().get_data());
	ERR_FAIL_COND_V(attachment == NULL, Ref<Shape2D>());
	ERR_FAIL_COND_V(attachment->type != SP_ATTACHMENT_BOUNDING_BOX, Ref<Shape2D>());
	spVertexAttachment *info = (spVertexAttachment *)attachment;

	Vector<Vector2> points;
	points.resize(info->verticesCount / 2);
	for (int idx = 0; idx < info->verticesCount / 2; idx++)
		points.write[idx] = Vector2(info->vertices[idx * 2], -info->vertices[idx * 2 + 1]);

	ConvexPolygonShape2D *shape = memnew(ConvexPolygonShape2D);
	shape->set_points(points);

	return shape;
}

bool Spine::add_bounding_box(const String &p_bone_name, const String &p_slot_name, const String &p_attachment_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	Object *obj = p_node;
	ERR_FAIL_COND_V(obj == NULL, false);
	CollisionObject2D *node = Object::cast_to<CollisionObject2D>(obj);
	ERR_FAIL_COND_V(node == NULL, false);
	Ref<Shape2D> shape = get_bounding_box(p_slot_name, p_attachment_name);
	if (shape.is_null())
		return false;
	node->shape_owner_add_shape(0, shape);

	return add_attachment_node(p_bone_name, p_node);
}

bool Spine::remove_bounding_box(const String &p_bone_name, const Variant &p_node) {

	return remove_attachment_node(p_bone_name, p_node);
}

void Spine::set_animation_process_mode(Spine::AnimationProcessMode p_mode) {

	if (animation_process_mode == p_mode)
		return;

	bool pr = processing;
	if (pr)
		_set_process(false);
	animation_process_mode = p_mode;
	if (pr)
		_set_process(true);
}

Spine::AnimationProcessMode Spine::get_animation_process_mode() const {

	return animation_process_mode;
}

void Spine::set_fx_slot_prefix(const String &p_prefix) {

	fx_slot_prefix = p_prefix.utf8();
	update();
}

String Spine::get_fx_slot_prefix() const {

	String s;
	s.parse_utf8(fx_slot_prefix.get_data());
	return s;
}

void Spine::set_debug_bones(bool p_enable) {

	debug_bones = p_enable;
	update();
}

bool Spine::is_debug_bones() const {

	return debug_bones;
}

void Spine::set_debug_attachment(DebugAttachmentMode p_mode, bool p_enable) {

	switch (p_mode) {

		case DEBUG_ATTACHMENT_REGION:
			debug_attachment_region = p_enable;
			break;
		case DEBUG_ATTACHMENT_MESH:
			debug_attachment_mesh = p_enable;
			break;
		case DEBUG_ATTACHMENT_SKINNED_MESH:
			debug_attachment_skinned_mesh = p_enable;
			break;
		case DEBUG_ATTACHMENT_BOUNDING_BOX:
			debug_attachment_bounding_box = p_enable;
			break;
	};
	update();
}

bool Spine::is_debug_attachment(DebugAttachmentMode p_mode) const {

	switch (p_mode) {

		case DEBUG_ATTACHMENT_REGION:
			return debug_attachment_region;
		case DEBUG_ATTACHMENT_MESH:
			return debug_attachment_mesh;
		case DEBUG_ATTACHMENT_SKINNED_MESH:
			return debug_attachment_skinned_mesh;
		case DEBUG_ATTACHMENT_BOUNDING_BOX:
			return debug_attachment_bounding_box;
	};
	return false;
}

void Spine::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_resource", "spine"), &Spine::set_resource);
	ClassDB::bind_method(D_METHOD("get_resource"), &Spine::get_resource);

	ClassDB::bind_method(D_METHOD("get_animation_names"), &Spine::get_animation_names);
	ClassDB::bind_method(D_METHOD("has_animation", "name"), &Spine::has_animation);

	ClassDB::bind_method(D_METHOD("mix", "from", "to", "duration"), &Spine::mix, 0);
	ClassDB::bind_method(D_METHOD("play", "name", "cunstom_scale", "loop", "track", "delay"), &Spine::play, 1.0f, false, 0, 0);
	ClassDB::bind_method(D_METHOD("add", "name", "cunstom_scale", "loop", "track", "delay"), &Spine::add, 1.0f, false, 0, 0);
	ClassDB::bind_method(D_METHOD("clear", "track"), &Spine::clear);
	ClassDB::bind_method(D_METHOD("stop"), &Spine::stop);
	ClassDB::bind_method(D_METHOD("is_playing", "track"), &Spine::is_playing);

	ClassDB::bind_method(D_METHOD("get_current_animation"), &Spine::get_current_animation);
	ClassDB::bind_method(D_METHOD("stop_all"), &Spine::stop_all);
	ClassDB::bind_method(D_METHOD("reset"), &Spine::reset);
	ClassDB::bind_method(D_METHOD("seek", "track", "pos"), &Spine::seek);
	ClassDB::bind_method(D_METHOD("tell", "track"), &Spine::tell);
	ClassDB::bind_method(D_METHOD("set_active", "active"), &Spine::set_active);
	ClassDB::bind_method(D_METHOD("is_active"), &Spine::is_active);
	ClassDB::bind_method(D_METHOD("get_animation_length", "animation"), &Spine::get_animation_length);
	ClassDB::bind_method(D_METHOD("set_speed", "speed"), &Spine::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &Spine::get_speed);
	ClassDB::bind_method(D_METHOD("set_skip_frames", "frames"), &Spine::set_skip_frames);
	ClassDB::bind_method(D_METHOD("get_skip_frames"), &Spine::get_skip_frames);
	ClassDB::bind_method(D_METHOD("set_flip_x", "fliped"), &Spine::set_flip_x);
	ClassDB::bind_method(D_METHOD("set_individual_textures", "individual_textures"), &Spine::set_individual_textures);
	ClassDB::bind_method(D_METHOD("get_individual_textures"), &Spine::get_individual_textures);
	ClassDB::bind_method(D_METHOD("is_flip_x"), &Spine::is_flip_x);
	ClassDB::bind_method(D_METHOD("set_flip_y", "fliped"), &Spine::set_flip_y);
	ClassDB::bind_method(D_METHOD("is_flip_y"), &Spine::is_flip_y);
	ClassDB::bind_method(D_METHOD("set_skin", "skin"), &Spine::set_skin);
	ClassDB::bind_method(D_METHOD("set_animation_process_mode", "mode"), &Spine::set_animation_process_mode);
	ClassDB::bind_method(D_METHOD("get_animation_process_mode"), &Spine::get_animation_process_mode);
	ClassDB::bind_method(D_METHOD("get_skeleton"), &Spine::get_skeleton);
	ClassDB::bind_method(D_METHOD("get_attachment", "slot_name", "attachment_name"), &Spine::get_attachment);
	ClassDB::bind_method(D_METHOD("get_bone", "bone_name"), &Spine::get_bone);
	ClassDB::bind_method(D_METHOD("get_slot", "slot_name"), &Spine::get_slot);
	ClassDB::bind_method(D_METHOD("set_attachment", "slot_name", "attachment"), &Spine::set_attachment);
	ClassDB::bind_method(D_METHOD("has_attachment_node", "bone_name", "node"), &Spine::has_attachment_node);
	ClassDB::bind_method(D_METHOD("add_attachment_node", "bone_name", "node", "ofs", "scale", "rot"), &Spine::add_attachment_node, Vector2(0, 0), Vector2(1, 1), 0);
	ClassDB::bind_method(D_METHOD("remove_attachment_node", "p_bone_name", "node"), &Spine::remove_attachment_node);
	ClassDB::bind_method(D_METHOD("get_bounding_box", "slot_name", "attachment_name"), &Spine::get_bounding_box);
	ClassDB::bind_method(D_METHOD("add_bounding_box", "bone_name", "slot_name", "attachment_name", "collision_object_2d", "ofs", "scale", "rot"), &Spine::add_bounding_box, Vector2(0, 0), Vector2(1, 1), 0);
	ClassDB::bind_method(D_METHOD("remove_bounding_box", "bone_name", "collision_object_2d"), &Spine::remove_bounding_box);

	ClassDB::bind_method(D_METHOD("set_fx_slot_prefix", "prefix"), &Spine::set_fx_slot_prefix);
	ClassDB::bind_method(D_METHOD("get_fx_slot_prefix"), &Spine::get_fx_slot_prefix);

	ClassDB::bind_method(D_METHOD("set_debug_bones", "enable"), &Spine::set_debug_bones);
	ClassDB::bind_method(D_METHOD("is_debug_bones"), &Spine::is_debug_bones);
	ClassDB::bind_method(D_METHOD("set_debug_attachment", "mode", "enable"), &Spine::set_debug_attachment);
	ClassDB::bind_method(D_METHOD("is_debug_attachment", "mode"), &Spine::is_debug_attachment);

	ClassDB::bind_method(D_METHOD("_on_fx_draw"), &Spine::_on_fx_draw);
	ClassDB::bind_method(D_METHOD("_animation_process"), &Spine::_animation_process);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "process_mode", PROPERTY_HINT_ENUM, "Fixed,Idle"), "set_animation_process_mode", "get_animation_process_mode");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "speed", PROPERTY_HINT_RANGE, "-64,64,0.01"), "set_speed", "get_speed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "skip_frames", PROPERTY_HINT_RANGE, "0, 100, 1"), "set_skip_frames", "get_skip_frames");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_bones"), "set_debug_bones", "is_debug_bones");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_x"), "set_flip_x", "is_flip_x");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "individual_textures"), "set_individual_textures", "get_individual_textures");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_y"), "set_flip_y", "is_flip_y");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "fx_prefix"), "set_fx_slot_prefix", "get_fx_slot_prefix");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "SpineResource"), "set_resource", "get_resource"); //, PROPERTY_USAGE_NOEDITOR));

	ADD_SIGNAL(MethodInfo("animation_start", PropertyInfo(Variant::INT, "track")));
	ADD_SIGNAL(MethodInfo("animation_complete", PropertyInfo(Variant::INT, "track"), PropertyInfo(Variant::INT, "loop_count")));
	ADD_SIGNAL(MethodInfo("animation_event", PropertyInfo(Variant::INT, "track"), PropertyInfo(Variant::DICTIONARY, "event")));
	ADD_SIGNAL(MethodInfo("animation_end", PropertyInfo(Variant::INT, "track")));
	ADD_SIGNAL(MethodInfo("predelete"));

	BIND_ENUM_CONSTANT(ANIMATION_PROCESS_FIXED);
	BIND_ENUM_CONSTANT(ANIMATION_PROCESS_IDLE);

	BIND_ENUM_CONSTANT(DEBUG_ATTACHMENT_REGION);
	BIND_ENUM_CONSTANT(DEBUG_ATTACHMENT_MESH);
	BIND_ENUM_CONSTANT(DEBUG_ATTACHMENT_SKINNED_MESH);
	BIND_ENUM_CONSTANT(DEBUG_ATTACHMENT_BOUNDING_BOX);
}

/*
Rect2 Spine::_edit_get_rect() const {

	if (skeleton == NULL)
		return Node2D::_edit_get_rect();

	float minX = 65535, minY = 65535, maxX = -65535, maxY = -65535;
	bool attached = false;
	for (int i = 0; i < skeleton->slotsCount; ++i) {

		spSlot *slot = skeleton->slots[i];
		if (!slot->attachment) continue;
		int verticesCount;
		if (slot->attachment->type == SP_ATTACHMENT_REGION) {
			spRegionAttachment *attachment = (spRegionAttachment *)slot->attachment;
			spRegionAttachment_computeWorldVertices(attachment, slot->bone, world_verts.ptrw(), 0, 2);
			verticesCount = 8;
		} else if (slot->attachment->type == SP_ATTACHMENT_MESH) {
			spMeshAttachment *mesh = (spMeshAttachment *)slot->attachment;
			spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, world_verts.ptrw(), 0, 2);
			verticesCount = ((spVertexAttachment *)mesh)->worldVerticesLength;
		} else
			continue;

		attached = true;

		for (int ii = 0; ii < verticesCount; ii += 2) {
			float x = world_verts[ii] * 1, y = world_verts[ii + 1] * 1;
			minX = MIN(minX, x);
			minY = MIN(minY, y);
			maxX = MAX(maxX, x);
			maxY = MAX(maxY, y);
		}
	}

	int h = maxY - minY;
	return attached ? Rect2(minX, -minY - h, maxX - minX, h) : Node2D::_edit_get_rect();
}
*/

void Spine::_update_verties_count() {

	ERR_FAIL_COND(skeleton == NULL);

	int verties_count = 0;
	for (int i = 0, n = skeleton->slotsCount; i < n; i++) {

		spSlot *slot = skeleton->drawOrder[i];
		if (!slot->attachment)
			continue;

		switch (slot->attachment->type) {

			case SP_ATTACHMENT_MESH:
			case SP_ATTACHMENT_LINKED_MESH:
			case SP_ATTACHMENT_BOUNDING_BOX:
				verties_count = MAX(verties_count, ((spVertexAttachment *)slot->attachment)->verticesCount + 1);
				break;
			default:
				continue;
		}
	}

	if (verties_count > world_verts.size()) {
		world_verts.resize(verties_count);
		memset(world_verts.ptrw(), 0, world_verts.size() * sizeof(float));
	}
}

Spine::Spine()
	: batcher(this), fx_node(memnew(Node2D)), fx_batcher(fx_node) {

	skeleton = NULL;
	root_bone = NULL;
	clipper = NULL;
	state = NULL;
	res = RES();
	world_verts.resize(1000); // Max number of vertices per mesh.
	memset(world_verts.ptrw(), 0, world_verts.size() * sizeof(float));
	speed_scale = 1;
	autoplay = "";
	animation_process_mode = ANIMATION_PROCESS_IDLE;
	processing = false;
	active = false;
	playing = false;
	forward = true;
	process_delta = 0;
	skip_frames = 0;
	frames_to_skip = 0;

	debug_bones = false;
	debug_attachment_region = false;
	debug_attachment_mesh = false;
	debug_attachment_skinned_mesh = false;
	debug_attachment_bounding_box = false;

	skin = "";
	current_animation = "[stop]";
	loop = true;
	fx_slot_prefix = String("fx/").utf8();
	state_hash = "";
	process_queued = false;

	modulate = Color(1, 1, 1, 1);
	flip_x = false;
	flip_y = false;
	individual_textures = false;

	performance_triangles_drawn = 0;
	performance_triangles_generated = 0;
}

Spine::~Spine() {

	// cleanup
	_spine_dispose();
}



// --------------------------------------------

#include <v4/spine/extension.h>
#include <v4/spine/spine.h>


#define spAlphaTimeline sp4AlphaTimeline
#define spAlphaTimeline_apply sp4AlphaTimeline_apply
#define spAlphaTimeline_create sp4AlphaTimeline_create
#define spAlphaTimeline_setFrame sp4AlphaTimeline_setFrame
#define spAnimation sp4Animation
#define spAnimation_apply sp4Animation_apply
#define spAnimation_create sp4Animation_create
#define spAnimation_dispose sp4Animation_dispose
#define spAnimation_hasTimeline sp4Animation_hasTimeline
#define spAnimationState sp4AnimationState
#define spAnimationState_addAnimation sp4AnimationState_addAnimation
#define spAnimationState_addAnimationByName sp4AnimationState_addAnimationByName
#define spAnimationState_addEmptyAnimation sp4AnimationState_addEmptyAnimation
#define spAnimationState_addPropertyID sp4AnimationState_addPropertyID
#define spAnimationState_addPropertyIDs sp4AnimationState_addPropertyIDs
#define spAnimationState_animationsChanged sp4AnimationState_animationsChanged
#define spAnimationState_apply sp4AnimationState_apply
#define spAnimationState_applyAttachmentTimeline sp4AnimationState_applyAttachmentTimeline
#define spAnimationState_applyMixingFrom sp4AnimationState_applyMixingFrom
#define spAnimationState_applyRotateTimeline sp4AnimationState_applyRotateTimeline
#define spAnimationState_clearListenerNotifications sp4AnimationState_clearListenerNotifications
#define spAnimationState_clearNext sp4AnimationState_clearNext
#define spAnimationState_clearTrack sp4AnimationState_clearTrack
#define spAnimationState_clearTracks sp4AnimationState_clearTracks
#define spAnimationState_create sp4AnimationState_create
#define spAnimationState_disableQueue sp4AnimationState_disableQueue
#define spAnimationState_dispose sp4AnimationState_dispose
#define spAnimationState_disposeStatics sp4AnimationState_disposeStatics
#define spAnimationState_disposeTrackEntries sp4AnimationState_disposeTrackEntries
#define spAnimationState_disposeTrackEntry sp4AnimationState_disposeTrackEntry
#define spAnimationState_enableQueue sp4AnimationState_enableQueue
#define spAnimationState_ensureCapacityPropertyIDs sp4AnimationState_ensureCapacityPropertyIDs
#define spAnimationState_expandToIndex sp4AnimationState_expandToIndex
#define spAnimationState_getCurrent sp4AnimationState_getCurrent
#define spAnimationState_queueEvents sp4AnimationState_queueEvents
#define spAnimationState_resizeTimelinesRotation sp4AnimationState_resizeTimelinesRotation
#define spAnimationState_setAnimation sp4AnimationState_setAnimation
#define spAnimationState_setAnimationByName sp4AnimationState_setAnimationByName
#define spAnimationState_setAttachment sp4AnimationState_setAttachment
#define spAnimationState_setCurrent sp4AnimationState_setCurrent
#define spAnimationState_setEmptyAnimation sp4AnimationState_setEmptyAnimation
#define spAnimationState_setEmptyAnimations sp4AnimationState_setEmptyAnimations
#define spAnimationState_trackEntry sp4AnimationState_trackEntry
#define spAnimationState_update sp4AnimationState_update
#define spAnimationState_updateMixingFrom sp4AnimationState_updateMixingFrom
#define spAnimationStateData sp4AnimationStateData
#define spAnimationStateData_create sp4AnimationStateData_create
#define spAnimationStateData_dispose sp4AnimationStateData_dispose
#define spAnimationStateData_getMix sp4AnimationStateData_getMix
#define spAnimationStateData_setMix sp4AnimationStateData_setMix
#define spAnimationStateData_setMixByName sp4AnimationStateData_setMixByName
#define spAnimationStateListener sp4AnimationStateListener
#define spArrayFloatArray_add sp4ArrayFloatArray_add
#define spArrayFloatArray_addAll sp4ArrayFloatArray_addAll
#define spArrayFloatArray_addAllValues sp4ArrayFloatArray_addAllValues
#define spArrayFloatArray_clear sp4ArrayFloatArray_clear
#define spArrayFloatArray_contains sp4ArrayFloatArray_contains
#define spArrayFloatArray_create sp4ArrayFloatArray_create
#define spArrayFloatArray_dispose sp4ArrayFloatArray_dispose
#define spArrayFloatArray_ensureCapacity sp4ArrayFloatArray_ensureCapacity
#define spArrayFloatArray_peek sp4ArrayFloatArray_peek
#define spArrayFloatArray_pop sp4ArrayFloatArray_pop
#define spArrayFloatArray_removeAt sp4ArrayFloatArray_removeAt
#define spArrayFloatArray_setSize sp4ArrayFloatArray_setSize
#define spArrayShortArray_add sp4ArrayShortArray_add
#define spArrayShortArray_addAll sp4ArrayShortArray_addAll
#define spArrayShortArray_addAllValues sp4ArrayShortArray_addAllValues
#define spArrayShortArray_clear sp4ArrayShortArray_clear
#define spArrayShortArray_contains sp4ArrayShortArray_contains
#define spArrayShortArray_create sp4ArrayShortArray_create
#define spArrayShortArray_dispose sp4ArrayShortArray_dispose
#define spArrayShortArray_ensureCapacity sp4ArrayShortArray_ensureCapacity
#define spArrayShortArray_peek sp4ArrayShortArray_peek
#define spArrayShortArray_pop sp4ArrayShortArray_pop
#define spArrayShortArray_removeAt sp4ArrayShortArray_removeAt
#define spArrayShortArray_setSize sp4ArrayShortArray_setSize
#define spAtlas sp4Atlas
#define spAtlas_create sp4Atlas_create
#define spAtlas_createFromFile sp4Atlas_createFromFile
#define spAtlas_dispose sp4Atlas_dispose
#define spAtlas_findRegion sp4Atlas_findRegion
#define spAtlasAttachmentLoader sp4AtlasAttachmentLoader
#define spAtlasAttachmentLoader_create sp4AtlasAttachmentLoader_create
#define spAtlasAttachmentLoader_createAttachment sp4AtlasAttachmentLoader_createAttachment
#define spAtlasPage sp4AtlasPage
#define spAtlasPage_create sp4AtlasPage_create
#define spAtlasPage_createTexture sp4AtlasPage_createTexture
#define spAtlasPage_dispose sp4AtlasPage_dispose
#define spAtlasPage_disposeTexture sp4AtlasPage_disposeTexture
#define spAtlasRegion sp4AtlasRegion
#define spAtlasRegion_create sp4AtlasRegion_create
#define spAtlasRegion_dispose sp4AtlasRegion_dispose
#define spAttachment sp4Attachment
#define spAttachment_copy sp4Attachment_copy
#define spAttachment_deinit sp4Attachment_deinit
#define spAttachment_dispose sp4Attachment_dispose
#define spAttachment_init sp4Attachment_init
#define spAttachmentLoader sp4AttachmentLoader
#define spAttachmentLoader_configureAttachment sp4AttachmentLoader_configureAttachment
#define spAttachmentLoader_createAttachment sp4AttachmentLoader_createAttachment
#define spAttachmentLoader_deinit sp4AttachmentLoader_deinit
#define spAttachmentLoader_dispose sp4AttachmentLoader_dispose
#define spAttachmentLoader_disposeAttachment sp4AttachmentLoader_disposeAttachment
#define spAttachmentLoader_init sp4AttachmentLoader_init
#define spAttachmentLoader_setError sp4AttachmentLoader_setError
#define spAttachmentLoader_setUnknownTypeError sp4AttachmentLoader_setUnknownTypeError
#define spAttachmentTimeline sp4AttachmentTimeline
#define spAttachmentTimeline_apply sp4AttachmentTimeline_apply
#define spAttachmentTimeline_create sp4AttachmentTimeline_create
#define spAttachmentTimeline_dispose sp4AttachmentTimeline_dispose
#define spAttachmentTimeline_setFrame sp4AttachmentTimeline_setFrame
#define spBlendMode sp4BlendMode
#define spBone sp4Bone
#define spBone_create sp4Bone_create
#define spBone_dispose sp4Bone_dispose
#define spBone_getWorldRotationX sp4Bone_getWorldRotationX
#define spBone_getWorldRotationY sp4Bone_getWorldRotationY
#define spBone_getWorldScaleX sp4Bone_getWorldScaleX
#define spBone_getWorldScaleY sp4Bone_getWorldScaleY
#define spBone_isYDown sp4Bone_isYDown
#define spBone_localToWorld sp4Bone_localToWorld
#define spBone_localToWorldRotation sp4Bone_localToWorldRotation
#define spBone_rotateWorld sp4Bone_rotateWorld
#define spBone_setToSetupPose sp4Bone_setToSetupPose
#define spBone_setYDown sp4Bone_setYDown
#define spBone_update sp4Bone_update
#define spBone_updateAppliedTransform sp4Bone_updateAppliedTransform
#define spBone_updateWorldTransform sp4Bone_updateWorldTransform
#define spBone_updateWorldTransformWith sp4Bone_updateWorldTransformWith
#define spBone_worldToLocal sp4Bone_worldToLocal
#define spBone_worldToLocalRotation sp4Bone_worldToLocalRotation
#define spBoneData sp4BoneData
#define spBoneData_create sp4BoneData_create
#define spBoneData_dispose sp4BoneData_dispose
#define spBoneDataArray_add sp4BoneDataArray_add
#define spBoneDataArray_addAll sp4BoneDataArray_addAll
#define spBoneDataArray_addAllValues sp4BoneDataArray_addAllValues
#define spBoneDataArray_clear sp4BoneDataArray_clear
#define spBoneDataArray_contains sp4BoneDataArray_contains
#define spBoneDataArray_create sp4BoneDataArray_create
#define spBoneDataArray_dispose sp4BoneDataArray_dispose
#define spBoneDataArray_ensureCapacity sp4BoneDataArray_ensureCapacity
#define spBoneDataArray_peek sp4BoneDataArray_peek
#define spBoneDataArray_pop sp4BoneDataArray_pop
#define spBoneDataArray_removeAt sp4BoneDataArray_removeAt
#define spBoneDataArray_setSize sp4BoneDataArray_setSize
#define spBoneTimeline sp4BoneTimeline
#define spBoundingBoxAttachment sp4BoundingBoxAttachment
#define spBoundingBoxAttachment_copy sp4BoundingBoxAttachment_copy
#define spBoundingBoxAttachment_create sp4BoundingBoxAttachment_create
#define spBoundingBoxAttachment_dispose sp4BoundingBoxAttachment_dispose
#define spCalloc sp4Calloc
#define spClippingAttachment sp4ClippingAttachment
#define spClippingAttachment_copy sp4ClippingAttachment_copy
#define spClippingAttachment_create sp4ClippingAttachment_create
#define spClippingAttachment_dispose sp4ClippingAttachment_dispose
#define spColor_addColor sp4Color_addColor
#define spColor_addFloats sp4Color_addFloats
#define spColor_addFloats3 sp4Color_addFloats3
#define spColor_clamp sp4Color_clamp
#define spColor_create sp4Color_create
#define spColor_dispose sp4Color_dispose
#define spColor_setFromColor sp4Color_setFromColor
#define spColor_setFromColor3 sp4Color_setFromColor3
#define spColor_setFromFloats sp4Color_setFromFloats
#define spColor_setFromFloats3 sp4Color_setFromFloats3
#define spConstraintData sp4ConstraintData
#define spCurveTimeline sp4CurveTimeline
#define spCurveTimeline_dispose sp4CurveTimeline_dispose
#define spCurveTimeline_getBezierValue sp4CurveTimeline_getBezierValue
#define spCurveTimeline_init sp4CurveTimeline_init
#define spCurveTimeline_setBezier sp4CurveTimeline_setBezier
#define spCurveTimeline_setLinear sp4CurveTimeline_setLinear
#define spCurveTimeline_setStepped sp4CurveTimeline_setStepped
#define spCurveTimeline1 sp4CurveTimeline1
#define spCurveTimeline1_getCurveValue sp4CurveTimeline1_getCurveValue
#define spCurveTimeline1_setFrame sp4CurveTimeline1_setFrame
#define spCurveTimeline2 sp4CurveTimeline2
#define spCurveTimeline2_setFrame sp4CurveTimeline2_setFrame
#define spDebug_printAnimation sp4Debug_printAnimation
#define spDebug_printBone sp4Debug_printBone
#define spDebug_printBoneData sp4Debug_printBoneData
#define spDebug_printBoneDatas sp4Debug_printBoneDatas
#define spDebug_printBones sp4Debug_printBones
#define spDebug_printCurveTimeline sp4Debug_printCurveTimeline
#define spDebug_printFloats sp4Debug_printFloats
#define spDebug_printSkeleton sp4Debug_printSkeleton
#define spDebug_printSkeletonData sp4Debug_printSkeletonData
#define spDebug_printTimeline sp4Debug_printTimeline
#define spDebug_printTimelineBase sp4Debug_printTimelineBase
#define spDeformTimeline sp4DeformTimeline
#define spDeformTimeline_apply sp4DeformTimeline_apply
#define spDeformTimeline_create sp4DeformTimeline_create
#define spDeformTimeline_dispose sp4DeformTimeline_dispose
#define spDeformTimeline_getCurvePercent sp4DeformTimeline_getCurvePercent
#define spDeformTimeline_setBezier sp4DeformTimeline_setBezier
#define spDeformTimeline_setFrame sp4DeformTimeline_setFrame
#define spDrawOrderTimeline sp4DrawOrderTimeline
#define spDrawOrderTimeline_apply sp4DrawOrderTimeline_apply
#define spDrawOrderTimeline_create sp4DrawOrderTimeline_create
#define spDrawOrderTimeline_dispose sp4DrawOrderTimeline_dispose
#define spDrawOrderTimeline_setFrame sp4DrawOrderTimeline_setFrame
#define spEvent sp4Event
#define spEventType sp4EventType

#define spEvent_create sp4Event_create
#define spEvent_dispose sp4Event_dispose
#define spEventData sp4EventData
#define spEventData_create sp4EventData_create
#define spEventData_dispose sp4EventData_dispose
#define spEventQueue_addEntry sp4EventQueue_addEntry
#define spEventQueue_addEvent sp4EventQueue_addEvent
#define spEventQueue_addType sp4EventQueue_addType
#define spEventQueue_clear sp4EventQueue_clear
#define spEventQueue_complete sp4EventQueue_complete
#define spEventQueue_create sp4EventQueue_create
#define spEventQueue_dispose sp4EventQueue_dispose
#define spEventQueue_drain sp4EventQueue_drain
#define spEventQueue_end sp4EventQueue_end
#define spEventQueue_ensureCapacity sp4EventQueue_ensureCapacity
#define spEventQueue_event sp4EventQueue_event
#define spEventQueue_free sp4EventQueue_free
#define spEventQueue_interrupt sp4EventQueue_interrupt
#define spEventQueue_start sp4EventQueue_start
#define spEventTimeline sp4EventTimeline
#define spEventTimeline_apply sp4EventTimeline_apply
#define spEventTimeline_create sp4EventTimeline_create
#define spEventTimeline_dispose sp4EventTimeline_dispose
#define spEventTimeline_setFrame sp4EventTimeline_setFrame
#define spFloatArray_add sp4FloatArray_add
#define spFloatArray_addAll sp4FloatArray_addAll
#define spFloatArray_addAllValues sp4FloatArray_addAllValues
#define spFloatArray_clear sp4FloatArray_clear
#define spFloatArray_contains sp4FloatArray_contains
#define spFloatArray_create sp4FloatArray_create
#define spFloatArray_dispose sp4FloatArray_dispose
#define spFloatArray_ensureCapacity sp4FloatArray_ensureCapacity
#define spFloatArray_peek sp4FloatArray_peek
#define spFloatArray_pop sp4FloatArray_pop
#define spFloatArray_removeAt sp4FloatArray_removeAt
#define spFloatArray_setSize sp4FloatArray_setSize
#define spFormat sp4Format
#define spFree sp4Free
#define spIkConstraint sp4IkConstraint
#define spIkConstraint_apply1 sp4IkConstraint_apply1
#define spIkConstraint_apply2 sp4IkConstraint_apply2
#define spIkConstraint_create sp4IkConstraint_create
#define spIkConstraint_dispose sp4IkConstraint_dispose
#define spIkConstraint_update sp4IkConstraint_update
#define spIkConstraintData sp4IkConstraintData
#define spIkConstraintData_create sp4IkConstraintData_create
#define spIkConstraintData_dispose sp4IkConstraintData_dispose
#define spIkConstraintDataArray_add sp4IkConstraintDataArray_add
#define spIkConstraintDataArray_addAll sp4IkConstraintDataArray_addAll
#define spIkConstraintDataArray_addAllValues sp4IkConstraintDataArray_addAllValues
#define spIkConstraintDataArray_clear sp4IkConstraintDataArray_clear
#define spIkConstraintDataArray_contains sp4IkConstraintDataArray_contains
#define spIkConstraintDataArray_create sp4IkConstraintDataArray_create
#define spIkConstraintDataArray_dispose sp4IkConstraintDataArray_dispose
#define spIkConstraintDataArray_ensureCapacity sp4IkConstraintDataArray_ensureCapacity
#define spIkConstraintDataArray_peek sp4IkConstraintDataArray_peek
#define spIkConstraintDataArray_pop sp4IkConstraintDataArray_pop
#define spIkConstraintDataArray_removeAt sp4IkConstraintDataArray_removeAt
#define spIkConstraintDataArray_setSize sp4IkConstraintDataArray_setSize
#define spIkConstraintTimeline sp4IkConstraintTimeline
#define spIkConstraintTimeline_apply sp4IkConstraintTimeline_apply
#define spIkConstraintTimeline_create sp4IkConstraintTimeline_create
#define spIkConstraintTimeline_setFrame sp4IkConstraintTimeline_setFrame
#define spIntArray_add sp4IntArray_add
#define spIntArray_addAll sp4IntArray_addAll
#define spIntArray_addAllValues sp4IntArray_addAllValues
#define spIntArray_clear sp4IntArray_clear
#define spIntArray_contains sp4IntArray_contains
#define spIntArray_create sp4IntArray_create
#define spIntArray_dispose sp4IntArray_dispose
#define spIntArray_ensureCapacity sp4IntArray_ensureCapacity
#define spIntArray_peek sp4IntArray_peek
#define spIntArray_pop sp4IntArray_pop
#define spIntArray_removeAt sp4IntArray_removeAt
#define spIntArray_setSize sp4IntArray_setSize
#define spInternalRandom sp4InternalRandom
#define spJitterVertexEffect_begin sp4JitterVertexEffect_begin
#define spJitterVertexEffect_create sp4JitterVertexEffect_create
#define spJitterVertexEffect_dispose sp4JitterVertexEffect_dispose
#define spJitterVertexEffect_end sp4JitterVertexEffect_end
#define spJitterVertexEffect_transform sp4JitterVertexEffect_transform
#define spKeyValueArray_add sp4KeyValueArray_add
#define spKeyValueArray_addAll sp4KeyValueArray_addAll
#define spKeyValueArray_addAllValues sp4KeyValueArray_addAllValues
#define spKeyValueArray_clear sp4KeyValueArray_clear
#define spKeyValueArray_contains sp4KeyValueArray_contains
#define spKeyValueArray_create sp4KeyValueArray_create
#define spKeyValueArray_dispose sp4KeyValueArray_dispose
#define spKeyValueArray_ensureCapacity sp4KeyValueArray_ensureCapacity
#define spKeyValueArray_peek sp4KeyValueArray_peek
#define spKeyValueArray_pop sp4KeyValueArray_pop
#define spKeyValueArray_setSize sp4KeyValueArray_setSize
#define spMalloc sp4Malloc
#define spMath_interpolate sp4Math_interpolate
#define spMath_pow2_apply sp4Math_pow2_apply
#define spMath_pow2out_apply sp4Math_pow2out_apply
#define spMath_random sp4Math_random
#define spMath_randomTriangular sp4Math_randomTriangular
#define spMath_randomTriangularWith sp4Math_randomTriangularWith
#define spMeshAttachment sp4MeshAttachment
#define spMeshAttachment_copy sp4MeshAttachment_copy
#define spMeshAttachment_create sp4MeshAttachment_create
#define spMeshAttachment_dispose sp4MeshAttachment_dispose
#define spMeshAttachment_newLinkedMesh sp4MeshAttachment_newLinkedMesh
#define spMeshAttachment_setParentMesh sp4MeshAttachment_setParentMesh
#define spMeshAttachment_updateUVs sp4MeshAttachment_updateUVs
#define spMixBlend sp4MixBlend
#define spMixDirection sp4MixDirection
#define spPathAttachment sp4PathAttachment
#define spPathAttachment_copy sp4PathAttachment_copy
#define spPathAttachment_create sp4PathAttachment_create
#define spPathAttachment_dispose sp4PathAttachment_dispose
#define spPathConstraint sp4PathConstraint
#define spPathConstraint_computeWorldPositions sp4PathConstraint_computeWorldPositions
#define spPathConstraint_create sp4PathConstraint_create
#define spPathConstraint_dispose sp4PathConstraint_dispose
#define spPathConstraint_update sp4PathConstraint_update
#define spPathConstraintData sp4PathConstraintData
#define spPathConstraintData_create sp4PathConstraintData_create
#define spPathConstraintData_dispose sp4PathConstraintData_dispose
#define spPathConstraintDataArray_add sp4PathConstraintDataArray_add
#define spPathConstraintDataArray_addAll sp4PathConstraintDataArray_addAll
#define spPathConstraintDataArray_addAllValues sp4PathConstraintDataArray_addAllValues
#define spPathConstraintDataArray_clear sp4PathConstraintDataArray_clear
#define spPathConstraintDataArray_contains sp4PathConstraintDataArray_contains
#define spPathConstraintDataArray_create sp4PathConstraintDataArray_create
#define spPathConstraintDataArray_dispose sp4PathConstraintDataArray_dispose
#define spPathConstraintDataArray_ensureCapacity sp4PathConstraintDataArray_ensureCapacity
#define spPathConstraintDataArray_peek sp4PathConstraintDataArray_peek
#define spPathConstraintDataArray_pop sp4PathConstraintDataArray_pop
#define spPathConstraintDataArray_removeAt sp4PathConstraintDataArray_removeAt
#define spPathConstraintDataArray_setSize sp4PathConstraintDataArray_setSize
#define spPathConstraintMixTimeline sp4PathConstraintMixTimeline
#define spPathConstraintMixTimeline_apply sp4PathConstraintMixTimeline_apply
#define spPathConstraintMixTimeline_create sp4PathConstraintMixTimeline_create
#define spPathConstraintMixTimeline_setFrame sp4PathConstraintMixTimeline_setFrame
#define spPathConstraintPositionTimeline sp4PathConstraintPositionTimeline
#define spPathConstraintPositionTimeline_apply sp4PathConstraintPositionTimeline_apply
#define spPathConstraintPositionTimeline_create sp4PathConstraintPositionTimeline_create
#define spPathConstraintPositionTimeline_setFrame sp4PathConstraintPositionTimeline_setFrame
#define spPathConstraintSpacingTimeline sp4PathConstraintSpacingTimeline
#define spPathConstraintSpacingTimeline_apply sp4PathConstraintSpacingTimeline_apply
#define spPathConstraintSpacingTimeline_create sp4PathConstraintSpacingTimeline_create
#define spPathConstraintSpacingTimeline_setFrame sp4PathConstraintSpacingTimeline_setFrame
#define spPointAttachment sp4PointAttachment
#define spPointAttachment_computeWorldPosition sp4PointAttachment_computeWorldPosition
#define spPointAttachment_computeWorldRotation sp4PointAttachment_computeWorldRotation
#define spPointAttachment_copy sp4PointAttachment_copy
#define spPointAttachment_create sp4PointAttachment_create
#define spPointAttachment_dispose sp4PointAttachment_dispose
#define spPolygon_containsPoint sp4Polygon_containsPoint
#define spPolygon_create sp4Polygon_create
#define spPolygon_dispose sp4Polygon_dispose
#define spPolygon_intersectsSegment sp4Polygon_intersectsSegment
#define spPositionMode sp4PositionMode
#define spPropertyIdArray_add sp4PropertyIdArray_add
#define spPropertyIdArray_addAll sp4PropertyIdArray_addAll
#define spPropertyIdArray_addAllValues sp4PropertyIdArray_addAllValues
#define spPropertyIdArray_clear sp4PropertyIdArray_clear
#define spPropertyIdArray_contains sp4PropertyIdArray_contains
#define spPropertyIdArray_create sp4PropertyIdArray_create
#define spPropertyIdArray_dispose sp4PropertyIdArray_dispose
#define spPropertyIdArray_ensureCapacity sp4PropertyIdArray_ensureCapacity
#define spPropertyIdArray_peek sp4PropertyIdArray_peek
#define spPropertyIdArray_pop sp4PropertyIdArray_pop
#define spPropertyIdArray_removeAt sp4PropertyIdArray_removeAt
#define spPropertyIdArray_setSize sp4PropertyIdArray_setSize
#define spRandom sp4Random
#define spReadFile sp4ReadFile
#define spRealloc sp4Realloc
#define spRegionAttachment sp4RegionAttachment
#define spRegionAttachment_computeWorldVertices sp4RegionAttachment_computeWorldVertices
#define spRegionAttachment_copy sp4RegionAttachment_copy
#define spRegionAttachment_create sp4RegionAttachment_create
#define spRegionAttachment_dispose sp4RegionAttachment_dispose
#define spRegionAttachment_setUVs sp4RegionAttachment_setUVs
#define spRegionAttachment_updateOffset sp4RegionAttachment_updateOffset
#define spRGB2Timeline sp4RGB2Timeline
#define spRGB2Timeline_apply sp4RGB2Timeline_apply
#define spRGB2Timeline_create sp4RGB2Timeline_create
#define spRGB2Timeline_setFrame sp4RGB2Timeline_setFrame
#define spRGBA2Timeline sp4RGBA2Timeline
#define spRGBA2Timeline_apply sp4RGBA2Timeline_apply
#define spRGBA2Timeline_create sp4RGBA2Timeline_create
#define spRGBA2Timeline_setFrame sp4RGBA2Timeline_setFrame
#define spRGBATimeline sp4RGBATimeline
#define spRGBATimeline_apply sp4RGBATimeline_apply
#define spRGBATimeline_create sp4RGBATimeline_create
#define spRGBATimeline_setFrame sp4RGBATimeline_setFrame
#define spRGBTimeline sp4RGBTimeline
#define spRGBTimeline_apply sp4RGBTimeline_apply
#define spRGBTimeline_create sp4RGBTimeline_create
#define spRGBTimeline_setFrame sp4RGBTimeline_setFrame
#define spRotateMode sp4RotateMode
#define spRotateTimeline sp4RotateTimeline
#define spRotateTimeline_apply sp4RotateTimeline_apply
#define spRotateTimeline_create sp4RotateTimeline_create
#define spRotateTimeline_setFrame sp4RotateTimeline_setFrame
#define spScaleTimeline sp4ScaleTimeline
#define spScaleTimeline_apply sp4ScaleTimeline_apply
#define spScaleTimeline_create sp4ScaleTimeline_create
#define spScaleTimeline_setFrame sp4ScaleTimeline_setFrame
#define spScaleXTimeline sp4ScaleXTimeline
#define spScaleXTimeline_apply sp4ScaleXTimeline_apply
#define spScaleXTimeline_create sp4ScaleXTimeline_create
#define spScaleXTimeline_setFrame sp4ScaleXTimeline_setFrame
#define spScaleYTimeline sp4ScaleYTimeline
#define spScaleYTimeline_apply sp4ScaleYTimeline_apply
#define spScaleYTimeline_create sp4ScaleYTimeline_create
#define spScaleYTimeline_setFrame sp4ScaleYTimeline_setFrame
#define spSetAttachment sp4SetAttachment
#define spSetDebugMalloc sp4SetDebugMalloc
#define spSetFree sp4SetFree
#define spSetMalloc sp4SetMalloc
#define spSetRandom sp4SetRandom
#define spSetRealloc sp4SetRealloc
#define spShearTimeline sp4ShearTimeline
#define spShearTimeline_apply sp4ShearTimeline_apply
#define spShearTimeline_create sp4ShearTimeline_create
#define spShearTimeline_setFrame sp4ShearTimeline_setFrame
#define spShearXTimeline sp4ShearXTimeline
#define spShearXTimeline_apply sp4ShearXTimeline_apply
#define spShearXTimeline_create sp4ShearXTimeline_create
#define spShearXTimeline_setFrame sp4ShearXTimeline_setFrame
#define spShearYTimeline sp4ShearYTimeline
#define spShearYTimeline_apply sp4ShearYTimeline_apply
#define spShearYTimeline_create sp4ShearYTimeline_create
#define spShearYTimeline_setFrame sp4ShearYTimeline_setFrame
#define spShortArray_add sp4ShortArray_add
#define spShortArray_addAll sp4ShortArray_addAll
#define spShortArray_addAllValues sp4ShortArray_addAllValues
#define spShortArray_clear sp4ShortArray_clear
#define spShortArray_contains sp4ShortArray_contains
#define spShortArray_create sp4ShortArray_create
#define spShortArray_dispose sp4ShortArray_dispose
#define spShortArray_ensureCapacity sp4ShortArray_ensureCapacity
#define spShortArray_peek sp4ShortArray_peek
#define spShortArray_pop sp4ShortArray_pop
#define spShortArray_removeAt sp4ShortArray_removeAt
#define spShortArray_setSize sp4ShortArray_setSize
#define spSkeleton sp4Skeleton
#define spSkeleton_create sp4Skeleton_create
#define spSkeleton_dispose sp4Skeleton_dispose
#define spSkeleton_findBone sp4Skeleton_findBone
#define spSkeleton_findIkConstraint sp4Skeleton_findIkConstraint
#define spSkeleton_findPathConstraint sp4Skeleton_findPathConstraint
#define spSkeleton_findSlot sp4Skeleton_findSlot
#define spSkeleton_findTransformConstraint sp4Skeleton_findTransformConstraint
#define spSkeleton_getAttachmentForSlotIndex sp4Skeleton_getAttachmentForSlotIndex
#define spSkeleton_getAttachmentForSlotName sp4Skeleton_getAttachmentForSlotName
#define spSkeleton_setAttachment sp4Skeleton_setAttachment
#define spSkeleton_setBonesToSetupPose sp4Skeleton_setBonesToSetupPose
#define spSkeleton_setSkin sp4Skeleton_setSkin
#define spSkeleton_setSkinByName sp4Skeleton_setSkinByName
#define spSkeleton_setSlotsToSetupPose sp4Skeleton_setSlotsToSetupPose
#define spSkeleton_setToSetupPose sp4Skeleton_setToSetupPose
#define spSkeleton_update sp4Skeleton_update
#define spSkeleton_updateCache sp4Skeleton_updateCache
#define spSkeleton_updateWorldTransform sp4Skeleton_updateWorldTransform
#define spSkeleton_updateWorldTransformWith sp4Skeleton_updateWorldTransformWith
#define spSkeletonBinary sp4SkeletonBinary
#define spSkeletonBinary_addLinkedMesh sp4SkeletonBinary_addLinkedMesh
#define spSkeletonBinary_create sp4SkeletonBinary_create
#define spSkeletonBinary_createWithLoader sp4SkeletonBinary_createWithLoader
#define spSkeletonBinary_dispose sp4SkeletonBinary_dispose
#define spSkeletonBinary_readAnimation sp4SkeletonBinary_readAnimation
#define spSkeletonBinary_readAttachment sp4SkeletonBinary_readAttachment
#define spSkeletonBinary_readSkeletonData sp4SkeletonBinary_readSkeletonData
#define spSkeletonBinary_readSkeletonDataFile sp4SkeletonBinary_readSkeletonDataFile
#define spSkeletonBinary_readSkin sp4SkeletonBinary_readSkin
#define spSkeletonBinary_setError sp4SkeletonBinary_setError
#define spSkeletonBounds sp4SkeletonBounds
#define spSkeletonBounds_aabbContainsPoint sp4SkeletonBounds_aabbContainsPoint
#define spSkeletonBounds_aabbIntersectsSegment sp4SkeletonBounds_aabbIntersectsSegment
#define spSkeletonBounds_aabbIntersectsSkeleton sp4SkeletonBounds_aabbIntersectsSkeleton
#define spSkeletonBounds_containsPoint sp4SkeletonBounds_containsPoint
#define spSkeletonBounds_create sp4SkeletonBounds_create
#define spSkeletonBounds_dispose sp4SkeletonBounds_dispose
#define spSkeletonBounds_getPolygon sp4SkeletonBounds_getPolygon
#define spSkeletonBounds_intersectsSegment sp4SkeletonBounds_intersectsSegment
#define spSkeletonBounds_update sp4SkeletonBounds_update
#define spSkeletonClipping_clipEnd sp4SkeletonClipping_clipEnd
#define spSkeletonClipping_clipEnd2 sp4SkeletonClipping_clipEnd2
#define spSkeletonClipping_clipStart sp4SkeletonClipping_clipStart
#define spSkeletonClipping_clipTriangles sp4SkeletonClipping_clipTriangles
#define spSkeletonClipping_create sp4SkeletonClipping_create
#define spSkeletonClipping_dispose sp4SkeletonClipping_dispose
#define spSkeletonClipping_isClipping sp4SkeletonClipping_isClipping
#define spSkeletonData sp4SkeletonData
#define spSkeletonData_create sp4SkeletonData_create
#define spSkeletonData_dispose sp4SkeletonData_dispose
#define spSkeletonData_findAnimation sp4SkeletonData_findAnimation
#define spSkeletonData_findBone sp4SkeletonData_findBone
#define spSkeletonData_findEvent sp4SkeletonData_findEvent
#define spSkeletonData_findIkConstraint sp4SkeletonData_findIkConstraint
#define spSkeletonData_findPathConstraint sp4SkeletonData_findPathConstraint
#define spSkeletonData_findSkin sp4SkeletonData_findSkin
#define spSkeletonData_findSlot sp4SkeletonData_findSlot
#define spSkeletonData_findTransformConstraint sp4SkeletonData_findTransformConstraint
#define spSkeletonJson sp4SkeletonJson
#define spSkeletonJson_addLinkedMesh sp4SkeletonJson_addLinkedMesh
#define spSkeletonJson_create sp4SkeletonJson_create
#define spSkeletonJson_createWithLoader sp4SkeletonJson_createWithLoader
#define spSkeletonJson_dispose sp4SkeletonJson_dispose
#define spSkeletonJson_readAnimation sp4SkeletonJson_readAnimation
#define spSkeletonJson_readSkeletonData sp4SkeletonJson_readSkeletonData
#define spSkeletonJson_readSkeletonDataFile sp4SkeletonJson_readSkeletonDataFile
#define spSkeletonJson_setError sp4SkeletonJson_setError
#define spSkeletonLoader sp4SkeletonLoader
#define spSkin sp4Skin
#define spSkin_addSkin sp4Skin_addSkin
#define spSkin_attachAll sp4Skin_attachAll
#define spSkin_clear sp4Skin_clear
#define spSkin_copySkin sp4Skin_copySkin
#define spSkin_create sp4Skin_create
#define spSkin_dispose sp4Skin_dispose
#define spSkin_getAttachment sp4Skin_getAttachment
#define spSkin_getAttachmentName sp4Skin_getAttachmentName
#define spSkin_getAttachments sp4Skin_getAttachments
#define spSkin_setAttachment sp4Skin_setAttachment
#define spSkinEntry sp4SkinEntry
#define spSlot sp4Slot
#define spSlot_create sp4Slot_create
#define spSlot_dispose sp4Slot_dispose
#define spSlot_getAttachmentTime sp4Slot_getAttachmentTime
#define spSlot_setAttachment sp4Slot_setAttachment
#define spSlot_setAttachmentTime sp4Slot_setAttachmentTime
#define spSlot_setToSetupPose sp4Slot_setToSetupPose
#define spSlotData sp4SlotData
#define spSlotData_create sp4SlotData_create
#define spSlotData_dispose sp4SlotData_dispose
#define spSlotData_setAttachmentName sp4SlotData_setAttachmentName
#define spSlotTimeline sp4SlotTimeline
#define spSpacingMode sp4SpacingMode
#define spSwirlVertexEffect_begin sp4SwirlVertexEffect_begin
#define spSwirlVertexEffect_create sp4SwirlVertexEffect_create
#define spSwirlVertexEffect_dispose sp4SwirlVertexEffect_dispose
#define spSwirlVertexEffect_end sp4SwirlVertexEffect_end
#define spSwirlVertexEffect_transform sp4SwirlVertexEffect_transform
#define spTextureFilter sp4TextureFilter
#define spTextureLoader sp4TextureLoader
#define spTextureWrap sp4TextureWrap
#define spTimeline sp4Timeline
#define spTimeline_apply sp4Timeline_apply
#define spTimeline_dispose sp4Timeline_dispose
#define spTimeline_getDuration sp4Timeline_getDuration
#define spTimeline_init sp4Timeline_init
#define spTimeline_setBezier sp4Timeline_setBezier
#define spTimelineArray_add sp4TimelineArray_add
#define spTimelineArray_addAll sp4TimelineArray_addAll
#define spTimelineArray_addAllValues sp4TimelineArray_addAllValues
#define spTimelineArray_clear sp4TimelineArray_clear
#define spTimelineArray_contains sp4TimelineArray_contains
#define spTimelineArray_create sp4TimelineArray_create
#define spTimelineArray_dispose sp4TimelineArray_dispose
#define spTimelineArray_ensureCapacity sp4TimelineArray_ensureCapacity
#define spTimelineArray_peek sp4TimelineArray_peek
#define spTimelineArray_pop sp4TimelineArray_pop
#define spTimelineArray_removeAt sp4TimelineArray_removeAt
#define spTimelineArray_setSize sp4TimelineArray_setSize
#define spTimelineTypeNames sp4TimelineTypeNames
#define spTrackEntry sp4TrackEntry
#define spTrackEntry_computeHold sp4TrackEntry_computeHold
#define spTrackEntry_getAnimationTime sp4TrackEntry_getAnimationTime
#define spTrackEntry_getTrackComplete sp4TrackEntry_getTrackComplete
#define spTrackEntryArray_add sp4TrackEntryArray_add
#define spTrackEntryArray_addAll sp4TrackEntryArray_addAll
#define spTrackEntryArray_addAllValues sp4TrackEntryArray_addAllValues
#define spTrackEntryArray_clear sp4TrackEntryArray_clear
#define spTrackEntryArray_contains sp4TrackEntryArray_contains
#define spTrackEntryArray_create sp4TrackEntryArray_create
#define spTrackEntryArray_dispose sp4TrackEntryArray_dispose
#define spTrackEntryArray_ensureCapacity sp4TrackEntryArray_ensureCapacity
#define spTrackEntryArray_peek sp4TrackEntryArray_peek
#define spTrackEntryArray_pop sp4TrackEntryArray_pop
#define spTrackEntryArray_removeAt sp4TrackEntryArray_removeAt
#define spTrackEntryArray_setSize sp4TrackEntryArray_setSize
#define spTransformConstraint sp4TransformConstraint
#define spTransformConstraint_applyAbsoluteLocal sp4TransformConstraint_applyAbsoluteLocal
#define spTransformConstraint_applyAbsoluteWorld sp4TransformConstraint_applyAbsoluteWorld
#define spTransformConstraint_applyRelativeLocal sp4TransformConstraint_applyRelativeLocal
#define spTransformConstraint_applyRelativeWorld sp4TransformConstraint_applyRelativeWorld
#define spTransformConstraint_create sp4TransformConstraint_create
#define spTransformConstraint_dispose sp4TransformConstraint_dispose
#define spTransformConstraint_update sp4TransformConstraint_update
#define spTransformConstraintData sp4TransformConstraintData
#define spTransformConstraintData_create sp4TransformConstraintData_create
#define spTransformConstraintData_dispose sp4TransformConstraintData_dispose
#define spTransformConstraintDataArray_add sp4TransformConstraintDataArray_add
#define spTransformConstraintDataArray_addAll sp4TransformConstraintDataArray_addAll
#define spTransformConstraintDataArray_addAllValues sp4TransformConstraintDataArray_addAllValues
#define spTransformConstraintDataArray_clear sp4TransformConstraintDataArray_clear
#define spTransformConstraintDataArray_contains sp4TransformConstraintDataArray_contains
#define spTransformConstraintDataArray_create sp4TransformConstraintDataArray_create
#define spTransformConstraintDataArray_dispose sp4TransformConstraintDataArray_dispose
#define spTransformConstraintDataArray_ensureCapacity sp4TransformConstraintDataArray_ensureCapacity
#define spTransformConstraintDataArray_peek sp4TransformConstraintDataArray_peek
#define spTransformConstraintDataArray_pop sp4TransformConstraintDataArray_pop
#define spTransformConstraintDataArray_removeAt sp4TransformConstraintDataArray_removeAt
#define spTransformConstraintDataArray_setSize sp4TransformConstraintDataArray_setSize
#define spTransformConstraintTimeline sp4TransformConstraintTimeline
#define spTransformConstraintTimeline_apply sp4TransformConstraintTimeline_apply
#define spTransformConstraintTimeline_create sp4TransformConstraintTimeline_create
#define spTransformConstraintTimeline_setFrame sp4TransformConstraintTimeline_setFrame
#define spTransformMode sp4TransformMode
#define spTranslateTimeline sp4TranslateTimeline
#define spTranslateTimeline_apply sp4TranslateTimeline_apply
#define spTranslateTimeline_create sp4TranslateTimeline_create
#define spTranslateTimeline_setFrame sp4TranslateTimeline_setFrame
#define spTranslateXTimeline sp4TranslateXTimeline
#define spTranslateXTimeline_apply sp4TranslateXTimeline_apply
#define spTranslateXTimeline_create sp4TranslateXTimeline_create
#define spTranslateXTimeline_setFrame sp4TranslateXTimeline_setFrame
#define spTranslateYTimeline sp4TranslateYTimeline
#define spTranslateYTimeline_apply sp4TranslateYTimeline_apply
#define spTranslateYTimeline_create sp4TranslateYTimeline_create
#define spTranslateYTimeline_setFrame sp4TranslateYTimeline_setFrame
#define spTriangulator_create sp4Triangulator_create
#define spTriangulator_decompose sp4Triangulator_decompose
#define spTriangulator_dispose sp4Triangulator_dispose
#define spTriangulator_triangulate sp4Triangulator_triangulate
#define spUnsignedShortArray_add sp4UnsignedShortArray_add
#define spUnsignedShortArray_addAll sp4UnsignedShortArray_addAll
#define spUnsignedShortArray_addAllValues sp4UnsignedShortArray_addAllValues
#define spUnsignedShortArray_clear sp4UnsignedShortArray_clear
#define spUnsignedShortArray_contains sp4UnsignedShortArray_contains
#define spUnsignedShortArray_create sp4UnsignedShortArray_create
#define spUnsignedShortArray_dispose sp4UnsignedShortArray_dispose
#define spUnsignedShortArray_ensureCapacity sp4UnsignedShortArray_ensureCapacity
#define spUnsignedShortArray_peek sp4UnsignedShortArray_peek
#define spUnsignedShortArray_pop sp4UnsignedShortArray_pop
#define spUnsignedShortArray_removeAt sp4UnsignedShortArray_removeAt
#define spUnsignedShortArray_setSize sp4UnsignedShortArray_setSize
#define spUtil_readFile sp4Util_readFile
#define spVertexAttachment sp4VertexAttachment
#define spVertexAttachment_computeWorldVertices sp4VertexAttachment_computeWorldVertices
#define spVertexAttachment_copyTo sp4VertexAttachment_copyTo
#define spVertexAttachment_deinit sp4VertexAttachment_deinit
#define spVertexAttachment_init sp4VertexAttachment_init


#define spSkeletonData_dispose sp4SkeletonData_dispose
//spAtlas_dispose

Spine4::Spine4Resource::Spine4Resource() {

	atlas = NULL;
	data = NULL;
}

Spine4::Spine4Resource::~Spine4Resource() {

	if (atlas != NULL)
		spAtlas_dispose(atlas);

	if (data != NULL)
		spSkeletonData_dispose(data);
}

Array *Spine4::invalid_names = NULL;
Array Spine4::get_invalid_names() {
	if (invalid_names == NULL) {
		invalid_names = memnew(Array());
	}
	return *invalid_names;
}

void Spine4::spine_animation_callback(spAnimationState *p_state, spEventType p_type, spTrackEntry *p_track, spEvent *p_event) {

	((Spine4 *)p_state->rendererObject)->_on_animation_state_event(p_track->trackIndex, p_type, p_event, 1);
}

String Spine4::build_state_hash() {
	if (!state) return "";
	PoolStringArray items;
	for (int i=0; i < state->tracksCount; i++) {
		spTrackEntry* track = state->tracks[i];
		if (!track || !track->animation) continue;

		items.push_back(track->animation->name);
		items.push_back(track->loop ?
			String::num(FMOD(track->trackTime, track->animation->duration), 3) :
			String::num(MIN(track->trackTime, track->animation->duration), 3)
		);
	}
	return items.join("::");
}

void Spine4::_on_animation_state_event(int p_track, spEventType p_type, spEvent *p_event, int p_loop_count) {

	switch (p_type) {
		case SP_ANIMATION_START:
			emit_signal("animation_start", p_track);
			break;
		case SP_ANIMATION_COMPLETE:
			emit_signal("animation_complete", p_track, p_loop_count);
			break;
		case SP_ANIMATION_EVENT: {
			Dictionary event;
			event["name"] = p_event->data->name;
			event["int"] = p_event->intValue;
			event["float"] = p_event->floatValue;
			event["string"] = p_event->stringValue ? p_event->stringValue : "";
			emit_signal("animation_event", p_track, event);
		} break;
		case SP_ANIMATION_END:
			emit_signal("animation_end", p_track);
			break;
	}
}

void Spine4::_spine_dispose() {

	if (playing) {
		// stop first
		stop();
	}

	if (state) {

		spAnimationStateData_dispose(state->data);
		spAnimationState_dispose(state);
	}

	if (skeleton)
		spSkeleton_dispose(skeleton);

	if (clipper)
		spSkeletonClipping_dispose(clipper);

	state = NULL;
	skeleton = NULL;
	clipper = NULL;
	res = RES();

	for (AttachmentNodes::Element *E = attachment_nodes.front(); E; E = E->next()) {

		AttachmentNode &node = E->get();
		memdelete(node.ref);
	}
	attachment_nodes.clear();

	update();
}

static Ref<Texture> spine_get_texture(spRegionAttachment *attachment) {

	if (Ref<Texture> *ref = static_cast<Ref<Texture> *>(((spAtlasRegion *)attachment->rendererObject)->page->rendererObject))
		return *ref;
	return NULL;
}

static Ref<Texture> spine_get_texture(spMeshAttachment *attachment) {

	if (Ref<Texture> *ref = static_cast<Ref<Texture> *>(((spAtlasRegion *)attachment->rendererObject)->page->rendererObject))
		return *ref;
	return NULL;
}

void Spine4::_on_fx_draw() {

	if (skeleton == NULL)
		return;
	fx_batcher.reset();
	RID eci = fx_node->get_canvas_item();
	// VisualServer::get_singleton()->canvas_item_add_set_blend_mode(eci, VS::MaterialBlendMode(fx_node->get_blend_mode()));
	fx_batcher.flush();
}

void Spine4::_animation_draw() {
	if (skeleton == NULL)
		return;

	spColor_setFromFloats(&skeleton->color, modulate.r, modulate.g, modulate.b, modulate.a);

	int additive = 0;
	int fx_additive = 0;
	Color color;
	float *uvs = NULL;
	int verties_count = 0;
	unsigned short *triangles = NULL;
	int triangles_count = 0;
	float r = 0, g = 0, b = 0, a = 0;

	RID ci = this->get_canvas_item();
	batcher.reset();
	// VisualServer::get_singleton()->canvas_item_add_set_blend_mode(ci, VS::MaterialBlendMode(get_blend_mode()));

	const char *fx_prefix = fx_slot_prefix.get_data();

	for (int i = 0, n = skeleton->slotsCount; i < n; i++) {

		spSlot *slot = skeleton->drawOrder[i];
		if (!slot->attachment || slot->color.a == 0) {
			spSkeletonClipping_clipEnd(clipper, slot);
			continue;
		}
		bool is_fx = false;
		Ref<Texture> texture;
		switch (slot->attachment->type) {

			case SP_ATTACHMENT_REGION: {

				spRegionAttachment *attachment = (spRegionAttachment *)slot->attachment;
				if (attachment->color.a == 0){
					spSkeletonClipping_clipEnd(clipper, slot);
					continue;
				}
				is_fx = strstr(attachment->path, fx_prefix) != NULL;
				spRegionAttachment_computeWorldVertices(attachment, slot->bone, world_verts.ptrw(), 0, 2);
				texture = spine_get_texture(attachment);
				uvs = attachment->uvs;
				verties_count = 8;
				static unsigned short quadTriangles[6] = { 0, 1, 2, 2, 3, 0 };
				triangles = quadTriangles;
				triangles_count = 6;
				r = attachment->color.r;
				g = attachment->color.g;
				b = attachment->color.b;
				a = attachment->color.a;
				break;
			}
			case SP_ATTACHMENT_MESH: {

				spMeshAttachment *attachment = (spMeshAttachment *)slot->attachment;
				if (attachment->color.a == 0){
					spSkeletonClipping_clipEnd(clipper, slot);
					continue;
				}
				is_fx = strstr(attachment->path, fx_prefix) != NULL;
				spVertexAttachment_computeWorldVertices(SUPER(attachment), slot, 0, attachment->super.worldVerticesLength, world_verts.ptrw(), 0, 2);
				texture = spine_get_texture(attachment);
				uvs = attachment->uvs;
				verties_count = ((spVertexAttachment *)attachment)->worldVerticesLength;

				triangles = attachment->triangles;
				triangles_count = attachment->trianglesCount;
				r = attachment->color.r;
				g = attachment->color.g;
				b = attachment->color.b;
				a = attachment->color.a;
				break;
			}

			case SP_ATTACHMENT_CLIPPING: {
				spClippingAttachment *attachment = (spClippingAttachment *)slot->attachment;
				spSkeletonClipping_clipStart(clipper, slot, attachment);
				continue;
			}

			default: {
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}
		}
		if (texture.is_null()){
			spSkeletonClipping_clipEnd(clipper, slot);
			continue;
		}
		/*
		if (is_fx && slot->data->blendMode != fx_additive) {

			fx_batcher.add_set_blender_mode(slot->data->additiveBlending
				? VisualServer::MATERIAL_BLEND_MODE_ADD
				: get_blend_mode()
			);
			fx_additive = slot->data->additiveBlending;
		}
		else if (slot->data->additiveBlending != additive) {

			batcher.add_set_blender_mode(slot->data->additiveBlending
				? VisualServer::MATERIAL_BLEND_MODE_ADD
				: fx_node->get_blend_mode()
			);
			additive = slot->data->additiveBlending;
		}
		 */

		color.a = skeleton->color.a * slot->color.a * a;
		color.r = skeleton->color.r * slot->color.r * r;
		color.g = skeleton->color.g * slot->color.g * g;
		color.b = skeleton->color.b * slot->color.b * b;

		if (spSkeletonClipping_isClipping(clipper)){
			spSkeletonClipping_clipTriangles(clipper, world_verts.ptrw(), verties_count, triangles, triangles_count, uvs, 2);
			if (clipper->clippedTriangles->size == 0){
				spSkeletonClipping_clipEnd(clipper, slot);
				continue;
			}
			batcher.add(texture, clipper->clippedVertices->items,
								 clipper->clippedUVs->items,
								 clipper->clippedVertices->size,
								 clipper->clippedTriangles->items,
								 clipper->clippedTriangles->size,
								 &color, flip_x, flip_y, (slot->data->index)*individual_textures);
		} else if (is_fx) {
			fx_batcher.add(texture, world_verts.ptr(), uvs, verties_count, triangles, triangles_count, &color, flip_x, flip_y, (slot->data->index)*individual_textures);
		} else {
			batcher.add(texture, world_verts.ptr(), uvs, verties_count, triangles, triangles_count, &color, flip_x, flip_y, (slot->data->index)*individual_textures);
		}
		spSkeletonClipping_clipEnd(clipper, slot);
	}


	spSkeletonClipping_clipEnd2(clipper);
	performance_triangles_drawn = performance_triangles_generated = batcher.triangles_count();
	batcher.flush();
	fx_node->update();

	// Slots.
	if (debug_attachment_region || debug_attachment_mesh || debug_attachment_skinned_mesh || debug_attachment_bounding_box) {

		Color color(0, 0, 1, 1);
		for (int i = 0, n = skeleton->slotsCount; i < n; i++) {

			spSlot *slot = skeleton->drawOrder[i];
			if (!slot->attachment)
				continue;
			switch (slot->attachment->type) {

				case SP_ATTACHMENT_REGION: {
					if (!debug_attachment_region)
						continue;
					spRegionAttachment *attachment = (spRegionAttachment *)slot->attachment;
					verties_count = 8;
					spRegionAttachment_computeWorldVertices(attachment, slot->bone, world_verts.ptrw(), 0, 2);
					color = Color(0, 0, 1, 1);
					triangles = NULL;
					triangles_count = 0;
					break;
				}
				case SP_ATTACHMENT_MESH: {

					if (!debug_attachment_mesh)
						continue;
					spMeshAttachment *attachment = (spMeshAttachment *)slot->attachment;
					spVertexAttachment_computeWorldVertices(SUPER(attachment), slot, 0, attachment->super.worldVerticesLength, world_verts.ptrw(), 0, 2);
					verties_count = ((spVertexAttachment *)attachment)->verticesCount;
					color = Color(0, 1, 1, 1);
					triangles = attachment->triangles;
					triangles_count = attachment->trianglesCount;
					break;
				}
				case SP_ATTACHMENT_BOUNDING_BOX: {

					if (!debug_attachment_bounding_box)
						continue;
					spBoundingBoxAttachment *attachment = (spBoundingBoxAttachment *)slot->attachment;
					spVertexAttachment_computeWorldVertices(SUPER(attachment), slot, 0, ((spVertexAttachment *)attachment)->verticesCount, world_verts.ptrw(), 0, 2);
					verties_count = ((spVertexAttachment *)attachment)->verticesCount;
					color = Color(0, 1, 0, 1);
					triangles = NULL;
					triangles_count = 0;
					break;
				}
			}

			Point2 *points = (Point2 *)world_verts.ptr();
			int points_size = verties_count / 2;

			for (int idx = 0; idx < points_size; idx++) {

				Point2 &pt = points[idx];
				if (flip_x)
					pt.x = -pt.x;
				if (!flip_y)
					pt.y = -pt.y;
			}

			if (triangles == NULL || triangles_count == 0) {

				for (int idx = 0; idx < points_size; idx++) {

					if (idx == points_size - 1)
						draw_line(points[idx], points[0], color);
					else
						draw_line(points[idx], points[idx + 1], color);
				}
			} else {

				for (int idx = 0; idx < triangles_count - 2; idx += 3) {

					int a = triangles[idx];
					int b = triangles[idx + 1];
					int c = triangles[idx + 2];

					draw_line(points[a], points[b], color);
					draw_line(points[b], points[c], color);
					draw_line(points[c], points[a], color);
				}
			}
		}
	}

	if (debug_bones) {
		// Bone lengths.
		for (int i = 0; i < skeleton->bonesCount; i++) {
			spBone *bone = skeleton->bones[i];
			float x = bone->data->length * bone->a + bone->worldX;
			float y = bone->data->length * bone->c + bone->worldY;
			draw_line(Point2(flip_x ? -bone->worldX : bone->worldX,
							  flip_y ? bone->worldY : -bone->worldY),
					Point2(flip_x ? -x : x, flip_y ? y : -y),
					Color(1, 0, 0, 1),
					2);
		}
		// Bone origins.
		for (int i = 0, n = skeleton->bonesCount; i < n; i++) {
			spBone *bone = skeleton->bones[i];
			Rect2 rt = Rect2(flip_x ? -bone->worldX - 1 : bone->worldX - 1,
					flip_y ? bone->worldY - 1 : -bone->worldY - 1,
					3,
					3);
			draw_rect(rt, (i == 0) ? Color(0, 1, 0, 1) : Color(0, 0, 1, 1));
		}
	}
}

void Spine4::queue_process() {
	if (process_queued) return;
	process_queued = true;
	call_deferred("_animation_process", 0.0);
}

void Spine4::_animation_process(float p_delta) {
	process_queued = false;
	performance_triangles_generated = 0;
	if (!is_inside_tree())
		return;
	if (speed_scale == 0)
		return;
	p_delta *= speed_scale;
	process_delta += p_delta;
	if (skip_frames) {
		frames_to_skip--;
		if (frames_to_skip >= 0) {
			return;
		} else {
			frames_to_skip = skip_frames;
		}
	}
    current_pos += forward ? process_delta : -process_delta;

	spAnimationState_update(state, forward ? process_delta : -process_delta);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
	process_delta = 0;

	// Calculate and draw mesh only if timelines has been changed
	String current_state_hash = build_state_hash();
	if (current_state_hash == state_hash) {
		return;
	} else {
		state_hash = current_state_hash;
	}


	for (AttachmentNodes::Element *E = attachment_nodes.front(); E; E = E->next()) {

		AttachmentNode &info = E->get();
		WeakRef *ref = info.ref;
		Object *obj = ref->get_ref();
		Node2D *node = (obj != NULL) ? Object::cast_to<Node2D>(obj) : NULL;
		if (obj == NULL || node == NULL) {

			AttachmentNodes::Element *NEXT = E->next();
			attachment_nodes.erase(E);
			E = NEXT;
			if (E == NULL)
				break;
			continue;
		}
        spSlot *slot = info.slot;
		spBone *bone = slot->bone;
		node->call("set_position", Vector2(bone->worldX + bone->skeleton->x, -bone->worldY + bone->skeleton->y) + info.ofs);
		node->call("set_scale", Vector2(spBone_getWorldScaleX(bone), spBone_getWorldScaleY(bone)) * info.scale);
		//node->call("set_rotation", Math::atan2(bone->c, bone->d) + Math::deg2rad(info.rot));
		node->call("set_rotation_degrees", spBone_getWorldRotationX(bone) + info.rot);
        Color c;
        c.a = slot->color.a;
        c.r = slot->color.r;
        c.g = slot->color.g;
        c.b = slot->color.b;
        node->call("set_modulate", c);
	}
	update();
}

void Spine4::_set_process(bool p_process, bool p_force) {

	if (processing == p_process && !p_force)
		return;

	switch (animation_process_mode) {

		case ANIMATION_PROCESS_FIXED: set_physics_process(p_process && active); break;
		case ANIMATION_PROCESS_IDLE: set_process(p_process && active); break;
	}

	processing = p_process;
}

bool Spine4::_set(const StringName &p_name, const Variant &p_value) {

	String name = p_name;
	if (name.begins_with("path")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size()!=3) return true;
		spPathConstraint *pc = spSkeleton_findPathConstraint(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(pc==NULL, false);
		if (params[2] == "position") {
			pc->position = p_value;
		} else if (params[2] == "tmix"){
			////pc->translateMix = p_value;
		} else if (params[2] == "rmix"){
			////pc->rotateMix = p_value;
		} else if (params[2] == "spacing"){
			pc->spacing = p_value;
		}
		spSkeleton_updateWorldTransform(skeleton);
	} else if (name.begins_with("bone")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spBone *bone = spSkeleton_findBone(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(bone==NULL, false);
		if (params[2] == "rotation"){
			bone->rotation = p_value;
		}
		else if (params[2] == "position"){
			Vector2 v(p_value);
			bone->x = v.x;
			bone->y = v.y;
		}
		spSkeleton_updateWorldTransform(skeleton);
	} else if (name.begins_with("slot")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spSlot *slot = spSkeleton_findSlot(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(slot==NULL, false);
		if (params[2] == "color"){
			Color c = p_value;
			slot->color.a = skeleton->color.a * c.a;
			slot->color.r = skeleton->color.r * c.r;
			slot->color.g = skeleton->color.r * c.g;
			slot->color.b = skeleton->color.b * c.b;
		}
	} else if (name == "playback/play") {

		String which = p_value;
		if (skeleton != NULL) {

			if (which == "[stop]")
				stop();
			else if (has_animation(which)) {
				reset();
				play(which, 1, loop);
			}
		} else
			current_animation = which;
	} else if (name == "playback/loop") {

		loop = p_value;
		if (skeleton != NULL && has_animation(current_animation))
			play(current_animation, 1, loop);
	} else if (name == "playback/forward") {

		forward = p_value;
	} else if (name == "playback/skin") {

		skin = p_value;
		if (skeleton != NULL)
			set_skin(skin);
	} else if (name == "debug/region")
		set_debug_attachment(DEBUG_ATTACHMENT_REGION, p_value);
	else if (name == "debug/mesh")
		set_debug_attachment(DEBUG_ATTACHMENT_MESH, p_value);
	else if (name == "debug/skinned_mesh")
		set_debug_attachment(DEBUG_ATTACHMENT_SKINNED_MESH, p_value);
	else if (name == "debug/bounding_box")
		set_debug_attachment(DEBUG_ATTACHMENT_BOUNDING_BOX, p_value);

	return true;
}

bool Spine4::_get(const StringName &p_name, Variant &r_ret) const {

	String name = p_name;

	if (name == "playback/play") {

		r_ret = current_animation;
	} else if (name == "playback/loop")
		r_ret = loop;
	else if (name == "playback/forward")
		r_ret = forward;
	else if (name == "playback/skin")
		r_ret = skin;
	else if (name == "debug/region")
		r_ret = is_debug_attachment(DEBUG_ATTACHMENT_REGION);
	else if (name == "debug/mesh")
		r_ret = is_debug_attachment(DEBUG_ATTACHMENT_MESH);
	else if (name == "debug/skinned_mesh")
		r_ret = is_debug_attachment(DEBUG_ATTACHMENT_SKINNED_MESH);
	else if (name == "debug/bounding_box")
		r_ret = is_debug_attachment(DEBUG_ATTACHMENT_BOUNDING_BOX);
	else if (name.begins_with("bone")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spBone *bone = spSkeleton_findBone(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(bone==NULL, false);
		if (params[2] == "rotation")
			r_ret = bone->rotation;
		else if (params[2] == "position")
			r_ret = Vector2(bone->x, bone->y);
	} else if (name == "performance/triangles_drawn") {
		r_ret = performance_triangles_drawn;
	} else if (name == "performance/triangles_generated") {
		r_ret = performance_triangles_generated;
	}

	return true;
}

float Spine4::get_animation_length(String p_animation) const {
	if (state == NULL) return 0;
	for (int i = 0; i < state->data->skeletonData->animationsCount; i++) {
		spAnimation *anim = state->data->skeletonData->animations[i];
		if (anim->name == p_animation) {
			return anim->duration;
		}
	}
	return 0;
}

void Spine4::_get_property_list(List<PropertyInfo> *p_list) const {

	List<String> names;

	if (state != NULL) {

		for (int i = 0; i < state->data->skeletonData->animationsCount; i++) {

			names.push_back(state->data->skeletonData->animations[i]->name);
		}
	}
	{
		names.sort();
		names.push_front("[stop]");
		String hint;
		for (List<String>::Element *E = names.front(); E; E = E->next()) {

			if (E != names.front())
				hint += ",";
			hint += E->get();
		}

		p_list->push_back(PropertyInfo(Variant::STRING, "playback/play", PROPERTY_HINT_ENUM, hint));
		p_list->push_back(PropertyInfo(Variant::BOOL, "playback/loop", PROPERTY_HINT_NONE));
		p_list->push_back(PropertyInfo(Variant::BOOL, "playback/forward", PROPERTY_HINT_NONE));
	}

	names.clear();
	{
		if (state != NULL) {

			for (int i = 0; i < state->data->skeletonData->skinsCount; i++) {

				names.push_back(state->data->skeletonData->skins[i]->name);
			}
		}

		String hint;
		for (List<String>::Element *E = names.front(); E; E = E->next()) {

			if (E != names.front())
				hint += ",";
			hint += E->get();
		}

		p_list->push_back(PropertyInfo(Variant::STRING, "playback/skin", PROPERTY_HINT_ENUM, hint));
	}
	p_list->push_back(PropertyInfo(Variant::BOOL, "debug/region", PROPERTY_HINT_NONE));
	p_list->push_back(PropertyInfo(Variant::BOOL, "debug/mesh", PROPERTY_HINT_NONE));
	p_list->push_back(PropertyInfo(Variant::BOOL, "debug/skinned_mesh", PROPERTY_HINT_NONE));
	p_list->push_back(PropertyInfo(Variant::BOOL, "debug/bounding_box", PROPERTY_HINT_NONE));
}

void Spine4::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_PREDELETE: {
			emit_signal("predelete");
		}

		case NOTIFICATION_ENTER_TREE: {

			if (!processing) {
				//make sure that a previous process state was not saved
				//only process if "processing" is set
				set_physics_process(false);
				set_process(false);
			}
		} break;
		case NOTIFICATION_READY: {

			// add fx node as child
			fx_node->connect("draw", this, "_on_fx_draw");
			fx_node->set_z_index(1);
			fx_node->set_z_as_relative(false);
			add_child(fx_node);

			if (!Engine::get_singleton()->is_editor_hint() && has_animation(autoplay)) {
				play(autoplay);
			}
		} break;
		case NOTIFICATION_PROCESS: {
			if (animation_process_mode == ANIMATION_PROCESS_FIXED)
				break;

			if (processing)
				_animation_process(get_process_delta_time());
		} break;
		case NOTIFICATION_PHYSICS_PROCESS: {

			if (animation_process_mode == ANIMATION_PROCESS_IDLE)
				break;

			if (processing)
				_animation_process(get_physics_process_delta_time());
		} break;

		case NOTIFICATION_DRAW: {

			_animation_draw();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			performance_triangles_generated = 0;
			performance_triangles_drawn = 0;
		};
		break;
	}
}

void Spine4::set_resource(Ref<Spine4::Spine4Resource> p_data) {

	if (res == p_data)
		return;

	_spine_dispose(); // cleanup

	res = p_data;
	if (res.is_null())
		return;

	ERR_FAIL_COND(!res->data);

	skeleton = spSkeleton_create(res->data);
	root_bone = skeleton->bones[0];
	clipper = spSkeletonClipping_create();

	state = spAnimationState_create(spAnimationStateData_create(skeleton->data));
	state->rendererObject = this;
	state->listener = spine_animation_callback;

	_update_verties_count();

	if (skin != "")
		set_skin(skin);
	if (current_animation != "[stop]")
		play(current_animation, 1, loop);
	else
		reset();

	_change_notify();
}

Ref<Spine4::Spine4Resource> Spine4::get_resource() {

	return res;
}

Array Spine4::get_animation_names() const {

	Array names;

	if (state != NULL) {
		for (int i = 0; i < state->data->skeletonData->animationsCount; i++) {
			spAnimation *anim = state->data->skeletonData->animations[i];
			names.push_back(anim->name);
		}
	}

	return names;
}

bool Spine4::has_animation(const String &p_name) {

	if (skeleton == NULL) return false;
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	return animation != NULL;
}

void Spine4::mix(const String &p_from, const String &p_to, real_t p_duration) {

	ERR_FAIL_COND(state == NULL);
	spAnimationStateData_setMixByName(state->data, p_from.utf8().get_data(), p_to.utf8().get_data(), p_duration);
}

bool Spine4::play(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, float p_delay) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	ERR_FAIL_COND_V(animation == NULL, false);
	spTrackEntry *entry = spAnimationState_setAnimation(state, p_track, animation, p_loop);
	entry->delay = p_delay;
	entry->timeScale = p_cunstom_scale;
	current_animation = p_name;
	if (skip_frames) {
		frames_to_skip = 0;
	}

	_set_process(true);
	playing = true;
	// update frame
	if (!is_active())
		_animation_process(0);

	return true;
}

void Spine4::set_animation_state(int p_track, String p_animation, float p_pos) {
	ERR_FAIL_COND(skeleton == NULL);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_animation.utf8().get_data());
	ERR_FAIL_COND(animation == NULL);
	spTrackEntry *entry = spAnimationState_setAnimation(state, p_track, animation, false);
	if (entry == NULL) return;
	entry->trackTime = p_pos;
	queue_process();
}

bool Spine4::add(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, int p_delay) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	ERR_FAIL_COND_V(animation == NULL, false);
	spTrackEntry *entry = spAnimationState_addAnimation(state, p_track, animation, p_loop, p_delay);

	_set_process(true);
	playing = true;

	return true;
}

void Spine4::clear(int p_track) {

	ERR_FAIL_COND(state == NULL);
	if (p_track == -1)
		spAnimationState_clearTracks(state);
	else
		spAnimationState_clearTrack(state, p_track);
}

void Spine4::stop() {

	_set_process(false);
	playing = false;
	current_animation = "[stop]";
	reset();
}

bool Spine4::is_playing(int p_track) const {
	if (!playing){
		return false;
	}
	spTrackEntry *entry = spAnimationState_getCurrent(state, p_track);
	if (entry == NULL) return false;
	return entry->loop || entry->trackTime < entry->animationEnd;
}

void Spine4::set_forward(bool p_forward) {

	forward = p_forward;
}

bool Spine4::is_forward() const {

	return forward;
}

void Spine4::set_skip_frames(int p_skip_frames) {
	skip_frames = p_skip_frames;
	frames_to_skip = 0;
}

int Spine4::get_skip_frames() const {
	return skip_frames;
}

String Spine4::get_current_animation(int p_track) const {

	ERR_FAIL_COND_V(state == NULL, "");
	spTrackEntry *entry = spAnimationState_getCurrent(state, p_track);
	if (entry == NULL || entry->animation == NULL)
		return "";
	return entry->animation->name;
}

void Spine4::stop_all() {

	stop();

	_set_process(false); // always process when starting an animation
}

void Spine4::reset() {
	if (skeleton == NULL) {
		return;
	}
	spSkeleton_setToSetupPose(skeleton);
	spAnimationState_update(state, 0);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
}

void Spine4::seek(int track, float p_pos) {
	if (state == NULL) return;
	spTrackEntry *entry = spAnimationState_getCurrent(state, track);
	if (entry == NULL) return;
	entry->trackTime = p_pos;
	// if (!processing) _animation_process(0.0);
}

float Spine4::tell(int track) const {
	if (state == NULL) return 0;
	spTrackEntry *entry = spAnimationState_getCurrent(state, track);
	if (entry == NULL) return 0;
	return entry->trackTime;
}

void Spine4::set_active(bool p_active) {

	if (active == p_active)
		return;

	active = p_active;
	_set_process(processing, true);
}

bool Spine4::is_active() const {

	return active;
}

void Spine4::set_speed(float p_speed) {

	speed_scale = p_speed;
}

float Spine4::get_speed() const {

	return speed_scale;
}

void Spine4::set_autoplay(const String &p_name) {

	autoplay = p_name;
}

String Spine4::get_autoplay() const {

	return autoplay;
}

void Spine4::set_modulate(const Color &p_color) {

	modulate = p_color;
	update();
}

Color Spine4::get_modulate() const {

	return modulate;
}

void Spine4::set_flip_x(bool p_flip) {

	flip_x = p_flip;
	update();
}

void Spine4::set_individual_textures(bool is_individual)	{
	individual_textures = is_individual;
	update();
}

bool Spine4::get_individual_textures() const {
	return individual_textures;
}

void Spine4::set_flip_y(bool p_flip) {

	flip_y = p_flip;
	update();
}

bool Spine4::is_flip_x() const {

	return flip_x;
}

bool Spine4::is_flip_y() const {

	return flip_y;
}

bool Spine4::set_skin(const String &p_name) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	return spSkeleton_setSkinByName(skeleton, p_name.utf8().get_data()) ? true : false;
}

Dictionary Spine4::get_skeleton() const {

	ERR_FAIL_COND_V(skeleton == NULL, Variant());
	Dictionary dict;

	dict["bonesCount"] = skeleton->bonesCount;
	dict["slotCount"] = skeleton->slotsCount;
	dict["ikConstraintsCount"] = skeleton->ikConstraintsCount;
	dict["time"] = skeleton->time;
	////dict["flipX"] = skeleton->flipX;
	////dict["flipY"] = skeleton->flipY;
	dict["x"] = skeleton->x;
	dict["y"] = skeleton->y;

	Array bones;
	for (int i=0; i<skeleton->bonesCount;i++){
		spBone *b = skeleton->bones[i];
		Dictionary bi;
		bi["name"] = b->data->name;
		//Variant name = Variant(String(b->parent->data->name));
		bi["parent"] = b->parent == NULL? Variant("") : Variant(b->parent->data->name);
		bones.append(bi);
	}
	dict["bones"] = bones;

	Array slots;
	spSkin* skin = skeleton->data->defaultSkin;
	for (int j=0; j<skeleton->slotsCount; j++){
		spSlot *s = skeleton->slots[j];
		Dictionary slot_dict;
		slot_dict["name"] = s->data->name;
		Array attachments;
		int k=0;
		while (true){
			const char* attachment = spSkin_getAttachmentName(skin, j, k);
			if (attachment == NULL){
				break;
			}
			attachments.append(attachment);
			k++;
		}
		slot_dict["attachments"] = attachments;
		slots.append(slot_dict);
	}
	dict["slots"] = slots;

 if (individual_textures) {
		Dictionary slot_dict;
		for (int i = 0, n = skeleton->slotsCount; i < n; i++) {
			spSlot *s = skeleton->drawOrder[i];
			slot_dict[s->data->name] = s->data->index;
		}
		dict["item_indexes"] = slot_dict;
	}

	return dict;
}

Dictionary Spine4::get_attachment(const String &p_slot_name, const String &p_attachment_name) const {

	ERR_FAIL_COND_V(skeleton == NULL, Variant());
	spAttachment *attachment = spSkeleton_getAttachmentForSlotName(skeleton, p_slot_name.utf8().get_data(), p_attachment_name.utf8().get_data());
	ERR_FAIL_COND_V(attachment == NULL, Variant());

	Dictionary dict;
	dict["name"] = attachment->name;

	switch (attachment->type) {
		case SP_ATTACHMENT_REGION: {

			spRegionAttachment *info = (spRegionAttachment *)attachment;
			dict["type"] = "region";
			dict["path"] = info->path;
			dict["x"] = info->x;
			dict["y"] = info->y;
			dict["scaleX"] = info->scaleX;
			dict["scaleY"] = info->scaleY;
			dict["rotation"] = info->rotation;
			dict["width"] = info->width;
			dict["height"] = info->height;
			dict["color"] = Color(info->color.r, info->color.g, info->color.b, info->color.a);
			dict["region"] = Rect2(info->regionOffsetX, info->regionOffsetY, info->regionWidth, info->regionHeight);
			dict["region_original_size"] = Size2(info->regionOriginalWidth, info->regionOriginalHeight);

			Vector<Vector2> offset, uvs;
			for (int idx = 0; idx < 4; idx++) {
				offset.push_back(Vector2(info->offset[idx * 2], info->offset[idx * 2 + 1]));
				uvs.push_back(Vector2(info->uvs[idx * 2], info->uvs[idx * 2 + 1]));
			}
			dict["offset"] = offset;
			dict["uvs"] = uvs;

		} break;

		case SP_ATTACHMENT_BOUNDING_BOX: {

			spVertexAttachment *info = (spVertexAttachment *)attachment;
			dict["type"] = "bounding_box";

			Vector<Vector2> vertices;
			for (int idx = 0; idx < info->verticesCount / 2; idx++)
				vertices.push_back(Vector2(info->vertices[idx * 2], -info->vertices[idx * 2 + 1]));
			dict["vertices"] = vertices;
		} break;

		case SP_ATTACHMENT_MESH: {

			spMeshAttachment *info = (spMeshAttachment *)attachment;
			dict["type"] = "mesh";
			dict["path"] = info->path;
			dict["color"] = Color(info->color.r, info->color.g, info->color.b, info->color.a);
		} break;
	}
	return dict;
}

Dictionary Spine4::get_bone(const String &p_bone_name) const {

	ERR_FAIL_COND_V(skeleton == NULL, Variant());
	spBone *bone = spSkeleton_findBone(skeleton, p_bone_name.utf8().get_data());
	ERR_FAIL_COND_V(bone == NULL, Variant());
	Dictionary dict;
	dict["x"] = bone->x;
	dict["y"] = bone->y;
	dict["rotation"] = bone->rotation;
	dict["rotationIK"] = 0; //bone->rotationIK;
	dict["scaleX"] = bone->scaleX;
	dict["scaleY"] = bone->scaleY;
	dict["flipX"] = 0; //bone->flipX;
	dict["flipY"] = 0; //bone->flipY;
	dict["m00"] = bone->a; //m00;
	dict["m01"] = bone->b; //m01;
	dict["m10"] = bone->c; //m10;
	dict["m11"] = bone->d; //m11;
	dict["worldX"] = bone->worldX;
	dict["worldY"] = bone->worldY;
	dict["worldRotation"] = spBone_getWorldRotationX(bone); //->worldRotation;
	dict["worldScaleX"] = spBone_getWorldScaleX(bone); //->worldScaleX;
	dict["worldScaleY"] = spBone_getWorldScaleY(bone); //->worldScaleY;
	dict["worldFlipX"] = 0; //bone->worldFlipX;
	dict["worldFlipY"] = 0; //bone->worldFlipY;

	return dict;
}

Dictionary Spine4::get_slot(const String &p_slot_name) const {

	ERR_FAIL_COND_V(skeleton == NULL, Variant());
	spSlot *slot = spSkeleton_findSlot(skeleton, p_slot_name.utf8().get_data());
	ERR_FAIL_COND_V(slot == NULL, Variant());
	Dictionary dict;
	dict["color"] = Color(slot->color.r, slot->color.g, slot->color.b, slot->color.a);
	if (slot->attachment == NULL) {
		dict["attachment"] = Variant();
	} else {
		dict["attachment"] = slot->attachment->name;
	}
	return dict;
}

bool Spine4::set_attachment(const String &p_slot_name, const Variant &p_attachment) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	if (p_attachment.get_type() == Variant::STRING)
		return spSkeleton_setAttachment(skeleton, p_slot_name.utf8().get_data(), ((const String)p_attachment).utf8().get_data()) != 0;
	else
		return spSkeleton_setAttachment(skeleton, p_slot_name.utf8().get_data(), NULL) != 0;
}

bool Spine4::has_attachment_node(const String &p_bone_name, const Variant &p_node) {

	return false;
}

bool Spine4::add_attachment_node(const String &p_bone_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	spSlot *slot = spSkeleton_findSlot(skeleton, p_bone_name.utf8().get_data());
	ERR_FAIL_COND_V(slot == NULL, false);
	Object *obj = p_node;
	ERR_FAIL_COND_V(obj == NULL, false);
	Node2D *node = Object::cast_to<Node2D>(obj);
	ERR_FAIL_COND_V(node == NULL, false);

	if (obj->has_meta("spine_meta")) {

		AttachmentNode *info = (AttachmentNode *)((uint64_t)obj->get_meta("spine_meta"));
		if (info->slot !=  slot) {
			// add to different bone, remove first
			remove_attachment_node(info->slot->data->name, p_node);
		} else {
			// add to same bone, update params
			info->ofs = p_ofs;
			info->scale = p_scale;
			info->rot = p_rot;
			return true;
		}
	}
	attachment_nodes.push_back(AttachmentNode());
	AttachmentNode &info = attachment_nodes.back()->get();
	info.E = attachment_nodes.back();
	info.slot = slot;
	info.ref = memnew(WeakRef);
	info.ref->set_obj(node);
	info.ofs = p_ofs;
	info.scale = p_scale;
	info.rot = p_rot;
	obj->set_meta("spine_meta", (uint64_t)&info);

	return true;
}

bool Spine4::remove_attachment_node(const String &p_bone_name, const Variant &p_node) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	spSlot *slot = spSkeleton_findSlot(skeleton, p_bone_name.utf8().get_data());
	ERR_FAIL_COND_V(slot == NULL, false);
	Object *obj = p_node;
	ERR_FAIL_COND_V(obj == NULL, false);
	Node2D *node = Object::cast_to<Node2D>(obj);
	ERR_FAIL_COND_V(node == NULL, false);

	if (!obj->has_meta("spine_meta"))
		return false;

	AttachmentNode *info = (AttachmentNode *)((uint64_t)obj->get_meta("spine_meta"));
	ERR_FAIL_COND_V(info->slot != slot, false);
	obj->set_meta("spine_meta", Variant());
	memdelete(info->ref);
	attachment_nodes.erase(info->E);

	return false;
}

Ref<Shape2D> Spine4::get_bounding_box(const String &p_slot_name, const String &p_attachment_name) {

	ERR_FAIL_COND_V(skeleton == NULL, Ref<Shape2D>());
	spAttachment *attachment = spSkeleton_getAttachmentForSlotName(skeleton, p_slot_name.utf8().get_data(), p_attachment_name.utf8().get_data());
	ERR_FAIL_COND_V(attachment == NULL, Ref<Shape2D>());
	ERR_FAIL_COND_V(attachment->type != SP_ATTACHMENT_BOUNDING_BOX, Ref<Shape2D>());
	spVertexAttachment *info = (spVertexAttachment *)attachment;

	Vector<Vector2> points;
	points.resize(info->verticesCount / 2);
	for (int idx = 0; idx < info->verticesCount / 2; idx++)
		points.write[idx] = Vector2(info->vertices[idx * 2], -info->vertices[idx * 2 + 1]);

	ConvexPolygonShape2D *shape = memnew(ConvexPolygonShape2D);
	shape->set_points(points);

	return shape;
}

bool Spine4::add_bounding_box(const String &p_bone_name, const String &p_slot_name, const String &p_attachment_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {

	ERR_FAIL_COND_V(skeleton == NULL, false);
	Object *obj = p_node;
	ERR_FAIL_COND_V(obj == NULL, false);
	CollisionObject2D *node = Object::cast_to<CollisionObject2D>(obj);
	ERR_FAIL_COND_V(node == NULL, false);
	Ref<Shape2D> shape = get_bounding_box(p_slot_name, p_attachment_name);
	if (shape.is_null())
		return false;
	node->shape_owner_add_shape(0, shape);

	return add_attachment_node(p_bone_name, p_node);
}

bool Spine4::remove_bounding_box(const String &p_bone_name, const Variant &p_node) {

	return remove_attachment_node(p_bone_name, p_node);
}

void Spine4::set_animation_process_mode(Spine4::AnimationProcessMode p_mode) {

	if (animation_process_mode == p_mode)
		return;

	bool pr = processing;
	if (pr)
		_set_process(false);
	animation_process_mode = p_mode;
	if (pr)
		_set_process(true);
}

Spine4::AnimationProcessMode Spine4::get_animation_process_mode() const {

	return animation_process_mode;
}

void Spine4::set_fx_slot_prefix(const String &p_prefix) {

	fx_slot_prefix = p_prefix.utf8();
	update();
}

String Spine4::get_fx_slot_prefix() const {

	String s;
	s.parse_utf8(fx_slot_prefix.get_data());
	return s;
}

void Spine4::set_debug_bones(bool p_enable) {

	debug_bones = p_enable;
	update();
}

bool Spine4::is_debug_bones() const {

	return debug_bones;
}

void Spine4::set_debug_attachment(DebugAttachmentMode p_mode, bool p_enable) {

	switch (p_mode) {

		case DEBUG_ATTACHMENT_REGION:
			debug_attachment_region = p_enable;
			break;
		case DEBUG_ATTACHMENT_MESH:
			debug_attachment_mesh = p_enable;
			break;
		case DEBUG_ATTACHMENT_SKINNED_MESH:
			debug_attachment_skinned_mesh = p_enable;
			break;
		case DEBUG_ATTACHMENT_BOUNDING_BOX:
			debug_attachment_bounding_box = p_enable;
			break;
	};
	update();
}

bool Spine4::is_debug_attachment(DebugAttachmentMode p_mode) const {

	switch (p_mode) {

		case DEBUG_ATTACHMENT_REGION:
			return debug_attachment_region;
		case DEBUG_ATTACHMENT_MESH:
			return debug_attachment_mesh;
		case DEBUG_ATTACHMENT_SKINNED_MESH:
			return debug_attachment_skinned_mesh;
		case DEBUG_ATTACHMENT_BOUNDING_BOX:
			return debug_attachment_bounding_box;
	};
	return false;
}

void Spine4::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_resource", "spine"), &Spine4::set_resource);
	ClassDB::bind_method(D_METHOD("get_resource"), &Spine4::get_resource);

	ClassDB::bind_method(D_METHOD("get_animation_names"), &Spine4::get_animation_names);
	ClassDB::bind_method(D_METHOD("has_animation", "name"), &Spine4::has_animation);

	ClassDB::bind_method(D_METHOD("mix", "from", "to", "duration"), &Spine4::mix, 0);
	ClassDB::bind_method(D_METHOD("play", "name", "cunstom_scale", "loop", "track", "delay"), &Spine4::play, 1.0f, false, 0, 0);
	ClassDB::bind_method(D_METHOD("add", "name", "cunstom_scale", "loop", "track", "delay"), &Spine4::add, 1.0f, false, 0, 0);
	ClassDB::bind_method(D_METHOD("clear", "track"), &Spine4::clear);
	ClassDB::bind_method(D_METHOD("stop"), &Spine4::stop);
	ClassDB::bind_method(D_METHOD("is_playing", "track"), &Spine4::is_playing);

	ClassDB::bind_method(D_METHOD("get_current_animation"), &Spine4::get_current_animation);
	ClassDB::bind_method(D_METHOD("stop_all"), &Spine4::stop_all);
	ClassDB::bind_method(D_METHOD("reset"), &Spine4::reset);
	ClassDB::bind_method(D_METHOD("seek", "track", "pos"), &Spine4::seek);
	ClassDB::bind_method(D_METHOD("tell", "track"), &Spine4::tell);
	ClassDB::bind_method(D_METHOD("set_active", "active"), &Spine4::set_active);
	ClassDB::bind_method(D_METHOD("is_active"), &Spine4::is_active);
	ClassDB::bind_method(D_METHOD("get_animation_length", "animation"), &Spine4::get_animation_length);
	ClassDB::bind_method(D_METHOD("set_speed", "speed"), &Spine4::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &Spine4::get_speed);
	ClassDB::bind_method(D_METHOD("set_skip_frames", "frames"), &Spine4::set_skip_frames);
	ClassDB::bind_method(D_METHOD("get_skip_frames"), &Spine4::get_skip_frames);
	ClassDB::bind_method(D_METHOD("set_flip_x", "fliped"), &Spine4::set_flip_x);
	ClassDB::bind_method(D_METHOD("set_individual_textures", "individual_textures"), &Spine4::set_individual_textures);
	ClassDB::bind_method(D_METHOD("get_individual_textures"), &Spine4::get_individual_textures);
	ClassDB::bind_method(D_METHOD("is_flip_x"), &Spine4::is_flip_x);
	ClassDB::bind_method(D_METHOD("set_flip_y", "fliped"), &Spine4::set_flip_y);
	ClassDB::bind_method(D_METHOD("is_flip_y"), &Spine4::is_flip_y);
	ClassDB::bind_method(D_METHOD("set_skin", "skin"), &Spine4::set_skin);
	ClassDB::bind_method(D_METHOD("set_animation_process_mode", "mode"), &Spine4::set_animation_process_mode);
	ClassDB::bind_method(D_METHOD("get_animation_process_mode"), &Spine4::get_animation_process_mode);
	ClassDB::bind_method(D_METHOD("get_skeleton"), &Spine4::get_skeleton);
	ClassDB::bind_method(D_METHOD("get_attachment", "slot_name", "attachment_name"), &Spine4::get_attachment);
	ClassDB::bind_method(D_METHOD("get_bone", "bone_name"), &Spine4::get_bone);
	ClassDB::bind_method(D_METHOD("get_slot", "slot_name"), &Spine4::get_slot);
	ClassDB::bind_method(D_METHOD("set_attachment", "slot_name", "attachment"), &Spine4::set_attachment);
	ClassDB::bind_method(D_METHOD("has_attachment_node", "bone_name", "node"), &Spine4::has_attachment_node);
	ClassDB::bind_method(D_METHOD("add_attachment_node", "bone_name", "node", "ofs", "scale", "rot"), &Spine4::add_attachment_node, Vector2(0, 0), Vector2(1, 1), 0);
	ClassDB::bind_method(D_METHOD("remove_attachment_node", "p_bone_name", "node"), &Spine4::remove_attachment_node);
	ClassDB::bind_method(D_METHOD("get_bounding_box", "slot_name", "attachment_name"), &Spine4::get_bounding_box);
	ClassDB::bind_method(D_METHOD("add_bounding_box", "bone_name", "slot_name", "attachment_name", "collision_object_2d", "ofs", "scale", "rot"), &Spine4::add_bounding_box, Vector2(0, 0), Vector2(1, 1), 0);
	ClassDB::bind_method(D_METHOD("remove_bounding_box", "bone_name", "collision_object_2d"), &Spine4::remove_bounding_box);

	ClassDB::bind_method(D_METHOD("set_fx_slot_prefix", "prefix"), &Spine4::set_fx_slot_prefix);
	ClassDB::bind_method(D_METHOD("get_fx_slot_prefix"), &Spine4::get_fx_slot_prefix);

	ClassDB::bind_method(D_METHOD("set_debug_bones", "enable"), &Spine4::set_debug_bones);
	ClassDB::bind_method(D_METHOD("is_debug_bones"), &Spine4::is_debug_bones);
	ClassDB::bind_method(D_METHOD("set_debug_attachment", "mode", "enable"), &Spine4::set_debug_attachment);
	ClassDB::bind_method(D_METHOD("is_debug_attachment", "mode"), &Spine4::is_debug_attachment);

	ClassDB::bind_method(D_METHOD("_on_fx_draw"), &Spine4::_on_fx_draw);
	ClassDB::bind_method(D_METHOD("_animation_process"), &Spine4::_animation_process);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "process_mode", PROPERTY_HINT_ENUM, "Fixed,Idle"), "set_animation_process_mode", "get_animation_process_mode");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "speed", PROPERTY_HINT_RANGE, "-64,64,0.01"), "set_speed", "get_speed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "skip_frames", PROPERTY_HINT_RANGE, "0, 100, 1"), "set_skip_frames", "get_skip_frames");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_bones"), "set_debug_bones", "is_debug_bones");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_x"), "set_flip_x", "is_flip_x");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "individual_textures"), "set_individual_textures", "get_individual_textures");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_y"), "set_flip_y", "is_flip_y");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "fx_prefix"), "set_fx_slot_prefix", "get_fx_slot_prefix");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "Spine4Resource"), "set_resource", "get_resource"); //, PROPERTY_USAGE_NOEDITOR));

	ADD_SIGNAL(MethodInfo("animation_start", PropertyInfo(Variant::INT, "track")));
	ADD_SIGNAL(MethodInfo("animation_complete", PropertyInfo(Variant::INT, "track"), PropertyInfo(Variant::INT, "loop_count")));
	ADD_SIGNAL(MethodInfo("animation_event", PropertyInfo(Variant::INT, "track"), PropertyInfo(Variant::DICTIONARY, "event")));
	ADD_SIGNAL(MethodInfo("animation_end", PropertyInfo(Variant::INT, "track")));
	ADD_SIGNAL(MethodInfo("predelete"));

	BIND_ENUM_CONSTANT(ANIMATION_PROCESS_FIXED);
	BIND_ENUM_CONSTANT(ANIMATION_PROCESS_IDLE);

	BIND_ENUM_CONSTANT(DEBUG_ATTACHMENT_REGION);
	BIND_ENUM_CONSTANT(DEBUG_ATTACHMENT_MESH);
	BIND_ENUM_CONSTANT(DEBUG_ATTACHMENT_SKINNED_MESH);
	BIND_ENUM_CONSTANT(DEBUG_ATTACHMENT_BOUNDING_BOX);
}

/*
Rect2 Spine::_edit_get_rect() const {

	if (skeleton == NULL)
		return Node2D::_edit_get_rect();

	float minX = 65535, minY = 65535, maxX = -65535, maxY = -65535;
	bool attached = false;
	for (int i = 0; i < skeleton->slotsCount; ++i) {

		spSlot *slot = skeleton->slots[i];
		if (!slot->attachment) continue;
		int verticesCount;
		if (slot->attachment->type == SP_ATTACHMENT_REGION) {
			spRegionAttachment *attachment = (spRegionAttachment *)slot->attachment;
			spRegionAttachment_computeWorldVertices(attachment, slot->bone, world_verts.ptrw(), 0, 2);
			verticesCount = 8;
		} else if (slot->attachment->type == SP_ATTACHMENT_MESH) {
			spMeshAttachment *mesh = (spMeshAttachment *)slot->attachment;
			spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, world_verts.ptrw(), 0, 2);
			verticesCount = ((spVertexAttachment *)mesh)->worldVerticesLength;
		} else
			continue;

		attached = true;

		for (int ii = 0; ii < verticesCount; ii += 2) {
			float x = world_verts[ii] * 1, y = world_verts[ii + 1] * 1;
			minX = MIN(minX, x);
			minY = MIN(minY, y);
			maxX = MAX(maxX, x);
			maxY = MAX(maxY, y);
		}
	}

	int h = maxY - minY;
	return attached ? Rect2(minX, -minY - h, maxX - minX, h) : Node2D::_edit_get_rect();
}
*/

void Spine4::_update_verties_count() {

	ERR_FAIL_COND(skeleton == NULL);

	int verties_count = 0;
	for (int i = 0, n = skeleton->slotsCount; i < n; i++) {

		spSlot *slot = skeleton->drawOrder[i];
		if (!slot->attachment)
			continue;

		switch (slot->attachment->type) {

			case SP_ATTACHMENT_MESH:
			case SP_ATTACHMENT_LINKED_MESH:
			case SP_ATTACHMENT_BOUNDING_BOX:
				verties_count = MAX(verties_count, ((spVertexAttachment *)slot->attachment)->verticesCount + 1);
				break;
			default:
				continue;
		}
	}

	if (verties_count > world_verts.size()) {
		world_verts.resize(verties_count);
		memset(world_verts.ptrw(), 0, world_verts.size() * sizeof(float));
	}
}

Spine4::Spine4()
	: batcher(this), fx_node(memnew(Node2D)), fx_batcher(fx_node) {

	skeleton = NULL;
	root_bone = NULL;
	clipper = NULL;
	state = NULL;
	res = RES();
	world_verts.resize(1000); // Max number of vertices per mesh.
	memset(world_verts.ptrw(), 0, world_verts.size() * sizeof(float));
	speed_scale = 1;
	autoplay = "";
	animation_process_mode = ANIMATION_PROCESS_IDLE;
	processing = false;
	active = false;
	playing = false;
	forward = true;
	process_delta = 0;
	skip_frames = 0;
	frames_to_skip = 0;

	debug_bones = false;
	debug_attachment_region = false;
	debug_attachment_mesh = false;
	debug_attachment_skinned_mesh = false;
	debug_attachment_bounding_box = false;

	skin = "";
	current_animation = "[stop]";
	loop = true;
	fx_slot_prefix = String("fx/").utf8();
	state_hash = "";
	process_queued = false;

	modulate = Color(1, 1, 1, 1);
	flip_x = false;
	flip_y = false;
	individual_textures = false;

	performance_triangles_drawn = 0;
	performance_triangles_generated = 0;
}

Spine4::~Spine4() {

	// cleanup
	_spine_dispose();
}


#endif // MODULE_SPINE_ENABLED
