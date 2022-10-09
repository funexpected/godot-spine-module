#ifndef SPINE_RUNTIME_IMPL
#define SPINE_RUNTIME_IMPL 4_2
#include "spine_4_1/runtime.h"
#endif

#define SPINE_CONC(A, B) SPINE_CONC_(A, B)
#define SPINE_CONC_(A, B) A##B
#define SPINE_STR(a) SPINE_STR_(a)
#define SPINE_STR_(a) #a

#define SPINE_RUNTIME_CLASS SPINE_CONC(SpineRuntime_, SPINE_RUNTIME_IMPL)
#define SPINE_TEXTURE_LOADER_CLASS SPINE_CONC(SpineTextureLoader_, SPINE_RUNTIME_IMPL)
#define SPINE_EXTENSION_CLASS SPINE_CONC(SpineExtension_, SPINE_RUNTIME_IMPL)
#define SPINE_RUNTIME_VERSION_STRING SPINE_STR(SPINE_RUNTIME_IMPL)


#include "core/os/file_access.h"
#include "scene/resources/texture.h"
#include "scene/resources/convex_polygon_shape_2d.h"
#include "scene/2d/collision_object_2d.h"

class SPINE_EXTENSION_CLASS: public sp::SpineExtension {
    virtual void *_alloc(size_t p_size, const char *file, int line) {
        if (p_size == 0)
            return NULL;
        return memalloc(p_size);
    }
    virtual void *_calloc(size_t p_size, const char *file, int line) {
        if (p_size == 0)
		    return NULL;
        void* ptr = memalloc(p_size);
        if (ptr) {
            memset(ptr, 0, p_size);
        }
        return ptr;
    }
    virtual void *_realloc(void *ptr, size_t p_size, const char *file, int line) {
        if (p_size == 0)
            return NULL;
        if (ptr == NULL) {
            return memalloc(p_size);
        } else {
            return memrealloc(ptr, p_size);
        }
    }
    virtual void _free(void *ptr, const char *file, int line) {
        if (ptr == NULL)
            return;
        memfree(ptr);
    }
    virtual char *_readFile(const sp::String &p_path, int *p_length) {
        String path = p_path.buffer();
        FileAccess *f = FileAccess::open(path, FileAccess::READ);
        ERR_FAIL_COND_V_MSG(!f, NULL, "Can't read file " + String(path));

        *p_length = f->get_len();

        char *data = (char *)_alloc(*p_length, __FILE__, __LINE__);
        ERR_FAIL_COND_V(data == NULL, NULL);

        f->get_buffer((uint8_t *)data, *p_length);

        memdelete(f);
        return data;
    }
};

sp::SpineExtension* sp::getDefaultExtension() {
    return new SPINE_EXTENSION_CLASS();
}

class SPINE_TEXTURE_LOADER_CLASS : public sp::TextureLoader {
	virtual void load(sp::AtlasPage &page, const sp::String &path) {
		Ref<Texture> *ref = memnew(Ref<Texture>);
		*ref = ResourceLoader::load(path.buffer());
		if (!ref->is_null()) {
			page.setRendererObject(ref);
			page.width = (*ref)->get_width();
			page.height = (*ref)->get_height();
		} else {
			memdelete(ref);
			Ref<Image> img = memnew(Image);
			if (img->load(path.buffer()) == OK) {
				Ref<ImageTexture> *imgtex = memnew(Ref<ImageTexture>);
				(*imgtex) = Ref<ImageTexture>(memnew(ImageTexture));
				(*imgtex)->create_from_image(img);
				page.setRendererObject(imgtex);
				page.width = (*imgtex)->get_width();
				page.height = (*imgtex)->get_height();
			} else {
				ERR_FAIL();
			}
		}
	}

	virtual void unload(void *texture) {

	}
};

