#include <Arduino.h>
#include <main.h>

void setup()
{
  Serial.begin(9600);

  pinMode(BUTTON_DOWN_PIN, INPUT);
  pinMode(BUTTON_UP_PIN, INPUT);

  pinMode(OLED_SDA_PIN, OUTPUT);
  pinMode(OLED_SLC_PIN, OUTPUT);

  pinMode(BAT_VOLTAGE_PIN, INPUT);

  pinMode(PWN_OUTPUT_BAT_LOW, OUTPUT);

  buttonDownPressedMilis = 0;
  buttonUpPressedMilis = 0;

  analogWriteResolution(8);
  analogWriteFreq(40000);

  analogWrite(PWN_OUTPUT_BAT_LOW, 0);

  initDisplay();
}

void loop()
{
  if (testStopMillis != 0)
  {
    printFinalResults();
    return;
  }

  if (!isTestRunning)
  {
    setupTestCurrent();
    return;
  }

  printOnlineResults();
}

void initDisplay(){
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  displayDischargeCurrent(testCurrentMah);
}

void printOnlineResults()
{
  unsigned long actualRunnedMillisLow = millis();
  isTestRunning = checkBateryLevel();
  if (testStopMillis != 0)
    actualRunnedMillisLow = testStopMillis;

  if (!isTestRunning)
  {
    Serial.println("Test finished");
  }

  printResults(actualRunnedMillisLow);
  delay(1000);
}

void printFinalResults()
{
  analogWrite(PWN_OUTPUT_BAT_LOW, 0);
  Serial.println("Test finished");
  printResults(testStopMillis);
  delay(1000);
}

void printResults(unsigned long actualRunnedMillisLow)
{
  unsigned long batCapacityMah = calculateCapacityInMAH(actualRunnedMillisLow, testCurrentMah);

  Serial.print("Capacity mAh ");
  Serial.print(batCapacityMah);
  Serial.println();

  double batVoltage = getBatteryVoltage();

  Serial.print("Battary voltage ");
  Serial.print(batVoltage);
  Serial.println();

  displayBatteryInfo(batVoltage, batCapacityMah, testStopMillis != 0);
}

double getBatteryVoltage()
{
  double voltageNotScaled = analogRead(BAT_VOLTAGE_PIN);
  double maxScale = 1023;

  double batVoltage = (voltageNotScaled / maxScale) * 3.3 * VOLTAGE_DIVIDER_PRECISE;

  return batVoltage;
}

bool checkBateryLevel()
{
  double batVoltage = getBatteryVoltage();
  Serial.println(batVoltage);
  if (batVoltage <= 2.5)
  {
    analogWrite(PWN_OUTPUT_BAT_LOW, 0);
    testStopMillis = millis();
    return false;
  }

  return true;
}

int calculatePWM()
{
  double voltage = (testCurrentMah / 1000) * resistorValueOhm;

  int pwm = (255 * voltage) / 3.3;

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

      if (testCurrentMah < minCurrentMah)
      {
        testCurrentMah = maxCurrentMah;
      }

      Serial.print("Setting test current to ");
      Serial.print(testCurrentMah);
      Serial.println();
      displayDischargeCurrent(testCurrentMah);
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

      if (testCurrentMah > maxCurrentMah)
      {
        testCurrentMah = minCurrentMah;
      }

      Serial.print("Setting test current to ");
      Serial.print(testCurrentMah);
      Serial.println();
      displayDischargeCurrent(testCurrentMah);
    }

    buttonUpPressedMilis = 0;
  }

  if (upPressedTime > debounceTimeMs && downPressedTime > debounceTimeMs)
  {
    Serial.println("Starting test");
    isTestRunning = true;
    upPressedTime = 0;
    downPressedTime = 0;
    
    testStartMillis = millis();
    Serial.print("Setting test start millis ");
    Serial.println(testStartMillis);

    int pwmValue = calculatePWM();
    Serial.print("PWM ");
    Serial.println(pwmValue);
    analogWrite(PWN_OUTPUT_BAT_LOW, pwmValue);
  }
}

double calculateCapacityInMAH(unsigned long testStop, unsigned int testCurrentMah)
{
  Serial.print("Test stop ");
  Serial.println(testStop);
  Serial.print("Test start ");
  Serial.println(testStartMillis);
  if (testStartMillis == 0 || testStop == 0)
  {
    return 0;
  }

  double testRun = testStop - testStartMillis;
  Serial.print("Test run ");
  Serial.println(testRun);

  double batteryCapacityMah = testCurrentMah * (testRun / 3600000);

  return batteryCapacityMah;
}

void displayDischargeCurrent(double current) {
  display.clearDisplay();

  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println("Discharge current: ");
  display.println();
  display.println(current);

  display.display();
}

void displayBatteryInfo(double voltage, double capacity, bool isFinished) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println("Battery voltage: ");
  display.println(voltage);
  display.println();

  display.println("Battery Capacity: ");
  display.println(capacity);
  display.println();

  display.println("Is test finished: ");
  display.println(isFinished);
  display.println();

  display.display();
}