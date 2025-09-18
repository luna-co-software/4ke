#!/bin/bash
# Build script for SSL 4000 EQ - both LV2 and VST3 versions

set -e

echo "==================================="
echo "SSL 4000 EQ - Multi-Format Builder"
echo "==================================="
echo ""
echo "This will build:"
echo "1. LV2 plugin with inline display (pure C, lightweight)"
echo "2. VST3 plugin with full GUI (JUCE-based)"
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Build directory
BUILD_DIR="build_release"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# =====================================
# Build LV2 (Pure C with inline display)
# =====================================
echo -e "${YELLOW}Building LV2 plugin with inline display...${NC}"
echo "----------------------------------------"

# Use the working fixed version
make -f Makefile clean
make -f Makefile SRC=ssl4keq_fixed.c
make -f Makefile install

echo -e "${GREEN}✓ LV2 plugin built and installed to ~/.lv2/SSL_4000_EQ.lv2${NC}"
echo ""

# =====================================
# Build VST3 (JUCE with full GUI)
# =====================================
echo -e "${YELLOW}Building VST3 plugin with JUCE GUI...${NC}"
echo "--------------------------------------"

# Create build directory for JUCE
mkdir -p "$BUILD_DIR/juce"
cd "$BUILD_DIR/juce"

# Create CMakeLists for VST3 build
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.15)
project(SSL4KEQ_VST3 VERSION 1.0.0)

# Add JUCE
add_subdirectory(/home/marc/Projects/JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# Define the plugin - VST3 only
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
    FORMATS VST3 Standalone
    PRODUCT_NAME "SSL 4000 EQ"
    VST3_CATEGORIES "Fx" "EQ"
)

# Add sources
target_sources(SSL4KEQ
    PRIVATE
        ../../SSL4KEQ.cpp
        ../../PluginEditor.cpp
        ../../SSLLookAndFeel.cpp
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
)

# Link libraries
target_link_libraries(SSL4KEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Set C++ standard
target_compile_features(SSL4KEQ PRIVATE cxx_std_17)
EOF

# Configure
echo "Configuring VST3 build..."
cmake . -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" | head -20

# Build
echo "Building VST3..."
if make -j$(nproc) 2>&1 | grep -E "(Building|Linking|\[.*%\]|error)" | head -30; then
    echo -e "${GREEN}✓ VST3 build completed${NC}"
else
    echo "VST3 build may have issues - check the output"
fi

# Find and report VST3 location
cd ../..
VST3_FILE=$(find "$BUILD_DIR" -name "*.vst3" -type d 2>/dev/null | head -1)

if [ -n "$VST3_FILE" ]; then
    # Install VST3 to standard location
    VST3_INSTALL_DIR="$HOME/.vst3/SSL4KEQ.vst3"
    rm -rf "$VST3_INSTALL_DIR"
    cp -r "$VST3_FILE" "$VST3_INSTALL_DIR"
    echo -e "${GREEN}✓ VST3 plugin installed to $VST3_INSTALL_DIR${NC}"
else
    echo "VST3 file not found in expected location"
    echo "Checking for build artifacts..."
    find "$BUILD_DIR" -name "*SSL4K*" -type f -o -name "*.vst3" -type d 2>/dev/null
fi

echo ""
echo "==================================="
echo "           Build Summary"
echo "==================================="
echo -e "${GREEN}✓${NC} LV2 Plugin (C with inline display):"
echo "   Location: ~/.lv2/SSL_4000_EQ.lv2"
echo "   Features: Lightweight, inline display in mixer"
echo ""

if [ -n "$VST3_FILE" ]; then
    echo -e "${GREEN}✓${NC} VST3 Plugin (JUCE with full GUI):"
    echo "   Location: ~/.vst3/SSL4KEQ.vst3"
    echo "   Features: Full custom GUI, cross-platform"
else
    echo -e "${YELLOW}⚠${NC} VST3 Plugin: Check build output for status"
fi

echo ""
echo "Both plugins can be used simultaneously in different DAWs!"
echo ""
echo "Testing:"
echo "- LV2:  Use in Ardour, Qtractor, Carla"
echo "- VST3: Use in Reaper, Bitwig, FL Studio, etc."