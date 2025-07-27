#include <Arduino.h>
#include <esp_dmx.h>
#include <Adafruit_NeoPixel.h>

#include "dmx.hpp"

// DMX start address (first address)
#define DMX_START_ADDRESS 94

#define NEOPIXEL_DATA_PIN 10
#define NEOPIXEL_LED_NUM 160 // 3 meters * 60 leds/metre - 20 (abgeschnitten) = 
unsigned long last_led_update = 0;

TaskHandle_t Core_0_task_handler;
TaskHandle_t Core_1_task_handler;

// DMX data buffer
byte dmx_data_buf[DMX_PACKET_SIZE];


// saved the time of dmx error
unsigned long last_dmx_error = 0;

// dmx data
volatile uint8_t red = 0;
volatile uint8_t green = 0;
volatile uint8_t blue = 0;
volatile uint8_t white = 0;
volatile uint8_t master_dim = 0;
volatile uint8_t shutter = 0;
volatile uint8_t animation_freq = 0;
volatile uint8_t animation_speed = 0;

// shutter values
volatile bool shutter_active = false;
unsigned long shutter_timer = 0;
volatile unsigned long shutter_interval = 0;
volatile bool shutter_closed = false;

// wave animation values
volatile bool animation_active = false;
unsigned long animation_timer = 0;
unsigned long animation_interval = 0;
volatile unsigned long animation_x_pos = 0;

// This runs on core 0 and handles DMX
void core_0_task(void *parameter) {
    // configure dmx library
    dmx_init();

    // loop
    while(true) {
        // place for dmx data packet
        dmx_packet_t dmx_packet;

        // wait dmx timeout for data
        if (dmx_receive(dmx_serial_port, &dmx_packet, DMX_TIMEOUT_TICK)) {
            // dmx data received

            // check for dmx errors
            if (!dmx_packet.err) {
                // read data into buffer
                dmx_read(dmx_serial_port, dmx_data_buf, DMX_PACKET_SIZE);

                // if data stabelized after error
                if (millis() - last_dmx_error > 1000) {
                    // turn off dmx data indicator led blink
                    dmx_data_present = true;

                    // dmx_data_buf[0] contains the dmx start code
                    red = dmx_data_buf[DMX_START_ADDRESS + 0];
                    green = dmx_data_buf[DMX_START_ADDRESS + 1];
                    blue = dmx_data_buf[DMX_START_ADDRESS + 2];
                    white = dmx_data_buf[DMX_START_ADDRESS + 3];
                    master_dim = dmx_data_buf[DMX_START_ADDRESS + 4];
                    shutter = dmx_data_buf[DMX_START_ADDRESS + 5];
                    animation_freq = dmx_data_buf[DMX_START_ADDRESS + 6];
                    animation_speed = dmx_data_buf[DMX_START_ADDRESS + 7];

                    // setup shutter according to given dmx input
                    if(shutter == 0) {
                        // turn off shutter
                        shutter_active = false;

                        // open shutter
                        shutter_closed = false;
                    }
                    else {
                        // turn on shutter
                        shutter_active = true;

                        // calculate shutter speed
                        shutter_interval = 1.0 / shutter * 8000;
                    }

                    // setup wave animation sccording to given dmx input
                    if(animation_freq == 0) {
                        // turn off wave animation
                        animation_active = false;

                        // reset wave animation
                        animation_x_pos = 0;
                    }
                    else {
                        // turn on wave animation
                        animation_active = true;
                    }
                    
                    if(animation_speed == 0) {
                        // reset wave animation
                        animation_x_pos = 0;
                    }

                    // calculate wave animation's speed
                    animation_interval = 1.0 / animation_speed * 8000;

                    //Serial.printf("r: 0x%02X g: 0x%02X b: 0x%02X w: 0x%02X\n", red, green, blue, white);
                }
            }
            else {// e.g. no dmx data
                // turn off dmx data indicator led blink
                dmx_data_present = false;

                // save time of this error
                last_dmx_error = millis();
            }
        }
        else { // DMX timeout e.g. no dmx data
            // turn off dmx data indicator led blink
            dmx_data_present = false;

            // save time of this error
            last_dmx_error = millis();
        }
    }
}

// this runs on core 1 and handles Neopixels
void core_1_task(void *parameter) {
    // create instance of adafruit neopixel
    Adafruit_NeoPixel neopixel(NEOPIXEL_LED_NUM, NEOPIXEL_DATA_PIN, NEO_GRBW + NEO_KHZ800);

    // start neopixel
    neopixel.begin();

    dmx_data_indicator_led_init();

    // loop
    while(true) {
        // -------------- dmx data indicator led --------------
        handle_indicator_led();


        // -------------- show neopixel --------------
        if(millis() - last_led_update >= 1) { // wait until neopixels have latched last frame
            for(int i = 0; i < NEOPIXEL_LED_NUM; i++) {
                // copy colors to be able to edit them
                uint8_t out_red = red;
                uint8_t out_green = green;
                uint8_t out_blue = blue;
                uint8_t out_white = white;

                // add master dimmer to colors
                out_red *= master_dim / 255.0;
                out_green *= master_dim / 255.0;
                out_blue *= master_dim / 255.0;
                out_white *= master_dim / 255.0;

                // add shutterto colors
                if(shutter_closed) {
                    out_red = 0;
                    out_green = 0;
                    out_blue = 0;
                    out_white = 0;
                }

                // add animation to colors
                if(animation_active) {
                    double x = PI * animation_freq / 2000.0 * ((unsigned long)i - animation_x_pos);
                    double scaler = 0.8 * sin(x) + 0.3;
                    if(scaler > 1) {
                        scaler = 1;
                    }
                    if(scaler < 0) {
                        scaler = 0;
                    }
                    out_red = round(out_red * scaler);
                    out_green = round(out_green * scaler);
                    out_blue = round(out_blue * scaler);
                    out_white = round(out_white * scaler);
                }

                neopixel.setPixelColor(i, neopixel.Color(out_red, out_green, out_blue, out_white));
            }
            neopixel.show();

            last_led_update = millis();
        }


        // -------------- shutter --------------
        // shutter on and time to toggle
        if(shutter_active && millis() - shutter_timer >= shutter_interval) {
            // toggle shutter
            shutter_closed = !shutter_closed;
            shutter_timer = millis();
        }


        // -------------- wave animation --------------
        // animation on and time to move wave
        if(animation_active && animation_speed > 0 && millis() - animation_timer >= animation_interval) {
            // move animation
            animation_x_pos += 1;
            animation_timer = millis();
        }
    }
}

void setup() {
    Serial.begin(115200);

    // start function core_0_task() on cpu 0
    xTaskCreatePinnedToCore(core_0_task, "capu 0", 10000, NULL, 1, &Core_0_task_handler, 0);

    // start function core_1_task() on cpu 1
    xTaskCreatePinnedToCore(core_1_task, "capu 1", 10000, NULL, 1, &Core_1_task_handler, 1);
}

void loop() {
    
}