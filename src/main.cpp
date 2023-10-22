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

typedef uint32_t LED_Color;
const LED_Color WHITE_BLUE = pixels.Color(128/2, 128/2, 255/2);
const LED_Color BRIGHT_WHITE_BLUE = pixels.Color(128, 128, 255);
const LED_Color OFF = pixels.Color(0, 0, 0);
const LED_Color DIM_WHITE = pixels.Color(20, 20, 20);
const LED_Color BRIGHT_WHITE = pixels.Color(255, 255, 255);
const LED_Color BRIGHT_PURPLE = pixels.Color(255, 0, 255);
const LED_Color BRIGHT_BLUE = pixels.Color(0, 0, 255);
const LED_Color WHITE_RED = pixels.Color(255/2, 128/2, 128/2);
const LED_Color YELLOW = pixels.Color(255/2, 255/2, 0);
const LED_Color BLUE = pixels.Color(0, 0, 255/2);
const LED_Color RED = pixels.Color(255/2, 0, 0);
const LED_Color PURPLE = pixels.Color(255/2, 0, 255/2);
const LED_Color GREEN = pixels.Color(0, 255/2, 0);

typedef struct {
    LED_Color color;
    double timing; // time before it should go to the next instruction in beats since turn on
} Instruction;

int instruction_num = 0;
Instruction instructions[] = {
    {WHITE_BLUE, 0},
    {BRIGHT_WHITE_BLUE, 4},
    {OFF, 5},
    {DIM_WHITE, 8},
    // 2 high stuff
    {BRIGHT_WHITE, 12},
    {BRIGHT_PURPLE, 16},
    {BRIGHT_BLUE, 20},
    {BRIGHT_WHITE_BLUE, 24},
    // FTS stuff
    {WHITE_RED, 28},
    {WHITE_BLUE, 32},
    {WHITE_RED, 36},
    {WHITE_BLUE, 38},
    {YELLOW, 40},
    // Fan stuff
    {BLUE, 44},
    {RED, 48},
    {BLUE, 56},
    // Dark king carp stuff
    {PURPLE, 58},
    {GREEN, 60},
    {WHITE_BLUE, 72},
    {OFF, 1800}
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
    instruction_num = 0; // start back at the beginning of the instructions array

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

    const int STARTING_OFFSET = 28; // 28 beats
    if (millis() - turn_on_time > (instructions[instruction_num].timing + STARTING_OFFSET) * MSPB) {
        instruction_num++;
    }
    
    for (int i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, instructions[instruction_num].color);
        pixels.show();
    }
    
    if (instruction_num >= sizeof(instructions) / sizeof(instructions[0]) - 1) {
        shut_down();
    }
}
