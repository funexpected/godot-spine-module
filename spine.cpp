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
#include "spine.h"
#include "core/io/resource_loader.h"


VARIANT_ENUM_CAST(Spine::AnimationProcessMode);
VARIANT_ENUM_CAST(Spine::DebugAttachmentMode);

Array *Spine::invalid_names = NULL;
Array Spine::get_invalid_names() {
	if (invalid_names == NULL) {
		invalid_names = memnew(Array());
	}
	return *invalid_names;
}



Ref<Resource> ResourceFormatLoaderSpine::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	return SpineRuntime::load_resource(p_path);
}



void ResourceFormatLoaderSpine::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("skel");
	p_extensions->push_back("json");
	p_extensions->push_back("atlas");
}

bool ResourceFormatLoaderSpine::handles_type(const String &p_type) const {
	return (p_type=="SpineResource");
}

String ResourceFormatLoaderSpine::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el=="json" || el=="skel")
		return "SpineResource";
	return "";
}


// void Spine::_on_animation_state_event(int p_track, spEventType p_type, spEvent *p_event, int p_loop_count) {

// 	switch (p_type) {
// 		case SP_ANIMATION_START:
// 			emit_signal("animation_start", p_track);
// 			break;
// 		case SP_ANIMATION_COMPLETE:
// 			emit_signal("animation_complete", p_track, p_loop_count);
// 			break;
// 		case SP_ANIMATION_EVENT: {
// 			Dictionary event;
// 			event["name"] = p_event->data->name;
// 			event["int"] = p_event->intValue;
// 			event["float"] = p_event->floatValue;
// 			event["string"] = p_event->stringValue ? p_event->stringValue : "";
// 			emit_signal("animation_event", p_track, event);
// 		} break;
// 		case SP_ANIMATION_END:
// 			emit_signal("animation_end", p_track);
// 			break;
// 	}
// }

void Spine::_spine_dispose() {

	if (playing) {
		// stop first
		stop();
	}

	res = Ref<SpineResource>();
	runtime = Ref<SpineRuntime>();

	queue_redraw();
}

