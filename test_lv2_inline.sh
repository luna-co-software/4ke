#!/bin/bash

# Test script to check if the LV2 plugin exports inline display extension

echo "Testing SSL 4000 EQ Pure LV2 plugin inline display..."
echo

PLUGIN_PATH="$HOME/.lv2/SSL_4000_EQ_Pure.lv2/ssl4keq.so"

if [ ! -f "$PLUGIN_PATH" ]; then
    echo "ERROR: Plugin not found at $PLUGIN_PATH"
    exit 1
fi

echo "1. Checking plugin exports..."
nm -D "$PLUGIN_PATH" | grep -E "(lv2_descriptor|extension_data)"
echo

echo "2. Checking TTL metadata for inline display..."
grep -i "inline" ~/.lv2/SSL_4000_EQ_Pure.lv2/*.ttl || echo "No inline display metadata found in TTL files"

echo
echo "Test complete. The pure LV2 plugin should now show inline display in Ardour."
