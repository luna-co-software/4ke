/*
 * SSL 4000 EQ - Pure LV2 Implementation
 * This version provides proper inline display support for Ardour
 */

#define _GNU_SOURCE 1
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>

// Inline display extension
#include <cairo/cairo.h>
#define LV2_INLINEDISPLAY__interface "http://lv2plug.in/ns/ext/inline-display#interface"
#define LV2_INLINEDISPLAY__queue_draw "http://lv2plug.in/ns/ext/inline-display#queue_draw"

typedef struct {
    LV2_Atom_Forge_Frame frame;
    const LV2_Atom_Object* obj;
    LV2_URID property;
} SetterData;

// Define Image Surface first
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    void* data;
} LV2_Inline_Display_Image_Surface;

// Then define Interface that uses it
typedef struct {
    LV2_Inline_Display_Image_Surface* (*render)(LV2_Handle instance, uint32_t w, uint32_t h);
} LV2_Inline_Display_Interface;

// Plugin URI
#define SSL4KEQ_URI "https://example.com/plugins/SSL_4000_EQ_Pure"

// Port indices
typedef enum {
    SSL4KEQ_INPUT_L = 0,
    SSL4KEQ_INPUT_R,
    SSL4KEQ_OUTPUT_L,
    SSL4KEQ_OUTPUT_R,
    SSL4KEQ_CONTROL_IN,
    SSL4KEQ_CONTROL_OUT,
    SSL4KEQ_LATENCY,
    SSL4KEQ_FREEWHEEL,
    SSL4KEQ_ENABLED,
    SSL4KEQ_PORT_COUNT
} PortIndex;

// Biquad filter structure
typedef struct {
    float a0, a1, a2, b0, b1, b2;
    float z1, z2;
} BiquadFilter;

// Plugin instance structure
typedef struct {
    // Port buffers
    const float* input[2];
    float* output[2];
    const LV2_Atom_Sequence* control_in;
    LV2_Atom_Sequence* control_out;
    float* latency;
    const float* freewheel;
    const float* enabled;

    // Features
    LV2_URID_Map* map;
    LV2_Log_Logger logger;
    LV2_Atom_Forge forge;

    // URIDs for parameters
    struct {
        LV2_URID atom_Path;
        LV2_URID atom_Float;
        LV2_URID atom_Int;
        LV2_URID atom_Bool;
        LV2_URID atom_eventTransfer;
        LV2_URID bufsz_maxBlockLength;
        LV2_URID bufsz_minBlockLength;
        LV2_URID bufsz_sequenceSize;
        LV2_URID midi_MidiEvent;
        LV2_URID param_sampleRate;
        LV2_URID patch_Get;
        LV2_URID patch_Set;
        LV2_URID patch_property;
        LV2_URID patch_value;
        LV2_URID time_Position;
        LV2_URID time_barBeat;
        LV2_URID time_beatsPerMinute;
        LV2_URID time_speed;

        // Parameter URIDs
        LV2_URID hpf_freq;
        LV2_URID lpf_freq;
        LV2_URID lf_gain;
        LV2_URID lf_freq;
        LV2_URID lf_bell;
        LV2_URID lm_gain;
        LV2_URID lm_freq;
        LV2_URID lm_q;
        LV2_URID hm_gain;
        LV2_URID hm_freq;
        LV2_URID hm_q;
        LV2_URID hf_gain;
        LV2_URID hf_freq;
        LV2_URID hf_bell;
        LV2_URID eq_type;
        LV2_URID bypass;
        LV2_URID output_gain;
        LV2_URID saturation;
        LV2_URID oversampling;
    } uris;

    // Parameters
    float hpf_freq;
    float lpf_freq;
    float lf_gain;
    float lf_freq;
    float lf_bell;
    float lm_gain;
    float lm_freq;
    float lm_q;
    float hm_gain;
    float hm_freq;
    float hm_q;
    float hf_gain;
    float hf_freq;
    float hf_bell;
    float eq_type;
    float bypass;
    float output_gain;
    float saturation;
    float oversampling;

    // Filter states
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
    bool needs_redraw;

} SSL4KEQ;

