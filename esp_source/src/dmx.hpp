#include <Arduino.h>
#include <esp_dmx.h>
#include <Adafruit_NeoPixel.h>


// -------------- dmx data indicator led --------------
// onboard neopixel led is connected to pin 21
#define DMX_DATA_LED_PIN 21

// create instance of adafruit neopixel
Adafruit_NeoPixel dmx_data_led(1, DMX_DATA_LED_PIN, NEO_RGB + NEO_KHZ800);


// -------------- dmx init --------------
// The ESP serial port to be used by DMX Library
dmx_port_t dmx_serial_port = 1;
#define DMX_TX_PIN 17
#define DMX_RX_PIN 16
#define DMX_ENABLE_PIN 21

void dmx_init() {
    // initialize and turn off dmx data indicator led
    pinMode(DMX_DATA_LED_PIN, OUTPUT);
    digitalWrite(DMX_DATA_LED_PIN, LOW);

    // configure DMX
    dmx_config_t dmx_config = DMX_CONFIG_DEFAULT;
    dmx_personality_t dmx_personalities[] = {
        {1, "Default Personality"}
    };
    int dmx_personality_count = 1;
    dmx_driver_install(dmx_serial_port, &dmx_config, dmx_personalities, dmx_personality_count);

    // set the DMX hardware pins to the pins that we want to use
    dmx_set_pin(dmx_serial_port, DMX_TX_PIN, DMX_RX_PIN, DMX_ENABLE_PIN);
}


// -------------- dmx data indicator led --------------
volatile bool dmx_data_present = false;
volatile unsigned long dmx_data_led_last_toggle = 0;
volatile bool dmx_data_led_state = false;

// initialize the onboard neopixel as dmx data indicator led
void dmx_data_indicator_led_init() {
    // start neopixel
    dmx_data_led.begin();

    // turn dmx data indicator led green (power on - no dmx data)
    dmx_data_led.setPixelColor(0, dmx_data_led.Color(0, 10, 0));
    dmx_data_led.show();
}

// dmx_data_present will be set by core 0
// this function will be called by core 1 frquently and handles dmx data indicator led
void handle_indicator_led() {
    // turn dmx data indicator led green (power on - no dmx data)
    if(!dmx_data_present) {
        dmx_data_led.setPixelColor(0, dmx_data_led.Color(0, 10, 0));
        dmx_data_led.show();
    }
    
    // turn dmx data indicator led red (power on - dmx data)
    if(!dmx_data_led_state && dmx_data_present && millis() - dmx_data_led_last_toggle >= 500) {
        dmx_data_led_state = true;
        
        dmx_data_led.setPixelColor(0, dmx_data_led.Color(10, 0, 0));
        dmx_data_led.show();

        dmx_data_led_last_toggle = millis();
    }

    // turn dmx data indicator led off (power on - dmx data)
    if(dmx_data_led_state && millis() - dmx_data_led_last_toggle >= 50) {
        dmx_data_led_state = false;
        
        dmx_data_led.setPixelColor(0, dmx_data_led.Color(0, 0, 0));
        dmx_data_led.show();

        dmx_data_led_last_toggle = millis();
    }
}