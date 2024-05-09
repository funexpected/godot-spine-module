#include "spine_runtime.h"
#ifdef SPINE_RUNTIME_3_6_ENABLED
#include "spine_3_6/runtime.h"
#endif
#ifdef SPINE_RUNTIME_3_7_ENABLED
#include "spine_3_7/runtime.h"
#endif
#ifdef SPINE_RUNTIME_3_8_ENABLED
#include "spine_3_8/runtime.h"
#endif
#ifdef SPINE_RUNTIME_4_0_ENABLED
#include "spine_4_0/runtime.h"
#endif
#ifdef SPINE_RUNTIME_4_1_ENABLED
#include "spine_4_1/runtime.h"
#endif
#include "core/io/file_access.h"

void SpineRuntime::init() {
#ifdef SPINE_RUNTIME_3_6_ENABLED    
    SpineRuntime_3_6::init();
#endif
#ifdef SPINE_RUNTIME_3_7_ENABLED    
    SpineRuntime_3_7::init();
#endif
#ifdef SPINE_RUNTIME_3_8_ENABLED    
    SpineRuntime_3_8::init();
#endif
#ifdef SPINE_RUNTIME_4_0_ENABLED    
    SpineRuntime_4_0::init();
#endif
#ifdef SPINE_RUNTIME_4_1_ENABLED	
    SpineRuntime_4_1::init();
#endif
}

Ref<SpineResource> SpineRuntime::load_resource(const String &p_path) {
    Ref<SpineResource> res;
    if (p_path.ends_with(".json")) {

        Error err;
        Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);
        if (err != OK) {
            return res;
        }
        String json_content = file->get_as_utf8_string().replace(" ", "").replace("\n", "");
        file->close();
#ifdef SPINE_RUNTIME_3_6_ENABLED
        if (json_content.find("\"spine\":\"3.6") >= 0) {
            res = SpineRuntime_3_6::load_resource(p_path);
        } else
#endif
#ifdef SPINE_RUNTIME_3_7_ENABLED
        if (json_content.find("\"spine\":\"3.7") >= 0) {
            res = SpineRuntime_3_7::load_resource(p_path);
        } else
#endif
#ifdef SPINE_RUNTIME_3_8_ENABLED
        if (json_content.find("\"spine\":\"3.8") >= 0) {
            res = SpineRuntime_3_8::load_resource(p_path);
        } else
#endif
#ifdef SPINE_RUNTIME_4_0_ENABLED
        if (json_content.find("\"spine\":\"4.0") >= 0) {
            res = SpineRuntime_4_0::load_resource(p_path);
        } else
#endif
#ifdef SPINE_RUNTIME_4_1_ENABLED
        if (json_content.find("\"spine\":\"4.1") >= 0) {
            res = SpineRuntime_4_1::load_resource(p_path);
        } else
#endif
        {
            ERR_FAIL_V_MSG(res, "No suitable spine runtime found");
        }
    } else if (p_path.ends_with(".skel")) {
        Error err;
        Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);
        if (err != OK) {
            return res;
        }
        uint8_t header[32];
        file->get_buffer(header, 32);
        file->close();

#ifdef SPINE_RUNTIME_3_6_ENABLED
        if (header[29] == '3' && header[30] == '.' && header[31] == '6') {
            res = SpineRuntime_3_6::load_resource(p_path);
        } else
#endif
#ifdef SPINE_RUNTIME_3_7_ENABLED
        if (header[29] == '3' && header[30] == '.' && header[31] == '7') {
            res = SpineRuntime_3_7::load_resource(p_path);
        } else
#endif
#ifdef SPINE_RUNTIME_3_8_ENABLED    
        if (header[29] == '3' && header[30] == '.' && header[31] == '8') {
            res = SpineRuntime_3_8::load_resource(p_path);
        } else
#endif
#ifdef SPINE_RUNTIME_4_0_ENABLED    
        if (header[9] == '4' && header[10] == '.' && header[11] == '0') {
            res = SpineRuntime_4_0::load_resource(p_path);
        } else
#endif
#ifdef SPINE_RUNTIME_4_1_ENABLED    
        if (header[9] == '4' && header[10] == '.' && header[11] == '1') {
            res = SpineRuntime_4_1::load_resource(p_path);
        } else
#endif
        {
            ERR_FAIL_V_MSG(res, "No suitable spine runtime found");
        }
    }
    return res;
}

void SpineRuntime::_bind_methods() {
    ADD_SIGNAL(MethodInfo("animation_start", PropertyInfo(Variant::INT, "track")));
	ADD_SIGNAL(MethodInfo("animation_complete", PropertyInfo(Variant::INT, "track"), PropertyInfo(Variant::INT, "loop_count")));
	ADD_SIGNAL(MethodInfo("animation_event", PropertyInfo(Variant::INT, "track"), PropertyInfo(Variant::DICTIONARY, "event")));
	ADD_SIGNAL(MethodInfo("animation_end", PropertyInfo(Variant::INT, "track")));
	ADD_SIGNAL(MethodInfo("predelete"));
}

Ref<SpineRuntime> SpineResource::create_runtime() {
#ifdef SPINE_RUNTIME_3_6_ENABLED    
    if (runtime_version == "3_6") {
        return SpineRuntime_3_6::with_resource(Ref<SpineResource>(this));
    } else 
#endif
#ifdef SPINE_RUNTIME_3_7_ENABLED    
    if (runtime_version == "3_7") {
        return SpineRuntime_3_7::with_resource(Ref<SpineResource>(this));
    } else 
#endif
#ifdef SPINE_RUNTIME_3_8_ENABLED    
    if (runtime_version == "3_8") {
        return SpineRuntime_3_8::with_resource(Ref<SpineResource>(this));
    } else 
#endif
#ifdef SPINE_RUNTIME_4_0_ENABLED    
    if (runtime_version == "4_0") {
        return SpineRuntime_4_0::with_resource(Ref<SpineResource>(this));
    } else
#endif
#ifdef SPINE_RUNTIME_4_1_ENABLED    
    if (runtime_version == "4_1") {
        return SpineRuntime_4_1::with_resource(Ref<SpineResource>(this));
    } else
#endif
    {
        ERR_FAIL_V_MSG(Ref<SpineResource>(), "No suitable spine runtime found");
    }
}

bool SpineResource::_set(const StringName &p_name, const Variant &p_value) {
    return false;
}
bool SpineResource::_get(const StringName &p_name, Variant &r_ret) const {
    String name = p_name.operator String();
    if (name == "runtime_version") {
        r_ret = runtime_version.replace("_", ".");
        return true;
    } else {
        return false;
    }
}
void SpineResource::_get_property_list(List<PropertyInfo> *p_list) const {
    p_list->push_back(PropertyInfo(Variant::STRING, "runtime_version", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR));
}