// Map URIs
static void map_uris(SSL4KEQ* self) {
    LV2_URID_Map* map = self->map;

    self->uris.atom_Path = map->map(map->handle, LV2_ATOM__Path);
    self->uris.atom_Float = map->map(map->handle, LV2_ATOM__Float);
    self->uris.atom_Int = map->map(map->handle, LV2_ATOM__Int);
    self->uris.atom_Bool = map->map(map->handle, LV2_ATOM__Bool);
    self->uris.atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    self->uris.bufsz_maxBlockLength = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
    self->uris.bufsz_minBlockLength = map->map(map->handle, LV2_BUF_SIZE__minBlockLength);
    self->uris.bufsz_sequenceSize = map->map(map->handle, LV2_BUF_SIZE__sequenceSize);
    self->uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
    self->uris.param_sampleRate = map->map(map->handle, LV2_PARAMETERS__sampleRate);
    self->uris.patch_Get = map->map(map->handle, LV2_PATCH__Get);
    self->uris.patch_Set = map->map(map->handle, LV2_PATCH__Set);
    self->uris.patch_property = map->map(map->handle, LV2_PATCH__property);
    self->uris.patch_value = map->map(map->handle, LV2_PATCH__value);
    self->uris.time_Position = map->map(map->handle, LV2_TIME__Position);
    self->uris.time_barBeat = map->map(map->handle, LV2_TIME__barBeat);
    self->uris.time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
    self->uris.time_speed = map->map(map->handle, LV2_TIME__speed);

    // Parameter URIs
    self->uris.hpf_freq = map->map(map->handle, SSL4KEQ_URI "#hpf_freq");
    self->uris.lpf_freq = map->map(map->handle, SSL4KEQ_URI "#lpf_freq");
    self->uris.lf_gain = map->map(map->handle, SSL4KEQ_URI "#lf_gain");
    self->uris.lf_freq = map->map(map->handle, SSL4KEQ_URI "#lf_freq");
    self->uris.lf_bell = map->map(map->handle, SSL4KEQ_URI "#lf_bell");
    self->uris.lm_gain = map->map(map->handle, SSL4KEQ_URI "#lm_gain");
    self->uris.lm_freq = map->map(map->handle, SSL4KEQ_URI "#lm_freq");
    self->uris.lm_q = map->map(map->handle, SSL4KEQ_URI "#lm_q");
    self->uris.hm_gain = map->map(map->handle, SSL4KEQ_URI "#hm_gain");
    self->uris.hm_freq = map->map(map->handle, SSL4KEQ_URI "#hm_freq");
    self->uris.hm_q = map->map(map->handle, SSL4KEQ_URI "#hm_q");
    self->uris.hf_gain = map->map(map->handle, SSL4KEQ_URI "#hf_gain");
    self->uris.hf_freq = map->map(map->handle, SSL4KEQ_URI "#hf_freq");
    self->uris.hf_bell = map->map(map->handle, SSL4KEQ_URI "#hf_bell");
    self->uris.eq_type = map->map(map->handle, SSL4KEQ_URI "#eq_type");
    self->uris.bypass = map->map(map->handle, SSL4KEQ_URI "#bypass");
    self->uris.output_gain = map->map(map->handle, SSL4KEQ_URI "#output_gain");
    self->uris.saturation = map->map(map->handle, SSL4KEQ_URI "#saturation");
    self->uris.oversampling = map->map(map->handle, SSL4KEQ_URI "#oversampling");
}

// Biquad filter coefficient calculation
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
}

