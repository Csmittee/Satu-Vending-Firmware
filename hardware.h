#ifndef HARDWARE_H
#define HARDWARE_H

#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <FastLED.h>

extern Adafruit_MCP23017 mcp1;
extern Adafruit_MCP23017 mcp2;
extern CRGB leds[NUM_LEDS];

// Initialization
void initHardware();
void initMCP23017();
void initLED();

// Relay control
void setRelay(int relayNum, bool state);

// Sensor reading
bool readSensor(int sensorNum);

// LED effects
void setLEDColor(int start, int end, CRGB color);
void celebrationAnimation();
void errorAnimation();
void idleAnimation();
void waterPumpAnimation();

// Door lock
void unlockDoor();
void lockDoor();

// Vending
void vendProduct(int lane);

#endif
