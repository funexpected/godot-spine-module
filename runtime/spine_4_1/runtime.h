#ifndef SPINE_RUNTIME_4_1_H
#define SPINE_RUNTIME_4_1_H
#include "modules/spine/runtime/spine_runtime.h"

class SpineRuntime_4_1: public SpineRuntime {
    GDCLASS(SpineRuntime_4_1, SpineRuntime);

#ifdef SPINE_RUNTIME_IMPL
    typedef struct AttachmentNode {
		List<AttachmentNode>::Element *E;
		sp::Slot *slot;
		WeakRef *ref;
		Vector2 ofs;
		Vector2 scale;
		real_t rot;
	} AttachmentNode;
	typedef List<AttachmentNode> AttachmentNodes;
	AttachmentNodes attachment_nodes;

    sp::Skeleton* skeleton;
    sp::Bone* root_bone;
	sp::AnimationState* state;
	sp::SkeletonClipping *clipper;
    sp::Vector<float> world_verts;
#endif

public:
    virtual bool _rt_set(const StringName &p_name, const Variant &p_value);
	virtual bool _rt_get(const StringName &p_name, Variant &r_ret) const;
	virtual void _rt_get_property_list(List<PropertyInfo> *p_list) const;

    static void init();
    static Ref<SpineResource> load_resource(const String &path);
    static Ref<SpineRuntime> with_resource(Ref<SpineResource>);

    virtual void batch(SpineBatcher* batcher, const Color &modulate, bool flip_x, bool flip_y, bool individual_textures);
    virtual void process(float delta);

    virtual float get_animation_length(String p_animation) const;
    virtual Array get_animation_names() const;
    virtual Array get_skin_names() const;
    virtual bool has_animation(const String& p_name);
    virtual bool set_skin(const String& p_name);

    virtual void mix(const String& p_from, const String& p_to, real_t p_duration);
    virtual bool play(const String& p_name, real_t p_cunstom_scale = 1.0f, bool p_loop = false, int p_track = 0, float p_delay = 0);
    virtual void set_animation_state(int track, String p_animation, float p_pos);
    virtual bool add(const String& p_name, real_t p_cunstom_scale = 1.0f, bool p_loop = false, int p_track = 0, float p_delay = 0);
    virtual void clear(int p_track = -1);
    virtual bool is_playing(int p_track = 0) const;
    virtual String get_current_animation(int p_track = 0) const;
    virtual void reset();
    virtual void seek(int track, float p_pos);
    virtual float tell(int track) const;

    virtual Dictionary get_skeleton(bool individual_textures=false) const;
    virtual Dictionary get_attachment(const String& p_slot_name, const String& p_attachment_name) const;
    virtual Dictionary get_bone(const String& p_bone_name) const;
    virtual Dictionary get_slot(const String& p_slot_name) const;
    virtual bool set_attachment(const String& p_slot_name, const Variant& p_attachment);

    virtual bool has_attachment_node(const String& p_bone_name, const Variant& p_node);
	virtual bool add_attachment_node(const String& p_bone_name, const Variant& p_node, const Vector2& p_ofs = Vector2(0, 0), const Vector2& p_scale = Vector2(1, 1), const real_t p_rot = 0);
	virtual bool remove_attachment_node(const String& p_bone_name, const Variant& p_node);
    
    virtual Ref<Shape2D> get_bounding_box(const String& p_slot_name, const String& p_attachment_name);
    virtual bool add_bounding_box(const String& p_bone_name, const String& p_slot_name, const String& p_attachment_name, const Variant& p_node, const Vector2& p_ofs = Vector2(0, 0), const Vector2& p_scale = Vector2(1, 1), const real_t p_rot = 0);

    virtual Vector2 get_bone_position(const String &bone_name);
    virtual float get_bone_rotation(const String &bone_name);

    SpineRuntime_4_1(Ref<SpineResource> resource);
    ~SpineRuntime_4_1();
};


#endif