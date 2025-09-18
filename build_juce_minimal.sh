#!/bin/bash
# Minimal JUCE LV2 build - tries to build without webkit dependency

set -e

echo "Building SSL 4000 EQ JUCE LV2 plugin (minimal version)..."

# Clean previous attempts
rm -rf build_juce
mkdir -p build_juce
cd build_juce

# Create a minimal CMakeLists.txt that doesn't require webkit
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
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE SSLm
    PLUGIN_CODE SSL4
    FORMATS LV2
    PRODUCT_NAME "SSL 4000 EQ JUCE"
    LV2_URI "https://example.com/plugins/ssl4keq_juce"
)

# Add sources
target_sources(SSL4KEQ
    PRIVATE
        ../SSL4KEQ.cpp
        ../PluginEditor.cpp
        ../SSLLookAndFeel.cpp
)

# Compile definitions - disable features that need webkit/curl
target_compile_definitions(SSL4KEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
        JUCE_PLUGINHOST_LV2=1
)

# Link libraries
target_link_libraries(SSL4KEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_events
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# Set C++ standard
target_compile_features(SSL4KEQ PRIVATE cxx_std_17)

# Set output directory
set_target_properties(SSL4KEQ_LV2 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/SSL4KEQ_JUCE.lv2"
)
EOF

# Configure with CMake (disable curl requirement)
echo "Configuring with CMake..."
cmake . -DCMAKE_BUILD_TYPE=Release \
        -DJUCE_BUILD_EXTRAS=OFF \
        -DJUCE_BUILD_EXAMPLES=OFF \
        -DJUCE_ENABLE_MODULE_SOURCE_GROUPS=OFF

# Build
echo "Building..."
make -j$(nproc) 2>&1 | tee build.log

# Check if LV2 was built
if [ -d "SSL4KEQ_artefacts/LV2" ]; then
    echo "Installing LV2 plugin..."
    LV2_DIR="$HOME/.lv2/SSL4KEQ_JUCE.lv2"
    rm -rf "$LV2_DIR"
    cp -r "SSL4KEQ_artefacts/LV2/SSL4KEQ.lv2" "$LV2_DIR"
    echo "Done! JUCE LV2 plugin installed to $LV2_DIR"
else
    echo "Build may have succeeded but LV2 output not found."
    echo "Check build.log for details."
    find . -name "*.lv2" -type d 2>/dev/null
fi