static void calc_bell(BiquadFilter* f, float freq, float gain_db, float q, float sr) {
    float A = powf(10.0f, gain_db / 40.0f);
    float omega = 2.0f * M_PI * freq / sr;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / (2.0f * q);

    float a0 = 1.0f + alpha / A;
    f->b0 = (1.0f + alpha * A) / a0;
    f->b1 = (-2.0f * cos_omega) / a0;
    f->b2 = (1.0f - alpha * A) / a0;
    f->a1 = (-2.0f * cos_omega) / a0;
    f->a2 = (1.0f - alpha / A) / a0;
}

static void calc_shelf(BiquadFilter* f, float freq, float gain_db, float sr, bool high_shelf) {
    float A = powf(10.0f, gain_db / 40.0f);
    float omega = 2.0f * M_PI * freq / sr;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float S = 1.0f; // Shelf slope
    float alpha = sin_omega / 2.0f * sqrtf((A + 1.0f/A) * (1.0f/S - 1.0f) + 2.0f);

    if (high_shelf) {
        float a0 = (A + 1.0f) - (A - 1.0f) * cos_omega + 2.0f * sqrtf(A) * alpha;
        f->b0 = A * ((A + 1.0f) + (A - 1.0f) * cos_omega + 2.0f * sqrtf(A) * alpha) / a0;
        f->b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_omega) / a0;
        f->b2 = A * ((A + 1.0f) + (A - 1.0f) * cos_omega - 2.0f * sqrtf(A) * alpha) / a0;
        f->a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_omega) / a0;
        f->a2 = ((A + 1.0f) - (A - 1.0f) * cos_omega - 2.0f * sqrtf(A) * alpha) / a0;
    } else {
        float a0 = (A + 1.0f) + (A - 1.0f) * cos_omega + 2.0f * sqrtf(A) * alpha;
        f->b0 = A * ((A + 1.0f) - (A - 1.0f) * cos_omega + 2.0f * sqrtf(A) * alpha) / a0;
        f->b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_omega) / a0;
        f->b2 = A * ((A + 1.0f) - (A - 1.0f) * cos_omega - 2.0f * sqrtf(A) * alpha) / a0;
        f->a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_omega) / a0;
        f->a2 = ((A + 1.0f) + (A - 1.0f) * cos_omega - 2.0f * sqrtf(A) * alpha) / a0;
    }
}

// Process biquad filter
static float process_biquad(BiquadFilter* f, float input) {
    float output = f->b0 * input + f->b1 * f->z1 + f->b2 * f->z2 - f->a1 * f->z1 - f->a2 * f->z2;
    f->z2 = f->z1;
    f->z1 = output;
    return output;
}

// Update filter coefficients
static void update_filters(SSL4KEQ* self) {
    float sr = self->sample_rate;

    // High-pass filter
    for (int ch = 0; ch < 2; ch++) {
        calc_highpass(&self->hpf[ch], self->hpf_freq, 0.7f, sr);
    }

    // Low-pass filter
    for (int ch = 0; ch < 2; ch++) {
        calc_lowpass(&self->lpf[ch], self->lpf_freq, 0.7f, sr);
    }

    // LF band - shelf or bell based on mode and EQ type
    bool is_black = self->eq_type > 0.5f;
    for (int ch = 0; ch < 2; ch++) {
        if (is_black && self->lf_bell > 0.5f) {
            calc_bell(&self->lf[ch], self->lf_freq, self->lf_gain, 0.7f, sr);
        } else {
            calc_shelf(&self->lf[ch], self->lf_freq, self->lf_gain, sr, false);
        }
    }

    // LM band - always bell
    for (int ch = 0; ch < 2; ch++) {
        calc_bell(&self->lm[ch], self->lm_freq, self->lm_gain, self->lm_q, sr);
    }

    // HM band - always bell
    for (int ch = 0; ch < 2; ch++) {
        calc_bell(&self->hm[ch], self->hm_freq, self->hm_gain, self->hm_q, sr);
    }

    // HF band - shelf or bell based on mode and EQ type
    for (int ch = 0; ch < 2; ch++) {
        if (is_black && self->hf_bell > 0.5f) {
            calc_bell(&self->hf[ch], self->hf_freq, self->hf_gain, 0.7f, sr);
        } else {
            calc_shelf(&self->hf[ch], self->hf_freq, self->hf_gain, sr, true);
        }
    }
}