static void spine_animation_callback(sp::AnimationState *state, sp::EventType type, sp::TrackEntry *entry, sp::Event *event) {
    SPINE_RUNTIME_CLASS* self = (SPINE_RUNTIME_CLASS*)state->getRendererObject();
    switch (type) {
		case sp::EventType_Start:
			self->emit_signal("animation_start", entry->getTrackIndex());
			break;
		case sp::EventType_Complete:
			self->emit_signal("animation_complete", entry->getTrackIndex(), 1);
			break;
		case sp::EventType_Event: {
			Dictionary data;
			data["name"] = event->getData().getName().buffer();;
			data["int"] = event->getIntValue();
			data["float"] = event->getFloatValue();
			data["string"] = event->getStringValue().buffer();
			self->emit_signal("animation_event", entry->getTrackIndex(), data);
		} break;
		case sp::EventType_End:
			self->emit_signal("animation_end", entry->getTrackIndex());
			break;
	}
}

static Ref<Texture> spine_get_texture(sp::RegionAttachment *attachment) {
    Ref<Texture> *ref = (Ref<Texture>*)((sp::AtlasRegion*)attachment->getRendererObject())->page->getRendererObject();
    return ref? *ref : NULL;
}
static Ref<Texture> spine_get_texture(sp::MeshAttachment *attachment) {
    Ref<Texture> *ref =  (Ref<Texture>*)((sp::AtlasRegion*)attachment->getRendererObject())->page->getRendererObject();
    return ref? *ref : NULL;
}

void SPINE_RUNTIME_CLASS::_rt_get_property_list(List<PropertyInfo> *p_list) const {
	
}

bool SPINE_RUNTIME_CLASS::_rt_get(const StringName &p_name, Variant &r_ret) const {
	String name = p_name;
	if (name.begins_with("bone/")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		sp::Bone *bone = skeleton->findBone(params[1].utf8().get_data());
		ERR_FAIL_COND_V(bone==NULL, false);
		if (params[2] == "rotation")
			r_ret = bone->getRotation();
		else if (params[2] == "position")
			r_ret = Vector2(bone->getX(), bone->getY());
		return true;
	} else {
		return false;
	}
}

bool SPINE_RUNTIME_CLASS::_rt_set(const StringName &p_name, const Variant &p_value) {
    String name = p_name;
	if (name.begins_with("path")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size()!=3) return true;
		sp::PathConstraint *pc = skeleton->findPathConstraint(params[1].utf8().get_data());
		ERR_FAIL_COND_V(pc==NULL, true);
		if (params[2] == "position") {
			pc->setPosition(p_value);
		} else if (params[2] == "tmix"){
#ifdef SPINE_RUNTIME_3
			pc->setTranslateMix(p_value);
#else
			pc->setMixX(p_value);
			pc->setMixY(p_value);
#endif
		} else if (params[2] == "rmix"){
#ifdef SPINE_RUNTIME_3
			pc->setRotateMix(p_value);
#else
			pc->setMixRotate(p_value);
#endif
		} else if (params[2] == "spacing"){
			pc->setSpacing(p_value);
		}
		skeleton->updateWorldTransform();
        return true;
	} else if (name.begins_with("bone")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		sp::Bone *bone = skeleton->findBone(params[1].utf8().get_data());
		ERR_FAIL_COND_V(bone==NULL, true);
		if (params[2] == "rotation"){
			bone->setRotation(p_value);
		}
		else if (params[2] == "position"){
			Vector2 v(p_value);
			bone->setX(v.x);
			bone->setY(v.y);
		}
		skeleton->updateWorldTransform();
        return true;
	} else if (name.begins_with("slot")){
		if (skeleton == NULL) return true;
		Vector<String> params = name.split("/");
		if (params.size() != 3) return true;
		sp::Slot *slot = skeleton->findSlot(params[1].utf8().get_data());
		ERR_FAIL_COND_V(slot==NULL, true);
		if (params[2] == "color"){
			Color c = p_value;
			slot->getColor().set(
				skeleton->getColor().r * c.r,
				skeleton->getColor().g * c.g,
				skeleton->getColor().b * c.b,
				skeleton->getColor().a * c.a
			);
		}
        return true;
	} else {
        return false;
    }
}

void SPINE_RUNTIME_CLASS::init() {

}

