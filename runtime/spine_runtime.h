#ifndef SPINE_RUNTIME_H
#define SPINE_RUNTIME_H

#include "core/resource.h"
#include "scene/resources/shape_2d.h"

#include "modules/spine/spine_batcher.h"

class SpineRuntime;
class SpineResource: public Resource {
    GDCLASS(SpineResource, Resource);
protected:
    bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
    void *atlas;
    void *data;
    String runtime_version;
    Ref<SpineRuntime> create_runtime();
};

class SpineRuntime: public Reference {
    GDCLASS(SpineRuntime, Reference);

protected:
    static void _bind_methods();

public:
    virtual bool _rt_set(const StringName &p_name, const Variant &p_value) { return false; }
	virtual bool _rt_get(const StringName &p_name, Variant &r_ret) const { return false; }
	virtual void _rt_get_property_list(List<PropertyInfo> *p_list) const { };

    static void init();
    static Ref<SpineResource> load_resource(const String &path);
    static Ref<SpineRuntime> with_resource(Ref<SpineResource>) { return Ref<SpineRuntime>(); };

    virtual void batch(SpineBatcher* batcher, const Color &modulate, bool flip_x, bool flip_y, bool individual_textures) { }
    virtual void process(float delta) { }

    virtual float get_animation_length(String p_animation) const { return 0.0; }
    virtual Array get_animation_names() const { return Array(); }
    virtual Array get_skin_names() const { return Array(); }
    virtual bool has_animation(const String& p_name) { return false; }
    virtual bool set_skin(const String& p_name) { return false; }

    virtual void mix(const String& p_from, const String& p_to, real_t p_duration) { }
    virtual bool play(const String& p_name, real_t p_cunstom_scale = 1.0f, bool p_loop = false, int p_track = 0, float p_delay = 0) { return false; }
    virtual void set_animation_state(int track, String p_animation, float p_pos) { }
    virtual bool add(const String& p_name, real_t p_cunstom_scale = 1.0f, bool p_loop = false, int p_track = 0, float p_delay = 0) { return false; }
    virtual void clear(int p_track = -1) { }
    virtual bool is_playing(int p_track = 0) const { return false; }
    virtual String get_current_animation(int p_track = 0) const { return String(); }
    virtual void reset() { }
    virtual void seek(int track, float p_pos) { }
    virtual float tell(int track) const { return 0.0; }

    virtual Dictionary get_skeleton(bool individual_textures=false) const { return Dictionary(); }
    virtual Dictionary get_attachment(const String& p_slot_name, const String& p_attachment_name) const { return Dictionary(); }
    virtual Dictionary get_bone(const String& p_bone_name) const { return Dictionary(); }
    virtual Dictionary get_slot(const String& p_slot_name) const { return Dictionary(); }
    virtual bool set_attachment(const String& p_slot_name, const Variant& p_attachment) { return false; }


    virtual bool has_attachment_node(const String& p_bone_name, const Variant& p_node) { return false; }
	virtual bool add_attachment_node(const String& p_bone_name, const Variant& p_node, const Vector2& p_ofs = Vector2(0, 0), const Vector2& p_scale = Vector2(1, 1), const real_t p_rot = 0) { return false; }
	virtual bool remove_attachment_node(const String& p_bone_name, const Variant& p_node) { return false; }
    
    virtual Ref<Shape2D> get_bounding_box(const String& p_slot_name, const String& p_attachment_name) { return Ref<Shape2D>(); }
    virtual bool add_bounding_box(const String& p_bone_name, const String& p_slot_name, const String& p_attachment_name, const Variant& p_node, const Vector2& p_ofs = Vector2(0, 0), const Vector2& p_scale = Vector2(1, 1), const real_t p_rot = 0) { return false; }

    virtual Vector2 get_bone_position(const String &bone_name) { return Vector2(); }
    virtual float get_bone_rotation(const String &bone_name) { return 0.0; }

    SpineRuntime() { };
};


#endif