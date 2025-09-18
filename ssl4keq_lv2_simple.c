/*
 * SSL 4000 EQ - Simple Pure LV2 Implementation with Control Ports
 * This version uses control ports directly and provides inline display
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
#include <cairo/cairo.h>

// Inline display extension
#define LV2_INLINEDISPLAY__interface "http://lv2plug.in/ns/ext/inline-display#interface"

// Plugin URI
#define SSL4KEQ_URI "https://example.com/plugins/SSL_4000_EQ_Pure"

// Port indices matching TTL exactly
typedef enum {
    PORT_IN_L = 0,
    PORT_IN_R = 1,
    PORT_OUT_L = 2,
    PORT_OUT_R = 3,
    PORT_HPF_FREQ = 4,
    PORT_LPF_FREQ = 5,
    PORT_LF_GAIN = 6,
    PORT_LF_FREQ = 7,
    PORT_LF_BELL = 8,
    PORT_LM_GAIN = 9,
    PORT_LM_FREQ = 10,
    PORT_LM_Q = 11,
    PORT_HM_GAIN = 12,
    PORT_HM_FREQ = 13,
    PORT_HM_Q = 14,
    PORT_HF_GAIN = 15,
    PORT_HF_FREQ = 16,
    PORT_HF_BELL = 17,
    PORT_EQ_TYPE = 18,
    PORT_BYPASS = 19,
    PORT_OUTPUT_GAIN = 20,
    PORT_SATURATION = 21,
    PORT_COUNT = 22
} PortIndex;

// Biquad filter
typedef struct {
    float a0, a1, a2, b0, b1, b2;
    float z1, z2;
} BiquadFilter;

// LV2 Inline Display structures (must be defined before use)
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    void* data;
} LV2_Inline_Display_Image_Surface;

typedef struct {
    LV2_Inline_Display_Image_Surface* (*render)(LV2_Handle instance, uint32_t w, uint32_t h);
} LV2_Inline_Display_Interface;

// Plugin instance
typedef struct {
    // Port buffers
    const float* input[2];
    float* output[2];
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

    // DSP state
    float sample_rate;
    BiquadFilter hpf[2];
    BiquadFilter lpf[2];
    BiquadFilter lf[2];
    BiquadFilter lm[2];
    BiquadFilter hm[2];
    BiquadFilter hf[2];

    // Display
    cairo_surface_t* display_surface;
    LV2_Inline_Display_Image_Surface display_image;

    // Previous parameter values for change detection
    float prev_hpf_freq;
    float prev_lpf_freq;
    float prev_lf_gain;
    float prev_lf_freq;
    float prev_lf_bell;
    float prev_lm_gain;
    float prev_lm_freq;
    float prev_lm_q;
    float prev_hm_gain;
    float prev_hm_freq;
    float prev_hm_q;
    float prev_hf_gain;
    float prev_hf_freq;
    float prev_hf_bell;
    float prev_eq_type;
} SSL4KEQ;

// Calculate filter coefficients
static void calc_highpass(BiquadFilter* f, float freq, float q, float sr) {
    float omega = 2.0f * M_PI * freq / sr;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / (2.0f * q);

    float a0 = 1.0f + alpha;
    f->b0 = ((1.0f + cos_omega) / 2.0f) / a0;
    f->b1 = (-(1.0f + cos_omega)) / a0;
    f->b2 = ((1.0f + cos_omega) / 2.0f) / a0;
    f->a1 = (-2.0f * cos_omega) / a0;
    f->a2 = (1.0f - alpha) / a0;
    f->a0 = 1.0f;
}

static void calc_lowpass(BiquadFilter* f, float freq, float q, float sr) {
    float omega = 2.0f * M_PI * freq / sr;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / (2.0f * q);

    float a0 = 1.0f + alpha;
    f->b0 = ((1.0f - cos_omega) / 2.0f) / a0;
    f->b1 = (1.0f - cos_omega) / a0;
    f->b2 = ((1.0f - cos_omega) / 2.0f) / a0;
    f->a1 = (-2.0f * cos_omega) / a0;
    f->a2 = (1.0f - alpha) / a0;
    f->a0 = 1.0f;
}

static void calc_bell(BiquadFilter* f, float freq, float gain_db, float q, float sr) {
    float omega = 2.0f * M_PI * freq / sr;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float A = powf(10.0f, gain_db / 40.0f);
    float alpha = sin_omega / (2.0f * q);

    float a0 = 1.0f + alpha / A;
    f->b0 = (1.0f + alpha * A) / a0;
    f->b1 = (-2.0f * cos_omega) / a0;
    f->b2 = (1.0f - alpha * A) / a0;
    f->a1 = (-2.0f * cos_omega) / a0;
    f->a2 = (1.0f - alpha / A) / a0;
    f->a0 = 1.0f;
}

static void calc_shelf(BiquadFilter* f, float freq, float gain_db, bool is_high, float sr) {
    float omega = 2.0f * M_PI * freq / sr;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float A = powf(10.0f, gain_db / 40.0f);
    float S = 1.0f;
    float alpha = sin_omega / 2.0f * sqrtf((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);

    if (is_high) {
        float a0 = (A + 1.0f) - (A - 1.0f) * cos_omega + 2.0f * sqrtf(A) * alpha;
        f->b0 = (A * ((A + 1.0f) + (A - 1.0f) * cos_omega + 2.0f * sqrtf(A) * alpha)) / a0;
        f->b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_omega)) / a0;
        f->b2 = (A * ((A + 1.0f) + (A - 1.0f) * cos_omega - 2.0f * sqrtf(A) * alpha)) / a0;
        f->a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cos_omega)) / a0;
        f->a2 = ((A + 1.0f) - (A - 1.0f) * cos_omega - 2.0f * sqrtf(A) * alpha) / a0;
    } else {
        float a0 = (A + 1.0f) + (A - 1.0f) * cos_omega + 2.0f * sqrtf(A) * alpha;
        f->b0 = (A * ((A + 1.0f) - (A - 1.0f) * cos_omega + 2.0f * sqrtf(A) * alpha)) / a0;
        f->b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_omega)) / a0;
        f->b2 = (A * ((A + 1.0f) - (A - 1.0f) * cos_omega - 2.0f * sqrtf(A) * alpha)) / a0;
        f->a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cos_omega)) / a0;
        f->a2 = ((A + 1.0f) + (A - 1.0f) * cos_omega - 2.0f * sqrtf(A) * alpha) / a0;
    }
    f->a0 = 1.0f;
}

static float process_biquad(BiquadFilter* f, float input) {
    float output = f->b0 * input + f->b1 * f->z1 + f->b2 * f->z2 - f->a1 * f->z1 - f->a2 * f->z2;
    f->z2 = f->z1;
    f->z1 = output;
    return output;
}

static void update_filters(SSL4KEQ* self) {
    float sr = self->sample_rate;

    // Only update if parameters changed
    bool changed = false;

    if (*self->hpf_freq != self->prev_hpf_freq) {
        for (int ch = 0; ch < 2; ch++) {
            calc_highpass(&self->hpf[ch], *self->hpf_freq, 0.7f, sr);
        }
        self->prev_hpf_freq = *self->hpf_freq;
        changed = true;
    }

    if (*self->lpf_freq != self->prev_lpf_freq) {
        for (int ch = 0; ch < 2; ch++) {
            calc_lowpass(&self->lpf[ch], *self->lpf_freq, 0.7f, sr);
        }
        self->prev_lpf_freq = *self->lpf_freq;
        changed = true;
    }

    // LF band
    if (*self->lf_gain != self->prev_lf_gain ||
        *self->lf_freq != self->prev_lf_freq ||
        *self->lf_bell != self->prev_lf_bell ||
        *self->eq_type != self->prev_eq_type) {

        bool is_black = *self->eq_type > 0.5f;
        for (int ch = 0; ch < 2; ch++) {
            if (is_black && *self->lf_bell > 0.5f) {
                calc_bell(&self->lf[ch], *self->lf_freq, *self->lf_gain, 0.7f, sr);
            } else {
                calc_shelf(&self->lf[ch], *self->lf_freq, *self->lf_gain, false, sr);
            }
        }
        self->prev_lf_gain = *self->lf_gain;
        self->prev_lf_freq = *self->lf_freq;
        self->prev_lf_bell = *self->lf_bell;
        changed = true;
    }

    // LM band
    if (*self->lm_gain != self->prev_lm_gain ||
        *self->lm_freq != self->prev_lm_freq ||
        *self->lm_q != self->prev_lm_q) {

        for (int ch = 0; ch < 2; ch++) {
            calc_bell(&self->lm[ch], *self->lm_freq, *self->lm_gain, *self->lm_q, sr);
        }
        self->prev_lm_gain = *self->lm_gain;
        self->prev_lm_freq = *self->lm_freq;
        self->prev_lm_q = *self->lm_q;
        changed = true;
    }

    // HM band
    if (*self->hm_gain != self->prev_hm_gain ||
        *self->hm_freq != self->prev_hm_freq ||
        *self->hm_q != self->prev_hm_q) {

        for (int ch = 0; ch < 2; ch++) {
            calc_bell(&self->hm[ch], *self->hm_freq, *self->hm_gain, *self->hm_q, sr);
        }
        self->prev_hm_gain = *self->hm_gain;
        self->prev_hm_freq = *self->hm_freq;
        self->prev_hm_q = *self->hm_q;
        changed = true;
    }

    // HF band
    if (*self->hf_gain != self->prev_hf_gain ||
        *self->hf_freq != self->prev_hf_freq ||
        *self->hf_bell != self->prev_hf_bell ||
        *self->eq_type != self->prev_eq_type) {

        bool is_black = *self->eq_type > 0.5f;
        for (int ch = 0; ch < 2; ch++) {
            if (is_black && *self->hf_bell > 0.5f) {
                calc_bell(&self->hf[ch], *self->hf_freq, *self->hf_gain, 0.7f, sr);
            } else {
                calc_shelf(&self->hf[ch], *self->hf_freq, *self->hf_gain, true, sr);
            }
        }
        self->prev_hf_gain = *self->hf_gain;
        self->prev_hf_freq = *self->hf_freq;
        self->prev_hf_bell = *self->hf_bell;
        changed = true;
    }

    self->prev_eq_type = *self->eq_type;
}

static float saturate(float input, float amount) {
    float drive = 1.0f + amount / 25.0f;
    return tanhf(input * drive) / drive;
}

// LV2 Functions
static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                              double rate,
                              const char* bundle_path,
                              const LV2_Feature* const* features) {
    SSL4KEQ* self = (SSL4KEQ*)calloc(1, sizeof(SSL4KEQ));
    if (!self) return NULL;

    self->sample_rate = rate;

    // Initialize previous values to force update
    self->prev_hpf_freq = -1;
    self->prev_lpf_freq = -1;

    return (LV2_Handle)self;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    switch (port) {
        case PORT_IN_L: self->input[0] = (const float*)data; break;
        case PORT_IN_R: self->input[1] = (const float*)data; break;
        case PORT_OUT_L: self->output[0] = (float*)data; break;
        case PORT_OUT_R: self->output[1] = (float*)data; break;
        case PORT_HPF_FREQ: self->hpf_freq = (const float*)data; break;
        case PORT_LPF_FREQ: self->lpf_freq = (const float*)data; break;
        case PORT_LF_GAIN: self->lf_gain = (const float*)data; break;
        case PORT_LF_FREQ: self->lf_freq = (const float*)data; break;
        case PORT_LF_BELL: self->lf_bell = (const float*)data; break;
        case PORT_LM_GAIN: self->lm_gain = (const float*)data; break;
        case PORT_LM_FREQ: self->lm_freq = (const float*)data; break;
        case PORT_LM_Q: self->lm_q = (const float*)data; break;
        case PORT_HM_GAIN: self->hm_gain = (const float*)data; break;
        case PORT_HM_FREQ: self->hm_freq = (const float*)data; break;
        case PORT_HM_Q: self->hm_q = (const float*)data; break;
        case PORT_HF_GAIN: self->hf_gain = (const float*)data; break;
        case PORT_HF_FREQ: self->hf_freq = (const float*)data; break;
        case PORT_HF_BELL: self->hf_bell = (const float*)data; break;
        case PORT_EQ_TYPE: self->eq_type = (const float*)data; break;
        case PORT_BYPASS: self->bypass = (const float*)data; break;
        case PORT_OUTPUT_GAIN: self->output_gain = (const float*)data; break;
        case PORT_SATURATION: self->saturation = (const float*)data; break;
    }
}

static void run(LV2_Handle instance, uint32_t n_samples) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    // Update filters if needed
    update_filters(self);

    // Process audio
    for (uint32_t i = 0; i < n_samples; i++) {
        for (int ch = 0; ch < 2; ch++) {
            float sample = self->input[ch][i];

            if (*self->bypass < 0.5f) {
                // Apply filters
                sample = process_biquad(&self->hpf[ch], sample);
                sample = process_biquad(&self->lf[ch], sample);
                sample = process_biquad(&self->lm[ch], sample);
                sample = process_biquad(&self->hm[ch], sample);
                sample = process_biquad(&self->hf[ch], sample);
                sample = process_biquad(&self->lpf[ch], sample);

                // Apply saturation
                if (*self->saturation > 0.0f) {
                    sample = saturate(sample, *self->saturation);
                }

                // Apply output gain
                float gain = powf(10.0f, *self->output_gain / 20.0f);
                sample *= gain;
            }

            self->output[ch][i] = sample;
        }
    }
}

static void cleanup(LV2_Handle instance) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    if (self->display_surface) {
        cairo_surface_destroy(self->display_surface);
    }

    free(self);
}

// Inline display render
static LV2_Inline_Display_Image_Surface* render_inline(LV2_Handle instance, uint32_t w, uint32_t h) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    // Ensure minimum size
    if (w < 100) w = 100;
    if (h < 50) h = 50;

    // Create or resize surface
    if (!self->display_surface ||
        cairo_image_surface_get_width(self->display_surface) != w ||
        cairo_image_surface_get_height(self->display_surface) != h) {

        if (self->display_surface) {
            cairo_surface_destroy(self->display_surface);
        }
        self->display_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    }

    cairo_t* cr = cairo_create(self->display_surface);

    // Clear background
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);

    // Draw title
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);
    cairo_move_to(cr, 5, 12);
    cairo_show_text(cr, "SSL 4000 EQ");

    // Draw frequency response curve
    float y_center = h * 0.5f;
    cairo_set_line_width(cr, 1.5);
    cairo_set_source_rgb(cr, 0.0, 0.7, 0.9);

    cairo_move_to(cr, 0, y_center);
    cairo_line_to(cr, w, y_center);
    cairo_stroke(cr);

    // Simple response visualization
    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    cairo_move_to(cr, 0, y_center);

    for (int x = 0; x < w; x++) {
        float freq = 20.0f * powf(1000.0f, (float)x / w);
        float response = 0.0f;

        // Add filter contributions (simplified)
        if (freq < *self->hpf_freq) response -= 10.0f;
        if (freq > *self->lpf_freq) response -= 10.0f;

        float y = y_center - response * 2;
        cairo_line_to(cr, x, y);
    }
    cairo_stroke(cr);

    // Show bypass status
    if (*self->bypass > 0.5f) {
        cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.8);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, w/2 - 30, h/2);
        cairo_show_text(cr, "BYPASS");
    }

    cairo_destroy(cr);
    cairo_surface_flush(self->display_surface);

    // Set up image surface structure
    self->display_image.width = w;
    self->display_image.height = h;
    self->display_image.stride = cairo_image_surface_get_stride(self->display_surface);
    self->display_image.data = cairo_image_surface_get_data(self->display_surface);

    return &self->display_image;
}

// Extension data
static const void* extension_data(const char* uri) {
    static const LV2_Inline_Display_Interface display = { render_inline };

    if (!strcmp(uri, LV2_INLINEDISPLAY__interface)) {
        return &display;
    }
    return NULL;
}

// LV2 Descriptor
static const LV2_Descriptor descriptor = {
    SSL4KEQ_URI,
    instantiate,
    connect_port,
    NULL, // activate
    run,
    NULL, // deactivate
    cleanup,
    extension_data
};

// Export
LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : NULL;
}