#pragma once

#include <JuceHeader.h>
#include <cstring>
#include <memory>

// LV2 Inline Display extension headers
extern "C" {
    // Standard LV2 inline display interface
    typedef struct _LV2_Inline_Display_Interface {
        // Required: Render the inline display
        void* (*render)(void* instance,
                       uint32_t w, uint32_t h);

        // Optional: Get the preferred size
        void (*get_size)(void* instance,
                        uint32_t* w, uint32_t* h);
    } LV2_Inline_Display_Interface;

    typedef struct _LV2_Inline_Display_Image_Surface {
        unsigned char* data;
        int32_t  width;
        int32_t  height;
        int32_t  stride;
        uint32_t format; // 0x20028888 for ARGB32
    } LV2_Inline_Display_Image_Surface;
}

//==============================================================================
/**
    LV2 Inline Display implementation for SSL 4000 EQ

    Provides a compact graphical interface for Ardour's mixer strip inline controls
*/
class LV2InlineDisplay
{
public:
    LV2InlineDisplay(class SSL4KEQ* processor);
    ~LV2InlineDisplay();

    // Render the inline display
    LV2_Inline_Display_Image_Surface* render(uint32_t width, uint32_t height);

    // Get the LV2 extension interface
    static const LV2_Inline_Display_Interface* getInterface();

private:
    SSL4KEQ* audioProcessor;
    std::unique_ptr<juce::Image> renderBuffer;
    std::unique_ptr<LV2_Inline_Display_Image_Surface> surface;

    // Rendering helpers
    void drawCompactEQ(juce::Graphics& g, int width, int height);
    void drawMiniKnob(juce::Graphics& g, float x, float y, float radius,
                      float value, const juce::String& label,
                      const juce::Colour& colour = juce::Colours::white);
    void drawMiniSlider(juce::Graphics& g, float x, float y, float width, float height,
                        float value, const juce::String& label, bool isVertical = true);
    void drawToggle(juce::Graphics& g, float x, float y, float size,
                    bool state, const juce::String& label);

    // Parameter helpers
    float getParameterValue(const juce::String& paramID);
    void setParameterValue(const juce::String& paramID, float value);

    // Mouse interaction (if supported by host)
    bool handleMouseEvent(int x, int y, int button, bool pressed);

    // Color scheme
    juce::Colour backgroundColour;
    juce::Colour knobColour;
    juce::Colour activeColour;
    juce::Colour textColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LV2InlineDisplay)
};

//==============================================================================
// LV2 C interface functions
extern "C" {
    void* ssl4keq_inline_display_render(void* instance, uint32_t w, uint32_t h);
    const void* ssl4keq_extension_data(const char* uri);
}