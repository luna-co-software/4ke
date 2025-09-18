/*
 * SSL 4000 EQ - LV2 Implementation with Debug Logging
 * Debug version to track inline display calls
 */

#define _GNU_SOURCE 1
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

// Inline display extension - Use Ardour/Harrison namespace
#include <cairo/cairo.h>
#define LV2_INLINEDISPLAY__interface "http://harrisonconsoles.com/lv2/inlinedisplay#interface"
#define LV2_INLINEDISPLAY__queue_draw "http://harrisonconsoles.com/lv2/inlinedisplay#queue_draw"

// Debug logging
#define DEBUG_LOG(fmt, ...) do { \
    FILE* f = fopen("/tmp/ssl4keq_debug.log", "a"); \
    if (f) { \
        fprintf(f, "[SSL4KEQ] " fmt "\n", ##__VA_ARGS__); \
        fclose(f); \
    } \
} while(0)

// Image Surface structure
typedef struct {
    unsigned char* data;
    int width;
    int height;
    int stride;
} LV2_Inline_Display_Image_Surface;

// Interface structure - must match Ardour's expectation
typedef struct {
    LV2_Inline_Display_Image_Surface* (*render)(LV2_Handle instance, uint32_t w, uint32_t h);
} LV2_Inline_Display_Interface;

// Plugin URI
#define SSL4KEQ_URI "https://example.com/plugins/SSL_4000_EQ_Pure"

// Port indices matching the TTL file exactly
typedef enum {
    SSL4KEQ_INPUT_L = 0,
    SSL4KEQ_INPUT_R = 1,
    SSL4KEQ_OUTPUT_L = 2,
    SSL4KEQ_OUTPUT_R = 3,
    SSL4KEQ_HPF_FREQ = 4,
    SSL4KEQ_LPF_FREQ = 5,
    SSL4KEQ_LF_GAIN = 6,
    SSL4KEQ_LF_FREQ = 7,
    SSL4KEQ_LF_BELL = 8,
    SSL4KEQ_LM_GAIN = 9,
    SSL4KEQ_LM_FREQ = 10,
    SSL4KEQ_LM_Q = 11,
    SSL4KEQ_HM_GAIN = 12,
    SSL4KEQ_HM_FREQ = 13,
    SSL4KEQ_HM_Q = 14,
    SSL4KEQ_HF_GAIN = 15,
    SSL4KEQ_HF_FREQ = 16,
    SSL4KEQ_HF_BELL = 17,
    SSL4KEQ_EQ_TYPE = 18,
    SSL4KEQ_BYPASS = 19,
    SSL4KEQ_OUTPUT_GAIN = 20,
    SSL4KEQ_SATURATION = 21,
    SSL4KEQ_PORT_COUNT = 22
} PortIndex;

// Simplified plugin structure for debug
typedef struct {
    // Port buffers - Audio ports
    const float* input[2];
    float* output[2];

    // Control ports
    const float* hpf_freq;
    const float* lpf_freq;
    const float* lf_gain;
    const float* lf_freq;
    const float* lf_bell;
    const float* lm_gain;
    const float* lm_freq;
    const float* lm_q;
    const float* hm_gain;
    const float* hm_freq;
    const float* hm_q;
    const float* hf_gain;
    const float* hf_freq;
    const float* hf_bell;
    const float* eq_type;
    const float* bypass;
    const float* output_gain;
    const float* saturation;

    double sample_rate;

    // Inline display
    cairo_surface_t* display_surface;
    LV2_Inline_Display_Image_Surface display_image;
    int render_count;
} SSL4KEQ;

// Instantiate
static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                              double rate,
                              const char* bundle_path,
                              const LV2_Feature* const* features) {
    DEBUG_LOG("instantiate() called - rate: %f, bundle: %s", rate, bundle_path);

    SSL4KEQ* self = (SSL4KEQ*)calloc(1, sizeof(SSL4KEQ));
    if (!self) {
        DEBUG_LOG("instantiate() failed - out of memory");
        return NULL;
    }

    self->sample_rate = rate;
    self->render_count = 0;

    DEBUG_LOG("instantiate() succeeded - instance: %p", self);
    return (LV2_Handle)self;
}

