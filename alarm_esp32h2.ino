// Copyright 2025 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief This example demonstrates Zigbee binary input/output device.
 *
 * The example demonstrates how to use Zigbee library to create an end device binary sensor/switch device.
 *
 * Proper Zigbee mode must be selected in Tools->Zigbee mode
 * and also the correct partition scheme must be selected in Tools->Partition Scheme.
 *
 * Please check the README.md for instructions and more detailed description.
 *
 * Created by Jan Procházka (https://github.com/P-R-O-C-H-Y/)
 */

 //For arduino config look here: https://github.com/espressif/arduino-esp32/tree/master/libraries/Zigbee/examples/Zigbee_Binary_Input_Output

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "driver/rtc_io.h"
#include "Zigbee.h"

/* Zigbee binary sensor device configuration */
#define BINARY_DEVICE_ENDPOINT_NUMBER 1

/*Application type*/
#define BINARY_INPUT_APPLICATION_TYPE_SECURITY_INTRUSION_DETECTION  0x01000001

/*Deep sleep*/
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_ACTIVATION_GPIO   GPIO_NUM_14    // Only RTC IO are allowed - ESP32 Pin example.
#define WAKEUP_INTERNAL_GPIO   GPIO_NUM_11    // Only RTC IO are allowed - ESP32 Pin example.
#define WAKEUP_EXTERNAL_GPIO   GPIO_NUM_12    // Only RTC IO are allowed - ESP32 Pin example.
//RTC_DATA_ATTR int bootCount = 0;

/*Time deep sleep*/
#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  86400     //86400 - 24h-*/    /* Time ESP32 will go to sleep (in seconds)

//Pinout according with https://docs.espressif.com/projects/esp-idf/en/stable/esp32h2/api-reference/peripherals/gpio.html
//https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32h2/_images/esp32-h2-devkitm-1-v1.2_pinlayout.png
uint8_t button = BOOT_PIN;
uint8_t alarm_int = 11;
uint8_t alarm_ext = 12;
uint8_t alarm_activation = 14;
uint8_t wu_bypass = 4;
uint8_t voltage_divider = 10;

//ADC. https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html
uint8_t adc_pin = 1; //Use GPIO01

//ESP32H2 supermini blue led
uint8_t blue_led = 13;

//Endpoint definition:

//Binary ep
ZigbeeBinary zbAlarmActivation = ZigbeeBinary(BINARY_DEVICE_ENDPOINT_NUMBER);
ZigbeeBinary zbBinaryInternal = ZigbeeBinary(BINARY_DEVICE_ENDPOINT_NUMBER + 1);
ZigbeeBinary zbBinaryExternal = ZigbeeBinary(BINARY_DEVICE_ENDPOINT_NUMBER + 2);

//Battery ep
ZigbeeAnalog zbBattery = ZigbeeAnalog(BINARY_DEVICE_ENDPOINT_NUMBER + 3);
ZigbeeAnalog zbBatteryPercent = ZigbeeAnalog(BINARY_DEVICE_ENDPOINT_NUMBER + 4);


/*Get effettive battery value. The ADC measure divider voltage*/
float getBatteryValue(int adc_value){
  float r1 = 27*10^3;
  float r2 = 100*10^3;
  float alpha = r2/(r1+r2); 
  return ( (float)adc_value / alpha );
}

int getBatteryPercent(int adc_value){
  int maxValueBattery = 4200; //4.2V
  float battery_value = getBatteryValue(adc_value);
  return ((float)battery_value / maxValueBattery) * 100;
}


void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  Serial.println("WAKE UP REASON:");
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      Serial.println("Wakeup caused by ULP program"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

bool isTimerWakeUp(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER){
    return true;
  }else{
    return false;
  }
}

bool isGPIO_ext1_WakeUp(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT1){
    return true;
  }else{
    return false;
  }
}

