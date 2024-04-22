/*
 * Simple example for how to use multiple SinricPro Switch device:
 * - setup 4 switch devices
 * - handle request using multiple callbacks
 * 
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */


#ifdef ENABLE_DEBUG
   #define DEBUG_ESP_PORT Serial
   #define NODEBUG_WEBSOCKETS
   #define NDEBUG
#endif 

#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
  #include <WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "SinricProFanUS.h"
#include <map>

#define WIFI_SSID         "Try"
#define WIFI_PASS         "k@li12345678"
#define APP_KEY           "8e70b5e0-366b-45ec-af72-33bc246cdbdf"
#define APP_SECRET        "01284874-3124-4616-851d-f7463ee6645c-b210c3e2-306d-43fc-95ed-99366918d413"

#define SWITCH_ID_1       "6611818d7c9e6c6fe865bec7"
#define RELAYPIN_1        4
#define SWITCHPIN_1       0
#define FAN_ID            "66118d557c9e6c6fe865c295"

#define BAUD_RATE         115200                // Change baudrate to your need

#define DEBOUNCE_TIME     250

//Flip Switch Control
typedef struct {      // struct for the std::map below
  int relayPIN;
  int flipSwitchPIN;
} deviceConfig_t;

// this is the main configuration
// please put in your deviceId, the PIN for Relay and PIN for flipSwitch
// this can be up to N devices...depending on how much pin's available on your device ;)
// right now we have 4 devicesIds going to 4 relays and 4 flip switches to switch the relay manually
std::map<String, deviceConfig_t> devices = {
    //{deviceId, {relayPIN,  flipSwitchPIN}}
    {SWITCH_ID_1, {  RELAYPIN_1, SWITCHPIN_1 }},     
};


