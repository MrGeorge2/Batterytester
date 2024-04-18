#include <Arduino.h>

void printResults(unsigned long actualRunnedMillisLow, unsigned long actualRunnedMillisHigh);
double getBatteryVoltage(unsigned long batPwmPin, uint8_t lowHigh);
unsigned long calculateCapacityInMAH(unsigned long *testStart, unsigned long *testStop, unsigned int testCurrentMah);
void setupTestCurrent();
int calculatePWM();
void startTestIfNotStarted();
bool checkBateryLevel(unsigned long batPwmPin, uint8_t lowHigh, unsigned long *testStopPointer);

static const unsigned int resistorValueOhm = 1;

static const unsigned int debounceTimeMs = 300;

static const unsigned int plusCurrentMah = 100;
static const unsigned int minCurrentMah = 100;
static const unsigned int maxCurrentMah = 1500;

static const uint16_t BUTTON_DOWN_PIN = D0;
static const uint16_t BUTTON_UP_PIN = D5;
static const uint16_t OLED_SDA_PIN = D4;
static const uint16_t OLED_SLC_PIN = D5;
static const uint16_t BAT_SWITCH_PIN = D2;
static const uint16_t BAT_VOLTAGE_PIN = A0;
static const uint16_t PWN_OUTPUT_BAT_HIGH = D7;
static const uint16_t PWN_OUTPUT_BAT_LOW = D8;

bool isTestRunning = false;

unsigned long *testStartLow;
unsigned long *testStartHigh;
unsigned long *testStopLow;
unsigned long *testStopHigh;

unsigned int testCurrentMah = 700;

unsigned long buttonDownPressedMilis = 0;
unsigned long buttonUpPressedMilis = 0;

void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON_DOWN_PIN, INPUT);
  pinMode(BUTTON_UP_PIN, INPUT);

  pinMode(OLED_SDA_PIN, OUTPUT);
  pinMode(OLED_SLC_PIN, OUTPUT);

  pinMode(BAT_SWITCH_PIN, OUTPUT);
  pinMode(BAT_VOLTAGE_PIN, INPUT);

  pinMode(PWN_OUTPUT_BAT_HIGH, OUTPUT);
  pinMode(PWN_OUTPUT_BAT_LOW, OUTPUT);
}

void loop()
{
  if (!isTestRunning)
  {
    setupTestCurrent();
    return;
  }

  if (isTestRunning)
  {
    startTestIfNotStarted();
  }

  unsigned long actualRunnedMillisLow = millis();
  bool testLowIsRunning = checkBateryLevel(PWN_OUTPUT_BAT_LOW, LOW, testStopLow);
  if (testStopLow != nullptr)
    actualRunnedMillisLow = *testStopLow;

  unsigned long actualRunnedMillisHigh = millis();
  bool testHighIsRunning = checkBateryLevel(PWN_OUTPUT_BAT_HIGH, HIGH, testStopHigh);
  if (testStopHigh != nullptr)
    actualRunnedMillisHigh = *testStopHigh;

  if (!testHighIsRunning && !testLowIsRunning)
  {
    isTestRunning = false;
    Serial.println("Test finished");
  }

  printResults(actualRunnedMillisLow, actualRunnedMillisHigh);
}

void printResults(unsigned long actualRunnedMillisLow, unsigned long actualRunnedMillisHigh)
{
  unsigned long lowCapacityMah = calculateCapacityInMAH(testStartLow, &actualRunnedMillisLow, testCurrentMah);
  unsigned long highCapacityMah = calculateCapacityInMAH(testStartHigh, &actualRunnedMillisHigh, testCurrentMah);

  Serial.println("Low capacity mAh " + lowCapacityMah);
  Serial.println("High capacity mAh " + highCapacityMah);

  double lowBatVoltage = getBatteryVoltage(PWN_OUTPUT_BAT_HIGH, HIGH);
  double highBatVoltage = getBatteryVoltage(PWN_OUTPUT_BAT_LOW, LOW);

  char lowBatVoltageBuffer[10];
  dtostrf(lowBatVoltage, 6, 2, lowBatVoltageBuffer);
  Serial.println("Low battary voltage " + String(lowBatVoltageBuffer));

  char highBatVoltageBuffer[10];
  dtostrf(highBatVoltage, 6, 2, highBatVoltageBuffer);
  Serial.println("High battary voltage " + String(highBatVoltageBuffer));
  Serial.println();
}

