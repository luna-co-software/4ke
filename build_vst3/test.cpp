#include "JuceHeader.h"
#if JucePlugin_Build_LV2
#error "LV2 is ON"
#else
#error "LV2 is OFF"
#endif
