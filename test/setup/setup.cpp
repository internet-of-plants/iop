#include <unity.h>

void setup();
void loop();

int main(int argc, char** argv) {
    UNITY_BEGIN();
    setup();
    loop();
    UNITY_END();
    return 0;
}