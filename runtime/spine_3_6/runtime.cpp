#include <spine/Atlas.h>
#include <spine/extension.h>
#include <spine/spine.h>
#define SPINE_RUNTIME_CLASS

#include "runtime.h"
#include "modules/spine/spine_batcher.h"
#include "scene/resources/convex_polygon_shape_2d.h"
#include "scene/2d/collision_object_2d.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "scene/resources/texture.h"
#include "scene/resources/image_texture.h"

typedef Ref<Texture2D> TextureRef;
typedef Ref<ImageTexture> ImageTextureRef;

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path) {
	TextureRef *ref = memnew(TextureRef);
	*ref = ResourceLoader::load(path);
	if (!ref->is_null()){
		self->rendererObject = ref;
		self->width = (*ref)->get_width();
		self->height = (*ref)->get_height();
	} else {
		memdelete(ref);
		Ref<Image> img = memnew(Image);
		if (img->load(path) == OK){
			ImageTextureRef *imgtex = memnew(ImageTextureRef);
			(*imgtex) = Ref<ImageTexture>(memnew(ImageTexture));
			(*imgtex)->create_from_image(img);
			self->rendererObject = imgtex;
			self->width = (*imgtex)->get_width();
			self->height = (*imgtex)->get_height();
		} else {
			ERR_FAIL();
		}
	}
}

void _spAtlasPage_disposeTexture(spAtlasPage* self) {

	if(TextureRef *ref = static_cast<TextureRef *>(self->rendererObject))
		memdelete(ref);
}


char* _spUtil_readFile(const char* p_path, int* p_length) {

	String str_path = String::utf8(p_path);
	
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);
	ERR_FAIL_COND_V_MSG(file.is_null(), NULL, "Cannot open file '" + String(p_path) + "'.");

	*p_length = file->get_length();

	char *data = (char *)_spMalloc(*p_length, __FILE__, __LINE__);
	// data = (char *)_spMalloc(*p_length, __FILE__, __LINE__);
	ERR_FAIL_COND_V(data == NULL, NULL);

	file->get_buffer((uint8_t *)data, *p_length);
	file->close();
	return data;
}

static void *spine_malloc(size_t p_size) {

	if (p_size == 0)
		return NULL;
	return memalloc(p_size);
}

static void *spine_realloc(void *ptr, size_t p_size) {

	if (p_size == 0)
		return NULL;
	return memrealloc(ptr, p_size);
}

static void spine_free(void *ptr) {

	if (ptr == NULL)
		return;
	memfree(ptr);
}

void _on_animation_state_event(SpineRuntime_3_6* self, int p_track, spEventType p_type, spEvent *p_event, int p_loop_count) {

	switch (p_type) {
		case SP_ANIMATION_START:
			self->emit_signal("animation_start", p_track);
			break;
		case SP_ANIMATION_COMPLETE:
			self->emit_signal("animation_complete", p_track, p_loop_count);
			break;
		case SP_ANIMATION_EVENT: {
			Dictionary event;
			event["name"] = p_event->data->name;
			event["int"] = p_event->intValue;
			event["float"] = p_event->floatValue;
			event["string"] = p_event->stringValue ? p_event->stringValue : "";
			self->emit_signal("animation_event", p_track, event);
		} break;
		case SP_ANIMATION_END:
			self->emit_signal("animation_end", p_track);
			break;
	}
}

