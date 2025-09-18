// LV2 Extension Wrapper for SSL4KEQ
// This file bridges JUCE's LV2 implementation with our custom inline display

#include <cstring>

// Forward declarations
extern "C" {
    const void* ssl4keq_extension_data(const char* uri);

    // This function should be called by JUCE's LV2 wrapper
    __attribute__((visibility("default")))
    const void* get_extension_data(const char* uri)
    {
        // First check our custom extensions
        const void* result = ssl4keq_extension_data(uri);
        if (result != nullptr)
            return result;

        // Fall back to default JUCE extensions if any
        return nullptr;
    }
}

// Hook to override JUCE's extension data
#ifdef __cplusplus
namespace juce
{
    // If JUCE has a hook for custom LV2 extensions, we can override it here
    // This is implementation-specific to JUCE's LV2 wrapper
}
#endif