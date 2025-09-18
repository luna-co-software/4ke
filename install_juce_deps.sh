#!/bin/bash
# Install dependencies for building JUCE LV2 plugins on Ubuntu

echo "Installing JUCE LV2 build dependencies..."
echo "This will require sudo access."

# Update package list
sudo apt update

# Install essential build tools
echo "Installing build essentials..."
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    git

# Install audio development libraries
echo "Installing audio libraries..."
sudo apt install -y \
    libasound2-dev \
    libjack-jackd2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxcomposite-dev \
    mesa-common-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev

# Install GTK and webkit for JUCE GUI
echo "Installing GTK and WebKit..."
# Try both webkit package names for compatibility
sudo apt install -y libgtk-3-dev
sudo apt install -y libwebkit2gtk-4.1-dev 2>/dev/null || sudo apt install -y libwebkit2gtk-4.0-dev

# Install curl development files
echo "Installing curl development files..."
sudo apt install -y \
    libcurl4-openssl-dev

# Install additional dependencies
echo "Installing additional dependencies..."
sudo apt install -y \
    libfontconfig1-dev \
    libssl-dev

echo "All dependencies installed!"
echo ""
echo "You can now build the JUCE LV2 plugin."