// Saturation function
static float saturate(float input, float amount) {
    float drive = 1.0f + amount * 0.1f;
    return tanhf(input * drive) / drive;
}

// Instantiate
static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                              double rate,
                              const char* bundle_path,
                              const LV2_Feature* const* features) {
    SSL4KEQ* self = (SSL4KEQ*)calloc(1, sizeof(SSL4KEQ));
    if (!self) return NULL;

    // Get required features
    for (int i = 0; features[i]; i++) {
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            self->map = (LV2_URID_Map*)features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_LOG__log)) {
            self->logger.log = (LV2_Log_Log*)features[i]->data;
        }
    }

    if (!self->map) {
        free(self);
        return NULL;
    }

    // Initialize
    self->sample_rate = rate;
    map_uris(self);
    lv2_atom_forge_init(&self->forge, self->map);

    // Default parameter values
    self->hpf_freq = 20.0f;
    self->lpf_freq = 20000.0f;
    self->lf_gain = 0.0f;
    self->lf_freq = 100.0f;
    self->lf_bell = 0.0f;
    self->lm_gain = 0.0f;
    self->lm_freq = 600.0f;
    self->lm_q = 0.7f;
    self->hm_gain = 0.0f;
    self->hm_freq = 2000.0f;
    self->hm_q = 0.7f;
    self->hf_gain = 0.0f;
    self->hf_freq = 8000.0f;
    self->hf_bell = 0.0f;
    self->eq_type = 0.0f; // Brown
    self->bypass = 0.0f;
    self->output_gain = 0.0f;
    self->saturation = 20.0f;
    self->oversampling = 0.0f; // 2x

    update_filters(self);

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
        case SSL4KEQ_CONTROL_IN:
            self->control_in = (const LV2_Atom_Sequence*)data;
            break;
        case SSL4KEQ_CONTROL_OUT:
            self->control_out = (LV2_Atom_Sequence*)data;
            break;
        case SSL4KEQ_LATENCY:
            self->latency = (float*)data;
            break;
        case SSL4KEQ_FREEWHEEL:
            self->freewheel = (const float*)data;
            break;
        case SSL4KEQ_ENABLED:
            self->enabled = (const float*)data;
            break;
    }
}

// Process parameter changes
static void handle_atom_object(SSL4KEQ* self, const LV2_Atom_Object* obj) {
    const LV2_Atom* property = NULL;
    const LV2_Atom* value = NULL;

    lv2_atom_object_get(obj,
                        self->uris.patch_property, &property,
                        self->uris.patch_value, &value,
                        NULL);

    if (!property || property->type != self->uris.atom_Float) return;

    LV2_URID prop = ((LV2_Atom_URID*)property)->body;
    float val = ((LV2_Atom_Float*)value)->body;

    // Update parameters
    bool need_update = false;

    if (prop == self->uris.hpf_freq) {
        self->hpf_freq = val;
        need_update = true;
    } else if (prop == self->uris.lpf_freq) {
        self->lpf_freq = val;
        need_update = true;
    } else if (prop == self->uris.lf_gain) {
        self->lf_gain = val;
        need_update = true;
    } else if (prop == self->uris.lf_freq) {
        self->lf_freq = val;
        need_update = true;
    } else if (prop == self->uris.lf_bell) {
        self->lf_bell = val;
        need_update = true;
    } else if (prop == self->uris.lm_gain) {
        self->lm_gain = val;
        need_update = true;
    } else if (prop == self->uris.lm_freq) {
        self->lm_freq = val;
        need_update = true;
    } else if (prop == self->uris.lm_q) {
        self->lm_q = val;
        need_update = true;
    } else if (prop == self->uris.hm_gain) {
        self->hm_gain = val;
        need_update = true;
    } else if (prop == self->uris.hm_freq) {
        self->hm_freq = val;
        need_update = true;
    } else if (prop == self->uris.hm_q) {
        self->hm_q = val;
        need_update = true;
    } else if (prop == self->uris.hf_gain) {
        self->hf_gain = val;
        need_update = true;
    } else if (prop == self->uris.hf_freq) {
        self->hf_freq = val;
        need_update = true;
    } else if (prop == self->uris.hf_bell) {
        self->hf_bell = val;
        need_update = true;
    } else if (prop == self->uris.eq_type) {
        self->eq_type = val;
        need_update = true;
    } else if (prop == self->uris.bypass) {
        self->bypass = val;
    } else if (prop == self->uris.output_gain) {
        self->output_gain = val;
    } else if (prop == self->uris.saturation) {
        self->saturation = val;
    } else if (prop == self->uris.oversampling) {
        self->oversampling = val;
    }

    if (need_update) {
        update_filters(self);
        self->needs_redraw = true;
    }
}

