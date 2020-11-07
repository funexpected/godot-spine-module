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
#include "spine_batcher.h"

#define BATCH_CAPACITY 1024


SpineCommandPool* SpineCommandPool::instance = NULL;

SpineCommandPool* SpineCommandPool::get_instance() {
	if (SpineCommandPool::instance == NULL) {
		SpineCommandPool::instance = memnew(SpineCommandPool);
		SpineCommandPool::instance->mut = Mutex::create();
		SpineCommandPool::instance->requested = 0;
		SpineCommandPool::instance->released = 0;
	};
	return SpineCommandPool::instance;
}

SpineBatchCommand* SpineCommandPool::request() {
	mut->lock();
	requested++;
	if (!pool.size()) {
		for (int i=0; i<512; i++) {
			SpineBatchCommand* cmd = memnew(SpineBatchCommand);
			pool.push_back(cmd);
		}
	}
	SpineBatchCommand* cmd = pool.front()->get();
	pool.pop_front();
	mut->unlock();
	return cmd;
}

void SpineCommandPool::release(SpineBatchCommand* cmd){
	cmd->cleanup();
	mut->lock();
	released++;
	pool.push_back(cmd);
	mut->unlock();
}


SpineBatchCommand::SpineBatchCommand() {
	vertices = memnew_arr(Vector2, BATCH_CAPACITY);
	colors = memnew_arr(Color, BATCH_CAPACITY);
	uvs = memnew_arr(Vector2, BATCH_CAPACITY);
	indies = memnew_arr(int, BATCH_CAPACITY * 3);
	vertices_count = 0;
	indies_count = 0;
};

void SpineBatchCommand::cleanup() {
	VisualServer::get_singleton()->free(mesh);
	mesh = RID();
	texture = Ref<Texture>();
	vertices_count = 0;
	indies_count = 0;
}

SpineBatchCommand::~SpineBatchCommand() {
	memdelete_arr(vertices);
	memdelete_arr(colors);
	memdelete_arr(uvs);
	memdelete_arr(indies);
	VisualServer::get_singleton()->free(mesh);
	//mesh.unref()
	//VisualServer::get_singleton()->free(mesh->get_rid());
}

void SpineBatchCommand::draw(RID ci) {
	if (!mesh.is_valid()) return;
	VisualServer::get_singleton()->canvas_item_add_mesh(
		ci, mesh,
		Transform2D(), 
		Color(1,1,1), 
		texture->get_rid()
	);
}

void SpineBatchCommand::build() {
	if (mesh.is_valid()) return;
	mesh = VisualServer::get_singleton()->mesh_create();

	Vector<int> p_indices;
	p_indices.resize(indies_count);
	memcpy(p_indices.ptrw(), indies, indies_count * sizeof(int) );

	Vector<Vector2> p_points;
	p_points.resize(vertices_count);
	memcpy(p_points.ptrw(), vertices, vertices_count * sizeof(Vector2));

	Vector<Color> p_colors;
	p_colors.resize(vertices_count);
	memcpy(p_colors.ptrw(), colors, vertices_count * sizeof(Color));

	Vector<Vector2> p_uvs;
	p_uvs.resize(vertices_count);
	memcpy(p_uvs.ptrw(), uvs, vertices_count * sizeof(Vector2));

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = p_points;
	arrays[Mesh::ARRAY_INDEX] = p_indices;
	arrays[Mesh::ARRAY_COLOR] = p_colors;
	arrays[Mesh::ARRAY_TEX_UV] = p_uvs;
	VisualServer::get_singleton()->mesh_add_surface_from_arrays(
		mesh,
		VisualServer::PRIMITIVE_TRIANGLES,
		arrays, Array(),
		VisualServer::ARRAY_FLAG_USE_2D_VERTICES
	);
}

