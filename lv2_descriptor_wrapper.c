// Wrapper to intercept lv2_descriptor and add inline display support
// This file should be compiled and linked AFTER JUCE's LV2 wrapper

#include <string.h>
#include <stdint.h>

// LV2 types
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

// Import our inline display extension
extern const void* ssl4keq_extension_data(const char* uri);

// Import JUCE's lv2_descriptor (renamed by linker)
extern const LV2_Descriptor* __real_lv2_descriptor(uint32_t index);

// Static storage for modified descriptor
static LV2_Descriptor modified_descriptor;
static const void* (*original_extension_data)(const char*) = 0;
static int initialized = 0;

// Wrapped extension_data function
static const void* wrapped_extension_data(const char* uri)
{
    // Check our custom extensions first
    const void* result = ssl4keq_extension_data(uri);
    if (result != 0) {
        return result;
    }

    // Fall back to JUCE's extensions
    if (original_extension_data) {
        return original_extension_data(uri);
    }

    return 0;
}

// Our wrapper lv2_descriptor - this becomes the new lv2_descriptor
__attribute__((visibility("default")))
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    if (index != 0) {
        return 0;
    }

    if (!initialized) {
        // Get JUCE's descriptor
        const LV2_Descriptor* juce_desc = __real_lv2_descriptor(index);
        if (juce_desc) {
            // Copy it
            modified_descriptor = *juce_desc;

            // Save original extension_data
            original_extension_data = juce_desc->extension_data;

            // Replace with our wrapper
            modified_descriptor.extension_data = wrapped_extension_data;
        }
        initialized = 1;
    }

    return &modified_descriptor;
}