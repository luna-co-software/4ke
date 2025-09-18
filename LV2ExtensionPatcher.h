#pragma once

#include <JuceHeader.h>
#include <cstring>
#include <dlfcn.h>

// LV2 descriptor structure
typedef struct {
    const char* URI;
    void* (*instantiate)(void*, double, const char*, const void* const*);
    void (*connect_port)(void*, uint32_t, void*);
    void (*activate)(void*);
    void (*run)(void*, uint32_t);
    void (*deactivate)(void*);
    void (*cleanup)(void*);
    const void* (*extension_data)(const char*);
} LV2_Descriptor;

class LV2ExtensionPatcher
{
public:
    // Function to patch the LV2 descriptor with our extension_data
    static void patchDescriptor(const LV2_Descriptor* desc);

private:
    // Store original extension_data function
    static const void* (*original_extension_data)(const char*);

    // Our wrapper function
    static const void* patched_extension_data(const char* uri);
};