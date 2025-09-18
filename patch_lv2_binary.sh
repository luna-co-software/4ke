#!/bin/bash

# Patch the LV2 binary to intercept lv2_descriptor
LV2_SO="/home/marc/.lv2/SSL 4000 EQ.lv2/libSSL 4000 EQ.so"
WRAPPER_OBJ="build/CMakeFiles/SSL4KEQ.dir/lv2_descriptor_wrapper.c.o"

# Backup original
cp "$LV2_SO" "$LV2_SO.backup"

# Extract symbols to rename
echo "Renaming JUCE's lv2_descriptor to __real_lv2_descriptor..."
objcopy --redefine-sym=lv2_descriptor=__real_lv2_descriptor "$LV2_SO"

# Compile wrapper with correct symbol names
echo "Compiling wrapper..."
gcc -c -fPIC -o lv2_wrapper.o /home/marc/projects/SSL4000EQ/lv2_descriptor_wrapper.c

# Link wrapper into the library
echo "Linking wrapper..."
gcc -shared -o "$LV2_SO.new" \
    -Wl,--whole-archive "$LV2_SO" -Wl,--no-whole-archive \
    lv2_wrapper.o

# Replace original
mv "$LV2_SO.new" "$LV2_SO"

echo "Done! Testing..."
nm -D "$LV2_SO" | grep -E "lv2_descriptor|__real|__wrap"