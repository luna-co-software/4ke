#include "LV2InlineDisplay.h"
#include "SSL4KEQ.h"
#include <cmath>

// LV2 extension URI
#define LV2_INLINE_DISPLAY_URI "http://lv2plug.in/ns/ext/inline-display#interface"

//==============================================================================
LV2InlineDisplay::LV2InlineDisplay(SSL4KEQ* processor)
    : audioProcessor(processor)
{
    // SSL-style colors
    backgroundColour = juce::Colour(0xff1a1a1a);
    knobColour = juce::Colour(0xff4a4a4a);
    activeColour = juce::Colour(0xff00b4d8);  // SSL blue
    textColour = juce::Colour(0xffd4d4d4);
}

LV2InlineDisplay::~LV2InlineDisplay() = default;

//==============================================================================
LV2_Inline_Display_Image_Surface* LV2InlineDisplay::render(uint32_t width, uint32_t height)
{
    // Create or resize render buffer
    if (!renderBuffer || renderBuffer->getWidth() != (int)width || renderBuffer->getHeight() != (int)height)
    {
        renderBuffer = std::make_unique<juce::Image>(juce::Image::ARGB, width, height, true);
        surface = std::make_unique<LV2_Inline_Display_Image_Surface>();
    }

    // Create graphics context and render
    {
        juce::Graphics g(*renderBuffer);
        drawCompactEQ(g, width, height);
    }

    // Update surface data
    juce::Image::BitmapData bitmap(*renderBuffer, juce::Image::BitmapData::readOnly);
    surface->data = bitmap.data;
    surface->width = static_cast<int32_t>(width);
    surface->height = static_cast<int32_t>(height);
    surface->stride = bitmap.lineStride;
    surface->format = 0x20028888; // ARGB32

    return surface.get();
}

