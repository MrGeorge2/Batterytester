#include <Arduino.h>
#include <main.h>

void setup()
{
  Serial.begin(9600);

  pinMode(BUTTON_DOWN_PIN, INPUT);
  pinMode(BUTTON_UP_PIN, INPUT);

  pinMode(OLED_SDA_PIN, OUTPUT);
  pinMode(OLED_SLC_PIN, OUTPUT);

  pinMode(BAT_SWITCH_PIN, OUTPUT);
  pinMode(BAT_VOLTAGE_PIN, INPUT);

  pinMode(PWN_OUTPUT_BAT_HIGH, OUTPUT);
  pinMode(PWN_OUTPUT_BAT_LOW, OUTPUT); 

 buttonDownPressedMilis = 0;
 buttonUpPressedMilis = 0;

 analogWriteResolution(8);
 analogWriteFreq(40000);

 analogWrite(PWN_OUTPUT_BAT_HIGH, 0);
 analogWrite(PWN_OUTPUT_BAT_LOW, 0);
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


  if (isTestRunning)
  {
    printResults(actualRunnedMillisLow, actualRunnedMillisHigh);
    delay(300);
  }
}

void printResults(unsigned long actualRunnedMillisLow, unsigned long actualRunnedMillisHigh)
{
  unsigned long lowCapacityMah = calculateCapacityInMAH(testStartLow, &actualRunnedMillisLow, testCurrentMah);
  unsigned long highCapacityMah = calculateCapacityInMAH(testStartHigh, &actualRunnedMillisHigh, testCurrentMah);

  Serial.print("Low capacity mAh ");
  Serial.print(lowCapacityMah);
  Serial.println();

  Serial.print("High capacity mAh " );
  Serial.print(highCapacityMah);
  Serial.println();

  double lowBatVoltage = getBatteryVoltage(PWN_OUTPUT_BAT_HIGH, HIGH);
  double highBatVoltage = getBatteryVoltage(PWN_OUTPUT_BAT_LOW, LOW);

  Serial.print("Low battary voltage ");
  Serial.print(lowBatVoltage);
  Serial.println();

  Serial.print("High battary voltage ");
  Serial.print(highBatVoltage);
  Serial.println();
}

double getBatteryVoltage(unsigned long batPwmPin, uint8_t lowHigh)
{
  digitalWrite(BAT_SWITCH_PIN, lowHigh);

  // For to be cartain wait some time before reading  battery voltage
  delay(10);

  double voltageNotScaled = analogRead(BAT_VOLTAGE_PIN);
  double maxScale = 1023;

  double batVoltage = (voltageNotScaled / maxScale) * 3.3 * 2;
  
  // Add N MOSphet loss
  if (batPwmPin == HIGH){
    batVoltage += nMosLoss;
  }else {
    batVoltage += pMosLoss;
  }

  return batVoltage;
}

bool checkBateryLevel(unsigned long batPwmPin, uint8_t lowHigh, unsigned long *testStopPointer)
{
  return true;
  double batVoltage = getBatteryVoltage(batPwmPin, lowHigh);
  Serial.println(batVoltage);
  if (batVoltage <= 2.5)
  {
    int pwm = analogRead(BAT_SWITCH_PIN);
    if (pwm == 0){
      return false;
    }

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

  int pwm =  (255 * voltage) / 3.3;

  Serial.print("Test current ");
  Serial.println(testCurrentMah);
  Serial.println();

  Serial.print("Voltage ");
  Serial.println(voltage);
  Serial.println();

  Serial.print("PWM ");
  Serial.println(pwm);
  Serial.println();

  return pwm;
}

void setupTestCurrent()
{
  unsigned long upPressedTime = 0;
  unsigned long downPressedTime = 0;
  
  if (digitalRead(BUTTON_DOWN_PIN) == LOW)
  {
    Serial.println("Button low pressed");

    if (buttonDownPressedMilis == 0)
    {
      buttonDownPressedMilis = millis();
    }

    downPressedTime = millis() - buttonDownPressedMilis;
  }

  if (digitalRead(BUTTON_DOWN_PIN) == HIGH)
  {
    if (downPressedTime > debounceTimeMs && upPressedTime == 0)
    {
      testCurrentMah -= plusCurrentMah;

      if (testCurrentMah < minCurrentMah){
        testCurrentMah = maxCurrentMah;
      }

      Serial.print("Setting test current to ");
      Serial.print(testCurrentMah);
      Serial.println();
    }

    buttonDownPressedMilis = 0;
  }

  if (digitalRead(BUTTON_UP_PIN) == LOW)
  {
    Serial.println("Button up pressed");
    if (buttonUpPressedMilis == 0)
    {
      buttonUpPressedMilis = millis();
    }

    upPressedTime = millis() - buttonUpPressedMilis;
  }

  if (digitalRead(BUTTON_UP_PIN) == HIGH)
  {
    if (upPressedTime > debounceTimeMs && downPressedTime == 0)
    {
      testCurrentMah += plusCurrentMah;

      if (testCurrentMah > maxCurrentMah){
        testCurrentMah = minCurrentMah;
      }

      Serial.print("Setting test current to ");
      Serial.print(testCurrentMah);
      Serial.println();
    }
    
    buttonUpPressedMilis = 0;
  }


  if (upPressedTime > debounceTimeMs && downPressedTime > debounceTimeMs)
  {
    Serial.println("Starting test");
    isTestRunning = true;
    upPressedTime = 0;
    downPressedTime = 0;
    return;
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
    Serial.print("PWM ");
    Serial.println(pwmValue);
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

  unsigned long testRun = *testStop - *testStart;

  unsigned long batteryCapacityMah = (testCurrentMah * testRun) / 3600000;

  return batteryCapacityMah;
}