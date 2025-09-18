cmake_minimum_required(VERSION 3.15)
project(SSL4KEQ VERSION 1.0.0)

# Add JUCE
add_subdirectory(/home/marc/Projects/JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# Define the plugin - Build BOTH LV2 and VST3 from same source
juce_add_plugin(SSL4KEQ
    COMPANY_NAME "SSL Emulation"
    COMPANY_WEBSITE "https://example.com"
    COMPANY_EMAIL "info@example.com"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE SSLm
    PLUGIN_CODE SSL4
    FORMATS LV2 VST3 Standalone
    PRODUCT_NAME "SSL 4000 EQ"
    LV2_URI "https://example.com/plugins/SSL_4000_EQ"
    VST3_CATEGORIES "Fx" "EQ"
)

# Generate JuceHeader.h
juce_generate_juce_header(SSL4KEQ)

# Add sources
target_sources(SSL4KEQ
    PRIVATE
        ../SSL4KEQ.cpp
        ../PluginEditor.cpp
        ../SSLLookAndFeel.cpp
)

# Include directories for headers
target_include_directories(SSL4KEQ
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Compile definitions
target_compile_definitions(SSL4KEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        # Enable LV2 features
        JucePlugin_Build_LV2=1
        JucePlugin_LV2URI="https://example.com/plugins/SSL_4000_EQ"
)

# Link libraries
target_link_libraries(SSL4KEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_gui_extra
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Set C++ standard
target_compile_features(SSL4KEQ PRIVATE cxx_std_17)
