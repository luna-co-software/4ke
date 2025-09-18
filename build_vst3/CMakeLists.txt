cmake_minimum_required(VERSION 3.15)
project(SSL4KEQ_VST3 VERSION 1.0.0)

# Add JUCE
add_subdirectory(/home/marc/Projects/JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# Define the plugin - VST3 ONLY
juce_add_plugin(SSL4KEQ
    COMPANY_NAME "SSLEmulation"
    BUNDLE_ID "com.sslemulation.ssl4keq"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE SSLm
    PLUGIN_CODE SSL4
    FORMATS VST3
    PRODUCT_NAME "SSL 4000 EQ"
    VST3_CATEGORIES "Fx" "EQ"
)

# Generate JuceHeader.h FIRST
juce_generate_juce_header(SSL4KEQ)

# Add the generated JuceHeader location to include directories
target_include_directories(SSL4KEQ
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/SSL4KEQ_artefacts/JuceLibraryCode
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Add sources AFTER setting up includes
target_sources(SSL4KEQ
    PRIVATE
        ../SSL4KEQ.cpp
        ../PluginEditor.cpp
        ../SSLLookAndFeel.cpp
        ../LV2InlineDisplay_stub.cpp
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
        JucePlugin_Build_VST3=1
        JucePlugin_Build_LV2=0
        JucePlugin_Build_AU=0
        JucePlugin_Build_Standalone=0
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
        juce::juce_recommended_warning_flags
)

# Set C++ standard
target_compile_features(SSL4KEQ PRIVATE cxx_std_17)

# Set optimization for release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(SSL4KEQ PRIVATE -O3)
endif()

# Ensure VST3 symbols are properly exported
target_compile_options(SSL4KEQ_VST3 PRIVATE
    -fvisibility=hidden
)

target_compile_definitions(SSL4KEQ_VST3 PRIVATE
    JUCE_DLL_BUILD=1
)