void setWakeUp(){

  //Update state alarm int
  if (digitalRead(alarm_int) == LOW) {
    esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(WAKEUP_INTERNAL_GPIO), ESP_EXT1_WAKEUP_ANY_HIGH);
  }
  
  if(digitalRead(alarm_int) == HIGH){
    esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(WAKEUP_INTERNAL_GPIO), ESP_EXT1_WAKEUP_ANY_LOW);
  }

  if (digitalRead(alarm_ext) == LOW) {
    esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(WAKEUP_EXTERNAL_GPIO), ESP_EXT1_WAKEUP_ANY_HIGH);
  }
  
  if (digitalRead(alarm_ext) == HIGH){
    esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(WAKEUP_EXTERNAL_GPIO), ESP_EXT1_WAKEUP_ANY_LOW);
  }

  //Alarm activation
  if (digitalRead(alarm_activation) == LOW) {
    esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(WAKEUP_ACTIVATION_GPIO), ESP_EXT1_WAKEUP_ANY_HIGH);
  }
  
  if(digitalRead(alarm_activation) == HIGH){
    //Deep sleep for GPIO activation
    esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(WAKEUP_ACTIVATION_GPIO), ESP_EXT1_WAKEUP_ANY_LOW);
  }

  /*Deep sleep on timer*/
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  //disable or enable pull up/down of RTC wake up pins
  gpio_pulldown_dis(WAKEUP_ACTIVATION_GPIO);
  gpio_pullup_en(WAKEUP_ACTIVATION_GPIO); 

  gpio_pulldown_dis(WAKEUP_INTERNAL_GPIO);
  gpio_pullup_en(WAKEUP_INTERNAL_GPIO); 

  gpio_pulldown_dis(WAKEUP_EXTERNAL_GPIO);
  gpio_pullup_en(WAKEUP_EXTERNAL_GPIO); 

  Serial.println("Going to sleep now");
  esp_deep_sleep_start(); //start deep sleep

}

//Update alarm state
void updateAlarmState(){

  //Update state alarm int
  if (digitalRead(alarm_int) == LOW) {
    Serial.println("alarm_int ACTIVE!");
    zbBinaryInternal.setBinaryInput(true);
  }
  
  if(digitalRead(alarm_int) == HIGH){
    Serial.println("alarm_int DEACTIVATE!");
    zbBinaryInternal.setBinaryInput(false);
  }

  //Update state alarm ext
  if (digitalRead(alarm_ext) == LOW) {
    Serial.println("alarm_ext ACTIVE!");
    zbBinaryExternal.setBinaryInput(true);
  }
  
  if (digitalRead(alarm_ext) == HIGH){
    Serial.println("alarm_ext DEACTIVATE!");
    zbBinaryExternal.setBinaryInput(false);
  }

  //Alarm activation
  if (digitalRead(alarm_activation) == LOW) {
    Serial.println("alarm_activation ACTIVE!");
    zbAlarmActivation.setBinaryInput(true);
  }
  
  if(digitalRead(alarm_activation) == HIGH){
    Serial.println("alarm_activation DEACTIVATE!");
    zbAlarmActivation.setBinaryInput(false);
  }      

  //report all variables
  zbAlarmActivation.reportBinaryInput();
  zbBinaryInternal.reportBinaryInput();
  zbBinaryExternal.reportBinaryInput();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup function Alarm Zigbee...");

  //Increment boot number and print it every reboot
  //++bootCount;
  //Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  //Init input gpio
  pinMode(alarm_int, INPUT_PULLUP);
  pinMode(alarm_ext, INPUT_PULLUP);
  pinMode(alarm_activation, INPUT_PULLUP);

  //Esp32H2 supermini turn off blue led
  pinMode(blue_led, OUTPUT);
  digitalWrite(blue_led, LOW);
  
  // Init button switch
  pinMode(button, INPUT_PULLUP);

  //Bypass wake up pin
  pinMode(wu_bypass, INPUT_PULLUP);

  //Init voltage divider control pin.
  //Use OUTPUT: https://docs.arduino.cc/language-reference/en/functions/digital-io/digitalwrite/
  pinMode(voltage_divider, OUTPUT);
  
  //Set ADC analog resolution to 12 (default): https://docs.espressif.com/projects/esp-idf/en/latest/esp32h2/api-reference/peripherals/adc/index.html#overview
  analogReadResolution(12);
  analogSetPinAttenuation(adc_pin, ADC_ATTENDB_MAX); //Whit max attenuation ATTEN3, effective measurement range of 0 ~ 3300 mV. https://documentation.espressif.com/esp32-h2_datasheet_en.pdf

  // Optional: set Zigbee device name and model
  zbAlarmActivation.setManufacturerAndModel("Espressif", "ESP32-H2");

  //Alarm activation EP
  zbAlarmActivation.addBinaryInput();
  zbAlarmActivation.setBinaryInputApplication(BINARY_INPUT_APPLICATION_TYPE_SECURITY_INTRUSION_DETECTION);
  zbAlarmActivation.setBinaryInputDescription("alarm_activation");

  // Set up binary zone armed input (Security)
  zbBinaryInternal.addBinaryInput();
  zbBinaryInternal.setBinaryInputApplication(BINARY_INPUT_APPLICATION_TYPE_SECURITY_INTRUSION_DETECTION);
  zbBinaryInternal.setBinaryInputDescription("alarm_int");

  // Set up binary zone armed input (Security)
  zbBinaryExternal.addBinaryInput();
  zbBinaryExternal.setBinaryInputApplication(BINARY_INPUT_APPLICATION_TYPE_SECURITY_INTRUSION_DETECTION);
  zbBinaryExternal.setBinaryInputDescription("alarm_ext");

  //Set battery EP
  zbBattery.addAnalogInput();
  zbBattery.setAnalogInputApplication(ESP_ZB_ZCL_AI_APP_TYPE_ENERGY);
  zbBattery.setAnalogInputDescription("voltage");
  zbBattery.setAnalogInputResolution(1);

  zbBatteryPercent.addAnalogInput();
  zbBatteryPercent.setAnalogInputApplication(ESP_ZB_ZCL_AI_APP_TYPE_ENERGY);
  zbBatteryPercent.setAnalogInputDescription("battery");
  zbBatteryPercent.setAnalogInputResolution(1);

  // Add endpoints to Zigbee Core
  //Zigbee.addEndpoint(&zbBinaryZone);
  Zigbee.addEndpoint(&zbBinaryInternal);
  Zigbee.addEndpoint(&zbBinaryExternal);
  Zigbee.addEndpoint(&zbBattery);
  Zigbee.addEndpoint(&zbBatteryPercent);
  Zigbee.addEndpoint(&zbAlarmActivation);

  Serial.println("Starting Zigbee...");
  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin()) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  } else {
    Serial.println("Zigbee started successfully!");
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Connected");
}