void SpineBatcher::add(Ref<Texture> p_texture,
	const float* p_vertices, const float* p_uvs, int p_vertices_count,
	const unsigned short* p_indies, int p_indies_count,
	Color *p_color, bool flip_x, bool flip_y, int index_item) {

	if (p_texture != current_command->texture
		|| current_command->vertices_count + (p_vertices_count >> 1) > BATCH_CAPACITY
		|| current_command->indies_count + p_indies_count > BATCH_CAPACITY * 3) {

		push_command();
		current_command->texture = p_texture;
	}

	for (int i = 0; i < p_indies_count; ++i, ++current_command->indies_count)
		current_command->indies[current_command->indies_count] = p_indies[i] + current_command->vertices_count;

	for (int i = 0; i < p_vertices_count; i += 2, ++current_command->vertices_count) {

		current_command->vertices[current_command->vertices_count].x = flip_x ? -p_vertices[i] : p_vertices[i];
		current_command->vertices[current_command->vertices_count].y = flip_y ? p_vertices[i + 1] : -p_vertices[i + 1];
		current_command->colors[current_command->vertices_count] = *p_color;
		current_command->uvs[current_command->vertices_count].x = p_uvs[i] + index_item;
		current_command->uvs[current_command->vertices_count].y = p_uvs[i + 1];
	}
}

void SpineBatcher::add_set_blender_mode(bool p_mode) {

	//push_command();
	//element_list.push_back(memnew(SetBlendMode(p_mode)));
}

void SpineBatcher::build() {
	push_command();
	List<SpineBatchCommand *> building_list;
	for (List<SpineBatchCommand *>::Element *E = element_list.front(); E; E = E->next()) {

		SpineBatchCommand *e = E->get();
		e->build();
		building_list.push_back(e);
	}
	element_list.clear();

	mut->lock();
	// cleanup previously built list
	for (List<SpineBatchCommand *>::Element *E = built_list.front(); E; E = E->next()) {
		SpineCommandPool::get_instance()->release(E->get());
	}
	built_list.clear();
	for (List<SpineBatchCommand *>::Element *E = building_list.front(); E; E = E->next()) {
		built_list.push_back(E->get());
	}
	mut->unlock();
}

void SpineBatcher::draw() {
	// cleanup previous drawing first
	for (List<SpineBatchCommand *>::Element *E = drawed_list.front(); E; E = E->next()) {
		SpineBatchCommand *e = E->get();
		//memdelete(e);
		SpineCommandPool::get_instance()->release(e);
	}
	drawed_list.clear();

	RID ci = owner->get_canvas_item();
	mut->lock();
	for (List<SpineBatchCommand *>::Element *E = built_list.front(); E; E = E->next()) {
		SpineBatchCommand *e = E->get();
		e->draw(ci);
		drawed_list.push_back(e);
	}
	built_list.clear();
	mut->unlock();

}

void SpineBatcher::push_command() {

	if (current_command->vertices_count <= 0 || current_command->indies_count <= 0)
		return;

	element_list.push_back(current_command);
	//elements = memnew(Elements);
	current_command = SpineCommandPool::get_instance()->request();
}

SpineBatcher::SpineBatcher(Node2D *owner) : owner(owner) {

	current_command = SpineCommandPool::get_instance()->request();
	mut = Mutex::create();
}

SpineBatcher::~SpineBatcher() {

	for (List<SpineBatchCommand *>::Element *E = element_list.front(); E; E = E->next()) {

		SpineBatchCommand *e = E->get();
		//memdelete(e);
		SpineCommandPool::get_instance()->release(e);
		//memdelete(e);
	}
	element_list.clear();

	for (List<SpineBatchCommand *>::Element *E = built_list.front(); E; E = E->next()) {

		SpineBatchCommand *e = E->get();
		SpineCommandPool::get_instance()->release(e);
		//memdelete(e);
	}
	built_list.clear();

	for (List<SpineBatchCommand *>::Element *E = drawed_list.front(); E; E = E->next()) {

		SpineBatchCommand *e = E->get();
		//memdelete(e);
		SpineCommandPool::get_instance()->release(e);
	}
	drawed_list.clear();
	SpineCommandPool::get_instance()->release(current_command);
	//memdelete(elements);

}

#endif // MODULE_SPINE_ENABLED