Ref<SpineResource> SPINE_RUNTIME_CLASS::load_resource(const String &p_path) {
	sp::TextureLoader *loader = new SPINE_TEXTURE_LOADER_CLASS();

	Ref<SpineResource> resource;
    sp::SkeletonData *skeletonData = NULL;

	String p_atlas = p_path.get_basename() + ".atlas";
	sp::Atlas *atlas = new sp::Atlas(p_atlas.utf8().get_data(), loader);
	if (!atlas->getPages().size()) {
		return resource;
	}

	String err_msg;
	if (p_path.get_extension() == "json") {
		sp::SkeletonJson json(atlas);
		json.setScale(1);
		skeletonData = json.readSkeletonDataFile(p_path.utf8().get_data());
		err_msg = json.getError().buffer();
	} else if (p_path.get_extension() == "skel") {
		sp::SkeletonBinary binary(atlas);
		binary.setScale(1);
		skeletonData = binary.readSkeletonDataFile(p_path.utf8().get_data());
		err_msg = binary.getError().buffer();
	}

    if (!skeletonData) {
		ERR_FAIL_V_MSG(Ref<SpineResource>(), err_msg);
    }
    
    resource.instance();
    resource->atlas = atlas;
    resource->data = skeletonData;
    resource->runtime_version = SPINE_RUNTIME_VERSION_STRING;
    return resource;
}

Ref<SpineRuntime> SPINE_RUNTIME_CLASS::with_resource(Ref<SpineResource> res) {
    if (res.is_null()) { return Ref<SpineRuntime>(); }
    if (!res->data) { return Ref<SpineRuntime>(); }
    return Ref<SpineRuntime>(memnew(SPINE_RUNTIME_CLASS(res)));
}

SPINE_RUNTIME_CLASS::SPINE_RUNTIME_CLASS(Ref<SpineResource> resource) {
    skeleton = NULL;
    root_bone = NULL;
    clipper = NULL;
    state = NULL;

    if (resource.is_null())
		return;

	if (!resource->data) {
        return;
    }
    world_verts.setSize(2048, 0);
    sp::SkeletonData* data = (sp::SkeletonData*)resource->data;
	skeleton = new sp::Skeleton(data);
	root_bone = skeleton->getBones()[0];
	clipper = new sp::SkeletonClipping();

	state = new sp::AnimationState(new sp::AnimationStateData(data));
	state->setRendererObject(this);
	state->setListener(spine_animation_callback);
}

SPINE_RUNTIME_CLASS::~SPINE_RUNTIME_CLASS() {
    if (state) delete state;
	if (skeleton) delete skeleton;
	if (clipper) delete clipper;

	state = NULL;
	skeleton = NULL;
	clipper = NULL;
    
    for (AttachmentNodes::Element *E = attachment_nodes.front(); E; E = E->next()) {
		AttachmentNode &node = E->get();
		memdelete(node.ref);
	}
	attachment_nodes.clear();
}