static void spine_animation_callback(spAnimationState *p_state, spEventType p_type, spTrackEntry *p_track, spEvent *p_event) {
	_on_animation_state_event(((SpineRuntime_3_6 *)p_state->rendererObject), p_track->trackIndex, p_type, p_event, 1);
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

void SpineRuntime_3_6::init() {
    _spSetMalloc(spine_malloc);
	_spSetRealloc(spine_realloc);
	_spSetFree(spine_free);
}


Ref<SpineResource> SpineRuntime_3_6::load_resource(const String &p_path) {
    Ref<SpineResource> res;
    res.instantiate();
    String p_atlas = p_path.get_basename() + ".atlas";
    spAtlas* atlas = NULL;
    spSkeletonData* data = NULL;
    atlas = spAtlas_createFromFile(p_atlas.utf8().get_data(), 0);
    ERR_FAIL_COND_V(atlas == NULL, Ref<SpineResource>());
    if (p_path.get_extension() == "json"){
        spSkeletonJson *json = spSkeletonJson_create(atlas);
        if (json == NULL) {
            spAtlas_dispose(atlas);
            return Ref<SpineResource>();
        }
        json->scale = 1;

        data = spSkeletonJson_readSkeletonDataFile(json, p_path.utf8().get_data());
        String err_msg = json->error ? json->error : "";
        spSkeletonJson_dispose(json);
        if (data == NULL) {
            spAtlas_dispose(atlas);
			ERR_FAIL_V_MSG(Ref<SpineResource>(), err_msg);
        }
    } else {
        spSkeletonBinary* bin  = spSkeletonBinary_create(atlas);
        if (bin == NULL) {
            spAtlas_dispose(atlas);
            return Ref<SpineResource>();
        }
        bin->scale = 1;
        data = spSkeletonBinary_readSkeletonDataFile(bin, p_path.utf8().get_data());
        String err_msg = bin->error ? bin->error : "";
        spSkeletonBinary_dispose(bin);
        if (data == NULL) {
            spAtlas_dispose(atlas);
            ERR_FAIL_V_MSG(Ref<SpineResource>(), err_msg);
        }
    }

    res->atlas = (void*)atlas;
    res->data = (void*)data;
    res->set_path(p_path);
    res->runtime_version = "3_6";
    return res;
}

Ref<SpineRuntime> SpineRuntime_3_6::with_resource(Ref<SpineResource> res) {
	if (res.is_null())
		return Ref<SpineRuntime>();

	if (!res->data) {
        return Ref<SpineRuntime>();
    }

    Ref<SpineRuntime_3_6> rt;
    rt.instantiate();

	rt->skeleton = spSkeleton_create((spSkeletonData*)res->data);
	rt->root_bone = rt->skeleton->bones[0];
	rt->clipper = spSkeletonClipping_create();

	rt->state = spAnimationState_create(spAnimationStateData_create(rt->skeleton->data));
	rt->state->rendererObject = rt.ptr();
	rt->state->listener = spine_animation_callback;
    // return memnew(SpineRuntime_3_6);
    return rt;
}

void SpineRuntime_3_6::_rt_get_property_list(List<PropertyInfo> *p_list) const {
	
}

bool SpineRuntime_3_6::_rt_get(const StringName &p_name, Variant &r_ret) const {
	String name = p_name;
	if (name.begins_with("bone/")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spBone *bone = spSkeleton_findBone(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(bone==NULL, false);
		if (params[2] == "rotation")
			r_ret = bone->rotation;
		else if (params[2] == "position")
			r_ret = Vector2(bone->x, bone->y);
		return true;
	} else {
		return false;
	}
}

bool SpineRuntime_3_6::_rt_set(const StringName &p_name, const Variant &p_value) {
    String name = p_name;
	if (name.begins_with("path")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size()!=3) return true;
		spPathConstraint *pc = spSkeleton_findPathConstraint(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(pc==NULL, true);
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
        return true;
	} else if (name.begins_with("bone")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spBone *bone = spSkeleton_findBone(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(bone==NULL, true);
		if (params[2] == "rotation"){
			bone->rotation = p_value;
		}
		else if (params[2] == "position"){
			Vector2 v(p_value);
			bone->x = v.x;
			bone->y = v.y;
		}
		spSkeleton_updateWorldTransform(skeleton);
        return true;
	} else if (name.begins_with("slot")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		spSlot *slot = spSkeleton_findSlot(skeleton, params[1].utf8().get_data());
		ERR_FAIL_COND_V(slot==NULL, true);
		if (params[2] == "color"){
			Color c = p_value;
			slot->color.a = skeleton->color.a * c.a;
			slot->color.r = skeleton->color.r * c.r;
			slot->color.g = skeleton->color.r * c.g;
			slot->color.b = skeleton->color.b * c.b;
		}
        return true;
	} else {
        return false;
    }
}

void SpineRuntime_3_6::batch(SpineBatcher* batcher, const Color &modulate, bool flip_x, bool flip_y, bool individual_textures) {
    spColor_setFromFloats(&skeleton->color, modulate.r, modulate.g, modulate.b, modulate.a);

	int additive = 0;
	Color color;
	float *uvs = NULL;
	int verties_count = 0;
	unsigned short *triangles = NULL;
	int triangles_count = 0;
	float r = 0, g = 0, b = 0, a = 0;

	batcher->reset();


	for (int i = 0, n = skeleton->slotsCount; i < n; i++) {

		spSlot *slot = skeleton->drawOrder[i];
		if (!slot->attachment || slot->color.a == 0) {
			spSkeletonClipping_clipEnd(clipper, slot);
			continue;
		}
		Ref<Texture> texture;
		switch (slot->attachment->type) {

			case SP_ATTACHMENT_REGION: {

				spRegionAttachment *attachment = (spRegionAttachment *)slot->attachment;
				if (attachment->color.a == 0){
					spSkeletonClipping_clipEnd(clipper, slot);
					continue;
				}
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
			batcher->add(texture, clipper->clippedVertices->items,
								 clipper->clippedUVs->items,
								 clipper->clippedVertices->size,
								 clipper->clippedTriangles->items,
								 clipper->clippedTriangles->size,
								 &color, flip_x, flip_y, (slot->data->index)*individual_textures);
		} else {
			batcher->add(texture, world_verts.ptr(), uvs, verties_count, triangles, triangles_count, &color, flip_x, flip_y, (slot->data->index)*individual_textures);
		}
		spSkeletonClipping_clipEnd(clipper, slot);
	}


	spSkeletonClipping_clipEnd2(clipper);
	// performance_triangles_drawn = performance_triangles_generated = batcher.triangles_count();
	batcher->flush();
}

void SpineRuntime_3_6::process(float delta) {
    spAnimationState_update(state, delta);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);

	// Calculate and draw mesh only if timelines has been changed
	// String current_state_hash = build_state_hash();
	// if (current_state_hash == state_hash) {
	// 	return;
	// } else {
	// 	state_hash = current_state_hash;
	// }


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
}

float SpineRuntime_3_6::get_animation_length(String p_animation) const {
    if (state == NULL) return 0;
	for (int i = 0; i < state->data->skeletonData->animationsCount; i++) {
		spAnimation *anim = state->data->skeletonData->animations[i];
		if (anim->name == p_animation) {
			return anim->duration;
		}
	}
	return 0.0;
}

Array SpineRuntime_3_6::get_animation_names() const {
    Array names;
	if (state != NULL) {
		for (int i = 0; i < state->data->skeletonData->animationsCount; i++) {
			spAnimation *anim = state->data->skeletonData->animations[i];
			names.push_back(anim->name);
		}
	}
	return names;
}

Array SpineRuntime_3_6::get_skin_names() const {
    Array names;
    if (state != NULL) {
        for (int i = 0; i < state->data->skeletonData->skinsCount; i++) {
            names.push_back(state->data->skeletonData->skins[i]->name);
        }
    }
    return names;
}

bool SpineRuntime_3_6::has_animation(const String &p_name) {
	if (skeleton == NULL) return false;
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	return animation != NULL;
}

bool SpineRuntime_3_6::set_skin(const String &p_name) {
	ERR_FAIL_COND_V(skeleton == NULL, false);
	return spSkeleton_setSkinByName(skeleton, p_name.utf8().get_data()) ? true : false;
}



void SpineRuntime_3_6::mix(const String &p_from, const String &p_to, real_t p_duration) {
	ERR_FAIL_COND(state == NULL);
	spAnimationStateData_setMixByName(state->data, p_from.utf8().get_data(), p_to.utf8().get_data(), p_duration);
}

bool SpineRuntime_3_6::play(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, float p_delay) {
	ERR_FAIL_COND_V(skeleton == NULL, false);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	ERR_FAIL_COND_V(animation == NULL, false);
	spTrackEntry *entry = spAnimationState_setAnimation(state, p_track, animation, p_loop);
	entry->delay = p_delay;
	entry->timeScale = p_cunstom_scale;
	return true;
}

void SpineRuntime_3_6::set_animation_state(int p_track, String p_animation, float p_pos) {
	ERR_FAIL_COND(skeleton == NULL);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_animation.utf8().get_data());
	ERR_FAIL_COND(animation == NULL);
	spTrackEntry *entry = spAnimationState_setAnimation(state, p_track, animation, false);
	if (entry == NULL) return;
	entry->trackTime = p_pos;
}

bool SpineRuntime_3_6::add(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, float p_delay) {
	ERR_FAIL_COND_V(skeleton == NULL, false);
	spAnimation *animation = spSkeletonData_findAnimation(skeleton->data, p_name.utf8().get_data());
	ERR_FAIL_COND_V(animation == NULL, false);
	spTrackEntry *entry = spAnimationState_addAnimation(state, p_track, animation, p_loop, p_delay);
	return true;
}

void SpineRuntime_3_6::clear(int p_track) {
	ERR_FAIL_COND(state == NULL);
	if (p_track == -1)
		spAnimationState_clearTracks(state);
	else
		spAnimationState_clearTrack(state, p_track);
}

bool SpineRuntime_3_6::is_playing(int p_track) const {
	spTrackEntry *entry = spAnimationState_getCurrent(state, p_track);
	if (entry == NULL) return false;
	return entry->loop || entry->trackTime < entry->animationEnd;
}

String SpineRuntime_3_6::get_current_animation(int p_track) const {
	ERR_FAIL_COND_V(state == NULL, "");
	spTrackEntry *entry = spAnimationState_getCurrent(state, p_track);
	if (entry == NULL || entry->animation == NULL)
		return "";
	return entry->animation->name;
}

void SpineRuntime_3_6::reset() {
	if (skeleton == NULL) {
		return;
	}
	spSkeleton_setToSetupPose(skeleton);
	spAnimationState_update(state, 0);
	spAnimationState_apply(state, skeleton);
	spSkeleton_updateWorldTransform(skeleton);
}

void SpineRuntime_3_6::seek(int track, float p_pos) {
	if (state == NULL) return;
	spTrackEntry *entry = spAnimationState_getCurrent(state, track);
	if (entry == NULL) return;
	entry->trackTime = p_pos;
}

float SpineRuntime_3_6::tell(int track) const {
	if (state == NULL) return 0;
	spTrackEntry *entry = spAnimationState_getCurrent(state, track);
	if (entry == NULL) return 0;
	return entry->trackTime;
}

Dictionary SpineRuntime_3_6::get_skeleton(bool individual_textures) const {

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

Dictionary SpineRuntime_3_6::get_attachment(const String &p_slot_name, const String &p_attachment_name) const {

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

Dictionary SpineRuntime_3_6::get_bone(const String &p_bone_name) const {

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

Dictionary SpineRuntime_3_6::get_slot(const String &p_slot_name) const {
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

bool SpineRuntime_3_6::set_attachment(const String &p_slot_name, const Variant &p_attachment) {
	ERR_FAIL_COND_V(skeleton == NULL, false);
	if (p_attachment.get_type() == Variant::STRING)
		return spSkeleton_setAttachment(skeleton, p_slot_name.utf8().get_data(), ((const String)p_attachment).utf8().get_data()) != 0;
	else
		return spSkeleton_setAttachment(skeleton, p_slot_name.utf8().get_data(), NULL) != 0;
}

bool SpineRuntime_3_6::has_attachment_node(const String &p_bone_name, const Variant &p_node) {

	return false;
}

bool SpineRuntime_3_6::add_attachment_node(const String &p_bone_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {

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

bool SpineRuntime_3_6::remove_attachment_node(const String &p_bone_name, const Variant &p_node) {

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

Ref<Shape2D> SpineRuntime_3_6::get_bounding_box(const String& p_slot_name, const String& p_attachment_name) {
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

bool SpineRuntime_3_6::add_bounding_box(const String &p_bone_name, const String &p_slot_name, const String &p_attachment_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {

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

Vector2 SpineRuntime_3_6::get_bone_position(const String &bone_name) {
    spBone *bone = spSkeleton_findBone(skeleton, bone_name.utf8().get_data());
    if (bone == NULL) return Vector2();
    return Vector2(bone->x, bone->y);
}
float SpineRuntime_3_6::get_bone_rotation(const String &bone_name) {
    spBone *bone = spSkeleton_findBone(skeleton, bone_name.utf8().get_data());
    if (bone == NULL) return 0.0;
    return bone->rotation;
}



SpineRuntime_3_6::SpineRuntime_3_6() {
    world_verts.resize(1000); // Max number of vertices per mesh.
	memset(world_verts.ptrw(), 0, world_verts.size() * sizeof(float));
}

SpineRuntime_3_6::~SpineRuntime_3_6() {
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
    
    for (AttachmentNodes::Element *E = attachment_nodes.front(); E; E = E->next()) {
		AttachmentNode &node = E->get();
		memdelete(node.ref);
	}
	attachment_nodes.clear();
}