typedef struct {      // struct for the std::map below
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;    // this map is used to map flipSwitch PINs to deviceId and handling debounce and last flipSwitch state checks
                                                  // it will be setup in "setupFlipSwitches" function, using informations from devices map

void setupRelays() { 
  for (auto &device : devices) {           // for each device (relay, flipSwitch combination)
    int relayPIN = device.second.relayPIN; // get the relay pin
    pinMode(relayPIN, OUTPUT);             // set relay pin to OUTPUT
    digitalWrite(relayPIN, HIGH);
  }
}

void setupFlipSwitches() {
  for (auto &device : devices)  {                     // for each device (relay / flipSwitch combination)
    flipSwitchConfig_t flipSwitchConfig;              // create a new flipSwitch configuration

    flipSwitchConfig.deviceId = device.first;         // set the deviceId
    flipSwitchConfig.lastFlipSwitchChange = 0;        // set debounce time
    flipSwitchConfig.lastFlipSwitchState = true;     // set lastFlipSwitchState to false (LOW)--

    int flipSwitchPIN = device.second.flipSwitchPIN;  // get the flipSwitchPIN

    flipSwitches[flipSwitchPIN] = flipSwitchConfig;   // save the flipSwitch config to flipSwitches map
    pinMode(flipSwitchPIN, INPUT_PULLUP);                   // set the flipSwitch pin to INPUT
  }
}


void handleFlipSwitches() {
  unsigned long actualMillis = millis();                                          // get actual millis
  for (auto &flipSwitch : flipSwitches) {                                         // for each flipSwitch in flipSwitches map
    unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;  // get the timestamp when flipSwitch was pressed last time (used to debounce / limit events)

    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {                    // if time is > debounce time...

      int flipSwitchPIN = flipSwitch.first;                                       // get the flipSwitch pin from configuration
      bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;           // get the lastFlipSwitchState
      bool flipSwitchState = digitalRead(flipSwitchPIN);                          // read the current flipSwitch state
      if (flipSwitchState != lastFlipSwitchState) {                               // if the flipSwitchState has changed...
#ifdef TACTILE_BUTTON
        if (flipSwitchState) {                                                    // if the tactile button is pressed 
#endif      
          flipSwitch.second.lastFlipSwitchChange = actualMillis;                  // update lastFlipSwitchChange time
          String deviceId = flipSwitch.second.deviceId;                           // get the deviceId from config
          int relayPIN = devices[deviceId].relayPIN;                              // get the relayPIN from config
          bool newRelayState = !digitalRead(relayPIN);                            // set the new relay State
          digitalWrite(relayPIN, newRelayState);                                  // set the trelay to the new state

          SinricProSwitch &mySwitch = SinricPro[deviceId];                        // get Switch device from SinricPro
          mySwitch.sendPowerStateEvent(!newRelayState);                            // send the event
#ifdef TACTILE_BUTTON
        }
#endif      
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;                  // update lastFlipSwitchState
      }
    }
  }
}

//Fan Control
struct {
  bool powerState = false;
  int fanSpeed = 1;
} device_state;

bool onPowerState2(const String &deviceId, bool &state) {
  Serial.printf("Fan turned %s\r\n", state?"on":"off");
  digitalWrite(D5, state? HIGH: LOW);
  device_state.powerState = state;
  return true; // request handled properly
}

// Fan rangeValue is from 1..3
bool onRangeValue(const String &deviceId, int& rangeValue) {
  device_state.fanSpeed = rangeValue;
  switch(rangeValue){
    case 1:
      analogWrite(D5, 30);
      break;
    case 2:
      analogWrite(D5, 150);
      break;
    case 3:
      analogWrite(D5, 255);
      break;
  }
  Serial.printf("Fan speed changed to %d\r\n", device_state.fanSpeed);
  return true;
}

// Fan rangeValueDelta is from -3..+3
bool onAdjustRangeValue(const String &deviceId, int& rangeValueDelta) {
  device_state.fanSpeed += rangeValueDelta;
  /*
  switch(rangeValueDelta){
    case 1:
      analogWrite(D5, 255);
      break;
    case 2:
      analogWrite(D5, 512);
      break;
    case 3:
      analogWrite(D5, HIGH);
      break;
  }
  */
  Serial.printf("Fan speed changed about %i to %d\r\n", rangeValueDelta, device_state.fanSpeed);

  rangeValueDelta = device_state.fanSpeed; // return absolute fan speed
  return true;
}

bool onPowerState1(const String &deviceId, bool &state) {
 Serial.printf("Device 1 turned %s", state?"on":"off");
 digitalWrite(RELAYPIN_1, state ? HIGH:LOW);
 return true; // request handled properly
}

// setup function for WiFi connection
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");

  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP); 
    WiFi.setAutoReconnect(true);
  #elif defined(ESP32)
    WiFi.setSleep(false); 
    WiFi.setAutoReconnect(true);
  #endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }

  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

#if 0
// setup function for SinricPro
void setupSinricPro() {
  // add devices and callbacks to SinricPro
    
  SinricProSwitch& mySwitch1 = SinricPro[SWITCH_ID_1];
  SinricProFanUS& myFan = SinricPro[FAN_ID];

  // set callback function to device
  myFan.onPowerState(onPowerState2);
  mySwitch1.onPowerState(onPowerState1);
  myFan.onRangeValue(onRangeValue);
  myFan.onAdjustRangeValue(onAdjustRangeValue);
  
  
  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
   
  SinricPro.begin(APP_KEY, APP_SECRET);
}
#endif

void setupSinricPro()
{

  pinMode(D5, OUTPUT);
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState1);
  }

  SinricProFanUS& myFan = SinricPro[FAN_ID];
  myFan.onPowerState(onPowerState2);
  myFan.onRangeValue(onRangeValue);
  myFan.onAdjustRangeValue(onAdjustRangeValue);
  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}
// main setup function
void setup() {
  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  /*
  setupWiFi();
  setupSinricPro();
  */
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);

  setupRelays();
  setupFlipSwitches();
  setupWiFi();
  setupSinricPro();
}

void loop() {
  SinricPro.handle();
  handleFlipSwitches();
}
