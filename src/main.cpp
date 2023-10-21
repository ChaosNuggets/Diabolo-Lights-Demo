#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

const int BUTTON_PIN = 2;
const int LED_PIN = 0;
const int NUM_LEDS = 6;

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

enum LED_State {OFF, RED, GREEN, BLUE, COUNT};
LED_State led_state = OFF;

unsigned long last_debounce_time = 0;
const int DEBOUNCE_DELAY = 50; //ms
int debounce_button_state;
int button_state;

void setup() {
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    pixels.clear(); // Set all pixel colors to 'off'

    pinMode(BUTTON_PIN, INPUT);
    button_state = digitalRead(BUTTON_PIN);
    debounce_button_state = button_state;
}

void loop() {
    int reading = digitalRead(BUTTON_PIN);
    if (reading != debounce_button_state) {
        last_debounce_time = millis();
        debounce_button_state = reading;
    }

    if ((millis() - last_debounce_time) > DEBOUNCE_DELAY) {
        if (reading != button_state) {
            button_state = reading;

            if (button_state == HIGH) {
                led_state = static_cast<LED_State>(led_state < COUNT - 1 ? led_state + 1 : 0);
            }
        }
    }

    uint32_t color
        = led_state == OFF ? pixels.Color(0, 0, 0)
        : led_state == RED ? pixels.Color(50, 0, 0)
        : led_state == GREEN ? pixels.Color(0, 50, 0)
        : led_state == BLUE ? pixels.Color(0, 0, 50)
        : pixels.Color(50, 50, 50);
    
    for(int i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, color);
        pixels.show();
    }
}