double getBatteryVoltage(unsigned long batPwmPin, uint8_t lowHigh)
{
  digitalWrite(BAT_SWITCH_PIN, lowHigh);

  // For to be cartain wait some time before reading  battery voltage
  delay(10);

  int voltageNotScaled = analogRead(BAT_VOLTAGE_PIN);
  double batVoltage = (1023 * voltageNotScaled * 2) / 3.3;

  return batVoltage;
}

bool checkBateryLevel(unsigned long batPwmPin, uint8_t lowHigh, unsigned long *testStopPointer)
{
  double batVoltage = getBatteryVoltage(batPwmPin, lowHigh);
  if (batVoltage < 3)
  {
    analogWrite(batPwmPin, 0);
    unsigned long millisNow = millis();
    testStopPointer = &millisNow;
    return false;
  }

  return true;
}

int calculatePWM()
{
  double voltage = (testCurrentMah / 1000) * resistorValueOhm;

  return (255 * voltage) / 3.3;
}

void setupTestCurrent()
{
  unsigned long upPressedTime = 0;
  unsigned long downPressedTime = 0;

  if (digitalRead(BUTTON_DOWN_PIN) == LOW)
  {
    if (buttonDownPressedMilis == 0)
    {
      buttonDownPressedMilis = millis();
    }

    downPressedTime = millis() - buttonDownPressedMilis;
  }

  if (digitalRead(BUTTON_DOWN_PIN == HIGH) && buttonDownPressedMilis != 0)
  {
    buttonDownPressedMilis = 0;
  }

  if (digitalRead(BUTTON_UP_PIN) == LOW)
  {
    if (buttonUpPressedMilis == 0)
    {
      buttonUpPressedMilis = millis();
    }

    upPressedTime = millis() - buttonUpPressedMilis;
  }

  if (digitalRead(BUTTON_UP_PIN) == LOW && buttonUpPressedMilis != 0)
  {
    buttonUpPressedMilis = 0;
  }

  if (upPressedTime > debounceTimeMs && downPressedTime == 0)
  {
    testCurrentMah += plusCurrentMah;
    Serial.println("Setting test current to " + testCurrentMah);
  }

  if (downPressedTime > debounceTimeMs && upPressedTime == 0)
  {
    testCurrentMah -= plusCurrentMah;
    Serial.println("Setting test current to " + testCurrentMah);
  }

  if (upPressedTime > debounceTimeMs && downPressedTime > debounceTimeMs)
  {
    Serial.println("Starting test");
    isTestRunning = true;
  }
}

void startTestIfNotStarted()
{
  // test started but not initialized yet
  if (testStartLow == nullptr || testStartHigh == nullptr)
  {
    unsigned long millisNow = millis();
    testStartLow = &millisNow;
    testStartHigh = &millisNow;

    int pwmValue = calculatePWM();
    analogWrite(PWN_OUTPUT_BAT_HIGH, pwmValue);
    analogWrite(PWN_OUTPUT_BAT_LOW, pwmValue);
  }
}

unsigned long calculateCapacityInMAH(unsigned long *testStart, unsigned long *testStop, unsigned int testCurrentMah)
{
  if (testStart == nullptr || testStop == nullptr)
  {
    return 0;
  }

  unsigned long testRun = testStop - testStart;

  unsigned long batteryCapacityMah = (testCurrentMah * testRun) / 3600000;

  return batteryCapacityMah;
}