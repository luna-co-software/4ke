/* Direct test of ssl4keq_extension_data
 * Compile: gcc -o test_direct test_direct_call.c -L"/home/marc/.lv2/SSL 4000 EQ.lv2" -l"SSL 4000 EQ" -Wl,-rpath="/home/marc/.lv2/SSL 4000 EQ.lv2"
 */

#include <stdio.h>

extern const void* ssl4keq_extension_data(const char* uri);

int main() {
    const char* uri = "http://lv2plug.in/ns/ext/inline-display#interface";
    printf("Testing with URI: %s\n", uri);

    const void* result = ssl4keq_extension_data(uri);
    printf("Result: %p\n", result);

    if (result) {
        printf("SUCCESS: Interface found!\n");
    } else {
        printf("FAILED: Interface not found\n");
    }

    return 0;
}