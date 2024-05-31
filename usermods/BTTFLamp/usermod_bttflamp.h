#pragma once

#include "wled.h"
#include "TM1637.h"

//Pin defaults for ESP32-WROOM32
#ifndef SEGMENT_PIN_1
    #define SEGMENT_PIN_1 22
#endif
#ifndef SEGMENT_PIN_2
    #define SEGMENT_PIN_2 17
#endif
#ifndef SEGMENT_PIN_3
    #define SEGMENT_PIN_3 21
#endif
#ifndef SEGMENT_PIN_AM
    #define SEGMENT_PIN_AM 18
#endif
#ifndef SEGMENT_PIN_PM
    #define SEGMENT_PIN_PM 19
#endif
#ifndef SEGMENT_PIN_CLK
    #define SEGMENT_PIN_CLK 4
#endif
#ifndef UTC
    #define UTC 2
#endif
#ifndef DISPLAY_BACKLIGHT
    #define DISPLAY_BACKLIGHT 7
#endif
#ifndef USERMOD_BTTFLAMP_UPDATE_INTERVAL
  #define USERMOD_BTTFLAMP_UPDATE_INTERVAL 250
#endif

class UsermodBTTFLamp : public Usermod {

    bool initDone = false;
    
    // GPIO pin used for segments (with a default compile-time fallback)
    uint8_t firstSegmentPin = SEGMENT_PIN_1;
    uint8_t secondSegmentPin = SEGMENT_PIN_2;
    uint8_t thirdSegmentPin = SEGMENT_PIN_3;
    uint8_t amPin = SEGMENT_PIN_AM;
    uint8_t pmPin = SEGMENT_PIN_PM;  
    uint8_t clk = SEGMENT_PIN_CLK;
    int utc = UTC; // UTC + value in hour - Summer time
    uint8_t displaybacklight = DISPLAY_BACKLIGHT; // Set displays brightness 0 to 7;
    bool first = true;

    unsigned long updateInterval = USERMOD_BTTFLAMP_UPDATE_INTERVAL;
    // set last reading as "2 sec before boot", so first update is after 20 sec
    unsigned long lastUpdated = UINT32_MAX - USERMOD_BTTFLAMP_UPDATE_INTERVAL;

    // strings to reduce flash memory usage (used more than twice)
    static const char _name[];
    static const char _enabled[];
    static const char _readInterval[];

    TM1637 *firstSegment;
    TM1637 *secondSegment;
    TM1637 *thirdSegment;

  public:

    /*
     * API calls te enable data exchange between WLED modules
     */
    uint16_t getId() { return USERMOD_ID_BTTFLAMP; }

    void setup();
    void loop();

    void addToConfig(JsonObject &root);
    bool readFromConfig(JsonObject &root);
};

void UsermodBTTFLamp::setup() {

  DEBUG_PRINTLN(F("Allocating Pins for BTTF Lamp"));
  pinManager.allocatePin(firstSegmentPin, true, PinOwner::UM_BTTF);
  pinManager.allocatePin(secondSegmentPin, true, PinOwner::UM_BTTF);
  pinManager.allocatePin(thirdSegmentPin, true, PinOwner::UM_BTTF);
  pinManager.allocatePin(amPin, true, PinOwner::UM_BTTF);
  pinManager.allocatePin(pmPin, true, PinOwner::UM_BTTF);
  pinManager.allocatePin(clk, true, PinOwner::UM_BTTF);
  
  firstSegment = new TM1637(clk, firstSegmentPin);
  secondSegment = new TM1637(clk, secondSegmentPin);
  thirdSegment = new TM1637(clk, thirdSegmentPin);

  firstSegment->setBrightness(displaybacklight);
  secondSegment->setBrightness(displaybacklight);
  thirdSegment->setBrightness(displaybacklight);

  lastUpdated = millis() - updateInterval + 19500;

  initDone = true;
}

void UsermodBTTFLamp::loop() {
  unsigned long now = millis();
  if(first)
  {
    digitalWrite(pmPin, HIGH);
    digitalWrite(amPin, HIGH);
    first = false;
  }

  if (now - lastUpdated < updateInterval || !initDone) return;

  DEBUG_PRINTLN(F("BTTF Get local time"));

  struct tm timeinfo;
  if(getLocalTime(&timeinfo))
  {    
    if(timeinfo.tm_hour<12) {
      pinMode(amPin, OUTPUT);
      digitalWrite(amPin, HIGH);
      digitalWrite(pmPin, LOW);
    }
    else{
      pinMode(pmPin, OUTPUT);
      digitalWrite(amPin, LOW);
      digitalWrite(pmPin, HIGH);
    }
  }

  firstSegment->display(timeinfo.tm_year);
  secondSegment->display(timeinfo.tm_mon);
  secondSegment->display(timeinfo.tm_mday);
  thirdSegment->display(timeinfo.tm_hour);
  thirdSegment->display(timeinfo.tm_min);
  
  lastUpdated = now;
}

/**
 * addToConfig() (called from set.cpp) stores persistent properties to cfg.json
 */
void UsermodBTTFLamp::addToConfig(JsonObject &root) {
  // we add JSON object: {"Temperature": {"pin": 0, "degC": true}}
  JsonObject top = root.createNestedObject(FPSTR(_name)); // usermodname
  top[FPSTR(_readInterval)] = updateInterval / 1000;
  DEBUG_PRINTLN(F("Temperature config saved."));
}

/**
 * readFromConfig() is called before setup() to populate properties from values stored in cfg.json
 *
 * The function should return true if configuration was successfully loaded or false if there was no configuration.
 */
bool UsermodBTTFLamp::readFromConfig(JsonObject &root) {
  // we look for JSON object: {"Temperature": {"pin": 0, "degC": true}}
  DEBUG_PRINT(FPSTR(_name));

  JsonObject top = root[FPSTR(_name)];
  if (top.isNull()) {
    DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
    return false;
  }

  updateInterval   = top[FPSTR(_readInterval)] | updateInterval/1000;
  updateInterval   = min(120,max(10,(int)updateInterval)) * 1000;  // convert to ms

  if (!initDone) {
    // first run: reading from cfg.json
    DEBUG_PRINTLN(F(" config loaded."));
  } else {
    DEBUG_PRINTLN(F(" config (re)loaded."));
    // changing paramters from settings page
    
    DEBUG_PRINTLN(F("Re-init BTTF Lamp."));
    delete firstSegment;

    // deallocate pin and release memory
    pinManager.deallocatePin(firstSegmentPin, PinOwner::UM_BTTF);
    pinManager.deallocatePin(secondSegmentPin, PinOwner::UM_BTTF);
    pinManager.deallocatePin(thirdSegmentPin, PinOwner::UM_BTTF);
    pinManager.deallocatePin(amPin, PinOwner::UM_BTTF);
    pinManager.deallocatePin(pmPin, PinOwner::UM_BTTF);
    pinManager.deallocatePin(clk, PinOwner::UM_BTTF);
    // initialise
    setup();
  }

  return !top[FPSTR(_readInterval)].isNull();
}

  // strings to reduce flash memory usage (used more than twice)
const char UsermodBTTFLamp::_name[]         PROGMEM = "BTTFLamp";
const char UsermodBTTFLamp::_readInterval[] PROGMEM = "read-interval-s";