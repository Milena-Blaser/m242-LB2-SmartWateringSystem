#include <Arduino.h>
#include <M5Core2.h>
#include "view.h"


#define INPUT_PIN 36
#define PUMP_PIN  26

lv_obj_t * left_button;
lv_obj_t * right_button;
lv_obj_t * message_box;
lv_obj_t * humidity_label;

bool flag = true;
int rawADC;

void event_handler_button(struct _lv_obj_t * obj, lv_event_t event) {
  if(event == LV_EVENT_PRESSED) {
    Serial.println("Button pressed");
    digitalWrite(PUMP_PIN, flag);
    flag = !flag;
  }
    message_box = show_message_box_no_buttons("Hello world");
    close_message_box(message_box);
}


void init_gui_elements() {
  add_label("HUMIDITY", 0, 10);
  humidity_label = add_label("Temperature", 0, 40);
  add_label("Water in ml", 0, 100);
  add_slider(1, 100, 20, 170);
  left_button = add_button("STOP", event_handler_button, 30, -20);
  right_button = add_button("PUMP", event_handler_button, 170, -20);

}

unsigned long next_lv_task = 0;
unsigned long last_read = 0;

void loop() {
    // rawADC = analogRead(INPUT_PIN);
    // M5.lcd.fillRect(80, 100, 240, 50, BLACK);
    // M5.Lcd.setCursor(80, 100);
    // M5.Lcd.print("ADC: " + String(rawADC));
    // Serial.print("Watering ADC value: ");
    // Serial.println(rawADC);
    // if (M5.BtnB.wasPressed()) {
    //     digitalWrite(PUMP_PIN, flag);
    //     flag = !flag;
    // }
    // M5.update();
    // delay(100);
    //read values all 10 seconds

  if(last_read < millis()) {
    last_read = millis() + 100000;
    rawADC = analogRead(INPUT_PIN);
    lv_label_set_text(humidity_label, String(rawADC).c_str());
  } 

    if(next_lv_task < millis()) {
    lv_task_handler();
    next_lv_task = millis() + -10;
  }

  //start pumping 3 ml

  //stop pumping

}

void setup() {
    init_m5();
    init_display();
    Serial.begin(115200);
    M5.begin();
    init_gui_elements();
    // pinMode(INPUT_PIN, INPUT);
    // pinMode(PUMP_PIN, OUTPUT);
    // pinMode(25, OUTPUT);
    // digitalWrite(25, 0);
}

