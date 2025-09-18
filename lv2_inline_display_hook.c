// Simple LD_PRELOAD hook to add inline display extension
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>

typedef struct {
    const char* URI;
    void* (*instantiate)(const void*, double, const char*, const void* const*);
    void (*connect_port)(void*, uint32_t, void*);
    void (*activate)(void*);
    void (*run)(void*, uint32_t);
    void (*deactivate)(void*);
    void (*cleanup)(void*);
    const void* (*extension_data)(const char*);
} LV2_Descriptor;

typedef struct {
    void* (*render)(void* instance, uint32_t w, uint32_t h);
    void* (*get_size)(void* instance, uint32_t* w, uint32_t* h);
} LV2_Inline_Display_Interface;

// Import the inline display render function
extern void* ssl4keq_inline_display_render(void* instance, uint32_t w, uint32_t h);

static const LV2_Descriptor* (*real_lv2_descriptor)(uint32_t) = NULL;
static LV2_Descriptor patched_descriptor;
static const void* (*original_extension_data)(const char*) = NULL;

// Our inline display interface
static LV2_Inline_Display_Interface inline_display_interface = {
    ssl4keq_inline_display_render,
    NULL  // get_size is optional
};

// Patched extension_data function
static const void* patched_extension_data(const char* uri) {
    // Check for inline display extension
    if (strcmp(uri, "http://lv2plug.in/ns/ext/inline-display#interface") == 0) {
        return &inline_display_interface;
    }

    // Fall back to original
    if (original_extension_data) {
        return original_extension_data(uri);
    }

    return NULL;
}

// Override lv2_descriptor
const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    if (!real_lv2_descriptor) {
        // Get the real function
        real_lv2_descriptor = (const LV2_Descriptor* (*)(uint32_t))dlsym(RTLD_NEXT, "lv2_descriptor");
    }

    if (!real_lv2_descriptor) {
        return NULL;
    }

    const LV2_Descriptor* original = real_lv2_descriptor(index);

    if (original && index == 0) {
        // Patch the descriptor
        patched_descriptor = *original;
        original_extension_data = original->extension_data;
        patched_descriptor.extension_data = patched_extension_data;
        return &patched_descriptor;
    }

    return original;
}