//==============================================================================
void LV2InlineDisplay::drawCompactEQ(juce::Graphics& g, int width, int height)
{
    // Fill background
    g.fillAll(backgroundColour);

    // Draw border
    g.setColour(knobColour);
    g.drawRect(0, 0, width, height, 1);

    // Layout calculation for compact display
    const float margin = 2.0f;
    const float knobSize = std::min(width / 10.0f, height / 3.0f);
    const float spacing = 2.0f;

    // Title bar (very compact)
    g.setColour(textColour);
    g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));

    bool isBlack = getParameterValue("eq_type") > 0.5f;
    bool isBypassed = getParameterValue("bypass") > 0.5f;

    juce::String title = "SSL ";
    title += isBlack ? "Black" : "Brown";
    if (isBypassed) title += " [BYPASS]";

    g.drawText(title, margin, margin, width - margin * 2, 12,
               juce::Justification::centred);

    // Main control area
    float y = 15;
    float x = margin;

    // Row 1: Filters and LF/HF bands (gains)
    // HPF
    drawMiniKnob(g, x, y, knobSize * 0.4f,
                 (getParameterValue("hpf_freq") - 20.0f) / 480.0f,
                 "HP", activeColour);
    x += knobSize * 0.8f + spacing;

    // LF Gain
    drawMiniKnob(g, x, y, knobSize * 0.4f,
                 (getParameterValue("lf_gain") + 20.0f) / 40.0f,
                 "LF", textColour);
    x += knobSize * 0.8f + spacing;

    // LM Gain
    drawMiniKnob(g, x, y, knobSize * 0.4f,
                 (getParameterValue("lm_gain") + 20.0f) / 40.0f,
                 "LM", textColour);
    x += knobSize * 0.8f + spacing;

    // HM Gain
    drawMiniKnob(g, x, y, knobSize * 0.4f,
                 (getParameterValue("hm_gain") + 20.0f) / 40.0f,
                 "HM", textColour);
    x += knobSize * 0.8f + spacing;

    // HF Gain
    drawMiniKnob(g, x, y, knobSize * 0.4f,
                 (getParameterValue("hf_gain") + 20.0f) / 40.0f,
                 "HF", textColour);
    x += knobSize * 0.8f + spacing;

    // LPF
    drawMiniKnob(g, x, y, knobSize * 0.4f,
                 (getParameterValue("lpf_freq") - 3000.0f) / 17000.0f,
                 "LP", activeColour);
    x += knobSize * 0.8f + spacing;

    // Output gain
    drawMiniKnob(g, x, y, knobSize * 0.4f,
                 (getParameterValue("output_gain") + 12.0f) / 24.0f,
                 "Out", juce::Colours::orange);

    // Row 2: Frequencies
    y += knobSize * 0.8f + spacing * 2;
    x = margin + knobSize * 0.8f + spacing; // Skip HPF position

    // LF Freq
    float lfFreq = getParameterValue("lf_freq");
    drawMiniKnob(g, x, y, knobSize * 0.35f,
                 (lfFreq - 20.0f) / 580.0f,
                 juce::String(lfFreq < 100 ? (int)lfFreq : (int)(lfFreq/10)*10) + "Hz",
                 knobColour);
    x += knobSize * 0.8f + spacing;

    // LM Freq
    float lmFreq = getParameterValue("lm_freq");
    drawMiniKnob(g, x, y, knobSize * 0.35f,
                 (lmFreq - 200.0f) / 2300.0f,
                 lmFreq >= 1000 ? juce::String(lmFreq/1000.0f, 1) + "k" : juce::String((int)lmFreq),
                 knobColour);
    x += knobSize * 0.8f + spacing;

    // HM Freq
    float hmFreq = getParameterValue("hm_freq");
    drawMiniKnob(g, x, y, knobSize * 0.35f,
                 (hmFreq - 600.0f) / 6400.0f,
                 hmFreq >= 1000 ? juce::String(hmFreq/1000.0f, 1) + "k" : juce::String((int)hmFreq),
                 knobColour);
    x += knobSize * 0.8f + spacing;

    // HF Freq
    float hfFreq = getParameterValue("hf_freq");
    drawMiniKnob(g, x, y, knobSize * 0.35f,
                 (hfFreq - 1500.0f) / 18500.0f,
                 hfFreq >= 1000 ? juce::String(hfFreq/1000.0f, 1) + "k" : juce::String((int)hfFreq),
                 knobColour);

    // Row 3: Q values and Bell switches (if Black mode)
    if (isBlack && height > 80)
    {
        y += knobSize * 0.7f + spacing;
        x = margin + knobSize * 0.8f + spacing;

        // LF Bell toggle
        bool lfBell = getParameterValue("lf_bell") > 0.5f;
        drawToggle(g, x + knobSize * 0.2f, y, knobSize * 0.3f, lfBell, "Bell");
        x += knobSize * 0.8f + spacing;

        // LM Q
        float lmQ = getParameterValue("lm_q");
        drawMiniKnob(g, x, y, knobSize * 0.3f, (lmQ - 0.5f) / 4.5f,
                     "Q" + juce::String(lmQ, 1), knobColour);
        x += knobSize * 0.8f + spacing;

        // HM Q
        float hmQ = getParameterValue("hm_q");
        drawMiniKnob(g, x, y, knobSize * 0.3f, (hmQ - 0.5f) / 4.5f,
                     "Q" + juce::String(hmQ, 1), knobColour);
        x += knobSize * 0.8f + spacing;

        // HF Bell toggle
        bool hfBell = getParameterValue("hf_bell") > 0.5f;
        drawToggle(g, x + knobSize * 0.2f, y, knobSize * 0.3f, hfBell, "Bell");
    }

    // Saturation indicator (bottom right corner if space)
    if (width > 180 && height > 70)
    {
        float saturation = getParameterValue("saturation");
        if (saturation > 0.0f)
        {
            g.setColour(juce::Colours::red.withAlpha(0.3f + saturation / 200.0f));
            g.fillEllipse(width - 12, height - 12, 8, 8);
            g.setColour(textColour);
            g.setFont(juce::Font(juce::FontOptions(7.0f)));
            g.drawText("S", width - 12, height - 12, 8, 8, juce::Justification::centred);
        }
    }
}

