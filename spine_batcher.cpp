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

SpineBatcher::DrawCommand::DrawCommand() {
	vertices = memnew_arr(Vector2, BATCH_CAPACITY);
	colors = memnew_arr(Color, BATCH_CAPACITY);
	uvs = memnew_arr(Vector2, BATCH_CAPACITY);
	indies = memnew_arr(int, BATCH_CAPACITY * 3);
	vertices_count = 0;
	indies_count = 0;
	mesh = VisualServer::get_singleton()->mesh_create();
};

SpineBatcher::DrawCommand::~DrawCommand() {
	memdelete_arr(vertices);
	memdelete_arr(colors);
	memdelete_arr(uvs);
	memdelete_arr(indies);
	VisualServer::get_singleton()->free(mesh);
}

void SpineBatcher::DrawCommand::draw(RID ci) {
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
	VisualServer::get_singleton()->canvas_item_add_mesh(ci, mesh, Transform2D(), Color(1,1,1), texture->get_rid());
}


void SpineBatcher::add(Ref<Texture> p_texture,
	const float* p_vertices, const float* p_uvs, int p_vertices_count,
	const unsigned short* p_indies, int p_indies_count,
	Color *p_color, bool flip_x, bool flip_y, int index_item) {

	if (p_texture != command->texture
		|| command->vertices_count + (p_vertices_count >> 1) > BATCH_CAPACITY
		|| command->indies_count + p_indies_count > BATCH_CAPACITY * 3) {

		push_command();
		command->texture = p_texture;
	}

	for (int i = 0; i < p_indies_count; ++i, ++command->indies_count)
		command->indies[command->indies_count] = p_indies[i] + command->vertices_count;

	for (int i = 0; i < p_vertices_count; i += 2, ++command->vertices_count) {
		command->vertices[command->vertices_count].x = flip_x ? -p_vertices[i] : p_vertices[i];
		command->vertices[command->vertices_count].y = flip_y ? p_vertices[i + 1] : -p_vertices[i + 1];
		command->colors[command->vertices_count] = *p_color;
		command->uvs[command->vertices_count].x = p_uvs[i] + index_item;
		command->uvs[command->vertices_count].y = p_uvs[i + 1];
	}
}

int SpineBatcher::triangles_count() {
	int count = command->indies_count / 3;
	for (List<DrawCommand *>::Element *E = command_list.front(); E; E = E->next()) {
		count += E->get()->indies_count/3;
	}
	for (List<DrawCommand*>::Element *E = drawed_list.front(); E; E = E->next()) {
		count += E->get()->indies_count/3;
	}
	return count;
}

void SpineBatcher::flush() {

	RID ci = owner->get_canvas_item();
	push_command();

	for (List<DrawCommand *>::Element *E = command_list.front(); E; E = E->next()) {
		DrawCommand *e = E->get();
		e->draw(ci);
		drawed_list.push_back(e);
	}
	command_list.clear();
}

void SpineBatcher::push_command() {

	if (command->vertices_count <= 0 || command->indies_count <= 0)
		return;

	command_list.push_back(command);
	command = create_command();
}

SpineBatcher::DrawCommand* SpineBatcher::create_command() {
	return memnew(SpineBatcher::DrawCommand);
}

void SpineBatcher::reset() {
	for (List<DrawCommand*>::Element *E = drawed_list.front(); E; E = E->next()) {
		DrawCommand *e = E->get();
		memdelete(e);
	}
	drawed_list.clear();
}

SpineBatcher::SpineBatcher(Node2D *owner) : owner(owner) {
	command = create_command();
}

SpineBatcher::~SpineBatcher() {

	for (List<DrawCommand *>::Element *E = command_list.front(); E; E = E->next()) {

		DrawCommand *e = E->get();
		memdelete(e);
	}
	command_list.clear();

	for (List<DrawCommand*>::Element *E = drawed_list.front(); E; E = E->next()) {

		DrawCommand *e = E->get();
		memdelete(e);
	}
	drawed_list.clear();

	memdelete(command);
}

#endif // MODULE_SPINE_ENABLED
