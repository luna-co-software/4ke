#!/bin/bash
# Build script for JUCE-based SSL 4000 EQ with LV2 support

set -e

echo "Building SSL 4000 EQ JUCE LV2 plugin..."

# Create build directory
mkdir -p build_juce
cd build_juce

# Create a simple CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.15)
project(SSL4KEQ VERSION 1.0.0)

# Add JUCE
add_subdirectory(/home/marc/Projects/JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# Define the plugin
juce_add_plugin(SSL4KEQ
    COMPANY_NAME "SSL Emulation"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE SSLm
    PLUGIN_CODE SSL4
    FORMATS LV2
    PRODUCT_NAME "SSL 4000 EQ"
    LV2_URI "https://example.com/plugins/ssl4keq_juce"
)

# Add sources
target_sources(SSL4KEQ
    PRIVATE
        ../SSL4KEQ.cpp
        ../PluginEditor.cpp
        ../SSLLookAndFeel.cpp
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

# Link libraries
target_link_libraries(SSL4KEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Set C++ standard
target_compile_features(SSL4KEQ PRIVATE cxx_std_17)
EOF

# Configure with CMake
echo "Configuring with CMake..."
cmake . -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
make -j$(nproc)

# Install
echo "Installing LV2 plugin..."
LV2_DIR="$HOME/.lv2/SSL4KEQ_JUCE.lv2"
mkdir -p "$LV2_DIR"

# Find and copy the built LV2 files
find . -name "*.lv2" -type d -exec cp -r {} "$LV2_DIR/" \;

echo "Done! JUCE LV2 plugin installed to $LV2_DIR"