// Connect ports
static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    if (port < 4 || port > 21) {
        DEBUG_LOG("connect_port() - port %d, data: %p", port, data);
    }

    switch (port) {
        case SSL4KEQ_INPUT_L:
            self->input[0] = (const float*)data;
            break;
        case SSL4KEQ_INPUT_R:
            self->input[1] = (const float*)data;
            break;
        case SSL4KEQ_OUTPUT_L:
            self->output[0] = (float*)data;
            break;
        case SSL4KEQ_OUTPUT_R:
            self->output[1] = (float*)data;
            break;
        case SSL4KEQ_HPF_FREQ:
            self->hpf_freq = (const float*)data;
            break;
        case SSL4KEQ_LPF_FREQ:
            self->lpf_freq = (const float*)data;
            break;
        case SSL4KEQ_LF_GAIN:
            self->lf_gain = (const float*)data;
            break;
        case SSL4KEQ_LF_FREQ:
            self->lf_freq = (const float*)data;
            break;
        case SSL4KEQ_LF_BELL:
            self->lf_bell = (const float*)data;
            break;
        case SSL4KEQ_LM_GAIN:
            self->lm_gain = (const float*)data;
            break;
        case SSL4KEQ_LM_FREQ:
            self->lm_freq = (const float*)data;
            break;
        case SSL4KEQ_LM_Q:
            self->lm_q = (const float*)data;
            break;
        case SSL4KEQ_HM_GAIN:
            self->hm_gain = (const float*)data;
            break;
        case SSL4KEQ_HM_FREQ:
            self->hm_freq = (const float*)data;
            break;
        case SSL4KEQ_HM_Q:
            self->hm_q = (const float*)data;
            break;
        case SSL4KEQ_HF_GAIN:
            self->hf_gain = (const float*)data;
            break;
        case SSL4KEQ_HF_FREQ:
            self->hf_freq = (const float*)data;
            break;
        case SSL4KEQ_HF_BELL:
            self->hf_bell = (const float*)data;
            break;
        case SSL4KEQ_EQ_TYPE:
            self->eq_type = (const float*)data;
            break;
        case SSL4KEQ_BYPASS:
            self->bypass = (const float*)data;
            break;
        case SSL4KEQ_OUTPUT_GAIN:
            self->output_gain = (const float*)data;
            break;
        case SSL4KEQ_SATURATION:
            self->saturation = (const float*)data;
            break;
    }
}

// Activate
static void activate(LV2_Handle instance) {
    DEBUG_LOG("activate() called");
}

// Run
static void run(LV2_Handle instance, uint32_t n_samples) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    // Simple passthrough for debug
    for (uint32_t i = 0; i < n_samples; i++) {
        for (int ch = 0; ch < 2; ch++) {
            if (self->output[ch] && self->input[ch]) {
                self->output[ch][i] = self->input[ch][i];
            }
        }
    }
}

// Cleanup
static void cleanup(LV2_Handle instance) {
    SSL4KEQ* self = (SSL4KEQ*)instance;
    DEBUG_LOG("cleanup() called - render was called %d times", self->render_count);

    if (self->display_surface) {
        cairo_surface_destroy(self->display_surface);
    }
    free(self);
}

// Inline display render function
static LV2_Inline_Display_Image_Surface* render_inline(LV2_Handle instance, uint32_t w, uint32_t h) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    self->render_count++;
    DEBUG_LOG("render_inline() called #%d - size: %dx%d", self->render_count, w, h);

    // Limit the height
    uint32_t actual_h = (uint32_t)fmin(h, ceilf(w * 9.f / 16.f));

    // Create or resize surface
    if (!self->display_surface ||
        cairo_image_surface_get_width(self->display_surface) != w ||
        cairo_image_surface_get_height(self->display_surface) != actual_h) {

        if (self->display_surface) {
            cairo_surface_destroy(self->display_surface);
        }
        self->display_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, actual_h);
        DEBUG_LOG("Created new surface: %dx%d", w, actual_h);
    }

    cairo_t* cr = cairo_create(self->display_surface);

    // Clear background with bright color for visibility
    cairo_set_source_rgba(cr, 0.1, 0.3, 0.5, 1.0);  // Blue-ish background
    cairo_rectangle(cr, 0, 0, w, actual_h);
    cairo_fill(cr);

    // Draw a simple test pattern
    cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);  // Yellow
    cairo_set_line_width(cr, 3.0);

    // Draw an X to make it obvious
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, w, actual_h);
    cairo_move_to(cr, w, 0);
    cairo_line_to(cr, 0, actual_h);
    cairo_stroke(cr);

    // Draw text
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, 10, 20);
    cairo_show_text(cr, "SSL 4K EQ DEBUG");

    // Show render count
    char text[64];
    snprintf(text, sizeof(text), "Render #%d", self->render_count);
    cairo_move_to(cr, 10, 40);
    cairo_show_text(cr, text);

    cairo_destroy(cr);

    // Flush surface
    cairo_surface_flush(self->display_surface);

    // Setup the image surface structure
    self->display_image.data = cairo_image_surface_get_data(self->display_surface);
    self->display_image.width = cairo_image_surface_get_width(self->display_surface);
    self->display_image.height = cairo_image_surface_get_height(self->display_surface);
    self->display_image.stride = cairo_image_surface_get_stride(self->display_surface);

    DEBUG_LOG("render_inline() returning surface: data=%p, size=%dx%d, stride=%d",
              self->display_image.data,
              self->display_image.width,
              self->display_image.height,
              self->display_image.stride);

    return &self->display_image;
}

// Extension data
static const void* extension_data(const char* uri) {
    DEBUG_LOG("extension_data() called with URI: %s", uri);

    static const LV2_Inline_Display_Interface display = { render_inline };

    if (!strcmp(uri, LV2_INLINEDISPLAY__interface)) {
        DEBUG_LOG("extension_data() returning inline display interface");
        return &display;
    }

    DEBUG_LOG("extension_data() returning NULL for unknown URI");
    return NULL;
}

// Plugin descriptor
static const LV2_Descriptor descriptor = {
    SSL4KEQ_URI,
    instantiate,
    connect_port,
    activate,
    run,
    NULL, // deactivate
    cleanup,
    extension_data
};

// LV2 symbol export
LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    DEBUG_LOG("lv2_descriptor(%d) called", index);
    return index == 0 ? &descriptor : NULL;
}