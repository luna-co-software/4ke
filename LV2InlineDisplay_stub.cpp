// Stub implementation for VST3 builds that don't need LV2 inline display
#ifndef JucePlugin_Build_LV2

#include "LV2InlineDisplay.h"
#include "SSL4KEQ.h"

// Empty stub implementation
LV2InlineDisplay::LV2InlineDisplay(SSL4KEQ* processor)
    : audioProcessor(processor)
{
}

LV2InlineDisplay::~LV2InlineDisplay() = default;

LV2_Inline_Display_Image_Surface* LV2InlineDisplay::render(uint32_t, uint32_t)
{
    return nullptr;
}

void LV2InlineDisplay::drawCompactEQ(juce::Graphics&, int, int) {}
void LV2InlineDisplay::drawMiniKnob(juce::Graphics&, float, float, float, float, const juce::String&, const juce::Colour&) {}
void LV2InlineDisplay::drawToggle(juce::Graphics&, float, float, float, bool, const juce::String&) {}
float LV2InlineDisplay::getParameterValue(const juce::String&) { return 0.0f; }
void LV2InlineDisplay::setParameterValue(const juce::String&, float) {}

const LV2_Inline_Display_Interface* LV2InlineDisplay::getInterface()
{
    return nullptr;
}

// Stub C functions
extern "C" {
    void* ssl4keq_inline_display_render(void*, uint32_t, uint32_t) { return nullptr; }
    const void* ssl4keq_extension_data(const char*) { return nullptr; }
}

#endif // !JucePlugin_Build_LV2