// LD_PRELOAD library to add inline display support to JUCE LV2 plugins
// Compile: gcc -shared -fPIC -o lv2_inline_display_preload.so lv2_inline_display_preload.c -ldl
// Usage: LD_PRELOAD=./lv2_inline_display_preload.so <host>

#define _GNU_SOURCE
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

// Function pointer to original lv2_descriptor
static const LV2_Descriptor* (*original_lv2_descriptor)(uint32_t) = NULL;
static LV2_Descriptor patched_descriptor;
static const void* (*original_extension_data)(const char*) = NULL;
static int initialized = 0;

// Try to get ssl4keq_extension_data from the loaded library
static const void* (*get_ssl4keq_extension_data(void))(const char*)
{
    static const void* (*ssl_ext)(const char*) = NULL;
    if (!ssl_ext) {
        // Try to find the symbol in the loaded library
        void* handle = dlopen("/home/marc/.lv2/SSL 4000 EQ.lv2/libSSL 4000 EQ.so", RTLD_NOW | RTLD_NOLOAD);
        if (handle) {
            ssl_ext = dlsym(handle, "ssl4keq_extension_data");
        }
    }
    return ssl_ext;
}

// Patched extension_data function
static const void* patched_extension_data(const char* uri)
{
    // Check for inline display extension
    if (strcmp(uri, "http://lv2plug.in/ns/ext/inline-display#interface") == 0) {
        // Try to get our custom extension
        const void* (*ssl_ext)(const char*) = get_ssl4keq_extension_data();
        if (ssl_ext) {
            const void* result = ssl_ext(uri);
            if (result) return result;
        }
    }

    // Fall back to original
    if (original_extension_data) {
        return original_extension_data(uri);
    }

    return NULL;
}

// Override lv2_descriptor
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    // Initialize on first call
    if (!original_lv2_descriptor) {
        original_lv2_descriptor = dlsym(RTLD_NEXT, "lv2_descriptor");
        if (!original_lv2_descriptor) {
            fprintf(stderr, "Failed to find original lv2_descriptor\n");
            return NULL;
        }
    }

    // Only patch index 0
    if (index == 0 && !initialized) {
        const LV2_Descriptor* orig = original_lv2_descriptor(index);
        if (orig) {
            // Check if this is our SSL plugin
            if (strstr(orig->URI, "SSL_4000_EQ")) {
                // Copy the descriptor
                patched_descriptor = *orig;
                original_extension_data = orig->extension_data;
                patched_descriptor.extension_data = patched_extension_data;
                initialized = 1;

                fprintf(stderr, "Patched SSL 4000 EQ descriptor for inline display\n");
                return &patched_descriptor;
            }
        }
        return orig;
    }

    // For other indices or after initialization
    if (index == 0 && initialized) {
        return &patched_descriptor;
    }

    return original_lv2_descriptor(index);
}