// Run
static void run(LV2_Handle instance, uint32_t n_samples) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

    // Set latency (none for this simple implementation)
    if (self->latency) {
        *self->latency = 0.0f;
    }

    // Process control messages
    LV2_ATOM_SEQUENCE_FOREACH(self->control_in, ev) {
        if (ev->body.type == self->uris.atom_Float) {
            const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == self->uris.patch_Set) {
                handle_atom_object(self, obj);
            }
        }
    }

    // Process audio
    for (uint32_t i = 0; i < n_samples; i++) {
        for (int ch = 0; ch < 2; ch++) {
            float sample = self->input[ch][i];

            if (self->bypass < 0.5f && self->enabled && *self->enabled > 0.5f) {
                // Apply filters
                sample = process_biquad(&self->hpf[ch], sample);
                sample = process_biquad(&self->lf[ch], sample);
                sample = process_biquad(&self->lm[ch], sample);
                sample = process_biquad(&self->hm[ch], sample);
                sample = process_biquad(&self->hf[ch], sample);
                sample = process_biquad(&self->lpf[ch], sample);

                // Apply saturation
                if (self->saturation > 0.0f) {
                    sample = saturate(sample, self->saturation);
                }

                // Apply output gain
                float gain = powf(10.0f, self->output_gain / 20.0f);
                sample *= gain;
            }

            self->output[ch][i] = sample;
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

// Inline display render
static LV2_Inline_Display_Image_Surface* render_inline(LV2_Handle instance, uint32_t w, uint32_t h) {
    SSL4KEQ* self = (SSL4KEQ*)instance;

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
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);

    // Draw title
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, "SSL 4000 EQ");

    // Show EQ type
    cairo_set_font_size(cr, 10);
    cairo_move_to(cr, w - 50, 15);
    cairo_show_text(cr, self->eq_type > 0.5f ? "Black" : "Brown");

    // Draw bypass indicator
    if (self->bypass > 0.5f) {
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
        cairo_move_to(cr, w - 80, 15);
        cairo_show_text(cr, "BYPASS");
    }

    // Draw frequency response curve
    float y_center = h * 0.5f;
    float x_scale = w / 4.4f; // log scale from 20Hz to 20kHz
    float y_scale = h * 0.015f; // ±20dB range

    cairo_set_line_width(cr, 1.5);
    cairo_set_source_rgb(cr, 0.0, 0.7, 0.9);

    // Draw grid
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
    cairo_set_line_width(cr, 0.5);

    // Frequency grid lines (100Hz, 1kHz, 10kHz)
    float freqs[] = {100, 1000, 10000};
    for (int i = 0; i < 3; i++) {
        float x = logf(freqs[i] / 20.0f) * x_scale;
        cairo_move_to(cr, x, 20);
        cairo_line_to(cr, x, h - 5);
        cairo_stroke(cr);
    }

    // Gain grid lines
    for (int db = -15; db <= 15; db += 5) {
        float y = y_center - db * y_scale;
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, w, y);
        cairo_stroke(cr);
    }

    // Draw EQ curve
    cairo_set_source_rgb(cr, 0.0, 0.7, 0.9);
    cairo_set_line_width(cr, 2);

    bool first = true;
    for (int x_pos = 0; x_pos < w; x_pos++) {
        float freq = 20.0f * expf(x_pos * 4.4f / w);

        // Calculate combined frequency response
        float response_db = 0.0f;

        // Add contributions from each filter
        // (Simplified - in reality would need proper complex math)
        if (freq < self->hpf_freq) {
            response_db -= 12.0f * (1.0f - freq / self->hpf_freq);
        }
        if (freq > self->lpf_freq) {
            response_db -= 12.0f * (freq / self->lpf_freq - 1.0f);
        }

        // LF contribution
        if (fabsf(self->lf_gain) > 0.1f) {
            float dist = logf(freq / self->lf_freq);
            response_db += self->lf_gain * expf(-dist * dist);
        }

        // LM contribution
        if (fabsf(self->lm_gain) > 0.1f) {
            float dist = logf(freq / self->lm_freq);
            response_db += self->lm_gain * expf(-dist * dist / (self->lm_q * self->lm_q));
        }

        // HM contribution
        if (fabsf(self->hm_gain) > 0.1f) {
            float dist = logf(freq / self->hm_freq);
            response_db += self->hm_gain * expf(-dist * dist / (self->hm_q * self->hm_q));
        }

        // HF contribution
        if (fabsf(self->hf_gain) > 0.1f) {
            float dist = logf(freq / self->hf_freq);
            response_db += self->hf_gain * expf(-dist * dist);
        }

        // Clamp and draw
        response_db = fmaxf(-20.0f, fminf(20.0f, response_db));
        float y = y_center - response_db * y_scale;

        if (first) {
            cairo_move_to(cr, x_pos, y);
            first = false;
        } else {
            cairo_line_to(cr, x_pos, y);
        }
    }
    cairo_stroke(cr);

    // Draw parameter values
    cairo_set_font_size(cr, 9);
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);

    char text[64];
    int y_text = h - 15;

    // HPF
    snprintf(text, sizeof(text), "HPF:%.0f", self->hpf_freq);
    cairo_move_to(cr, 5, y_text);
    cairo_show_text(cr, text);

    // LF
    snprintf(text, sizeof(text), "LF:%.1fdB@%.0f", self->lf_gain, self->lf_freq);
    cairo_move_to(cr, 60, y_text);
    cairo_show_text(cr, text);

    // LM
    snprintf(text, sizeof(text), "LM:%.1fdB@%.0f", self->lm_gain, self->lm_freq);
    cairo_move_to(cr, 140, y_text);
    cairo_show_text(cr, text);

    // HM
    snprintf(text, sizeof(text), "HM:%.1fdB@%.0f", self->hm_gain, self->hm_freq);
    cairo_move_to(cr, 220, y_text);
    cairo_show_text(cr, text);

    // HF
    snprintf(text, sizeof(text), "HF:%.1fdB@%.0f", self->hf_gain, self->hf_freq);
    cairo_move_to(cr, 300, y_text);
    cairo_show_text(cr, text);

    // LPF
    snprintf(text, sizeof(text), "LPF:%.0f", self->lpf_freq);
    cairo_move_to(cr, 380, y_text);
    cairo_show_text(cr, text);

    // Output
    snprintf(text, sizeof(text), "Out:%.1fdB", self->output_gain);
    cairo_move_to(cr, 440, y_text);
    cairo_show_text(cr, text);

    cairo_destroy(cr);

    // Flush to ensure all drawing operations are complete
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
    static const LV2_State_Interface state = { NULL, NULL };
    static const LV2_Inline_Display_Interface display = { render_inline };

    if (!strcmp(uri, LV2_STATE__interface)) {
        return &state;
    } else if (!strcmp(uri, LV2_INLINEDISPLAY__interface)) {
        return &display;
    }
    return NULL;
}

// Descriptor
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