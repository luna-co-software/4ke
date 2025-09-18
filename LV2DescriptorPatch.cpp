// LV2 Descriptor Patch for Inline Display Support
// This patches JUCE's LV2 descriptor to add inline display extension

#include <cstring>
#include <dlfcn.h>

extern "C" {

// LV2 structures
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

// Our extension data function
extern const void* ssl4keq_extension_data(const char* uri);

// Original JUCE descriptor
static const LV2_Descriptor* juce_descriptor = nullptr;
static const void* (*original_extension_data)(const char* uri) = nullptr;

// Patched extension data that includes our inline display
static const void* patched_extension_data(const char* uri)
{
    // First try our custom extensions
    const void* result = ssl4keq_extension_data(uri);
    if (result != nullptr)
        return result;

    // Fall back to original JUCE extensions
    if (original_extension_data != nullptr)
        return original_extension_data(uri);

    return nullptr;
}

// Patched descriptor
static LV2_Descriptor patched_descriptor;

// The actual LV2 entry point
__attribute__((visibility("default")))
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    // Get the original JUCE descriptor
    if (juce_descriptor == nullptr && index == 0)
    {
        // Find the original JUCE lv2_descriptor
        void* handle = dlopen(nullptr, RTLD_LAZY);
        if (handle)
        {
            // Get next symbol after ours (should be JUCE's)
            typedef const LV2_Descriptor* (*LV2DescriptorFunc)(uint32_t);
            LV2DescriptorFunc* funcs[10];
            int count = 0;

            // This is a workaround - in production, use proper symbol resolution
            void* sym = dlsym(handle, "lv2_descriptor");
            if (sym && sym != (void*)lv2_descriptor)
            {
                LV2DescriptorFunc orig_func = (LV2DescriptorFunc)sym;
                juce_descriptor = orig_func(0);

                if (juce_descriptor != nullptr)
                {
                    // Copy the descriptor
                    patched_descriptor = *juce_descriptor;

                    // Save original extension_data
                    original_extension_data = juce_descriptor->extension_data;

                    // Replace with our patched version
                    patched_descriptor.extension_data = patched_extension_data;

                    return &patched_descriptor;
                }
            }
            dlclose(handle);
        }
    }

    if (index == 0 && juce_descriptor != nullptr)
        return &patched_descriptor;

    return nullptr;
}

} // extern "C"