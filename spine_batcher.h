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

#ifndef SPINE_BATCHER_H
#define SPINE_BATCHER_H

#include "scene/2d/node_2d.h"

class SpineBatchCommand {
public:
	RID mesh;
	Ref<Texture> texture;
	int vertices_count;
	int indies_count;
	Vector2 *vertices;
	Color *colors;
	Vector2 *uvs;
	int* indies;
	int pool_idx;
	SpineBatchCommand();
	~SpineBatchCommand();
	void draw(RID ci);
	void build();
	void cleanup();
};

class SpineCommandPool {
	static SpineCommandPool *instance;
	List<SpineBatchCommand *> pool;
	Mutex *mut;
	int requested;
	int released;
public:
	SpineBatchCommand* request();
	void release(SpineBatchCommand* cmd);
	static SpineCommandPool *get_instance();
};

class SpineBatcher {

	Node2D *owner;
	Mutex *mut;

public:


	SpineBatchCommand *current_command;

	List<SpineBatchCommand *> element_list;
	List<SpineBatchCommand *> built_list;
	List<SpineBatchCommand *> drawed_list;

	//static List<SpineBatchCommand *> used_pool;

	void push_command();

public:

	void add(Ref<Texture> p_texture,
		const float* p_vertices, const float* p_uvs, int p_vertices_count,
		const unsigned short* p_indies, int p_indies_count,
		Color *p_color, bool flip_x, bool flip_y, int index_item = 0);

	void add_set_blender_mode(bool p_mode);

	void build();
	void draw();

	SpineBatcher(Node2D *owner);
	~SpineBatcher();
};


#endif // SPINE_BATCHER_H

#endif // MODULE_SPINE_ENABLED