void loop() {

  // Checking button for factory reset and reporting
  if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        delay(1000);
        Zigbee.factoryReset();
      }
    }
  }

  //If by pass is enable skip all
  if(digitalRead(wu_bypass) == LOW){

    //Enable voltage divider and wait 1s to stabize value before reading
    digitalWrite(voltage_divider, HIGH);

    delay(1000);

    bool is_gpio_ext1_wu = isGPIO_ext1_WakeUp();
    bool is_time_wu = isTimerWakeUp();

    //Update alarm state
    //updateAlarmState();

    //Make 60 ADC measurements with delay of 1sec and make avg to get battery usage
    int analogVoltsAvg = 0;
    int numberOfSamples = 200;
    int sampleDelayMs = 5;
    for(int measCount = 0; measCount<=numberOfSamples; measCount++){

      //Read from ADC
      int analogVolts = analogReadMilliVolts(adc_pin);
      analogVoltsAvg += analogVolts;
      int analogValue = analogRead(adc_pin);
      
      // print out the values you read:
      Serial.printf("ADC analog value = %d\n", analogValue);
      Serial.printf("ADC millivolts value = %d\n", analogVolts);

      delay(sampleDelayMs);
    }

    //Avg over all samples
    analogVoltsAvg = analogVoltsAvg/numberOfSamples;

    int batteryVolts = getBatteryValue(analogVoltsAvg);
    int battery_percent = getBatteryPercent(analogVoltsAvg);
    
    Serial.printf("Battery millivolts value = %d\n", batteryVolts);
    Serial.printf("Battery percent = %d %\n", battery_percent);

    //Report voltage and battery value.
    zbBattery.setAnalogInput(batteryVolts);
    zbBatteryPercent.setAnalogInput(battery_percent);  

    //Report battey consumption
    zbBattery.reportAnalogInput(); 
    zbBatteryPercent.reportAnalogInput();

    //Update alarm state for variation during battery update
    updateAlarmState();

    //wait to report
    delay(1000);

    //set wake up conditions
    setWakeUp();
  }

  delay(500); 
}