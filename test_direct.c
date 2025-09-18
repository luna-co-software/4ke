/* Direct test of the patched extension_data
 * Compile: gcc -o test_direct test_direct.c -ldl
 */

#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>

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

int main() {
    // Load the library
    void* lib = dlopen("/home/marc/.lv2/SSL 4000 EQ.lv2/libSSL 4000 EQ.so", RTLD_NOW);
    if (!lib) {
        printf("Failed to load: %s\n", dlerror());
        return 1;
    }

    // Get lv2_descriptor
    const LV2_Descriptor* (*lv2_desc)(uint32_t) = dlsym(lib, "lv2_descriptor");
    if (!lv2_desc) {
        printf("No lv2_descriptor\n");
        return 1;
    }

    // Get descriptor
    const LV2_Descriptor* desc = lv2_desc(0);
    if (!desc) {
        printf("No descriptor\n");
        return 1;
    }

    printf("Plugin URI: %s\n", desc->URI);

    // Test extension_data
    if (desc->extension_data) {
        printf("Calling extension_data...\n");

        // Test with inline display URI
        const char* uri = "http://lv2plug.in/ns/ext/inline-display#interface";
        const void* ext = desc->extension_data(uri);

        printf("Result for %s: %p\n", uri, ext);

        // Also check if ssl4keq_extension_data is in the process
        void* self = dlopen(NULL, RTLD_LAZY);
        if (self) {
            const void* (*ssl_ext)(const char*) = dlsym(self, "ssl4keq_extension_data");
            if (ssl_ext) {
                printf("ssl4keq_extension_data found in process: %p\n", ssl_ext);
                const void* direct = ssl_ext(uri);
                printf("Direct call result: %p\n", direct);
            } else {
                printf("ssl4keq_extension_data NOT in process\n");
            }
        }
    }

    dlclose(lib);
    return 0;
}