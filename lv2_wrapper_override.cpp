// Override JUCE's lv2_descriptor to add inline display support
#include <cstring>
#include <cstdint>

// LV2 descriptor structure
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

// Get our inline display extension
extern "C" const void* ssl4keq_extension_data(const char* uri);

// Get JUCE's original descriptor (it's defined in juce_audio_plugin_client_LV2.cpp)
extern "C" const LV2_Descriptor* __juce_internal_lv2_descriptor(uint32_t index);

// Storage for the modified descriptor
static LV2_Descriptor modified_descriptor;
static const void* (*original_extension_data)(const char*) = nullptr;

// Our wrapper for extension_data
static const void* wrapped_extension_data(const char* uri)
{
    // First check our inline display extension
    const void* result = ssl4keq_extension_data(uri);
    if (result != nullptr) {
        return result;
    }

    // Fall back to JUCE's extensions
    if (original_extension_data) {
        return original_extension_data(uri);
    }

    return nullptr;
}

// Override the lv2_descriptor export
extern "C" __attribute__((visibility("default")))
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    static bool initialized = false;

    if (index != 0) {
        return nullptr;
    }

    if (!initialized) {
        // This will call JUCE's implementation internally
        // We need to rename JUCE's symbol in the linking stage
        const LV2_Descriptor* juce_desc = __juce_internal_lv2_descriptor(index);

        if (juce_desc) {
            // Copy JUCE's descriptor
            modified_descriptor = *juce_desc;

            // Save the original extension_data
            original_extension_data = juce_desc->extension_data;

            // Replace with our wrapper
            modified_descriptor.extension_data = wrapped_extension_data;
        }

        initialized = true;
    }

    return &modified_descriptor;
}