void SPINE_RUNTIME_CLASS::batch(SpineBatcher* batcher, const Color &modulate, bool flip_x, bool flip_y, bool individual_textures) {
    skeleton->getColor().set(modulate.r, modulate.g, modulate.b, modulate.a);

	int additive = 0;
	Color color;
	float *uvs = NULL;
	int vertices_count = 0;
    unsigned short *triangles = NULL;
	int triangles_count = 0;
	float r = 0, g = 0, b = 0, a = 0;

	batcher->reset();

    sp::Vector<sp::Slot*> slots = skeleton->getDrawOrder();
	for (int i = 0, n = slots.size(); i < n; i++) {

		sp::Slot *slot = slots[i];
		if (!slot->getAttachment() || slot->getColor().a == 0) {
			clipper->clipEnd(*slot);
			continue;
		}
		Ref<Texture> texture;
        if (slot->getAttachment()->getRTTI().isExactly(sp::RegionAttachment::rtti)) {
			sp::RegionAttachment* attachment = (sp::RegionAttachment*)slot->getAttachment();
            vertices_count = 8;
#ifdef SPINE_RUNTIME_3
            attachment->computeWorldVertices(slot->getBone(), world_verts, 0, 2);
#elifdef SPINE_RUNTIME_4_0
			attachment->computeWorldVertices(slot->getBone(), world_verts, 0, 2);
#else
            attachment->computeWorldVertices(*slot, world_verts, 0, 2);
#endif
            
            texture = spine_get_texture(attachment);
            uvs = attachment->getUVs().buffer();
            static unsigned short quadTriangles[6] = { 0, 1, 2, 2, 3, 0 };
            triangles = quadTriangles;
            triangles_count = 6;
            r = attachment->getColor().r;
            g = attachment->getColor().g;
            b = attachment->getColor().b;
            a = attachment->getColor().a;
        } else if (slot->getAttachment()->getRTTI().isExactly(sp::MeshAttachment::rtti)) {
			sp::MeshAttachment* attachment = (sp::MeshAttachment*)slot->getAttachment();
            vertices_count = attachment->getWorldVerticesLength();
            attachment->computeWorldVertices(*slot, world_verts);
            texture = spine_get_texture(attachment);
            uvs = attachment->getUVs().buffer();
            triangles = attachment->getTriangles().buffer();
            triangles_count = attachment->getTriangles().size();
            r = attachment->getColor().r;
            g = attachment->getColor().g;
            b = attachment->getColor().b;
            a = attachment->getColor().a;
        } else if (slot->getAttachment()->getRTTI().isExactly(sp::ClippingAttachment::rtti)) {
			sp::ClippingAttachment* attachment = (sp::ClippingAttachment*)slot->getAttachment();
            clipper->clipStart(*slot, attachment);
            continue;
        } 
		if (texture.is_null()){
            clipper->clipEnd(*slot);
			continue;
		}

		color.a = skeleton->getColor().a * slot->getColor().a * a;
		color.r = skeleton->getColor().r * slot->getColor().r * r;
		color.g = skeleton->getColor().g * slot->getColor().g * g;
		color.b = skeleton->getColor().b * slot->getColor().b * b;

        if (clipper->isClipping()) {
            clipper->clipTriangles(world_verts.buffer(), triangles, triangles_count, uvs, 2);
			if (clipper->getClippedTriangles().size() == 0){
				clipper->clipEnd(*slot);
				continue;
			}
			batcher->add(texture, 
                clipper->getClippedVertices().buffer(),
                clipper->getClippedUVs().buffer(),
                clipper->getClippedVertices().size(),
                clipper->getClippedTriangles().buffer(),
                clipper->getClippedTriangles().size(),
                &color, flip_x, flip_y,
                slot->getData().getIndex()*individual_textures
            );
		} else {
			batcher->add(texture, 
                world_verts.buffer(), 
                uvs, 
                vertices_count, 
                triangles, 
                triangles_count,
                &color, flip_x, flip_y,
                slot->getData().getIndex()*individual_textures
            );
		}
		clipper->clipEnd(*slot);
	}
	clipper->clipEnd();
	// performance_triangles_drawn = performance_triangles_generated = batcher.triangles_count();
	batcher->flush();
}

void SPINE_RUNTIME_CLASS::process(float delta) {
    if (state == NULL) return;
    state->update(delta);
    state->apply(*skeleton);
    skeleton->updateWorldTransform();

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
        sp::Slot *slot = info.slot;
		sp::Bone bone = slot->getBone();
		node->call("set_position", Vector2(bone.getWorldX() + bone.getSkeleton().getX(), -bone.getWorldY() + bone.getSkeleton().getY()) + info.ofs);
		node->call("set_scale", Vector2(bone.getWorldScaleX(), bone.getWorldScaleY()) * info.scale);
		node->call("set_rotation_degrees", bone.getWorldRotationX() + info.rot);
        Color c;
        c.a = slot->getColor().a;
        c.r = slot->getColor().r;
        c.g = slot->getColor().g;
        c.b = slot->getColor().b;
        node->call("set_modulate", c);
	}
}

float SPINE_RUNTIME_CLASS::get_animation_length(String p_animation) const {
    if (state == NULL) return 0;
    sp::Animation* animation = state->getData()->getSkeletonData()->findAnimation(p_animation.utf8().get_data());
    if (animation == NULL) {
        return 0;
    } else {
        return animation->getDuration();
    }
}

