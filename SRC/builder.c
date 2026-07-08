#include "builder.h"

mp_error_t mp_build_array(mp_zone_t* zone, mp_object_t* obj, uint32_t size) {
    if (!zone || !obj) return MP_ERROR_BAD_ARG;
    
    obj->type = MP_TYPE_ARRAY;
    obj->via.array.size = size;
    
    if (size > 0) {
        obj->via.array.ptr = (mp_object_t*)mp_zone_alloc(zone, sizeof(mp_object_t) * size);
        if (!obj->via.array.ptr) return MP_ERROR_NOMEM;
        
        // Initialize all elements to NIL just to be safe
        for (uint32_t i = 0; i < size; i++) {
            obj->via.array.ptr[i].type = MP_TYPE_NIL;
        }
    } else {
        obj->via.array.ptr = NULL;
    }
    
    return MP_OK;
}

mp_error_t mp_build_map(mp_zone_t* zone, mp_object_t* obj, uint32_t size) {
    if (!zone || !obj) return MP_ERROR_BAD_ARG;
    
    obj->type = MP_TYPE_MAP;
    obj->via.map.size = size;
    
    if (size > 0) {
        obj->via.map.ptr = (mp_object_kv_t*)mp_zone_alloc(zone, sizeof(mp_object_kv_t) * size);
        if (!obj->via.map.ptr) return MP_ERROR_NOMEM;
        
        // Initialize all key/value pairs to NIL
        for (uint32_t i = 0; i < size; i++) {
            obj->via.map.ptr[i].key.type = MP_TYPE_NIL;
            obj->via.map.ptr[i].val.type = MP_TYPE_NIL;
        }
    } else {
        obj->via.map.ptr = NULL;
    }
    
    return MP_OK;
}
