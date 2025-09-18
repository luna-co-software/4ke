/* LV2 Inline Display Override for JUCE-based plugins
 *
 * This creates a shared library that can override the extension_data
 * function to add inline display support to JUCE LV2 plugins.
 *
 * Compile with:
 * gcc -shared -fPIC -o lv2_inline_override.so lv2_inline_override.c -ldl
 *
 * Use with:
 * LD_PRELOAD=/path/to/lv2_inline_override.so ardour
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

// LV2 Inline Display structures
typedef struct {
    void* (*render)(void* instance, uint32_t w, uint32_t h);
} LV2_Inline_Display_Interface;

typedef struct {
    unsigned char* data;
    int32_t width;
    int32_t height;
    int32_t stride;
    uint32_t format;
} LV2_Inline_Display_Image_Surface;

// Our external functions from the C++ implementation
extern void* ssl4keq_inline_display_render(void* instance, uint32_t w, uint32_t h);
extern const void* ssl4keq_extension_data(const char* uri);

// Override the lv2_descriptor's extension_data
const void* extension_data(const char* uri)
{
    static const char* inline_display_uri = "http://lv2plug.in/ns/ext/inline-display#interface";

    // Check if this is a request for inline display
    if (uri && strcmp(uri, inline_display_uri) == 0) {
        // Return our inline display interface
        const void* result = ssl4keq_extension_data(uri);
        if (result) {
            fprintf(stderr, "LV2 Override: Returning inline display interface\n");
            return result;
        }
    }

    // Fall back to the original implementation
    static const void* (*original_extension_data)(const char*) = NULL;
    if (!original_extension_data) {
        original_extension_data = dlsym(RTLD_NEXT, "extension_data");
    }

    if (original_extension_data) {
        return original_extension_data(uri);
    }

    return NULL;
}