Array SPINE_RUNTIME_CLASS::get_animation_names() const {
    Array names;
    if (state == NULL) return names;
    sp::Vector<sp::Animation*> animations = state->getData()->getSkeletonData()->getAnimations();
    for (int i = 0; i < animations.size(); i++) {
        names.push_back(animations[i]->getName().buffer());
    }
	return names;
}

Array SPINE_RUNTIME_CLASS::get_skin_names() const {
    Array names;
    if (state == NULL) return names;
    sp::Vector<sp::Skin*> skins = state->getData()->getSkeletonData()->getSkins();
    for (int i = 0; i < skins.size(); i++) {
        names.push_back(skins[i]->getName().buffer());
    }
    return names;
}

bool SPINE_RUNTIME_CLASS::has_animation(const String &p_name) {
	if (skeleton == NULL) return false;
    return NULL != state->getData()->getSkeletonData()->findAnimation(p_name.utf8().get_data());
}

bool SPINE_RUNTIME_CLASS::set_skin(const String &p_name) {
    if (skeleton == NULL) return false;
	skeleton->setSkin(p_name.utf8().get_data());
    return true;
}

void SPINE_RUNTIME_CLASS::mix(const String &p_from, const String &p_to, real_t p_duration) {
	if (state == NULL) return;
    state->getData()->setMix(p_from.utf8().get_data(), p_to.utf8().get_data(), p_duration);
}

bool SPINE_RUNTIME_CLASS::play(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, float p_delay) {
    if (skeleton == NULL) return false;
	sp::Animation *animation = skeleton->getData()->findAnimation(p_name.utf8().get_data());
	if (animation == NULL) return false;
	sp::TrackEntry *entry = state->setAnimation(p_track, animation, p_loop);
    if (entry == NULL) return false;
	entry->setDelay(p_delay);
	entry->setTimeScale(p_cunstom_scale);
	return true;
}

void SPINE_RUNTIME_CLASS::set_animation_state(int p_track, String p_animation, float p_pos) {
	if (skeleton == NULL) return;
	sp::Animation *animation = skeleton->getData()->findAnimation(p_animation.utf8().get_data());
	if (animation == NULL) return;
	sp::TrackEntry *entry = state->setAnimation(p_track, animation, false);
    if (entry == NULL) return;
	entry->setTrackTime(p_pos);
}

bool SPINE_RUNTIME_CLASS::add(const String &p_name, real_t p_cunstom_scale, bool p_loop, int p_track, float p_delay) {
	if (skeleton == NULL) return false;
	sp::Animation *animation = skeleton->getData()->findAnimation(p_name.utf8().get_data());
	if (animation == NULL) return false;
	sp::TrackEntry *entry = state->addAnimation(p_track, animation, p_loop, p_delay);
    if (entry == NULL) return false;
	return true;
}

void SPINE_RUNTIME_CLASS::clear(int p_track) {
	if (state == NULL) return;
	if (p_track == -1)
        state->clearTracks();
	else
		state->clearTrack(p_track);
}

bool SPINE_RUNTIME_CLASS::is_playing(int p_track) const {
    if (state == NULL) return false;
	sp::TrackEntry *entry = state->getCurrent(p_track);
	if (entry == NULL) return false;
	return entry->getLoop() || entry->getTrackTime() < entry->getAnimationEnd();
}

String SPINE_RUNTIME_CLASS::get_current_animation(int p_track) const {
    if (state == NULL) return "";
	
	sp::TrackEntry *entry = state->getCurrent(p_track);
	if (entry == NULL || entry->getAnimation() == NULL)
		return "";
	return entry->getAnimation()->getName().buffer();
}

void SPINE_RUNTIME_CLASS::reset() {
	if (skeleton == NULL) return;
	skeleton->setToSetupPose();
	state->update(0);
	state->apply(*skeleton);
    skeleton->updateWorldTransform();
}

void SPINE_RUNTIME_CLASS::seek(int track, float p_pos) {
	if (state == NULL) return;
	sp::TrackEntry *entry = state->getCurrent(track);
	if (entry == NULL) return;
	entry->setTrackTime(p_pos);
}

