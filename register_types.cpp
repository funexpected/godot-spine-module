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

#include "core/os/file_access.h"
#include "core/os/os.h"
#include "core/io/resource_loader.h"
#include "scene/resources/texture.h"

#include <v3/spine/extension.h>
#include <v3/spine/spine.h>
#include "v3/spine/Atlas.h"
#include "spine.h"
#include "animation_node_spine.h"


#include <v4/spine/extension.h>
#include <v4/spine/spine.h>
#include "v4/spine/Atlas.h"

typedef Ref<Texture> TextureRef;
typedef Ref<ImageTexture> ImageTextureRef;

class ResourceFormatLoaderSpine;
class ResourceFormatLoaderSpine4;


Ref<ResourceFormatLoaderSpine> resource_loader_spine;
Ref<ResourceFormatLoaderSpine4> resource_loader_spine4;


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


void register_spine_types() {

	ClassDB::register_class<Spine>();
	ClassDB::register_class<Spine::SpineResource>();
	ClassDB::register_class<SpineMachine>();
	ClassDB::register_class<AnimationNodeSpineAnimation>();
	resource_loader_spine.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_spine);

	_spSetMalloc(spine_malloc);
	_spSetRealloc(spine_realloc);
	_spSetFree(spine_free);

	ClassDB::register_class<Spine4>();
	ClassDB::register_class<Spine4::Spine4Resource>();
	resource_loader_spine4.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_spine4);
	_sp4SetMalloc(spine_malloc);
	_sp4SetRealloc(spine_realloc);
	_sp4SetFree(spine_free);

}

void unregister_spine_types() {

	ResourceLoader::remove_resource_format_loader(resource_loader_spine);
	resource_loader_spine.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_spine4);
	resource_loader_spine4.unref();

}



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
		print_line("asking Spine3 res type");

		String el = p_path.get_extension().to_lower();
		if (el=="json" || el=="skel")
			return "SpineResource";
		return "";
	}
};





// ---------------------------------------------------


typedef Ref<Texture> TextureRef;
typedef Ref<ImageTexture> ImageTextureRef;

#define spA sp4A
#define spB sp4B
#define spC sp4C
#define spD sp4D
#define spE sp4E
#define spF sp4F
#define spG sp4G
#define spH sp4H
#define spI sp4I
#define spJ sp4J
#define spK sp4K
#define spL sp4L
#define spM sp4M
#define spN sp4N
#define spO sp4O
#define spP sp4P
#define spQ sp4Q
#define spR sp4R
#define spS sp4S
#define spT sp4T
#define spU sp4U
#define spV sp4V
#define spW sp4W
#define spX sp4X
#define spY sp4Y
#define spZ sp4Z

void _sp4AtlasPage_createTexture(sp4AtlasPage* self, const char* path) {
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

void _sp4AtlasPage_disposeTexture(sp4AtlasPage* self) {

	if(TextureRef *ref = static_cast<TextureRef *>(self->rendererObject))
		memdelete(ref);
}


char* _sp4Util_readFile(const char* p_path, int* p_length) {

	String str_path = String::utf8(p_path);
	FileAccess *f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(!f, NULL, "Can't read file " + String(p_path));

	*p_length = f->get_len();

	char *data = (char *)_sp4Malloc(*p_length, __FILE__, __LINE__);
	data = (char *)_sp4Malloc(*p_length, __FILE__, __LINE__);
	ERR_FAIL_COND_V(data == NULL, NULL);

	f->get_buffer((uint8_t *)data, *p_length);

	memdelete(f);
	return data;
}

class ResourceFormatLoaderSpine4 : public ResourceFormatLoader {
public:

	virtual RES load(const String &p_path, const String& p_original_path = "", Error *p_err=NULL) {
		float start = OS::get_singleton()->get_ticks_msec();
		Spine4::Spine4Resource *res = memnew(Spine4::Spine4Resource);
		Ref<Spine4::Spine4Resource> ref(res);
		String p_atlas = p_path.get_basename() + ".atlas_4";
		res->atlas = sp4Atlas_createFromFile(p_atlas.utf8().get_data(), 0);
		ERR_FAIL_COND_V(res->atlas == NULL, RES());

		if (p_path.get_extension() == "json_4"){
			sp4SkeletonJson *json = sp4SkeletonJson_create(res->atlas);
			ERR_FAIL_COND_V(json == NULL, RES());
			json->scale = 1;

			res->data = sp4SkeletonJson_readSkeletonDataFile(json, p_path.utf8().get_data());
			String err_msg = json->error ? json->error : "";
			sp4SkeletonJson_dispose(json);
			ERR_FAIL_COND_V_MSG(res->data == NULL, RES(), err_msg);
		} else {
			sp4SkeletonBinary* bin  = sp4SkeletonBinary_create(res->atlas);
			ERR_FAIL_COND_V(bin == NULL, RES());
			bin->scale = 1;
			res->data = sp4SkeletonBinary_readSkeletonDataFile(bin, p_path.utf8().get_data());
			String err_msg = bin->error ? bin->error : "";
			sp4SkeletonBinary_dispose(bin);
			ERR_FAIL_COND_V_MSG(res->data == NULL, RES(), err_msg);
		}

		res->set_path(p_path);
		float finish = OS::get_singleton()->get_ticks_msec();
		// print_line("Spine4 resource (" + p_path + ") loaded in " + itos(finish-start) + " msecs");
		return ref;
	}

	virtual void get_recognized_extensions(List<String> *p_extensions) const {
		p_extensions->push_back("skel_4");
		p_extensions->push_back("json_4");
		p_extensions->push_back("atlas_4");
	}

	virtual void get_dependencies(const String &p_path, List<String> *p_dependencies, bool p_add_types) {
		print_line("get_dependencies for " + p_path);
		String base_dir = p_path.get_base_dir();
		String base_name = p_path.get_basename();
		String atlas_path = base_name + ".atlas_4";
		if (!FileAccess::exists(atlas_path)) return;
		p_dependencies->push_back(atlas_path);
		Vector<uint8_t> bytes = FileAccess::get_file_as_array(atlas_path);
		sp4Atlas *atlas = sp4Atlas_create((const char*)bytes.ptr(), bytes.size(), base_dir.utf8(), NULL);
		sp4AtlasPage *page = atlas->pages;
		while (page) {
			p_dependencies->push_back(base_dir + "/" + page->name);
			page = page->next;
		}
		sp4Atlas_dispose(atlas);
	}

	virtual bool handles_type(const String& p_type) const {

		return p_type=="Spine4Resource";
	}

	virtual String get_resource_type(const String &p_path) const {
		print_line("asking Spine4 res type");
		String el = p_path.get_extension().to_lower();
		if (el=="json_4" || el=="skel_4")
			return "Spine4Resource";
		return "";
	}
};





#else

void register_spine_types() {}
void unregister_spine_types() {}

#endif // MODULE_SPINE_ENABLED
