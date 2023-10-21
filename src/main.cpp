#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

const int BUTTON_PIN = 2;
const int LED_PIN = 0;
const int NUM_LEDS = 6;

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

bool are_leds_on = true;
enum LED_State {
    SETUP,
    GETTING_BRIGHTER,
    OFF,
    DIM,

};

unsigned long last_debounce_time = 0;
const int DEBOUNCE_DELAY = 50; //ms
int debounce_button_state;
int button_state;
unsigned long turn_on_time = 0;

ISR(PCINT0_vect) { // This should only run when it is woken up from sleep mode
    turn_on_time = millis();
    last_debounce_time = millis();
    are_leds_on = true;
    button_state = HIGH;

    cli(); // disable interrupts
    PCMSK &= ~(1 << PCINT2); // turns of PCINT2 as interrupt pin
    sleep_disable(); // clear sleep enable bit
}

void shut_down() {
    GIMSK |= 1 << PCIE; // enable pin change interrupt
    PCMSK |= 1 << PCINT2; // turns on PCINT2 as interrupt pin
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable(); //set sleep enable bit
    sei(); // enable interrupts
    sleep_mode(); // put the microcontroller to sleep
}

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

    if ((millis() - last_debounce_time) > DEBOUNCE_DELAY && reading != button_state) {
        button_state = reading;

        if (are_leds_on && button_state == HIGH) {
            are_leds_on = false;
        } 

        if (!are_leds_on && button_state == LOW) {
            shut_down();
        }
    }

    uint32_t color = are_leds_on ? pixels.Color(50, 50, 50) : pixels.Color(0, 0, 0);
    
    for(int i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, color);
        pixels.show();
    }
}
