#include "sideled.h"

CRGB leds[SIDELED_NUM_LEDS];

uint8_t led_state = 0;


void LEDtask(void *arg){
  while (1){
    if(led_state == SIDELED_STATE_ALARM) {
       fill_solid(leds, SIDELED_NUM_LEDS, CRGB::Red);
      FastLED.show();
      delay(500);
      fill_solid(leds, SIDELED_NUM_LEDS, CRGB::White);
      FastLED.show();
      delay(500);
    }
    if(led_state == SIDELED_STATE_ACTIVE) {
      fill_solid(leds, SIDELED_NUM_LEDS, CRGB::Green);
      FastLED.show();
    }
    else if(led_state == SIDELED_STATE_OFF) {
      fill_solid(leds, SIDELED_NUM_LEDS, CRGB::Black);
      FastLED.show();
    }
  }
}

void init_sideled() {
    FastLED.addLeds<NEOPIXEL, SIDELED_DATA_PIN>(leds, SIDELED_NUM_LEDS);
    xTaskCreatePinnedToCore(LEDtask, "LEDTask", 4096, NULL, 2, NULL, 0);
}

void set_sideled_state(int state) {
    led_state = state;
}