#ifndef DIABOLO_LIGHT_H
#define DIABOLO_LIGHT_H

#include <Adafruit_NeoPixel.h>

namespace Diabolo_Light {
    const unsigned int LED_PIN = 1;
    const unsigned int NUM_LEDS = 6;
    const neoPixelType LED_TYPE = NEO_RGB + NEO_KHZ800;

    void begin(const unsigned int num_modes, const unsigned int hold_time = 500, void (*on_wake_up)() = [](){});
    void handle_button();
    unsigned int get_current_mode();
    void set_current_mode(const unsigned int new_mode);
    unsigned long awake_time();
    int get_button_state();
}

#endif
