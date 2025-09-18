#!/bin/bash

# Patch JUCE's LV2 wrapper to include inline display extension

JUCE_LV2_FILE="/home/marc/projects/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_LV2.cpp"
BACKUP_FILE="${JUCE_LV2_FILE}.backup"

# Create backup if it doesn't exist
if [ ! -f "$BACKUP_FILE" ]; then
    cp "$JUCE_LV2_FILE" "$BACKUP_FILE"
    echo "Created backup: $BACKUP_FILE"
fi

# Check if already patched
if grep -q "ssl4keq_extension_data" "$JUCE_LV2_FILE"; then
    echo "Already patched"
    exit 0
fi

# Create a temporary file with the patch
cat > /tmp/juce_lv2_patch.txt << 'EOF'
--- a/juce_audio_plugin_client_LV2.cpp
+++ b/juce_audio_plugin_client_LV2.cpp
@@ -1475,6 +1475,15 @@
         [] (const char* uri) -> const void*
         {
             const auto uriMatches = [&] (const LV2_Feature& f) { return std::strcmp (f.URI, uri) == 0; };
+
+            // Check for our custom inline display extension first
+            #ifdef JucePlugin_Build_LV2
+            extern "C" const void* ssl4keq_extension_data(const char*);
+            const void* custom_result = ssl4keq_extension_data(uri);
+            if (custom_result != nullptr) {
+                return custom_result;
+            }
+            #endif

             static RecallFeature recallFeature;

@@ -1190,6 +1199,7 @@
               "\tlv2:extensionData\n"
+              "\t\t<http://lv2plug.in/ns/ext/inline-display#interface> ,\n"
               "\t\tstate:interface ;\n"
EOF

# Apply the patch
echo "Patching JUCE LV2 wrapper..."
patch -p1 -N "$JUCE_LV2_FILE" < /tmp/juce_lv2_patch.txt

if [ $? -eq 0 ]; then
    echo "Patch applied successfully"
else
    echo "Failed to apply patch or already patched"
fi

rm /tmp/juce_lv2_patch.txt