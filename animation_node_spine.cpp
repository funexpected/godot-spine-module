// MIT License

// Copyright (c) 2021 Yakov Borevich, Funexpected LLC

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifdef MODULE_SPINE_ENABLED

#include "animation_node_spine.h"
#include "spine.h"
#include "core/message_queue.h"
#include "scene/gui/control.h"
#ifdef TOOLS_ENABLED
#include "editor/plugins/animation_tree_editor_plugin.h"
#endif

String SpineMachine::get_configuration_warning() const {
    String warning = AnimationTree::get_configuration_warning();

    if (!has_node(spine_player)) {
		if (warning != String()) {
			warning += "\n\n";
		}
		warning += TTR("Path to an Spine node containing spine animations is not set.");
        return warning;
	}

    Spine* fp = Object::cast_to<Spine>(get_node(spine_player));
    if (!fp) {
        if (warning != String()) {
			warning += "\n\n";
		}
		warning += TTR("Path set for Spine does not lead to an Spine node.");
        return warning;
    }

    if (!fp->get_resource().is_valid()) {
        if (warning != String()) {
			warning += "\n\n";
		}
		warning += TTR("Spine node missing resource with animations.");
        return warning;
    }

    return warning;
}

void SpineMachine::set_spine_player(const NodePath &p_player) {
    spine_player = p_player;
#ifdef TOOLS_ENABLED
    property_list_changed_notify();
    update_configuration_warning();
    Spine* sp = Object::cast_to<Spine>(get_node(spine_player));
    if (sp) {
        sp->connect("resource_changed", this, "update_configuration_warning");
        sp->connect("resource_changed", this, "property_list_changed_notify");
    }
#endif
}

NodePath SpineMachine::get_spine_player() const {
    return spine_player;
}

void SpineMachine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_spine_player", "spine_player"), &SpineMachine::set_spine_player);
	ClassDB::bind_method(D_METHOD("get_spine_player"), &SpineMachine::get_spine_player);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "spine_player", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Spine"), "set_spine_player", "get_spine_player");
}


bool SpineMachine::_set(const StringName &p_name, const Variant &p_value) {
    if (p_name == "track") {
        track = p_value;
        return true;
    }
    return false;
}

bool SpineMachine::_get(const StringName &p_name, Variant &r_ret) const {
    if (p_name == "track") {
        r_ret = track;
        return true;
    }
    return false;
}

void SpineMachine::_get_property_list(List<PropertyInfo> *p_list) const {
    if (!has_node(spine_player)) return;
    Spine* sp = Object::cast_to<Spine>(get_node(spine_player));
    if (!sp) return;

    p_list->push_back(PropertyInfo(Variant::INT, "track"));
}

SpineMachine::SpineMachine() {
    track = 0;
}

/*
 *          Spine Animation
 */

bool AnimationNodeSpineAnimation::_set(const StringName &p_name, const Variant &p_value) {
    if (p_name == "animation") {
        animation = p_value;
        return true;
    } else {
        return false;
    }
}

bool AnimationNodeSpineAnimation::_get(const StringName &p_name, Variant &r_ret) const {
    if (p_name == "animation") {
        r_ret = animation;
        return true;
    } else if (p_name == "warning") {
        r_ret = "";
        return true;
    } else {
        return false;
    }
}

void AnimationNodeSpineAnimation::_default_property_list(List<PropertyInfo> *p_list) const {
    p_list->push_back(PropertyInfo(Variant::STRING, "animation", PROPERTY_HINT_NONE, ""));
    p_list->push_back(PropertyInfo(Variant::STRING, "warning", PROPERTY_HINT_PLACEHOLDER_TEXT, "Invalid spine symbol state"));
}

void AnimationNodeSpineAnimation::_get_property_list(List<PropertyInfo> *p_list) const {
    Spine *sp;
    SpineMachine *tree;
#ifdef TOOLS_ENABLED
    AnimationTreeEditor *editor = AnimationTreeEditor::get_singleton();
    if (!editor || !editor->is_visible()) return _default_property_list(p_list);

    tree = Object::cast_to<SpineMachine>(editor->get_tree());
    if (!tree || !tree->has_node(tree->get_animation_player())) return _default_property_list(p_list);
    
    sp = Object::cast_to<Spine>(tree->get_node(tree->get_spine_player()));
    if (!sp) return _default_property_list(p_list);
#else
    return;
#endif

    Array animation_names = sp->get_animation_names();
    Vector<String> animations;
    StringName default_value;
    for (int i=0; i < animation_names.size(); i++) {
        if (i == 0) default_value = animation_names[i];
        animations.push_back(animation_names[i]);
    }
    p_list->push_back(PropertyInfo(Variant::STRING, "animation", PROPERTY_HINT_ENUM, String(",").join(animations)));

    if (animation == StringName()) {
        MessageQueue::get_singleton()->push_call(get_instance_id(), "set", "animation", default_value);
    }
}

void AnimationNodeSpineAnimation::get_parameter_list(List<PropertyInfo> *r_list) const {
	r_list->push_back(PropertyInfo(Variant::REAL, time, PROPERTY_HINT_NONE, "", 0));
}

String AnimationNodeSpineAnimation::get_caption() const {
	return "Spine Animation";
}

float AnimationNodeSpineAnimation::process(float p_time, bool p_seek) {
    if (animation == StringName()) return 0.0;

    SpineMachine *sm = Object::cast_to<SpineMachine>(state->tree);
    ERR_FAIL_COND_V(!sm, 0);
    Spine *sp = Object::cast_to<Spine>(sm->get_node(sm->get_spine_player()));
    ERR_FAIL_COND_V(!sp, 0);
    ERR_FAIL_COND_V(sp->processing == true, 0);
    int track = sm->get_track();

	float time = get_parameter(this->time);

	float step = p_time;

	if (p_seek) {
		time = p_time;
		step = 0;
	} else {
		time = MAX(0, time + p_time * sp->get_speed());
	}

    float anim_size = sp->get_animation_length(animation);
    if (time > anim_size) {
        time = anim_size;
        p_time = 0;
    }
    if (p_seek) {
        sp->set_animation_state(track, animation, time);
    } else {
        sp->queue_process_with_time(p_time);
    }
	set_parameter(this->time, time);
	return anim_size - time;
}

AnimationNodeSpineAnimation::AnimationNodeSpineAnimation() {
	time = "time";
}

#endif // MODULE_SPINE_ENABLED