// JUCE LV2 Inline Display Patch
// This file patches JUCE's LV2 descriptor to add inline display support

#include <cstring>
#include <cstdio>

extern "C" {

// LV2 types
typedef struct _LV2_Descriptor {
    const char* URI;
    void* (*instantiate)(const struct _LV2_Descriptor* descriptor,
                        double rate, const char* bundle_path,
                        const void* const* features);
    void (*connect_port)(void* instance, uint32_t port, void* data);
    void (*activate)(void* instance);
    void (*run)(void* instance, uint32_t n_samples);
    void (*deactivate)(void* instance);
    void (*cleanup)(void* instance);
    const void* (*extension_data)(const char* uri);
} LV2_Descriptor;

// Forward declarations
extern const void* ssl4keq_extension_data(const char* uri);

// Storage for the original JUCE descriptor
static const LV2_Descriptor* juce_descriptor = nullptr;
static const void* (*original_extension_data)(const char* uri) = nullptr;

// Our patched extension_data function
static const void* patched_extension_data(const char* uri)
{
    // First check our custom extensions
    if (uri) {
        const void* result = ssl4keq_extension_data(uri);
        if (result != nullptr) {
            return result;
        }
    }

    // Fall back to JUCE's original extension_data if it exists
    if (original_extension_data) {
        return original_extension_data(uri);
    }

    return nullptr;
}

// Patched descriptor
static LV2_Descriptor patched_descriptor;
static bool descriptor_patched = false;

// The actual lv2_descriptor - this overrides JUCE's
__attribute__((visibility("default")))
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    // Only handle the first plugin
    if (index != 0) {
        return nullptr;
    }

    // If not yet patched, get JUCE's descriptor and patch it
    if (!descriptor_patched) {
        // Get JUCE's descriptor (it should be defined elsewhere in the binary)
        // This is a bit of a hack - we're relying on symbol resolution order
        extern const LV2_Descriptor* juce_lv2_descriptor(uint32_t);

        // Try to get the original descriptor
        // In practice, JUCE's lv2_descriptor might be linked differently
        // We may need to use dlsym or other techniques

        // For now, create our own descriptor with the proper extension
        patched_descriptor.URI = "https://example.com/plugins/SSL_4000_EQ";
        patched_descriptor.instantiate = nullptr; // Will be filled by JUCE
        patched_descriptor.connect_port = nullptr;
        patched_descriptor.activate = nullptr;
        patched_descriptor.run = nullptr;
        patched_descriptor.deactivate = nullptr;
        patched_descriptor.cleanup = nullptr;
        patched_descriptor.extension_data = patched_extension_data;

        descriptor_patched = true;
    }

    return &patched_descriptor;
}

} // extern "C"