float SPINE_RUNTIME_CLASS::tell(int track) const {
	if (state == NULL) return 0;
	sp::TrackEntry *entry = state->getCurrent(track);
	if (entry == NULL) return 0;
	return entry->getTrackTime();
}

Dictionary SPINE_RUNTIME_CLASS::get_skeleton(bool individual_textures) const {
	Dictionary dict;
    if (skeleton == NULL) return dict;


	dict["bonesCount"] = (int)skeleton->getBones().size();
	dict["slotCount"] = (int)skeleton->getSlots().size();
	dict["ikConstraintsCount"] = (int)skeleton->getIkConstraints().size();
	dict["time"] = 0.0;
	dict["flipX"] = skeleton->getScaleX() < 0;
	dict["flipY"] = skeleton->getScaleY() < 0;
	dict["x"] = skeleton->getX();
	dict["y"] = skeleton->getY();

	Array bones;
    sp::Vector<sp::Bone*> bones_vector = skeleton->getBones();
	for (int i=0; i<bones_vector.size();i++){
		sp::Bone *b = bones_vector[i];
		Dictionary bi;
		bi["name"] = b->getData().getName().buffer();
		bi["parent"] = b->getParent() == NULL? Variant("") : Variant(b->getParent()->getData().getName().buffer());
		bones.append(bi);
	}
	dict["bones"] = bones;

	Array slots;
    sp::Vector<sp::Slot*> slots_vector = skeleton->getSlots();
	sp::Skin* skin = skeleton->getData()->getDefaultSkin();
	for (int j=0; j<slots_vector.size(); j++){
		sp::Slot *s = slots_vector[j];
		Dictionary slot_dict;
		slot_dict["name"] = s->getData().getName().buffer();
		Array attachments;
        sp::Vector<sp::Attachment*> attachments_vector;
        skin->findAttachmentsForSlot(s->getData().getIndex(), attachments_vector);
        for (int k=0; k<attachments_vector.size(); k++) {
            attachments.append(attachments_vector[k]->getName().buffer());
        }
		slot_dict["attachments"] = attachments;
		slots.append(slot_dict);
	}
	dict["slots"] = slots;

    
    slots_vector = skeleton->getDrawOrder();
 	if (individual_textures) {
		Dictionary slot_dict;
		for (int i = 0, n = slots_vector.size(); i < n; i++) {
			sp::Slot *s = slots_vector[i];
			slot_dict[s->getData().getName().buffer()] = s->getData().getIndex();
		}
		dict["item_indexes"] = slot_dict;
	}
	return dict;
}

