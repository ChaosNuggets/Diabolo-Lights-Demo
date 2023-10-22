#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>

const int BUTTON_PIN = 2;
const int LED_PIN = 0;
const int NUM_LEDS = 6;

const double BPM = 117;
const double MSPB = 60.0 * 1000.0 / BPM; // milliseconds per beat

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

bool are_leds_on = false;

struct LED_Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    uint32_t to_uint32() {
        return pixels.Color(r, g, b);
    }

    LED_Color operator*(float a) {
        return {r * a, g * a, b * a};
    }

    LED_Color operator+(LED_Color const& obj) {
        return {r + obj.r, g + obj.g, b + obj.b};
    }
};

const LED_Color WHITE_BLUE = {128/2, 128/2, 255/2};
const LED_Color BRIGHT_WHITE_BLUE = {128, 128, 255};
const LED_Color OFF = {0, 0, 0};
const LED_Color DIM_WHITE = {20, 20, 20};
const LED_Color BRIGHT_WHITE = {255, 255, 255};
const LED_Color BRIGHT_PURPLE = {255, 0, 255};
const LED_Color BRIGHT_BLUE = {0, 0, 255};
const LED_Color WHITE_RED = {255/2, 128/2, 128/2};
const LED_Color YELLOW = {255/2, 255/2, 0};
const LED_Color BLUE = {0, 0, 255/2};
const LED_Color RED = {255/2, 0, 0};
const LED_Color PURPLE = {255/2, 0, 255/2};
const LED_Color GREEN = {0, 255/2, 0};

struct Keyframe {
    LED_Color color;
    float time; // time in beats since turn on + OFFSET
};

int keyframe_idx = 0;
const Keyframe keyframes[] = {
    {WHITE_BLUE, 0},
    {BRIGHT_WHITE_BLUE, 4},
    {OFF, 4},
    {OFF, 5},
    {DIM_WHITE, 8},
    // 2 high stuff
    {BRIGHT_WHITE, 8},
    {BRIGHT_WHITE, 11.9},
    {BRIGHT_PURPLE, 12.1},
    {BRIGHT_PURPLE, 15.9},
    {BRIGHT_BLUE, 16.1},
    {BRIGHT_BLUE, 19.9},
    {BRIGHT_WHITE_BLUE, 20.1},
    {BRIGHT_WHITE_BLUE, 23.9},
    // FTS stuff
    {WHITE_RED, 24.1},
    {WHITE_RED, 27.9},
    {WHITE_BLUE, 28.1},
    {WHITE_BLUE, 31.9},
    {WHITE_RED, 32.1},
    {WHITE_RED, 35.9},
    {WHITE_BLUE, 36.1},
    {WHITE_BLUE, 37.9},
    {YELLOW, 38.1},
    {YELLOW, 40},
    // Fan stuff
    {BLUE, 40},
    {BLUE, 44},
    {RED, 44},
    {RED, 48},
    {BLUE, 48},
    {BLUE, 56},
    // Dark king carp stuff
    {PURPLE, 56},
    {PURPLE, 57.9},
    {GREEN, 58.1},
    {GREEN, 59.9},
    {WHITE_BLUE, 60.1},
    {WHITE_BLUE, 72},
    {OFF, 72},
};

unsigned long last_debounce_time = 0;
const int DEBOUNCE_DELAY = 50; //ms
int debounce_button_state;
int button_state = HIGH; // Set it to high so then the second if block inside handle_button runs on startup if the button is low
unsigned long turn_on_time = 0;

ISR(PCINT0_vect) { // This should only run when it is woken up from sleep mode
    //for(int i = 0; i < NUM_LEDS; i++) {
    //    pixels.setPixelColor(i, pixels.Color(0, 50, 0));
    //    pixels.show();
    //}
    //delay(500);
    //pixels.clear();
    //pixels.show();

    turn_on_time = millis();
    last_debounce_time = millis();
    are_leds_on = true;
    button_state = HIGH;
    keyframe_idx = 0; // start back at the beginning of the keyframes array

    cli(); // disable interrupts
    PCMSK &= ~(1 << PCINT2); // turns of PCINT2 as interrupt pin
    sleep_disable(); // clear sleep enable bit
}

static void shut_down() {
    //for(int i = 0; i < NUM_LEDS; i++) {
    //    pixels.setPixelColor(i, pixels.Color(50, 0, 0));
    //    pixels.show();
    //}
    //delay(500);
    //pixels.clear();
    //pixels.show();

    GIMSK |= 1 << PCIE; // enable pin change interrupt
    PCMSK |= 1 << PCINT2; // turns on PCINT2 as interrupt pin
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable(); //set sleep enable bit
    sei(); // enable interrupts
    sleep_mode(); // put the microcontroller to sleep
}

static void handle_button() {
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
}

LED_Color lerp(LED_Color a, LED_Color b, float f) {
    return a * (1 - f) + (b * f);
}

float inverse_lerp(float a, float b, float f) {
    return (f - a) / (b - a);
}

void setup() {
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

    pinMode(BUTTON_PIN, INPUT);
    debounce_button_state = digitalRead(BUTTON_PIN);
}

void loop() {
    handle_button();

    if (!are_leds_on) {
        pixels.clear();
        pixels.show();
        return;
    }

    const int OFFSET = 28; // 28 beats
    if (millis() - turn_on_time > (keyframes[keyframe_idx].time + OFFSET) * MSPB) {
        keyframe_idx++;
    }
    
    const Keyframe& left = keyframe_idx - 1 >= 0 ? keyframes[keyframe_idx - 1] : keyframes[keyframe_idx];
    const Keyframe& right = keyframe_idx < sizeof(keyframes) / sizeof(keyframes[0]) ? keyframes[keyframe_idx] : keyframes[keyframe_idx - 1];
    //pixels.setPixelColor(0, left.color == keyframes[0].color ? pixels.Color(100, 0, 0) : pixels.Color(0, 100, 0));
    //pixels.setPixelColor(1, right.color == keyframes[0].color ? pixels.Color(100, 0, 0) : pixels.Color(0, 100, 0));
    //pixels.show();
    //delay(200);
    LED_Color color = lerp(left.color, right.color, inverse_lerp((right.time + OFFSET) * MSPB, (left.time + OFFSET) * MSPB, millis() - turn_on_time));
    for (int i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, color.to_uint32());
        pixels.show();
    }
    
    if (keyframe_idx >= sizeof(keyframes) / sizeof(keyframes[0]) - 1) {
        shut_down();
    }
}
