cmake_minimum_required(VERSION 3.15)
project(SSL4KEQ VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE subdirectory
add_subdirectory(/home/marc/projects/JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# Define our VST3 and LV2 plugin target
juce_add_plugin(SSL4KEQ
    PLUGIN_NAME "SSL 4000 EQ"
    PLUGIN_CODE SSL4
    FORMATS VST3 LV2 AU Standalone
    PRODUCT_NAME "SSL 4000 EQ"
    COMPANY_NAME "SSL Emulation"
    COMPANY_WEBSITE "https://example.com"
    COMPANY_EMAIL "info@example.com"
    PLUGIN_MANUFACTURER_CODE SSLm
    PLUGIN_CODE SSL4
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Fx EQ
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST2_CATEGORY kPlugCategEffect
    LV2_URI https://example.com/plugins/ssl4keq
    LV2_SHARED_LIBRARY_NAME SSL4KEQ
)

# Generate JUCE header
juce_generate_juce_header(SSL4KEQ)

# Add source files
target_sources(SSL4KEQ
    PRIVATE
        SSL4KEQ.cpp
        SSL4KEQ.h
        PluginEditor.cpp
        PluginEditor.h
        SSLLookAndFeel.cpp
        SSLLookAndFeel.h
        LV2InlineDisplay.h
        LV2InlineDisplay.cpp
        lv2_descriptor_wrapper.c
)

# Set include directories
target_include_directories(SSL4KEQ
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
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
)

# Link with JUCE modules
target_link_libraries(SSL4KEQ
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Platform-specific settings
if(MSVC)
    target_compile_options(SSL4KEQ PRIVATE /W4 /WX-)
else()
    target_compile_options(SSL4KEQ PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set binary output directory
set_target_properties(SSL4KEQ PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
# Add linker wrapping for LV2 inline display support
if(TARGET SSL4KEQ_LV2)
    target_link_options(SSL4KEQ_LV2 PRIVATE
        -Wl,--wrap=lv2_descriptor
    )
endif()