void Spine::_animation_draw() {
	if (runtime.is_null())
		return;

	runtime->batch(&batcher, modulate, flip_x, flip_y, individual_textures);
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
	runtime->process(forward ? process_delta : -process_delta);
	process_delta = 0;

	// spAnimationState_update(state, forward ? process_delta : -process_delta);
	// spAnimationState_apply(state, skeleton);
	// spSkeleton_updateWorldTransform(skeleton);
	// process_delta = 0;

	// // Calculate and draw mesh only if timelines has been changed
	// String current_state_hash = build_state_hash();
	// if (current_state_hash == state_hash) {
	// 	return;
	// } else {
	// 	state_hash = current_state_hash;
	// }


	// for (AttachmentNodes::Element *E = attachment_nodes.front(); E; E = E->next()) {

	// 	AttachmentNode &info = E->get();
	// 	WeakRef *ref = info.ref;
	// 	Object *obj = ref->get_ref();
	// 	Node2D *node = (obj != NULL) ? Object::cast_to<Node2D>(obj) : NULL;
	// 	if (obj == NULL || node == NULL) {

	// 		AttachmentNodes::Element *NEXT = E->next();
	// 		attachment_nodes.erase(E);
	// 		E = NEXT;
	// 		if (E == NULL)
	// 			break;
	// 		continue;
	// 	}
    //     spSlot *slot = info.slot;
	// 	spBone *bone = slot->bone;
	// 	node->call("set_position", Vector2(bone->worldX + bone->skeleton->x, -bone->worldY + bone->skeleton->y) + info.ofs);
	// 	node->call("set_scale", Vector2(spBone_getWorldScaleX(bone), spBone_getWorldScaleY(bone)) * info.scale);
	// 	//node->call("set_rotation", Math::atan2(bone->c, bone->d) + Math::deg2rad(info.rot));
	// 	node->call("set_rotation_degrees", spBone_getWorldRotationX(bone) + info.rot);
    //     Color c;
    //     c.a = slot->color.a;
    //     c.r = slot->color.r;
    //     c.g = slot->color.g;
    //     c.b = slot->color.b;
    //     node->call("set_modulate", c);
	// }
	queue_redraw();
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
	if (runtime.is_valid() && runtime->_rt_set(p_name, p_value)) {
		return true;
	}

	String name = p_name;
	if (name == "playback/play") {

		String which = p_value;
		if (runtime.is_valid()) {

			if (which == "[stop]")
				stop();
			else if (has_animation(which)) {
				reset();
				play(which, 1, loop);
			}
		} else {
			current_animation = which;
		}
	} else if (name == "playback/loop") {

		loop = p_value;
		if (has_animation(current_animation))
			play(current_animation, 1, loop);
	} else if (name == "playback/forward") {

		forward = p_value;
	} else if (name == "playback/skin") {

		skin = p_value;
		if (runtime.is_valid())
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
	if (runtime.is_valid() && runtime->_rt_get(p_name, r_ret)) {
		return true;
	}

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
	else if (name == "performance/triangles_drawn") {
		r_ret = performance_triangles_drawn;
	} else if (name == "performance/triangles_generated") {
		r_ret = performance_triangles_generated;
	}

	return true;
}

float Spine::get_animation_length(String p_animation) const {
	if (runtime.is_valid()) {
		return runtime->get_animation_length(p_animation);
	} else {
		return 0.0;
	}
}

void Spine::_get_property_list(List<PropertyInfo> *p_list) const {
	if (runtime.is_valid()) {
		runtime->_rt_get_property_list(p_list);
	}

	List<String> names;
	Array names_array = get_animation_names();
	for (int i = 0; i < names_array.size(); i++) {
		names.push_back(names_array[i]);
	}
	if (runtime.is_valid()) {
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
	names_array.clear();
	if (runtime.is_valid()) {
		names_array = runtime->get_skin_names();
	}
	for (int i = 0; i < names_array.size(); i++) {
		names.push_back(names_array[i]);
	}
	if (runtime.is_valid()) {
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

void Spine::set_resource(Ref<SpineResource> p_data) {

	if (res == p_data)
		return;

	_spine_dispose(); // cleanup

	res = p_data;

	if (res.is_null()) {
		return;
	}
	
	runtime = res->create_runtime();
	// TODO: connect signals
	// runtime->connect("event", this, "emit_signal");
	// runtime->connect("event", callable_mp(this, &Spine::emit_spine_signal));


	// if (res.is_null())
	// 	return;

	// ERR_FAIL_COND(!res->data);

	// skeleton = spSkeleton_create(res->data);
	// root_bone = skeleton->bones[0];
	// clipper = spSkeletonClipping_create();

	// state = spAnimationState_create(spAnimationStateData_create(skeleton->data));
	// state->rendererObject = this;
	// state->listener = spine_animation_callback;

	// _update_verties_count();

	if (skin != "")
		set_skin(skin);
	if (current_animation != "[stop]")
		play(current_animation, 1, loop);
	else
		reset();

}

// void Spine::emit_spine_signal(String str, String str2) {
// 	emit_signal(str, str2);
// }

Ref<SpineResource> Spine::get_resource() {
	return res;
}

Array Spine::get_animation_names() const {
	if (runtime.is_valid()) {
		return runtime->get_animation_names();
	} else {
		return Array();
	}
}

bool Spine::has_animation(const String &p_name) {
	if (runtime.is_valid()) {
		return runtime->has_animation(p_name);
	} else {
		return false;
	}
}

void Spine::mix(const String &p_from, const String &p_to, real_t p_duration) {
	if (runtime.is_valid()) {
		runtime->mix(p_from, p_to, p_duration);
	}
}

bool Spine::play(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, float p_delay) {
	if (runtime.is_valid()) {
		if (!runtime->play(p_name, p_cunstom_scale, p_loop, p_track, p_delay)){
			return false;
		}
	}
	
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
	if (runtime.is_valid()) {
		runtime->set_animation_state(p_track, p_animation, p_pos);
		queue_process();
	}
}

bool Spine::add(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, float p_delay) {

	if (runtime.is_valid() && runtime->add(p_name, p_cunstom_scale, p_loop, p_track, p_delay)) {
		_set_process(true);
		playing = true;
		return true;
	} else {
		return false;
	}
	
}

void Spine::clear(int p_track) {
	if (runtime.is_valid()) {
		runtime->clear();
	}
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
	return runtime.is_valid() && runtime->is_playing(p_track);
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
	if (runtime.is_valid()) {
		return runtime->get_current_animation();
	} else {
		return String();
	}
}

void Spine::stop_all() {

	stop();

	_set_process(false); // always process when starting an animation
}

void Spine::reset() {
	if (runtime.is_valid()) {
		runtime->reset();
	}
}

void Spine::seek(int track, float p_pos) {
	if (runtime.is_valid()) {
		runtime->seek(track, p_pos);
	}
}

float Spine::tell(int track) const {
	if (runtime.is_valid()) {
		return runtime->tell(track);
	} else {
		return 0.0;
	}
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
	queue_redraw();
}

Color Spine::get_modulate() const {

	return modulate;
}

void Spine::set_flip_x(bool p_flip) {

	flip_x = p_flip;
	queue_redraw();
}

void Spine::set_individual_textures(bool is_individual)	{
	individual_textures = is_individual;
	queue_redraw();
}

bool Spine::get_individual_textures() const {
	return individual_textures;
}

void Spine::set_flip_y(bool p_flip) {

	flip_y = p_flip;
	queue_redraw();
}

bool Spine::is_flip_x() const {

	return flip_x;
}

bool Spine::is_flip_y() const {

	return flip_y;
}

bool Spine::set_skin(const String &p_name) {
	if (runtime.is_valid() && p_name.length() > 0) {
		return runtime->set_skin(p_name);
	} else {
		return false;
	}
}

Dictionary Spine::get_skeleton() const {
	if (runtime.is_valid()) {
		return runtime->get_skeleton(individual_textures);
	} else {
		return Dictionary();
	}
}

Dictionary Spine::get_attachment(const String &p_slot_name, const String &p_attachment_name) const {
	if (runtime.is_valid()) {
		return runtime->get_attachment(p_slot_name, p_attachment_name);
	} else {
		return Dictionary();
	}
}

Dictionary Spine::get_bone(const String &p_bone_name) const {
	if (runtime.is_valid()) {
		return runtime->get_bone(p_bone_name);
	} else {
		return Dictionary();
	}
}

Dictionary Spine::get_slot(const String &p_slot_name) const {
	if (runtime.is_valid()) {
		return runtime->get_slot(p_slot_name);
	} else {
		return Dictionary();
	}
}

bool Spine::set_attachment(const String &p_slot_name, const Variant &p_attachment) {
	if (runtime.is_valid()) {
		return runtime->set_attachment(p_slot_name, p_attachment);
	} else {
		return false;
	}
}

bool Spine::has_attachment_node(const String &p_bone_name, const Variant &p_node) {
	if (runtime.is_valid()) {
		return runtime->has_attachment_node(p_bone_name, p_node);
	} else {
		return false;
	}
}

bool Spine::add_attachment_node(const String &p_bone_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {
	if (runtime.is_valid()) {
		return runtime->add_attachment_node(p_bone_name, p_node, p_ofs, p_scale, p_rot);
	} else  {
		return false;
	}

	// ERR_FAIL_COND_V(skeleton == NULL, false);
	// spSlot *slot = spSkeleton_findSlot(skeleton, p_bone_name.utf8().get_data());
	// ERR_FAIL_COND_V(slot == NULL, false);
	// Object *obj = p_node;
	// ERR_FAIL_COND_V(obj == NULL, false);
	// Node2D *node = Object::cast_to<Node2D>(obj);
	// ERR_FAIL_COND_V(node == NULL, false);

	// if (obj->has_meta("spine_meta")) {

	// 	AttachmentNode *info = (AttachmentNode *)((uint64_t)obj->get_meta("spine_meta"));
	// 	if (info->slot !=  slot) {
	// 		// add to different bone, remove first
	// 		remove_attachment_node(info->slot->data->name, p_node);
	// 	} else {
	// 		// add to same bone, update params
	// 		info->ofs = p_ofs;
	// 		info->scale = p_scale;
	// 		info->rot = p_rot;
	// 		return true;
	// 	}
	// }
	// attachment_nodes.push_back(AttachmentNode());
	// AttachmentNode &info = attachment_nodes.back()->get();
	// info.E = attachment_nodes.back();
	// info.slot = slot;
	// info.ref = memnew(WeakRef);
	// info.ref->set_obj(node);
	// info.ofs = p_ofs;
	// info.scale = p_scale;
	// info.rot = p_rot;
	// obj->set_meta("spine_meta", (uint64_t)&info);

	// return true;
}

bool Spine::remove_attachment_node(const String &p_bone_name, const Variant &p_node) {
	if (runtime.is_valid()) {
		return runtime->remove_attachment_node(p_bone_name, p_node);
	} else {
		return false;
	}
 
	// ERR_FAIL_COND_V(skeleton == NULL, false);
	// spSlot *slot = spSkeleton_findSlot(skeleton, p_bone_name.utf8().get_data());
	// ERR_FAIL_COND_V(slot == NULL, false);
	// Object *obj = p_node;
	// ERR_FAIL_COND_V(obj == NULL, false);
	// Node2D *node = Object::cast_to<Node2D>(obj);
	// ERR_FAIL_COND_V(node == NULL, false);

	// if (!obj->has_meta("spine_meta"))
	// 	return false;

	// AttachmentNode *info = (AttachmentNode *)((uint64_t)obj->get_meta("spine_meta"));
	// ERR_FAIL_COND_V(info->slot != slot, false);
	// obj->set_meta("spine_meta", Variant());
	// memdelete(info->ref);
	// attachment_nodes.erase(info->E);

	// return false;
}

Ref<Shape2D> Spine::get_bounding_box(const String &p_slot_name, const String &p_attachment_name) {
	if (runtime.is_valid()) {
		return runtime->get_bounding_box(p_slot_name, p_attachment_name);
	} else {
		return Ref<Shape2D>();
	}
	// ERR_FAIL_COND_V(skeleton == NULL, Ref<Shape2D>());
	// spAttachment *attachment = spSkeleton_getAttachmentForSlotName(skeleton, p_slot_name.utf8().get_data(), p_attachment_name.utf8().get_data());
	// ERR_FAIL_COND_V(attachment == NULL, Ref<Shape2D>());
	// ERR_FAIL_COND_V(attachment->type != SP_ATTACHMENT_BOUNDING_BOX, Ref<Shape2D>());
	// spVertexAttachment *info = (spVertexAttachment *)attachment;

	// Vector<Vector2> points;
	// points.resize(info->verticesCount / 2);
	// for (int idx = 0; idx < info->verticesCount / 2; idx++)
	// 	points.write[idx] = Vector2(info->vertices[idx * 2], -info->vertices[idx * 2 + 1]);

	// ConvexPolygonShape2D *shape = memnew(ConvexPolygonShape2D);
	// shape->set_points(points);

	// return shape;
}

bool Spine::add_bounding_box(const String &p_bone_name, const String &p_slot_name, const String &p_attachment_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {
	if (runtime.is_valid()) {
		return runtime->add_bounding_box(p_bone_name, p_slot_name, p_attachment_name, p_node, p_ofs, p_scale, p_rot);
	} else {
		return false;
	}

	// ERR_FAIL_COND_V(skeleton == NULL, false);
	// Object *obj = p_node;
	// ERR_FAIL_COND_V(obj == NULL, false);
	// CollisionObject2D *node = Object::cast_to<CollisionObject2D>(obj);
	// ERR_FAIL_COND_V(node == NULL, false);
	// Ref<Shape2D> shape = get_bounding_box(p_slot_name, p_attachment_name);
	// if (shape.is_null())
	// 	return false;
	// node->shape_owner_add_shape(0, shape);

	// return add_attachment_node(p_bone_name, p_node);
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

// void Spine::set_fx_slot_prefix(const String &p_prefix) {

// 	fx_slot_prefix = p_prefix.utf8();
// 	queue_redraw();
// }

// String Spine::get_fx_slot_prefix() const {

// 	String s;
// 	s.parse_utf8(fx_slot_prefix.get_data());
// 	return s;
// }

void Spine::set_debug_bones(bool p_enable) {

	debug_bones = p_enable;
	queue_redraw();
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
	queue_redraw();
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

	// ClassDB::bind_method(D_METHOD("set_fx_slot_prefix", "prefix"), &Spine::set_fx_slot_prefix);
	// ClassDB::bind_method(D_METHOD("get_fx_slot_prefix"), &Spine::get_fx_slot_prefix);

	ClassDB::bind_method(D_METHOD("set_debug_bones", "enable"), &Spine::set_debug_bones);
	ClassDB::bind_method(D_METHOD("is_debug_bones"), &Spine::is_debug_bones);
	ClassDB::bind_method(D_METHOD("set_debug_attachment", "mode", "enable"), &Spine::set_debug_attachment);
	ClassDB::bind_method(D_METHOD("is_debug_attachment", "mode"), &Spine::is_debug_attachment);

	// ClassDB::bind_method(D_METHOD("_on_fx_draw"), &Spine::_on_fx_draw);
	ClassDB::bind_method(D_METHOD("_animation_process"), &Spine::_animation_process);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "process_mode", PROPERTY_HINT_ENUM, "Fixed,Idle"), "set_animation_process_mode", "get_animation_process_mode");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed", PROPERTY_HINT_RANGE, "-64,64,0.01"), "set_speed", "get_speed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "skip_frames", PROPERTY_HINT_RANGE, "0, 100, 1"), "set_skip_frames", "get_skip_frames");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_bones"), "set_debug_bones", "is_debug_bones");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_x"), "set_flip_x", "is_flip_x");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "individual_textures"), "set_individual_textures", "get_individual_textures");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_y"), "set_flip_y", "is_flip_y");
	// ADD_PROPERTY(PropertyInfo(Variant::STRING, "fx_prefix"), "set_fx_slot_prefix", "get_fx_slot_prefix");
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

// void Spine::_update_verties_count() {

// 	ERR_FAIL_COND(skeleton == NULL);

// 	int verties_count = 0;
// 	for (int i = 0, n = skeleton->slotsCount; i < n; i++) {

// 		spSlot *slot = skeleton->drawOrder[i];
// 		if (!slot->attachment)
// 			continue;

// 		switch (slot->attachment->type) {

// 			case SP_ATTACHMENT_MESH:
// 			case SP_ATTACHMENT_LINKED_MESH:
// 			case SP_ATTACHMENT_BOUNDING_BOX:
// 				verties_count = MAX(verties_count, ((spVertexAttachment *)slot->attachment)->verticesCount + 1);
// 				break;
// 			default:
// 				continue;
// 		}
// 	}

// 	if (verties_count > world_verts.size()) {
// 		world_verts.resize(verties_count);
// 		memset(world_verts.ptrw(), 0, world_verts.size() * sizeof(float));
// 	}
// }

Spine::Spine()
	: batcher(this) {
	res = Ref<SpineResource>();
	runtime = Ref<SpineRuntime>();
	// world_verts.resize(1000); // Max number of vertices per mesh.
	// memset(world_verts.ptrw(), 0, world_verts.size() * sizeof(float));
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