//==============================================================================
void LV2InlineDisplay::drawMiniKnob(juce::Graphics& g, float x, float y, float radius,
                                     float value, const juce::String& label,
                                     const juce::Colour& colour)
{
    const float cx = x + radius;
    const float cy = y + radius;

    // Background circle
    g.setColour(knobColour);
    g.fillEllipse(x, y, radius * 2, radius * 2);

    // Outer ring
    g.setColour(colour.withAlpha(0.7f));
    g.drawEllipse(x, y, radius * 2, radius * 2, 1.0f);

    // Value arc
    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float endAngle = juce::MathConstants<float>::pi * 2.75f;
    const float angle = startAngle + value * (endAngle - startAngle);

    juce::Path arc;
    arc.addCentredArc(cx, cy, radius - 2, radius - 2, 0,
                      startAngle, angle, true);
    g.setColour(colour);
    g.strokePath(arc, juce::PathStrokeType(2.0f));

    // Pointer
    const float pointerLength = radius * 0.7f;
    const float px = cx + pointerLength * std::cos(angle);
    const float py = cy + pointerLength * std::sin(angle);
    g.drawLine(cx, cy, px, py, 1.5f);

    // Label
    g.setColour(textColour);
    g.setFont(juce::Font(juce::FontOptions(7.0f)));
    g.drawText(label, x - radius, y + radius * 2, radius * 4, 10,
               juce::Justification::centred);
}

//==============================================================================
void LV2InlineDisplay::drawToggle(juce::Graphics& g, float x, float y, float size,
                                  bool state, const juce::String& label)
{
    // Button background
    g.setColour(state ? activeColour : knobColour);
    g.fillRoundedRectangle(x, y, size * 2, size, 2.0f);

    // LED indicator
    g.setColour(state ? juce::Colours::lightgreen : juce::Colours::darkred);
    g.fillEllipse(x + size * 1.5f, y + size * 0.25f, size * 0.5f, size * 0.5f);

    // Label
    g.setColour(textColour);
    g.setFont(juce::Font(juce::FontOptions(6.0f)));
    g.drawText(label, x, y + size, size * 2, 8, juce::Justification::centred);
}

//==============================================================================
float LV2InlineDisplay::getParameterValue(const juce::String& paramID)
{
    if (!audioProcessor) return 0.0f;

    auto* param = audioProcessor->parameters.getRawParameterValue(paramID);
    if (param) return param->load();

    return 0.0f;
}

void LV2InlineDisplay::setParameterValue(const juce::String& paramID, float value)
{
    if (!audioProcessor) return;

    if (auto* param = audioProcessor->parameters.getParameter(paramID))
    {
        param->setValueNotifyingHost(value);
    }
}

//==============================================================================
const LV2_Inline_Display_Interface* LV2InlineDisplay::getInterface()
{
    static LV2_Inline_Display_Interface interface = {
        ssl4keq_inline_display_render,
        nullptr  // get_size is optional
    };
    return &interface;
}

//==============================================================================
// C interface implementation
extern "C" {

void* ssl4keq_inline_display_render(void* instance, uint32_t w, uint32_t h)
{
    if (auto* processor = static_cast<SSL4KEQ*>(instance))
    {
        // Get or create inline display instance
        static thread_local std::unique_ptr<LV2InlineDisplay> display;
        if (!display)
        {
            display = std::make_unique<LV2InlineDisplay>(processor);
        }

        // Render and return surface
        return display->render(w, h);
    }
    return nullptr;
}

__attribute__((visibility("default")))
const void* ssl4keq_extension_data(const char* uri)
{
    if (std::strcmp(uri, LV2_INLINE_DISPLAY_URI) == 0)
    {
        return LV2InlineDisplay::getInterface();
    }
    return nullptr;
}

} // extern "C"