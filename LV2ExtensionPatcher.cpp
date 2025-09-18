#include "LV2ExtensionPatcher.h"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

// External function from LV2InlineDisplay.cpp
extern "C" const void* ssl4keq_extension_data(const char* uri);

// Static member definitions
const void* (*LV2ExtensionPatcher::original_extension_data)(const char*) = nullptr;

const void* LV2ExtensionPatcher::patched_extension_data(const char* uri)
{
    // First check our custom extensions
    const void* result = ssl4keq_extension_data(uri);
    if (result != nullptr) {
        return result;
    }

    // Fall back to JUCE's original extension_data if it exists
    if (original_extension_data) {
        return original_extension_data(uri);
    }

    return nullptr;
}

void LV2ExtensionPatcher::patchDescriptor(const LV2_Descriptor* desc)
{
    if (!desc) return;

    // Save the original extension_data function
    original_extension_data = desc->extension_data;

    // Get memory page size and align address
    size_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t desc_addr = (uintptr_t)desc;
    uintptr_t page_start = desc_addr & ~(page_size - 1);

    // Make memory writable
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE) == 0) {
        // Replace the extension_data function pointer
        const_cast<LV2_Descriptor*>(desc)->extension_data = patched_extension_data;

        // Restore memory protection
        mprotect((void*)page_start, page_size, PROT_READ | PROT_EXEC);
    }
}

// Hook into lv2_descriptor export
extern "C" {

// JUCE's original lv2_descriptor
extern const LV2_Descriptor* juce_lv2_descriptor(uint32_t index) __attribute__((weak));

// Our wrapper
__attribute__((visibility("default")))
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    static bool patched = false;

    // Call JUCE's lv2_descriptor if it exists
    const LV2_Descriptor* desc = nullptr;

    // Try to get JUCE's descriptor through dlsym
    static const LV2_Descriptor* (*original_lv2_descriptor)(uint32_t) = nullptr;
    if (!original_lv2_descriptor) {
        // Get handle to current executable
        void* handle = dlopen(nullptr, RTLD_LAZY);
        if (handle) {
            // Look for JUCE's lv2_descriptor (may have different name)
            original_lv2_descriptor = (const LV2_Descriptor* (*)(uint32_t))dlsym(handle, "juce_lv2_descriptor");
            if (!original_lv2_descriptor) {
                // If not found, might be the default name
                void* sym = dlsym(handle, "lv2_descriptor");
                if (sym && sym != (void*)lv2_descriptor) {
                    original_lv2_descriptor = (const LV2_Descriptor* (*)(uint32_t))sym;
                }
            }
            dlclose(handle);
        }
    }

    if (original_lv2_descriptor) {
        desc = original_lv2_descriptor(index);
    }

    // If we got a descriptor and haven't patched yet, patch it
    if (desc && !patched) {
        LV2ExtensionPatcher::patchDescriptor(desc);
        patched = true;
    }

    return desc;
}

} // extern "C"