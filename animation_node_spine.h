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

#ifdef MODULE_SPINE_WITH_ANIMATION_NODES
#ifndef ANIMATION_NODE_SPINE_H
#define ANIMATION_NODE_SPINE_H

#include "scene/animation/animation_tree.h"

class SpineMachine: public AnimationTree {
	GDCLASS(SpineMachine, AnimationTree);

	NodePath spine_player;
	int track;

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;


public:
	virtual String get_configuration_warning() const;
	void set_spine_player(const NodePath &p_player);
	NodePath get_spine_player() const;
	int get_track() const { return track; }

	SpineMachine();
};

class AnimationNodeSpineAnimation: public AnimationRootNode {
    GDCLASS(AnimationNodeSpineAnimation, AnimationRootNode);

    StringName animation;
	StringName time;

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void _default_property_list(List<PropertyInfo> *p_list) const;

public:
	void get_parameter_list(List<PropertyInfo> *r_list) const;

	virtual String get_caption() const;
	virtual float process(float p_time, bool p_seek);

	AnimationNodeSpineAnimation();
};

#endif // ANIMATION_NODE_SPINE_H
#endif // MODULE_SPINE_ENABLED
