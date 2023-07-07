#include <Arduino.h>
#include <M5Core2.h>
#include "view.h"
#include "networking.h"


#define INPUT_PIN 33
#define PUMP_PIN  32

lv_obj_t * left_button;
lv_obj_t * right_button;
lv_obj_t * message_box;
lv_obj_t * humidity_label;
lv_obj_t * slider; 

unsigned long next_lv_task = 0;
unsigned long last_read = 0;

bool flag = false;
int humidity;

void pump_water(int amount){
  last_read = millis();
  amount = amount * 200;
  Serial.println(!flag);
  while(millis() - last_read < amount){
  Serial.println(flag);
  digitalWrite(PUMP_PIN, true);
  }

digitalWrite(PUMP_PIN, false);
}

void event_handler_pump(struct _lv_obj_t * obj, lv_event_t event) {
  if(event == LV_EVENT_PRESSED) {
    int test = lv_slider_get_value(slider);
    pump_water(test);
    Serial.println("Button pressed");
  }
    message_box = show_message_box_no_buttons("Hello world");
    close_message_box(message_box);
}


void init_gui_elements() {
  add_label("HUMIDITY", 0, 10);
  //convert humidity to string
  humidity_label = add_label(String(humidity).c_str(), 0, 40);
  add_label("Water in ml", 0, 100);
  slider = add_slider(1, 350, 20, 170);
  left_button = add_button("PUMP", event_handler_pump, 30, -20);
  //right_button = add_button("STOP", event_handler_button, 170, -20);

}


// ----------------------------------------------------------------------------
// MQTT callback
// ----------------------------------------------------------------------------

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // Parse Payload into String
  char * buf = (char *)malloc((sizeof(char)*(length+1)));
  memcpy(buf, payload, length);
  buf[length] = '\0';
  String payloadS = String(buf);
  payloadS.trim();

  //write code when MQTT message is received
}

void loop() {
    // rawADC = analogRead(INPUT_PIN);
    // M5.lcd.fillRect(80, 100, 240, 50, BLACK);
    // M5.Lcd.setCursor(80, 100);
    // M5.Lcd.print("ADC: " + String(rawADC));
    // Serial.print("Watering ADC value: ");
    // Serial.println(rawADC);
    //if (M5.BtnB.wasPressed()) {
       // digitalWrite(PUMP_PIN, flag);
         //flag = !flag;
    //}
    // M5.update();
    // delay(100);
    //read values all 10 seconds

    //send values all 60 seconds
  if(last_read < millis()) {
    last_read = millis() + 60000;
    Serial.println("Sending data to MQTT broker");
    humidity = analogRead(INPUT_PIN);
    Serial.println(humidity);
    lv_label_set_text(humidity_label, String(humidity).c_str());
    mqtt_publish("SmartWateringSystem/Janna/Humidity", String(humidity).c_str());
  }
    
  if(next_lv_task < millis()) {
    lv_task_handler();
    next_lv_task = millis() + -10;
  }
  mqtt_loop();

}

void setup() {
    init_m5();
    init_display();
    Serial.begin(115200);
    
    lv_obj_t * wifiConnectingBox = show_message_box_no_buttons("Connecting to WiFi...");
    lv_task_handler();
    delay(5);
    setup_wifi();
    mqtt_init(mqtt_callback);
    close_message_box(wifiConnectingBox);

    init_gui_elements();
    digitalWrite(PUMP_PIN, flag);
    pinMode(INPUT_PIN, INPUT);
    pinMode(PUMP_PIN, OUTPUT);
}

