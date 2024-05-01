#ifndef BAT_MAIN
#define BAT_MAIN

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

void printResults(unsigned long actualRunnedMillisLow);
double getBatteryVoltage();
double calculateCapacityInMAH(unsigned long testStop, unsigned int testCurrentMah);
void setupTestCurrent();
int calculatePWM();
bool checkBateryLevel();
void printOnlineResults();
void printFinalResults();
void initDisplay();
void displayDischargeCurrent(double current);
void displayBatteryInfo(double voltage, double capacity, bool isFinished);

static const double resistorValueOhm = 1;
static const double VOLTAGE_DIVIDER_PRECISE = 2.1056; 
static const double MILLIS_TO_HOURS = 3600000;
static const unsigned int debounceTimeMs = 50;

static const double plusCurrentMah = 100;
static const double minCurrentMah = 100;
static const double maxCurrentMah = 2000;

static const uint16_t BUTTON_DOWN_PIN = D5;
static const uint16_t BUTTON_UP_PIN = D6;
static const uint16_t OLED_SDA_PIN = D4;
static const uint16_t OLED_SLC_PIN = D3;
static const uint16_t BAT_VOLTAGE_PIN = A0;
static const uint16_t PWN_OUTPUT_BAT_LOW = D8;

bool isTestRunning = false;

unsigned long testStartMillis = 0;
unsigned long testStopMillis = 0;

double testCurrentMah = 700;

unsigned long buttonDownPressedMilis = 0;
unsigned long buttonUpPressedMilis = 0;


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#endif