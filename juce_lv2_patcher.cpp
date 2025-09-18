// Runtime patcher to integrate inline display with JUCE's LV2 wrapper
// Compile this into the shared library to override lv2_descriptor

#include <cstring>
#include <dlfcn.h>

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

// Our inline display extension
extern "C" const void* ssl4keq_extension_data(const char* uri);

// Storage for original JUCE functions
static const LV2_Descriptor* juce_descriptor = nullptr;
static const void* (*juce_extension_data)(const char*) = nullptr;

// Patched extension_data that includes our inline display
static const void* patched_extension_data(const char* uri)
{
    // Check our custom extensions first
    const void* result = ssl4keq_extension_data(uri);
    if (result != nullptr) {
        return result;
    }

    // Fall back to JUCE's extensions
    if (juce_extension_data) {
        return juce_extension_data(uri);
    }

    return nullptr;
}

// Our override of lv2_descriptor
extern "C" __attribute__((visibility("default")))
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    // Only support index 0
    if (index != 0)
        return nullptr;

    // Create our patched descriptor once
    static LV2_Descriptor patched_descriptor;
    static bool initialized = false;

    if (!initialized) {
        // Get JUCE's original lv2_descriptor using dlsym
        void* handle = dlopen(nullptr, RTLD_LAZY | RTLD_NOLOAD);
        if (handle) {
            // Find the original symbol (look for weak symbols or renamed ones)
            typedef const LV2_Descriptor* (*LV2DescriptorFunc)(uint32_t);

            // Try different possible names
            LV2DescriptorFunc original = nullptr;

            // Look through all symbols for one that returns our URI
            Dl_info info;
            if (dladdr((void*)lv2_descriptor, &info)) {
                // Re-open the library to scan symbols
                void* lib_handle = dlopen(info.dli_fname, RTLD_LAZY | RTLD_LOCAL);
                if (lib_handle) {
                    // Try to find JUCE's implementation
                    // It should be defined somewhere else in the binary
                    original = (LV2DescriptorFunc)dlsym(lib_handle, "__juce_lv2_descriptor");
                    if (!original) {
                        original = (LV2DescriptorFunc)dlsym(lib_handle, "_ZN4juce14lv2_descriptorEj");
                    }
                    dlclose(lib_handle);
                }
            }

            // If we couldn't find it dynamically, use static JUCE descriptor
            // This requires JUCE to compile its descriptor with weak symbol
            extern const LV2_Descriptor* __real_lv2_descriptor(uint32_t) __attribute__((weak));
            if (__real_lv2_descriptor) {
                juce_descriptor = __real_lv2_descriptor(0);
            }

            dlclose(handle);
        }

        // If we still don't have JUCE's descriptor, create a minimal one
        // This should be populated by JUCE's initialization
        if (!juce_descriptor) {
            // Return null - JUCE's descriptor wasn't found
            return nullptr;
        }

        // Copy JUCE's descriptor
        patched_descriptor = *juce_descriptor;

        // Save JUCE's extension_data
        juce_extension_data = juce_descriptor->extension_data;

        // Replace with our patched version
        patched_descriptor.extension_data = patched_extension_data;

        initialized = true;
    }

    return &patched_descriptor;
}