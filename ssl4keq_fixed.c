/*
 * SSL 4000 EQ - LV2 Implementation with Proper Inline Display
 * Fixed version with correct control port mapping
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

// Queue draw structure - passed as feature data
typedef struct {
    void* handle;
    void (*queue_draw)(void* handle);
} LV2_Inline_Display;

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

// Biquad filter structure
typedef struct {
    double a0, a1, a2, b0, b1, b2;
    double z1, z2;
} BiquadFilter;

// Plugin instance structure
typedef struct {
    // Port buffers - Audio ports
    const float* input[2];
    float* output[2];

    // Control ports - direct pointers to host buffers
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

    // Filter states (one per channel)
    BiquadFilter hpf[2];
    BiquadFilter lpf[2];
    BiquadFilter lf[2];
    BiquadFilter lm[2];
    BiquadFilter hm[2];
    BiquadFilter hf[2];

    // Sample rate
    double sample_rate;

    // Inline display
    cairo_surface_t* display_surface;
    LV2_Inline_Display_Image_Surface display_image;
    LV2_Inline_Display* queue_draw;

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

// Biquad filter coefficient calculation
static void calc_highpass(BiquadFilter* f, double freq, double q, double sr) {
    double omega = 2.0 * M_PI * freq / sr;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double alpha = sin_omega / (2.0 * q);

    double a0 = 1.0 + alpha;
    f->b0 = ((1.0 + cos_omega) / 2.0) / a0;
    f->b1 = (-(1.0 + cos_omega)) / a0;
    f->b2 = ((1.0 + cos_omega) / 2.0) / a0;
    f->a1 = (-2.0 * cos_omega) / a0;
    f->a2 = (1.0 - alpha) / a0;
    f->a0 = 1.0;
}

static void calc_lowpass(BiquadFilter* f, double freq, double q, double sr) {
    double omega = 2.0 * M_PI * freq / sr;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double alpha = sin_omega / (2.0 * q);

    double a0 = 1.0 + alpha;
    f->b0 = ((1.0 - cos_omega) / 2.0) / a0;
    f->b1 = (1.0 - cos_omega) / a0;
    f->b2 = ((1.0 - cos_omega) / 2.0) / a0;
    f->a1 = (-2.0 * cos_omega) / a0;
    f->a2 = (1.0 - alpha) / a0;
    f->a0 = 1.0;
}

static void calc_bell(BiquadFilter* f, double freq, double gain_db, double q, double sr) {
    double A = pow(10.0, gain_db / 40.0);
    double omega = 2.0 * M_PI * freq / sr;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double alpha = sin_omega / (2.0 * q);

    double a0 = 1.0 + alpha / A;
    f->b0 = (1.0 + alpha * A) / a0;
    f->b1 = (-2.0 * cos_omega) / a0;
    f->b2 = (1.0 - alpha * A) / a0;
    f->a1 = (-2.0 * cos_omega) / a0;
    f->a2 = (1.0 - alpha / A) / a0;
    f->a0 = 1.0;
}

static void calc_shelf(BiquadFilter* f, double freq, double gain_db, double sr, bool high_shelf) {
    double A = pow(10.0, gain_db / 40.0);
    double omega = 2.0 * M_PI * freq / sr;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double S = 1.0; // Shelf slope
    double alpha = sin_omega / 2.0 * sqrt((A + 1.0/A) * (1.0/S - 1.0) + 2.0);

    if (high_shelf) {
        double a0 = (A + 1.0) - (A - 1.0) * cos_omega + 2.0 * sqrt(A) * alpha;
        f->b0 = A * ((A + 1.0) + (A - 1.0) * cos_omega + 2.0 * sqrt(A) * alpha) / a0;
        f->b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_omega) / a0;
        f->b2 = A * ((A + 1.0) + (A - 1.0) * cos_omega - 2.0 * sqrt(A) * alpha) / a0;
        f->a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_omega) / a0;
        f->a2 = ((A + 1.0) - (A - 1.0) * cos_omega - 2.0 * sqrt(A) * alpha) / a0;
        f->a0 = 1.0;
    } else {
        double a0 = (A + 1.0) + (A - 1.0) * cos_omega + 2.0 * sqrt(A) * alpha;
        f->b0 = A * ((A + 1.0) - (A - 1.0) * cos_omega + 2.0 * sqrt(A) * alpha) / a0;
        f->b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_omega) / a0;
        f->b2 = A * ((A + 1.0) - (A - 1.0) * cos_omega - 2.0 * sqrt(A) * alpha) / a0;
        f->a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_omega) / a0;
        f->a2 = ((A + 1.0) + (A - 1.0) * cos_omega - 2.0 * sqrt(A) * alpha) / a0;
        f->a0 = 1.0;
    }
}

// Process biquad filter
static inline double process_biquad(BiquadFilter* f, double input) {
    double output = f->b0 * input + f->z1;
    f->z1 = f->b1 * input - f->a1 * output + f->z2;
    f->z2 = f->b2 * input - f->a2 * output;
    return output;
}

// Helper function for frequency response calculation
static void apply_filter_response(BiquadFilter* f, double omega, double* real, double* imag) {
    double cos_omega = cos(omega);
    double sin_omega = sin(omega);
    double cos_2omega = cos(2*omega);
    double sin_2omega = sin(2*omega);

    double num_real = f->b0 + f->b1 * cos_omega + f->b2 * cos_2omega;
    double num_imag = f->b1 * sin_omega + f->b2 * sin_2omega;
    double den_real = 1.0 + f->a1 * cos_omega + f->a2 * cos_2omega;
    double den_imag = f->a1 * sin_omega + f->a2 * sin_2omega;

    // Complex division
    double den_mag_sq = den_real * den_real + den_imag * den_imag;
    double resp_real = (num_real * den_real + num_imag * den_imag) / den_mag_sq;
    double resp_imag = (num_imag * den_real - num_real * den_imag) / den_mag_sq;

    // Multiply with accumulated response
    double new_real = (*real) * resp_real - (*imag) * resp_imag;
    double new_imag = (*real) * resp_imag + (*imag) * resp_real;
    *real = new_real;
    *imag = new_imag;
}

// Calculate frequency response at given frequency
static double calc_frequency_response(SSL4KEQ* self, double freq) {
    double omega = 2.0 * M_PI * freq / self->sample_rate;

    // Complex number calculations for combined response
    double real = 1.0;
    double imag = 0.0;

    // Apply all filters (using first channel as reference)
    apply_filter_response(&self->hpf[0], omega, &real, &imag);
    apply_filter_response(&self->lpf[0], omega, &real, &imag);
    apply_filter_response(&self->lf[0], omega, &real, &imag);
    apply_filter_response(&self->lm[0], omega, &real, &imag);
    apply_filter_response(&self->hm[0], omega, &real, &imag);
    apply_filter_response(&self->hf[0], omega, &real, &imag);

    // Return magnitude in dB
    double magnitude = sqrt(real * real + imag * imag);
    return 20.0 * log10(fmax(magnitude, 1e-10));
}

// Update filter coefficients
static void update_filters(SSL4KEQ* self) {
    double sr = self->sample_rate;

    // Get current parameter values
    double hpf_freq = self->hpf_freq ? *self->hpf_freq : 20.0;
    double lpf_freq = self->lpf_freq ? *self->lpf_freq : 20000.0;
    double lf_gain = self->lf_gain ? *self->lf_gain : 0.0;
    double lf_freq = self->lf_freq ? *self->lf_freq : 100.0;
    double lf_bell = self->lf_bell ? *self->lf_bell : 0.0;
    double lm_gain = self->lm_gain ? *self->lm_gain : 0.0;
    double lm_freq = self->lm_freq ? *self->lm_freq : 600.0;
    double lm_q = self->lm_q ? *self->lm_q : 0.7;
    double hm_gain = self->hm_gain ? *self->hm_gain : 0.0;
    double hm_freq = self->hm_freq ? *self->hm_freq : 2000.0;
    double hm_q = self->hm_q ? *self->hm_q : 0.7;
    double hf_gain = self->hf_gain ? *self->hf_gain : 0.0;
    double hf_freq = self->hf_freq ? *self->hf_freq : 8000.0;
    double hf_bell = self->hf_bell ? *self->hf_bell : 0.0;
    double eq_type = self->eq_type ? *self->eq_type : 0.0;

    // High-pass filter
    for (int ch = 0; ch < 2; ch++) {
        calc_highpass(&self->hpf[ch], hpf_freq, 0.7, sr);
    }

    // Low-pass filter
    for (int ch = 0; ch < 2; ch++) {
        calc_lowpass(&self->lpf[ch], lpf_freq, 0.7, sr);
    }

    // LF band - shelf or bell based on mode and EQ type
    bool is_black = eq_type > 0.5;
    for (int ch = 0; ch < 2; ch++) {
        if (is_black && lf_bell > 0.5) {
            calc_bell(&self->lf[ch], lf_freq, lf_gain, 0.7, sr);
        } else {
            calc_shelf(&self->lf[ch], lf_freq, lf_gain, sr, false);
        }
    }

    // LM band - always bell
    for (int ch = 0; ch < 2; ch++) {
        calc_bell(&self->lm[ch], lm_freq, lm_gain, lm_q, sr);
    }

    // HM band - always bell
    for (int ch = 0; ch < 2; ch++) {
        calc_bell(&self->hm[ch], hm_freq, hm_gain, hm_q, sr);
    }

    // HF band - shelf or bell based on mode and EQ type
    for (int ch = 0; ch < 2; ch++) {
        if (is_black && hf_bell > 0.5) {
            calc_bell(&self->hf[ch], hf_freq, hf_gain, 0.7, sr);
        } else {
            calc_shelf(&self->hf[ch], hf_freq, hf_gain, sr, true);
        }
    }
}

// Saturation function
static inline float saturate(float input, float amount) {
    float drive = 1.0f + amount * 0.01f;
    return tanhf(input * drive) / drive;
}

// Instantiate
static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                              double rate,
                              const char* bundle_path,
                              const LV2_Feature* const* features) {
    SSL4KEQ* self = (SSL4KEQ*)calloc(1, sizeof(SSL4KEQ));
    if (!self) return NULL;

    self->sample_rate = rate;

    // Look for queue_draw feature
    if (features) {
        for (int i = 0; features[i]; i++) {
            if (!strcmp(features[i]->URI, LV2_INLINEDISPLAY__queue_draw)) {
                self->queue_draw = (LV2_Inline_Display*)features[i]->data;
            }
        }
    }

    // Initialize previous values to trigger initial update
    self->prev_hpf_freq = -1;
    self->prev_lpf_freq = -1;

    return (LV2_Handle)self;
}

// Connect ports
static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

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

// Activate (called before run)
static void activate(LV2_Handle instance) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    // Reset filter states
    for (int ch = 0; ch < 2; ch++) {
        memset(&self->hpf[ch], 0, sizeof(BiquadFilter));
        memset(&self->lpf[ch], 0, sizeof(BiquadFilter));
        memset(&self->lf[ch], 0, sizeof(BiquadFilter));
        memset(&self->lm[ch], 0, sizeof(BiquadFilter));
        memset(&self->hm[ch], 0, sizeof(BiquadFilter));
        memset(&self->hf[ch], 0, sizeof(BiquadFilter));
    }

    // Force filter update
    update_filters(self);
}

// Run
static void run(LV2_Handle instance, uint32_t n_samples) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    // Check for parameter changes and update filters if needed
    bool need_update = false;

    if (self->hpf_freq && *self->hpf_freq != self->prev_hpf_freq) {
        self->prev_hpf_freq = *self->hpf_freq;
        need_update = true;
    }
    if (self->lpf_freq && *self->lpf_freq != self->prev_lpf_freq) {
        self->prev_lpf_freq = *self->lpf_freq;
        need_update = true;
    }
    if (self->lf_gain && *self->lf_gain != self->prev_lf_gain) {
        self->prev_lf_gain = *self->lf_gain;
        need_update = true;
    }
    if (self->lf_freq && *self->lf_freq != self->prev_lf_freq) {
        self->prev_lf_freq = *self->lf_freq;
        need_update = true;
    }
    if (self->lf_bell && *self->lf_bell != self->prev_lf_bell) {
        self->prev_lf_bell = *self->lf_bell;
        need_update = true;
    }
    if (self->lm_gain && *self->lm_gain != self->prev_lm_gain) {
        self->prev_lm_gain = *self->lm_gain;
        need_update = true;
    }
    if (self->lm_freq && *self->lm_freq != self->prev_lm_freq) {
        self->prev_lm_freq = *self->lm_freq;
        need_update = true;
    }
    if (self->lm_q && *self->lm_q != self->prev_lm_q) {
        self->prev_lm_q = *self->lm_q;
        need_update = true;
    }
    if (self->hm_gain && *self->hm_gain != self->prev_hm_gain) {
        self->prev_hm_gain = *self->hm_gain;
        need_update = true;
    }
    if (self->hm_freq && *self->hm_freq != self->prev_hm_freq) {
        self->prev_hm_freq = *self->hm_freq;
        need_update = true;
    }
    if (self->hm_q && *self->hm_q != self->prev_hm_q) {
        self->prev_hm_q = *self->hm_q;
        need_update = true;
    }
    if (self->hf_gain && *self->hf_gain != self->prev_hf_gain) {
        self->prev_hf_gain = *self->hf_gain;
        need_update = true;
    }
    if (self->hf_freq && *self->hf_freq != self->prev_hf_freq) {
        self->prev_hf_freq = *self->hf_freq;
        need_update = true;
    }
    if (self->hf_bell && *self->hf_bell != self->prev_hf_bell) {
        self->prev_hf_bell = *self->hf_bell;
        need_update = true;
    }
    if (self->eq_type && *self->eq_type != self->prev_eq_type) {
        self->prev_eq_type = *self->eq_type;
        need_update = true;
    }

    if (need_update) {
        update_filters(self);
        // Request redraw of inline display
        if (self->queue_draw) {
            self->queue_draw->queue_draw(self->queue_draw->handle);
        }
    }

    // Get parameter values
    float bypass = self->bypass ? *self->bypass : 0.0f;
    float output_gain = self->output_gain ? *self->output_gain : 0.0f;
    float saturation = self->saturation ? *self->saturation : 20.0f;

    // Process audio
    for (uint32_t i = 0; i < n_samples; i++) {
        for (int ch = 0; ch < 2; ch++) {
            double sample = self->input[ch] ? self->input[ch][i] : 0.0;

            if (bypass < 0.5f) {
                // Apply filters in series
                sample = process_biquad(&self->hpf[ch], sample);
                sample = process_biquad(&self->lf[ch], sample);
                sample = process_biquad(&self->lm[ch], sample);
                sample = process_biquad(&self->hm[ch], sample);
                sample = process_biquad(&self->hf[ch], sample);
                sample = process_biquad(&self->lpf[ch], sample);

                // Apply saturation
                if (saturation > 0.0f) {
                    sample = saturate(sample, saturation);
                }

                // Apply output gain
                double gain = pow(10.0, output_gain / 20.0);
                sample *= gain;
            }

            if (self->output[ch]) {
                self->output[ch][i] = sample;
            }
        }
    }
}

// Cleanup
static void cleanup(LV2_Handle instance) {
    SSL4KEQ* self = (SSL4KEQ*)instance;
    if (self->display_surface) {
        cairo_surface_destroy(self->display_surface);
    }
    free(self);
}

// Inline display render function
static LV2_Inline_Display_Image_Surface* render_inline(LV2_Handle instance, uint32_t w, uint32_t h) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    // Limit the height to maintain aspect ratio
    uint32_t actual_h = (uint32_t)fmin(h, ceilf(w * 9.f / 16.f));

    // Create or resize surface if needed
    if (!self->display_surface ||
        cairo_image_surface_get_width(self->display_surface) != w ||
        cairo_image_surface_get_height(self->display_surface) != actual_h) {

        if (self->display_surface) {
            cairo_surface_destroy(self->display_surface);
        }
        self->display_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, actual_h);
    }

    cairo_t* cr = cairo_create(self->display_surface);

    // Clear background with dark gray
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
    cairo_rectangle(cr, 0, 0, w, actual_h);
    cairo_fill(cr);

    // Draw grid
    cairo_set_line_width(cr, 1.0);

    // Horizontal grid lines (gain)
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
    for (int db = -20; db <= 20; db += 10) {
        float y = actual_h * (0.5f - db / 40.0f);
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, w, y);
        cairo_stroke(cr);
    }

    // Center line
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 0.8);
    cairo_move_to(cr, 0, actual_h / 2);
    cairo_line_to(cr, w, actual_h / 2);
    cairo_stroke(cr);

    // Vertical grid lines (frequency)
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
    float freq_marks[] = {30, 100, 300, 1000, 3000, 10000};
    for (int i = 0; i < 6; i++) {
        float x = w * (log10f(freq_marks[i] / 20.0f) / log10f(1000.0f));
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, actual_h);
        cairo_stroke(cr);
    }

    // Get current parameter values
    float bypass = self->bypass ? *self->bypass : 0.0f;
    float eq_type = self->eq_type ? *self->eq_type : 0.0f;
    float output_gain = self->output_gain ? *self->output_gain : 0.0f;

    // Draw bypass indicator if active
    if (bypass > 0.5f) {
        cairo_set_source_rgba(cr, 1.0, 0.3, 0.3, 0.8);
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, w - 60, 20);
        cairo_show_text(cr, "BYPASS");
    }

    // Draw EQ type
    cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1.0);
    cairo_set_font_size(cr, 10);
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, eq_type > 0.5f ? "Black EQ" : "Brown EQ");

    // Draw frequency response curve
    cairo_set_line_width(cr, 2.0);
    cairo_set_source_rgba(cr, 0.3, 0.9, 1.0, 1.0);

    bool first = true;
    for (uint32_t x = 0; x < w; x++) {
        // Convert x position to frequency (20Hz to 20kHz on log scale)
        float freq = 20.0f * powf(1000.0f, (float)x / (float)w);

        // Calculate the frequency response at this frequency
        float response_db = calc_frequency_response(self, freq);

        // Add output gain to the response
        response_db += output_gain;

        // Clamp response to display range
        response_db = fmaxf(-20.0f, fminf(20.0f, response_db));

        // Convert dB to y position
        float y = actual_h * (0.5f - response_db / 40.0f);

        if (first) {
            cairo_move_to(cr, x, y);
            first = false;
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);

    // Draw band indicators with current values
    cairo_set_font_size(cr, 9);
    cairo_set_source_rgba(cr, 0.9, 0.9, 0.9, 1.0);

    // Get all parameter values for display
    float hpf_freq = self->hpf_freq ? *self->hpf_freq : 20.0f;
    float lpf_freq = self->lpf_freq ? *self->lpf_freq : 20000.0f;
    float lf_gain = self->lf_gain ? *self->lf_gain : 0.0f;
    float lf_freq = self->lf_freq ? *self->lf_freq : 100.0f;
    float lm_gain = self->lm_gain ? *self->lm_gain : 0.0f;
    float lm_freq = self->lm_freq ? *self->lm_freq : 600.0f;
    float hm_gain = self->hm_gain ? *self->hm_gain : 0.0f;
    float hm_freq = self->hm_freq ? *self->hm_freq : 2000.0f;
    float hf_gain = self->hf_gain ? *self->hf_gain : 0.0f;
    float hf_freq = self->hf_freq ? *self->hf_freq : 8000.0f;

    // Draw markers for each band if gain is non-zero
    cairo_set_line_width(cr, 1.0);

    // Helper function for drawing band markers
    void draw_band_marker(float freq, float gain, const char* label, float r, float g, float b) {
        if (fabs(gain) > 0.1f) {
            float x = w * (log10f(freq / 20.0f) / log10f(1000.0f));
            float y = actual_h * (0.5f - gain / 40.0f);

            // Draw vertical line
            cairo_set_source_rgba(cr, r, g, b, 0.5);
            cairo_move_to(cr, x, actual_h / 2);
            cairo_line_to(cr, x, y);
            cairo_stroke(cr);

            // Draw circle at peak
            cairo_arc(cr, x, y, 3, 0, 2 * M_PI);
            cairo_set_source_rgba(cr, r, g, b, 1.0);
            cairo_fill(cr);

            // Draw label
            char text[32];
            snprintf(text, sizeof(text), "%s: %.1fdB", label, gain);
            cairo_move_to(cr, x + 5, y - 5);
            cairo_show_text(cr, text);
        }
    }

    // Draw each band marker with different colors
    draw_band_marker(lf_freq, lf_gain, "LF", 1.0, 0.5, 0.2);
    draw_band_marker(lm_freq, lm_gain, "LM", 0.2, 1.0, 0.5);
    draw_band_marker(hm_freq, hm_gain, "HM", 0.5, 0.2, 1.0);
    draw_band_marker(hf_freq, hf_gain, "HF", 1.0, 0.2, 0.5);

    // Draw HPF/LPF indicators
    if (hpf_freq > 20.0f) {
        float x = w * (log10f(hpf_freq / 20.0f) / log10f(1000.0f));
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.5);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, actual_h);
        cairo_stroke(cr);
    }

    if (lpf_freq < 20000.0f) {
        float x = w * (log10f(lpf_freq / 20.0f) / log10f(1000.0f));
        cairo_set_source_rgba(cr, 1.0, 0.0, 1.0, 0.5);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, actual_h);
        cairo_stroke(cr);
    }

    // Draw title and output gain
    cairo_set_source_rgba(cr, 0.9, 0.9, 0.9, 1.0);
    cairo_set_font_size(cr, 12);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to(cr, 5, actual_h - 5);
    cairo_show_text(cr, "SSL 4000 EQ");

    if (fabs(output_gain) > 0.1f) {
        char gain_text[32];
        snprintf(gain_text, sizeof(gain_text), "Output: %.1fdB", output_gain);
        cairo_move_to(cr, w - 80, actual_h - 5);
        cairo_show_text(cr, gain_text);
    }

    cairo_destroy(cr);

    // Flush surface to ensure all drawing is complete
    cairo_surface_flush(self->display_surface);

    // Setup the image surface structure
    self->display_image.data = cairo_image_surface_get_data(self->display_surface);
    self->display_image.width = cairo_image_surface_get_width(self->display_surface);
    self->display_image.height = cairo_image_surface_get_height(self->display_surface);
    self->display_image.stride = cairo_image_surface_get_stride(self->display_surface);

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
    return index == 0 ? &descriptor : NULL;
}