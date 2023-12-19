// Video of this program: https://youtu.be/hJNBwNp8pKE

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Diabolo_Light.h>

using namespace Diabolo_Light;

const static double BPM = 117; // BPM of the song I used in the video
const static double MSPB = 60.0 * 1000.0 / BPM; // milliseconds per beat

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

typedef uint32_t LED_Color;
const static LED_Color OFF = pixels.Color(0, 0, 0);
const static LED_Color WHITE = pixels.Color(255/2, 255/2, 255/2);
const static LED_Color DIM_WHITE = pixels.Color(20, 20, 20);
const static LED_Color BRIGHT_WHITE = pixels.Color(255, 255, 255);
const static LED_Color BRIGHT_PURPLE = pixels.Color(255, 0, 255);
const static LED_Color BLUE = pixels.Color(0, 0, 255/2);
const static LED_Color BRIGHT_BLUE = pixels.Color(0, 0, 255);
const static LED_Color BRIGHT_YELLOW = pixels.Color(255/2, 255/2, 0);
const static LED_Color BRIGHT_RED = pixels.Color(255/2, 0, 0);
const static LED_Color BRIGHT_GREEN = pixels.Color(0, 255/2, 0);

// An Instruction consists of two colors and the time in beats at which it should move to the next instruction
struct Instruction {
    const LED_Color& color1;
    const LED_Color& color2;
    double timing; // time before it should go to the next instruction in beats since turn on

    Instruction(const LED_Color& color, double timing)
        : color1(color), color2(color), timing(timing) {}

    Instruction(const LED_Color& color1, const LED_Color& color2, double timing)
        : color1(color1), color2(color2), timing(timing) {}
};

static unsigned int instruction_num = 0; // Stores which instruction we are currently on in the instructions array
static Instruction instructions[] = { // A list of colors that the lights follow in order
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

void setup() {
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

    begin(1, 0, [](){ instruction_num = 0; }); // Set the board to have 1 on mode
}

void loop() {
    // Change current_mode and/or shut down the board if necessary
    handle_button();

    // The number of beats the board should wait before going through the instructions array
    const int STARTING_OFFSET = 28; // 28 beats
    // const int STARTING_OFFSET = 20;

    // If it's time to change to the next color in the instructions array, increment
    // instruction_num (used as an index for the instructions array)
    if (awake_time() >= (instructions[instruction_num].timing + STARTING_OFFSET) * MSPB) {
        instruction_num++;
    }
    
    // Set the pixels to the color in the instructions array
    for (unsigned int i = 0; i < NUM_LEDS; i += 2) {
        pixels.setPixelColor(i, instructions[instruction_num].color1);
    }
    for (unsigned int i = 1; i < NUM_LEDS; i += 2) {
        pixels.setPixelColor(i, instructions[instruction_num].color2);
    }
    pixels.show();
    
    // If we reach the end of the instructions array
    if (instruction_num >= sizeof(instructions) / sizeof(instructions[0]) - 1) {
        set_current_mode(0); // Set the board to off mode
    }
}
