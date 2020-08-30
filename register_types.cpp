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

#include <core/class_db.h>
#include <core/project_settings.h>
#include "register_types.h"

#include <spine/extension.h>
#include <spine/spine.h>
#include "include/spine/Atlas.h"
#include "spine.h"

#include "core/os/file_access.h"
#include "core/os/os.h"
#include "core/io/resource_loader.h"
#include "scene/resources/texture.h"

typedef Ref<Texture> TextureRef;
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
	FileAccess *f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(!f, NULL, "Can't read file " + String(p_path));

	*p_length = f->get_len();

	char *data = (char *)_spMalloc(*p_length, __FILE__, __LINE__);
	data = (char *)_spMalloc(*p_length, __FILE__, __LINE__);
	ERR_FAIL_COND_V(data == NULL, NULL);

	f->get_buffer((uint8_t *)data, *p_length);

	memdelete(f);
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

class ResourceFormatLoaderSpine : public ResourceFormatLoader {
public:

	virtual RES load(const String &p_path, const String& p_original_path = "", Error *p_err=NULL) {
		float start = OS::get_singleton()->get_ticks_msec();
		Spine::SpineResource *res = memnew(Spine::SpineResource);
		Ref<Spine::SpineResource> ref(res);
		String p_atlas = p_path.get_basename() + ".atlas";
		res->atlas = spAtlas_createFromFile(p_atlas.utf8().get_data(), 0);
		ERR_FAIL_COND_V(res->atlas == NULL, RES());

		if (p_path.get_extension() == "json"){
			spSkeletonJson *json = spSkeletonJson_create(res->atlas);
			ERR_FAIL_COND_V(json == NULL, RES());
			json->scale = 1;

			res->data = spSkeletonJson_readSkeletonDataFile(json, p_path.utf8().get_data());
			String err_msg = json->error ? json->error : "";
			spSkeletonJson_dispose(json);
			ERR_FAIL_COND_V_MSG(res->data == NULL, RES(), err_msg);
		} else {
			spSkeletonBinary* bin  = spSkeletonBinary_create(res->atlas);
			ERR_FAIL_COND_V(bin == NULL, RES());
			bin->scale = 1;
			res->data = spSkeletonBinary_readSkeletonDataFile(bin, p_path.utf8().get_data());
			String err_msg = bin->error ? bin->error : "";
			spSkeletonBinary_dispose(bin);
			ERR_FAIL_COND_V_MSG(res->data == NULL, RES(), err_msg);
		}

		res->set_path(p_path);
		float finish = OS::get_singleton()->get_ticks_msec();
		// print_line("Spine resource (" + p_path + ") loaded in " + itos(finish-start) + " msecs");
		return ref;
	}

	virtual void get_recognized_extensions(List<String> *p_extensions) const {
		p_extensions->push_back("skel");
		p_extensions->push_back("json");
		p_extensions->push_back("atlas");
	}

	virtual void get_dependencies(const String &p_path, List<String> *p_dependencies, bool p_add_types) {
		print_line("get_dependencies for " + p_path);
		String base_dir = p_path.get_base_dir();
		String base_name = p_path.get_basename();
		String atlas_path = base_name + ".atlas";
		if (!FileAccess::exists(atlas_path)) return;
		p_dependencies->push_back(atlas_path);
		Vector<uint8_t> bytes = FileAccess::get_file_as_array(atlas_path);
		spAtlas *atlas = spAtlas_create((const char*)bytes.ptr(), bytes.size(), base_dir.utf8(), NULL);
		spAtlasPage *page = atlas->pages;
		while (page) {
			p_dependencies->push_back(base_dir + "/" + page->name);
			page = page->next;
		}
		spAtlas_dispose(atlas);
	}

	virtual bool handles_type(const String& p_type) const {

		return p_type=="SpineResource";
	}

	virtual String get_resource_type(const String &p_path) const {

		String el = p_path.get_extension().to_lower();
		if (el=="json" || el=="skel")
			return "SpineResource";
		return "";
	}
};

Ref<ResourceFormatLoaderSpine> resource_loader_spine;

void register_spine_types() {

	ClassDB::register_class<Spine>();
	ClassDB::register_class<Spine::SpineResource>();
	resource_loader_spine.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_spine);

	_spSetMalloc(spine_malloc);
	_spSetRealloc(spine_realloc);
	_spSetFree(spine_free);
}

void unregister_spine_types() {

	ResourceLoader::remove_resource_format_loader(resource_loader_spine);
	resource_loader_spine.unref();

}

#else

void register_spine_types() {}
void unregister_spine_types() {}

#endif // MODULE_SPINE_ENABLED
