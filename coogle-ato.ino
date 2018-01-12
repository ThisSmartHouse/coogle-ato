/*
  +----------------------------------------------------------------------+
  | CoogleATO for ESP8266                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) 2017 John Coggeshall                                   |
  +----------------------------------------------------------------------+
  | Licensed under the Apache License, Version 2.0 (the "License");      |
  | you may not use this file except in compliance with the License. You |
  | may obtain a copy of the License at:                                 |
  |                                                                      |
  | http://www.apache.org/licenses/LICENSE-2.0                           |
  |                                                                      |
  | Unless required by applicable law or agreed to in writing, software  |
  | distributed under the License is distributed on an "AS IS" BASIS,    |
  | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      |
  | implied. See the License for the specific language governing         |
  | permissions and limitations under the License.                       |
  +----------------------------------------------------------------------+
  | Authors: John Coggeshall <john@coggeshall.org>                       |
  +----------------------------------------------------------------------+
*/

#include <CoogleIOT.h>

#define SERIAL_BAUD 115200
#define TOPIC_ID "coogle-topoff-1"
#define TOPOFF_TRIGGER_PIN D1
#define SWITCH_BASE_TOPIC "/" TOPIC_ID
#define POLL_FREQUENCY_MILLISECONDS 5000
#define SAMPLE_COUNT 50
#define TOPOFF_TRIGGER 250

CoogleIOT *iot;
PubSubClient *mqtt;

bool active = true;

void setup() {

  iot = new CoogleIOT(LED_BUILTIN);

  iot->enableSerial(SERIAL_BAUD);
  iot->initialize();

  iot->info("Coogle Auto-Topoff Initializing..");
  iot->info("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");

  iot->registerTimer(POLL_FREQUENCY_MILLISECONDS, checkWaterSensor);
  
  if(iot->mqttActive()) {
      mqtt = iot->getMQTTClient();

      mqtt->publish(SWITCH_BASE_TOPIC "/status", "off", true);

      iot->info("Publishing State to topic " SWITCH_BASE_TOPIC "/status");

      mqtt->setCallback(mqttCallback);
      mqtt->subscribe(SWITCH_BASE_TOPIC);
      
  } else {
    iot->error("Initialization failure, invalid MQTT Server Connection");
  }

  pinMode(TOPOFF_TRIGGER_PIN, OUTPUT);
  digitalWrite(TOPOFF_TRIGGER_PIN, LOW);
  
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String command = String((char *)payload);

  command = command.substring(0, length);
    
  if(command.compareTo("resume") == 0) {
      active = true;
      mqtt->publish(SWITCH_BASE_TOPIC "/status", "off", true);
      return;
  } else if(command.compareTo("halt") == 0) {
      active = false;
      digitalWrite(TOPOFF_TRIGGER_PIN, LOW);
      mqtt->publish(SWITCH_BASE_TOPIC "/status", "disabled", true);
  }
}

void checkWaterSensor() 
{
  if(!active) {
    iot->debug("ATO Currently Disabled");
    return;
  }
  
  int waterSensor;
  int sampleAverage;
  
  sampleAverage = 0;
  
  for(int i = 0; i < SAMPLE_COUNT; i++) {
    sampleAverage = sampleAverage + analogRead(A0);
    yield();
  }

  sampleAverage = round(sampleAverage / SAMPLE_COUNT);

  if(sampleAverage <= TOPOFF_TRIGGER) {
     digitalWrite(TOPOFF_TRIGGER_PIN, HIGH);

      if(iot->mqttActive()) {
          mqtt->publish(SWITCH_BASE_TOPIC "/status", "on", true);
      }
      
  } else {
     digitalWrite(TOPOFF_TRIGGER_PIN, LOW);

     if(iot->mqttActive()) {
         mqtt->publish(SWITCH_BASE_TOPIC "/status", "off", true);
     }
     
  }

  iot->debug("Polling of Water Sensor complete");
  iot->logPrintf(DEBUG, "Average Reading: %d", sampleAverage);
  
}

void loop() {
  iot->loop();
}
