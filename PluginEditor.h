#pragma once

#include <JuceHeader.h>
#include "SSL4KEQ.h"
#include "SSLLookAndFeel.h"

//==============================================================================
/**
    SSL 4000 EQ Plugin Editor

    Mimics the layout and styling of the SSL 4000 console EQ section
*/
class SSL4KEQEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit SSL4KEQEditor(SSL4KEQ&);
    ~SSL4KEQEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    //==============================================================================
    // Reference to processor
    SSL4KEQ& audioProcessor;

    // Look and feel
    SSLLookAndFeel sslLookAndFeel;

    // HPF Section
    juce::Slider hpfFreqSlider;
    juce::Label hpfLabel;

    // LPF Section
    juce::Slider lpfFreqSlider;
    juce::Label lpfLabel;

    // LF Band
    juce::Slider lfGainSlider;
    juce::Slider lfFreqSlider;
    juce::ToggleButton lfBellButton;
    juce::Label lfLabel;

    // LM Band
    juce::Slider lmGainSlider;
    juce::Slider lmFreqSlider;
    juce::Slider lmQSlider;
    juce::Label lmLabel;

    // HM Band
    juce::Slider hmGainSlider;
    juce::Slider hmFreqSlider;
    juce::Slider hmQSlider;
    juce::Label hmLabel;

    // HF Band
    juce::Slider hfGainSlider;
    juce::Slider hfFreqSlider;
    juce::ToggleButton hfBellButton;
    juce::Label hfLabel;

    // Global Controls
    juce::ComboBox eqTypeSelector;
    juce::ToggleButton bypassButton;
    juce::Slider outputGainSlider;
    juce::Slider saturationSlider;
    juce::ComboBox oversamplingSelector;

    // Parameter references for UI updates
    std::atomic<float>* eqTypeParam;
    std::atomic<float>* bypassParam;

    // Label storage
    std::vector<std::unique_ptr<juce::Label>> knobLabels;

    // Attachment classes for parameter binding
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    // Attachments
    std::unique_ptr<SliderAttachment> hpfFreqAttachment;
    std::unique_ptr<SliderAttachment> lpfFreqAttachment;

    std::unique_ptr<SliderAttachment> lfGainAttachment;
    std::unique_ptr<SliderAttachment> lfFreqAttachment;
    std::unique_ptr<ButtonAttachment> lfBellAttachment;

    std::unique_ptr<SliderAttachment> lmGainAttachment;
    std::unique_ptr<SliderAttachment> lmFreqAttachment;
    std::unique_ptr<SliderAttachment> lmQAttachment;

    std::unique_ptr<SliderAttachment> hmGainAttachment;
    std::unique_ptr<SliderAttachment> hmFreqAttachment;
    std::unique_ptr<SliderAttachment> hmQAttachment;

    std::unique_ptr<SliderAttachment> hfGainAttachment;
    std::unique_ptr<SliderAttachment> hfFreqAttachment;
    std::unique_ptr<ButtonAttachment> hfBellAttachment;

    std::unique_ptr<ComboBoxAttachment> eqTypeAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> saturationAttachment;
    std::unique_ptr<ComboBoxAttachment> oversamplingAttachment;

    // Helper methods
    void setupKnob(juce::Slider& slider, const juce::String& paramID,
                   const juce::String& label, bool centerDetented = false);
    void setupSSLButton(juce::ToggleButton& button, const juce::String& text);
    void drawKnobMarkings(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SSL4KEQEditor)
};