/*
 * Minimal LV2 Core Header for SSL 4000 EQ Plugin
 */

#ifndef LV2_H_INCLUDED
#define LV2_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_SYMBOL_EXPORT __attribute__((visibility("default")))

typedef void* LV2_Handle;

typedef struct _LV2_Feature {
    const char* URI;
    void* data;
} LV2_Feature;

typedef struct _LV2_Descriptor {
    const char* URI;

    LV2_Handle (*instantiate)(const struct _LV2_Descriptor* descriptor,
                              double sample_rate,
                              const char* bundle_path,
                              const LV2_Feature* const* features);

    void (*connect_port)(LV2_Handle instance,
                        uint32_t port,
                        void* data);

    void (*activate)(LV2_Handle instance);

    void (*run)(LV2_Handle instance,
               uint32_t sample_count);

    void (*deactivate)(LV2_Handle instance);

    void (*cleanup)(LV2_Handle instance);

    const void* (*extension_data)(const char* uri);
} LV2_Descriptor;

const LV2_Descriptor* lv2_descriptor(uint32_t index);

typedef LV2_Descriptor* (*LV2_Descriptor_Function)(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* LV2_H_INCLUDED */