Dictionary SPINE_RUNTIME_CLASS::get_attachment(const String &p_slot_name, const String &p_attachment_name) const {
	Dictionary dict;
	if (skeleton == NULL) return dict;
	sp::Attachment *attachment = skeleton->getAttachment(p_slot_name.utf8().get_data(), p_attachment_name.utf8().get_data());
	if (attachment == NULL) return dict;
	dict["name"] = attachment->getName().buffer();
	if (attachment->getRTTI().isExactly(sp::RegionAttachment::rtti)) {
		sp::RegionAttachment *info = (sp::RegionAttachment*)attachment;
		dict["type"] = "region";
		dict["path"] = info->getPath().buffer();
		dict["x"] = info->getX();
		dict["y"] = info->getY();
		dict["scaleX"] = info->getScaleX();
		dict["scaleY"] = info->getScaleY();
		dict["rotation"] = info->getRotation();
		dict["width"] = info->getWidth();
		dict["height"] = info->getHeight();
		dict["color"] = Color(info->getColor().r, info->getColor().g, info->getColor().b, info->getColor().a);
#ifdef SPINE_RUNTIME_3
		dict["region"] = Rect2(info->getRegionOffsetX(), info->getRegionOffsetY(), info->getRegionWidth(), info->getRegionHeight());
		dict["region_original_size"] = Size2(info->getRegionOriginalWidth(), info->getRegionOriginalHeight());
#elifdef SPINE_RUNTIME_4_0
		dict["region"] = Rect2(info->getRegionOffsetX(), info->getRegionOffsetY(), info->getRegionWidth(), info->getRegionHeight());
		dict["region_original_size"] = Size2(info->getRegionOriginalWidth(), info->getRegionOriginalHeight());
#else
		dict["region"] = Rect2(info->getOffset()[0], info->getOffset()[1], info->getWidth(), info->getHeight());
		dict["region_original_size"] = Size2(info->getWidth(), info->getHeight());
#endif

		Vector<Vector2> offset, uvs;
		for (int idx = 0; idx < 4; idx++) {
			offset.push_back(Vector2(info->getOffset()[idx * 2], info->getOffset()[idx * 2 + 1]));
			uvs.push_back(Vector2(info->getUVs()[idx * 2], info->getUVs()[idx * 2 + 1]));
		}
		dict["offset"] = offset;
		dict["uvs"] = uvs;
	} else
	if (attachment->getRTTI().isExactly(sp::BoundingBoxAttachment::rtti)) {
		sp::BoundingBoxAttachment* info = (sp::BoundingBoxAttachment*)attachment;
		dict["type"] = "bounding_box";

		Vector<Vector2> vertices;
		for (int idx = 0; idx < info->getVertices().size() / 2; idx++)
			vertices.push_back(Vector2(info->getVertices()[idx * 2], -info->getVertices()[idx * 2 + 1]));
		dict["vertices"] = vertices;
	} else
	if (attachment->getRTTI().isExactly(sp::MeshAttachment::rtti)) {
		sp::MeshAttachment* info = (sp::MeshAttachment*)attachment;
		dict["type"] = "mesh";
		dict["path"] = info->getPath().buffer();
		dict["color"] = Color(info->getColor().r, info->getColor().g, info->getColor().b, info->getColor().a);
	}
	return dict;
}

Dictionary SPINE_RUNTIME_CLASS::get_bone(const String &p_bone_name) const {
	Dictionary dict;
	if (skeleton == NULL) return dict;
	sp::Bone *bone = skeleton->findBone(p_bone_name.utf8().get_data());
	if (bone == NULL) return dict;
	dict["x"] = bone->getX();
	dict["y"] = bone->getY();
	dict["rotation"] = bone->getRotation();
	dict["rotationIK"] = 0; //bone->rotationIK;
	dict["scaleX"] = bone->getScaleX();
	dict["scaleY"] = bone->getScaleY();
	dict["flipX"] = 0; //bone->flipX;
	dict["flipY"] = 0; //bone->flipY;
	dict["m00"] = bone->getA(); //m00;
	dict["m01"] = bone->getB(); //m01;
	dict["m10"] = bone->getC(); //m10;
	dict["m11"] = bone->getD(); //m11;
	dict["worldX"] = bone->getWorldX();
	dict["worldY"] = bone->getWorldY();
	dict["worldRotation"] = bone->getWorldRotationX(); //->worldRotation;
	dict["worldScaleX"] = bone->getWorldScaleX(); //->worldScaleX;
	dict["worldScaleY"] = bone->getWorldScaleY(); //->worldScaleY;
	dict["worldFlipX"] = 0; //bone->worldFlipX;
	dict["worldFlipY"] = 0; //bone->worldFlipY;

	return dict;
}

Dictionary SPINE_RUNTIME_CLASS::get_slot(const String &p_slot_name) const {
    Dictionary dict;
    if (skeleton == NULL) return dict;
	sp::Slot *slot = skeleton->findSlot(p_slot_name.utf8().get_data());
    if (slot == NULL) return dict;
	dict["color"] = Color(slot->getColor().r, slot->getColor().g, slot->getColor().b, slot->getColor().a);
	if (slot->getAttachment() == NULL) {
		dict["attachment"] = Variant();
	} else {
		dict["attachment"] = slot->getAttachment()->getName().buffer();
	}
	return dict;
}

bool SPINE_RUNTIME_CLASS::set_attachment(const String &p_slot_name, const Variant &p_attachment) {
	if (skeleton == NULL) return false;
	if (p_attachment.get_type() == Variant::STRING)
		skeleton->setAttachment(p_slot_name.utf8().get_data(), ((const String)p_attachment).utf8().get_data());
	else
		skeleton->setAttachment(p_slot_name.utf8().get_data(), "");
    return true;
}

