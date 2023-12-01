#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>

const int BUTTON_PIN = 2;
const int LED_PIN = 1;
const int MOSFET_PIN = 0;
const int NUM_LEDS = 6;

const double BPM = 117;
const double MSPB = 60.0 * 1000.0 / BPM; // milliseconds per beat

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

bool are_leds_on = false;

typedef uint32_t LED_Color;
const LED_Color OFF = pixels.Color(0, 0, 0);
const LED_Color WHITE = pixels.Color(255/2, 255/2, 255/2);
const LED_Color DIM_WHITE = pixels.Color(20, 20, 20);
const LED_Color BRIGHT_WHITE = pixels.Color(255, 255, 255);
const LED_Color BRIGHT_PURPLE = pixels.Color(255, 0, 255);
const LED_Color BLUE = pixels.Color(0, 0, 255/2);
const LED_Color BRIGHT_BLUE = pixels.Color(0, 0, 255);
const LED_Color BRIGHT_YELLOW = pixels.Color(255/2, 255/2, 0);
const LED_Color BRIGHT_RED = pixels.Color(255/2, 0, 0);
const LED_Color BRIGHT_GREEN = pixels.Color(0, 255/2, 0);

struct Instruction {
    const LED_Color& color1;
    const LED_Color& color2;
    double timing; // time before it should go to the next instruction in beats since turn on

    Instruction(const LED_Color& color, double timing)
        : color1(color), color2(color), timing(timing) {}

    Instruction(const LED_Color& color1, const LED_Color& color2, double timing)
        : color1(color1), color2(color2), timing(timing) {}
};

int instruction_num = 0;
Instruction instructions[] = {
    Instruction(WHITE, BLUE, 0),
    Instruction(BRIGHT_WHITE, BLUE, 4),
    Instruction(OFF, 5),
    Instruction(DIM_WHITE, 8),
    // 2 high stuff
    Instruction(BRIGHT_WHITE, 12),
    Instruction(BRIGHT_PURPLE, 16),
    Instruction(BRIGHT_BLUE, 20),
    Instruction(BRIGHT_WHITE, BRIGHT_BLUE, 24),
    // FTS stuff
    Instruction(BRIGHT_WHITE, BRIGHT_RED, 28),
    Instruction(BRIGHT_WHITE, BRIGHT_BLUE, 32),
    Instruction(BRIGHT_WHITE, BRIGHT_RED, 36),
    Instruction(BRIGHT_WHITE, BRIGHT_BLUE, 38),
    Instruction(BRIGHT_WHITE, BRIGHT_YELLOW, 40),
    // Fan stuff
    Instruction(BRIGHT_BLUE, 44),
    Instruction(BRIGHT_RED, 48),
    Instruction(BRIGHT_BLUE, 56),
    // Dark king carp stuff
    Instruction(BRIGHT_PURPLE, 60),
    Instruction(BRIGHT_GREEN, 62),
    Instruction(BRIGHT_WHITE, BRIGHT_BLUE, 72),
    Instruction(OFF, 69420)
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

    digitalWrite(MOSFET_PIN, HIGH); // Connect the LEDs
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

    digitalWrite(MOSFET_PIN, LOW); // Disconnect the LEDs
    digitalWrite(LED_PIN, HIGH); // Make it so then ESD protection doesn't get in the way of reducing power
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
    ADCSRA &= ~(1 << ADEN); // Disable ADC
    ACSR &= ~(1 << ACIE); // Disable analog comparator interrupt
    ACSR |= 1 << ACD; // Disable analog comparator

    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

    pinMode(MOSFET_PIN, OUTPUT);

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
    //const int STARTING_OFFSET = 20;
    if (millis() - turn_on_time >= (instructions[instruction_num].timing + STARTING_OFFSET) * MSPB) {
        instruction_num++;
    }
    
    for (int i = 0; i < NUM_LEDS; i += 2) {
        pixels.setPixelColor(i, instructions[instruction_num].color1);
    }
    for (int i = 1; i < NUM_LEDS; i += 2) {
        pixels.setPixelColor(i, instructions[instruction_num].color2);
    }
    pixels.show();
    
    if (instruction_num >= sizeof(instructions) / sizeof(instructions[0]) - 1) {
        button_state = HIGH; // make it so the shut down command runs if the button is not pressed (I hate button debounce so much lmao)
        are_leds_on = false;
    }
}
