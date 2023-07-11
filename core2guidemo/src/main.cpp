#include <Arduino.h>
#include <M5Core2.h>
#include "view.h"
#include "networking.h"
#include "sideled.h"
#include "soundwav.h"

#define INPUT_PIN 33
#define PUMP_PIN 32

lv_obj_t* left_button;
lv_obj_t* right_button;
lv_obj_t* message_box;
lv_obj_t* humidity_label;
lv_obj_t* slider;
lv_obj_t* led;
lv_obj_t* wifiConnectingBox;
lv_obj_t* pumpingMsgBox; // Message box for pumping via MQTT

unsigned long next_sound_play = 0;
unsigned long next_lv_task = 0;
unsigned long last_read = 0;
unsigned long alarm_time = 0;
unsigned long last_alarm_time = 0;   // Time of the last alarm
unsigned long next_sensor_read = 0;  // Time of the next sensor read
bool low_humidity = false;           // Flag to indicate low humidity

size_t sound_pos = 0;

int humidity;
bool flag = false;
bool alarm_triggered = false;
bool sound_enabled = false;
const char* alarm_topic = "smartWateringSystem/alarmTriggered";
const char* water_topic = "smartWateringSystem/waterInMl";
const char* humidity_topic = "smartWateringSystem/humidity";

CRGB color;
Speaker speaker;
uint8_t state = SIDELED_STATE_OFF;
uint8_t led_start = 0;
uint8_t led_end = 0;
uint8_t last_state;
uint8_t new_state;

void pump_water(int amount, bool mqtt_message) {
  last_read = millis();
  int time = amount * 200;
  digitalWrite(PUMP_PIN, HIGH);
  while (millis() - last_read < time) {
    digitalWrite(PUMP_PIN, HIGH);
    if (mqtt_message && !pumpingMsgBox) {
      pumpingMsgBox = show_message_box_no_buttons("Pumping via MQTT...");
      delay(100);
    }
    // lv_slider_set_value(slider, amount, LV_ANIM_OFF);
  }
  digitalWrite(PUMP_PIN, LOW);
  if (mqtt_message && pumpingMsgBox) {
    close_message_box(pumpingMsgBox);
    pumpingMsgBox = NULL;
  }
}

void event_handler_pump(struct _lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_PRESSED) {
    int amount = lv_slider_get_value(slider);
    pump_water(amount, false);
    Serial.println("Button pressed");
  }
}

void event_handler_stop_pump(struct _lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_PRESSED) {
    digitalWrite(PUMP_PIN, LOW);
    Serial.println("Pump stopped");
  }
}

void init_gui_elements() {
  add_label("HUMIDITY", 0, 10);
  // convert humidity to string
  humidity_label = add_label("humidity", 0, 40);
  add_label("Water in ml", 0, 80);
  slider = add_slider(1, 350, 20, 150);
  left_button = add_button("PUMP", event_handler_pump, 30, -20);
  right_button = add_button("STOP", event_handler_stop_pump, 170, -20);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  char* buf = (char*)malloc((sizeof(char) * (length + 1)));
  memcpy(buf, payload, length);
  buf[length] = '\0';
  String payloadS = String(buf);
  payloadS.trim();
  Serial.println(payloadS);
  Serial.println(topic);
  if (strcmp(topic, water_topic) == 0 && payloadS.toInt() > 0) {
    pump_water(payloadS.toInt(), true);
  }
  if (strcmp(topic, alarm_topic) == 0) {
    set_sideled_state(alarm_triggered ? SIDELED_STATE_ALARM : sound_enabled ? SIDELED_STATE_ACTIVE : SIDELED_STATE_OFF);
    if (strcmp(payloadS.c_str(), "true") == 0) {
      alarm_triggered = sound_enabled = true;
      if (millis() - last_alarm_time > 300000) {
        last_alarm_time = millis();
        Serial.println("Sensors are at Ready...");
      } else {
        mqtt_publish(alarm_topic, "false");
      }
    } else {
      alarm_triggered = sound_enabled = false;
    }
  }
}

void read_sensor() {
  humidity = analogRead(INPUT_PIN);
  humidity = map(humidity, 0, 4095, 0, 100);
  Serial.println(humidity);
  lv_label_set_text(humidity_label, String(humidity).c_str());
  mqtt_publish(humidity_topic, String(humidity).c_str());

  if (humidity < 62 && millis() > next_sensor_read) {
    next_sensor_read = millis() + 10000;
    if (!alarm_triggered) {
      alarm_triggered = true;
      last_alarm_time = millis();
      mqtt_publish(alarm_topic, "true");
    }
  } else if (humidity >= 62) {
    alarm_triggered = false;
    mqtt_publish(alarm_topic, "false");
  }
}

void loop() {
  if (next_lv_task < millis()) {
    lv_task_handler();
    next_lv_task = millis() + 5;
  }

  if (last_read < millis()) {
    last_read = millis() + 60000;
    Serial.println("Sending data to MQTT broker");
    read_sensor();
  }

  mqtt_loop();

  if (sound_enabled && alarm_triggered && next_sound_play < millis()) {
    size_t byteswritten = speaker.PlaySound(sounddata + sound_pos, NUM_ELEMENTS);

    sound_pos = sound_pos + byteswritten;
    if (sound_pos >= NUM_ELEMENTS) {
      sound_pos = 0;
    }
    next_sound_play = millis() + 1000;
  }
}

void setup() {
  init_m5();
  init_display();
  Serial.begin(115200);

  wifiConnectingBox = show_message_box_no_buttons("Connecting to WiFi...");
  lv_task_handler();
  delay(5);
  setup_wifi();
  mqtt_init(mqtt_callback);
  close_message_box(wifiConnectingBox);

  init_gui_elements();
  init_sideled();
  digitalWrite(PUMP_PIN, LOW);
  pinMode(INPUT_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
}
