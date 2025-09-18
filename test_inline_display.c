/*
 * Test program for SSL 4000 EQ inline display
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>

typedef void* LV2_Handle;

typedef struct {
    unsigned char* data;
    int width;
    int height;
    int stride;
} LV2_Inline_Display_Image_Surface;

typedef struct {
    LV2_Inline_Display_Image_Surface* (*render)(LV2_Handle instance, uint32_t w, uint32_t h);
    void (*queue_draw)(LV2_Handle instance);
} LV2_Inline_Display_Interface;

typedef struct {
    const char* URI;
    void* data;
} LV2_Feature;

typedef struct {
    const char* URI;
    LV2_Handle (*instantiate)(const void* descriptor,
                              double sample_rate,
                              const char* bundle_path,
                              const LV2_Feature* const* features);
    void (*connect_port)(LV2_Handle instance,
                        uint32_t port,
                        void* data);
    void (*activate)(LV2_Handle instance);
    void (*run)(LV2_Handle instance,
               uint32_t sample_count);
    void (*deactivate)(LV2_Handle instance);
    void (*cleanup)(LV2_Handle instance);
    const void* (*extension_data)(const char* uri);
} LV2_Descriptor;

int main() {
    // Load the plugin
    void* handle = dlopen("./ssl4keq.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "Failed to load plugin: %s\n", dlerror());
        return 1;
    }

    // Get descriptor function
    const LV2_Descriptor* (*lv2_descriptor)(uint32_t) = dlsym(handle, "lv2_descriptor");
    if (!lv2_descriptor) {
        fprintf(stderr, "Failed to find lv2_descriptor: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    // Get plugin descriptor
    const LV2_Descriptor* descriptor = lv2_descriptor(0);
    if (!descriptor) {
        fprintf(stderr, "No plugin found at index 0\n");
        dlclose(handle);
        return 1;
    }

    printf("Plugin URI: %s\n", descriptor->URI);

    // Instantiate plugin
    LV2_Handle instance = descriptor->instantiate(descriptor, 48000.0, ".", NULL);
    if (!instance) {
        fprintf(stderr, "Failed to instantiate plugin\n");
        dlclose(handle);
        return 1;
    }

    // Create dummy buffers for ports
    float audio_in[2][256] = {0};
    float audio_out[2][256] = {0};
    float control_values[18] = {
        20.0f,    // HPF freq
        20000.0f, // LPF freq
        6.0f,     // LF gain
        100.0f,   // LF freq
        0.0f,     // LF bell
        -3.0f,    // LM gain
        600.0f,   // LM freq
        0.7f,     // LM Q
        3.0f,     // HM gain
        2000.0f,  // HM freq
        0.7f,     // HM Q
        -6.0f,    // HF gain
        8000.0f,  // HF freq
        0.0f,     // HF bell
        0.0f,     // EQ type (Brown)
        0.0f,     // Bypass
        0.0f,     // Output gain
        20.0f     // Saturation
    };

    // Connect ports
    descriptor->connect_port(instance, 0, audio_in[0]);
    descriptor->connect_port(instance, 1, audio_in[1]);
    descriptor->connect_port(instance, 2, audio_out[0]);
    descriptor->connect_port(instance, 3, audio_out[1]);

    // Connect control ports
    for (int i = 0; i < 18; i++) {
        descriptor->connect_port(instance, i + 4, &control_values[i]);
    }

    // Activate plugin
    if (descriptor->activate) {
        descriptor->activate(instance);
    }

    // Get inline display interface
    const LV2_Inline_Display_Interface* display =
        (const LV2_Inline_Display_Interface*)descriptor->extension_data("http://lv2plug.in/ns/ext/inline-display#interface");

    if (display && display->render) {
        printf("Inline display interface found!\n");

        // Test rendering at different sizes
        int widths[] = {400, 600, 800};
        int heights[] = {200, 300, 400};

        for (int i = 0; i < 3; i++) {
            printf("\nTesting render at %dx%d:\n", widths[i], heights[i]);

            LV2_Inline_Display_Image_Surface* surface = display->render(instance, widths[i], heights[i]);

            if (surface) {
                printf("  Rendered successfully!\n");
                printf("  Actual size: %dx%d\n", surface->width, surface->height);
                printf("  Stride: %d\n", surface->stride);
                printf("  Data pointer: %p\n", surface->data);

                // Save to PPM file for visual inspection
                char filename[64];
                snprintf(filename, sizeof(filename), "test_display_%dx%d.ppm", widths[i], heights[i]);
                FILE* fp = fopen(filename, "wb");
                if (fp) {
                    fprintf(fp, "P6\n%d %d\n255\n", surface->width, surface->height);
                    for (int y = 0; y < surface->height; y++) {
                        for (int x = 0; x < surface->width; x++) {
                            unsigned char* pixel = surface->data + y * surface->stride + x * 4;
                            // ARGB to RGB conversion
                            fputc(pixel[2], fp); // R
                            fputc(pixel[1], fp); // G
                            fputc(pixel[0], fp); // B
                        }
                    }
                    fclose(fp);
                    printf("  Saved to %s\n", filename);
                }
            } else {
                printf("  Render failed!\n");
            }
        }

        // Test with different parameter values
        printf("\nTesting with different EQ settings:\n");

        // Change some parameters
        control_values[2] = 12.0f;  // LF gain = +12dB
        control_values[5] = 6.0f;   // LM gain = +6dB
        control_values[8] = -6.0f;  // HM gain = -6dB
        control_values[11] = 9.0f;  // HF gain = +9dB

        // Run the plugin to update internal state
        descriptor->run(instance, 256);

        // Render again
        LV2_Inline_Display_Image_Surface* surface = display->render(instance, 600, 300);
        if (surface) {
            printf("  Rendered with new settings!\n");

            // Save to file
            FILE* fp = fopen("test_display_eq_curve.ppm", "wb");
            if (fp) {
                fprintf(fp, "P6\n%d %d\n255\n", surface->width, surface->height);
                for (int y = 0; y < surface->height; y++) {
                    for (int x = 0; x < surface->width; x++) {
                        unsigned char* pixel = surface->data + y * surface->stride + x * 4;
                        fputc(pixel[2], fp); // R
                        fputc(pixel[1], fp); // G
                        fputc(pixel[0], fp); // B
                    }
                }
                fclose(fp);
                printf("  Saved EQ curve to test_display_eq_curve.ppm\n");
            }
        }
    } else {
        printf("No inline display interface found!\n");
    }

    // Cleanup
    descriptor->cleanup(instance);
    dlclose(handle);

    printf("\nTest completed successfully!\n");
    return 0;
}