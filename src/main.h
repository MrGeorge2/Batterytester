#ifndef BAT_MAIN
#define BAT_MAIN

#include <Arduino.h>

void printResults(unsigned long actualRunnedMillisLow, unsigned long actualRunnedMillisHigh);
double getBatteryVoltage(unsigned long batPwmPin, uint8_t lowHigh);
unsigned long calculateCapacityInMAH(unsigned long *testStart, unsigned long *testStop, unsigned int testCurrentMah);
void setupTestCurrent();
int calculatePWM();
void startTestIfNotStarted();
bool checkBateryLevel(unsigned long batPwmPin, uint8_t lowHigh, unsigned long *testStopPointer);

static const double resistorValueOhm = 1;

static const unsigned int debounceTimeMs = 50;

static const double plusCurrentMah = 100;
static const double minCurrentMah = 100;
static const double maxCurrentMah = 2000;

static const uint16_t BUTTON_DOWN_PIN = D5;
static const uint16_t BUTTON_UP_PIN = D6;
static const uint16_t OLED_SDA_PIN = D4;
static const uint16_t OLED_SLC_PIN = D3;
static const uint16_t BAT_SWITCH_PIN = D2;
static const uint16_t BAT_VOLTAGE_PIN = A0;
static const uint16_t PWN_OUTPUT_BAT_HIGH = D7;
static const uint16_t PWN_OUTPUT_BAT_LOW = D8;

bool isTestRunning = false;

unsigned long *testStartLow = NULL;
unsigned long *testStartHigh = NULL;
unsigned long *testStopLow = NULL;
unsigned long *testStopHigh = NULL;

double testCurrentMah = 700;

unsigned long buttonDownPressedMilis = 0;
unsigned long buttonUpPressedMilis = 0;

static double nMosLoss = 1.05;
static double pMosLoss = 0;

#endif