bool SPINE_RUNTIME_CLASS::has_attachment_node(const String &p_bone_name, const Variant &p_node) {
	return false;
}

bool SPINE_RUNTIME_CLASS::add_attachment_node(const String &p_bone_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {

	if (skeleton == NULL) return false;
	sp::Slot *slot = skeleton->findSlot(p_bone_name.utf8().get_data());
    if (slot == NULL) return false;
	Object *obj = p_node;
    if (obj == NULL) return false;
	Node2D *node = Object::cast_to<Node2D>(obj);
	if (node == NULL) return false;

	if (obj->has_meta("spine_meta")) {

		AttachmentNode *info = (AttachmentNode *)((uint64_t)obj->get_meta("spine_meta"));
		if (info->slot !=  slot) {
			// add to different bone, remove first
			remove_attachment_node(info->slot->getData().getName().buffer(), p_node);
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

bool SPINE_RUNTIME_CLASS::remove_attachment_node(const String &p_bone_name, const Variant &p_node) {
    if (skeleton == NULL) return false;
	sp::Slot *slot = skeleton->findSlot(p_bone_name.utf8().get_data());
    if (slot == NULL) return false;
	Object *obj = p_node;
    if (obj == NULL) return false;
	Node2D *node = Object::cast_to<Node2D>(obj);
	if (node == NULL) return false;

	if (!obj->has_meta("spine_meta"))
		return false;

	AttachmentNode *info = (AttachmentNode *)((uint64_t)obj->get_meta("spine_meta"));
	if (info->slot != slot) return  false;
	obj->set_meta("spine_meta", Variant());
	memdelete(info->ref);
	attachment_nodes.erase(info->E);
	return false;
}

// this looks scary
Ref<Shape2D> SPINE_RUNTIME_CLASS::get_bounding_box(const String& p_slot_name, const String& p_attachment_name) {
    if (skeleton == NULL) return Ref<Shape2D>();
	sp::Attachment *attachment = skeleton->getAttachment(p_slot_name.utf8().get_data(), p_attachment_name.utf8().get_data());
	if (attachment == NULL) return Ref<Shape2D>();
	if (attachment->getRTTI().isExactly(sp::BoundingBoxAttachment::rtti)) return Ref<Shape2D>();
    sp::BoundingBoxAttachment *box = (sp::BoundingBoxAttachment*)attachment;

	Vector<Vector2> points;
    sp::Vector<float> vertices = box->getVertices();
	points.resize(vertices.size() / 2);
	for (int idx = 0; idx < vertices.size() / 2; idx++)
		points.write[idx] = Vector2(vertices[idx * 2], -vertices[idx * 2 + 1]);

	ConvexPolygonShape2D *shape = memnew(ConvexPolygonShape2D);
	shape->set_points(points);
	return shape;
}

// this looks scary as well
bool SPINE_RUNTIME_CLASS::add_bounding_box(const String &p_bone_name, const String &p_slot_name, const String &p_attachment_name, const Variant &p_node, const Vector2 &p_ofs, const Vector2 &p_scale, const real_t p_rot) {
	if (skeleton == NULL) return false;
	Object *obj = p_node;
	if (obj == NULL) return false;
	CollisionObject2D *node = Object::cast_to<CollisionObject2D>(obj);
	if (node == NULL) return false;
	Ref<Shape2D> shape = get_bounding_box(p_slot_name, p_attachment_name);
	if (shape.is_null()) return false;
	node->shape_owner_add_shape(0, shape);
	return add_attachment_node(p_bone_name, p_node);
}

Vector2 SPINE_RUNTIME_CLASS::get_bone_position(const String &bone_name) {
    sp::Bone *bone = skeleton->findBone(bone_name.utf8().get_data());
    if (bone == NULL) return Vector2();
    return Vector2(bone->getX(), bone->getY());
}
float SPINE_RUNTIME_CLASS::get_bone_rotation(const String &bone_name) {
    sp::Bone *bone = skeleton->findBone(bone_name.utf8().get_data());
    if (bone == NULL) return 0.0;
    return bone->getRotation();
}