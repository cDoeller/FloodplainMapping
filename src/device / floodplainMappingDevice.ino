// ============================================================
// INFO
// ============================================================
// Code for the Speculative Sensing II: floodplain mapping tool
// by Christian Doeller

// ============================================================
// STEPS BEFORE USE
// ============================================================
// 0) Install necessary libraries (see below)
// 1) check touch thresh (callibrate touch sensor)
// 2) check highest and lowest temp for scale callibration

// ============================================================
// CONNECTIONS
// ============================================================
// BME 280 SCL - SCL ESP
// BME 280 SDA - SDA ESP
// BME 280 VCC - 3V3 ESP
// BME 280 GND - GND ESP
// Touch Sensor - GPIO 4 ESP
// Servo Red - VCC ESP
// Servo Brown - GND ESP
// Servo Orange - GPIO26 ESP
// DS18B20 Red - VCC ESP
// DS18B20 Black - GND ESP
// DS18B20 Yellow - 2-4 kOhm to 3V3 ESP
// DS18B20 Yellow - GPIO19
// ============================================================

#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_NeoPixel.h>
#include <driver/adc.h>

// ============================================================
// VARIABLES
// ============================================================
// ============================================================
// DEBUG
// ============================================================

bool callibrate = false;

// ============================================================
// LED / BATTERY
// ============================================================
#define LED_PIN 5
#define NUM_LEDS 1
#define BAT_PIN 34
#define BAT_LOW_VOLTAGE 3.6f
#define BAT_CHECK_INTERVAL 30000
#define BAT_SHOW_MS 3000
unsigned long lastBatCheck = 0;

// ============================================================
// TOUCH
// ============================================================
#define TOUCH_PIN 4
#define TOUCH_THRESH 600           // *** CALLIBRATION: thresh val when tipping probe
#define DEBOUNCE_MS 20             // debounce ms: tipped?
#define LONG_PRESS_MS 2000         // long hold ms
#define ULTRA_LONG_PRESS_MS 10000  // long long hold for battery check

// ============================================================
// DS18B20
// ============================================================
#define DS18B20_ONE_WIRE_BUS 19
#define DS18B20_RESOLUTION_BIT 10  // 10 bit → ~188 ms Konvertierungszeit
#define TEMP_CONV_MS 200           // time buffer: 188 ms per measurement
#define LOWEST_TEMP 15.0f          // *** CALLIBRATION: lowest temp ground probe
#define HIGHEST_TEMP 30.0f         // *** CALLIBRATION: highest temp ground probe
#define TEMP_ERROR -127.0f         // error check: produces this value if measurement failed
bool DS18B20_attached = false;

// ============================================================
// BME280
// ============================================================
#define BME_DELAY 250
#define HIGHEST_BME_TEMP 30.0f  // *** CALLIBRATION: lowest temp air
#define LOWEST_BME_TEMP 15.0f   // *** CALLIBRATION: highest temp air
bool BME280_attached = false;

// ============================================================
// SERVO
// ============================================================
#define SERVO_PIN 26
#define SERVO_ANGLE_MIN 0
#define SERVO_ANGLE_MAX 175            // servo callibration failure
#define SERVO_ANGLE_DS18B20 45         // servo callibration failure
#define SERVO_ANGLE_BME280 135         // servo callibration failure
#define CONTINUOUS_MOVEMENT_STEP 1.0f  // degree per step when swiping
#define SERVO_SETTLE_MS 8              // ms between swiping steps
#define SERVO_FINAL_SETTLE_MS 500

// ============================================================
// OBJECTS
// ============================================================
// ============================================================
OneWire oneWire(DS18B20_ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo servo;
Adafruit_BME280 bme;
Adafruit_NeoPixel rgb(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ============================================================
// STATE MACHINES
// ============================================================
// ============================================================
// TOUCH STATE MACHINE
// ============================================================
enum TouchState { TS_IDLE,           // default mode: check if touched
                  TS_DEBOUNCING,     // debounce mode: check if once or long
                  TS_PRESSED,        // if once
                  TS_LONG_ACTIVE };  // if long
TouchState touchState = TS_IDLE;
unsigned long touchStateTimer = 0;

// ============================================================
// DS18B20 TEMPERATURE STATE MACHINE
// ============================================================
enum TempState { TEMP_IDLE,
                 TEMP_CONVERTING };
TempState DS18B20TempState = TEMP_IDLE;
unsigned long DS18B20TempStart = 0;
float latestValidTemp = TEMP_ERROR;

// ============================================================
// BME280 TEMPERATURE STATE MACHINE
// ============================================================
enum BMEState { BME_IDLE,
                BME_MEASURING };
BMEState BME280State = BME_IDLE;
unsigned long BME280Start = 0;

// ============================================================
// SERVO MOTOR STATE MACHINE
// ============================================================
enum ServoState { SERVO_IDLE,
                  SERVO_MOVING,
                  SERVO_SETTLING };
ServoState servoState = SERVO_IDLE;
unsigned long servoStart = 0;
float currentPos = SERVO_ANGLE_MIN;
float targetPos = SERVO_ANGLE_MIN;

// ============================================================
// SENSORS STATE MACHINE
// ============================================================
enum SensorsState { TEMP_SENSOR_DS18B20,
                    TEMP_SENSOR_BME280 };
SensorsState sensorState = TEMP_SENSOR_DS18B20;

// ============================================================
// SETUP
// ============================================================
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("[DEVICE] Device startup..");

  // DS18B20
  sensors.begin();
  if (sensors.getDeviceCount() == 0) {
    Serial.println("[WARN][DS18B20] Could not find a valid DS18B20 sensor!");
  } else {
    Serial.println("[DS18B20] sensor found!");
    DS18B20_attached = true;
    sensors.setWaitForConversion(false);  // NON-BLOCKING readings
    sensors.setResolution(DS18B20_RESOLUTION_BIT);
  }

  // BME
  unsigned bmeStatus;
  bmeStatus = bme.begin(0x76);

  if (!bmeStatus) {
    Serial.println("[WARN][BME280] Could not find a valid BME280 sensor!");
  } else {
    Serial.println("[BME280] sensor found!");
    BME280_attached = true;
  }

  // SERVO
  servo.attach(SERVO_PIN);
  servo.write(SERVO_ANGLE_MIN);  // go to start position
  delay(1000);
  servo.detach();

  // LED
  rgb.begin();
  rgb.setBrightness(25);
  rgb.setPixelColor(0, rgb.Color(0, 0, 0));
  rgb.show();

  Serial.print("Reset reason: ");
  Serial.println(esp_reset_reason());
}

// ============================================================
// LOOP
// ============================================================
// ============================================================
void loop() {
  unsigned long now = millis();

  if (callibrate) {
    callibrateTouch(0, 1000);
    callibrateD18B20();
    callibrateBME280();
    callibrateBattery();

  } else {
    handleTouch(now);
    handleDS18B20Temperature(now);
    handleBME280(now);
    handleServoMotor(now);
  }

  if (now - lastBatCheck >= BAT_CHECK_INTERVAL) {
    lastBatCheck = now;
    checkBattery();
  }
}

// ============================================================
// FUNCTIONS
// ============================================================
// ============================================================
// HELPER FUNCTIONS
// ============================================================
void callibrateTouch(int minVal, int maxVal) {
  int val = touchRead(TOUCH_PIN);

  Serial.print("[TOUCH] ");
  Serial.println(val);

  float angle = map(val, minVal, maxVal, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX);
  int constrainedAngle = int(constrain(angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX));

  servo.attach(SERVO_PIN);
  servo.write(constrain(constrainedAngle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX));
  delay(250);
  servo.detach();
  delay(250);
}

bool isTouched() {
  int touchVal = touchRead(TOUCH_PIN);
  return (touchVal < TOUCH_THRESH);
}

void callibrateD18B20() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC != TEMP_ERROR) {
    Serial.print("[D18B20] ");
    Serial.println(tempC);
    delay(TEMP_CONV_MS);
  }
}

void callibrateBME280() {
  float bmeTemp = bme.readTemperature();
  Serial.print("[BME280] ");
  Serial.println(bmeTemp);
  delay(BME_DELAY);
}

void callibrateBattery() {
  Serial.print("Battery Voltage Measurement: ");
  Serial.println(readBatteryVoltage());
}

// ============================================================
// BATTERY FUNCTIONS
// ============================================================

float readBatteryVoltage() {
  analogSetAttenuation(ADC_11db);
  uint32_t mv = analogReadMilliVolts(BAT_PIN);  // kalibrierte Messung
  float voltage = (mv / 1000.0f) * 2.0f;        // Spannungsteiler x2
  return voltage;
}

void checkBattery() {
  float vbat = readBatteryVoltage();

  if (vbat < BAT_LOW_VOLTAGE) {
    rgb.setPixelColor(0, rgb.Color(255, 0, 0));
    rgb.show();
  } else {
    rgb.clear();
    rgb.show();
  }
}

float voltageToPercent(float v) {
  // typische LiPo-Ruhespannungskurve (Stützpunkte)
  struct Point {
    float v;
    float pct;
  };
  static const Point curve[] = {
    { 4.20f, 100.0f },
    { 4.00f, 85.0f },
    { 3.85f, 70.0f },
    { 3.70f, 50.0f },
    { 3.60f, 25.0f },
    { 3.50f, 10.0f },
    { 3.40f, 5.0f },
    { 3.30f, 0.0f },
  };
  const int n = sizeof(curve) / sizeof(curve[0]);

  if (v >= curve[0].v) return 100.0f;
  if (v <= curve[n - 1].v) return 0.0f;

  for (int i = 0; i < n - 1; i++) {
    if (v <= curve[i].v && v >= curve[i + 1].v) {
      float span = curve[i].v - curve[i + 1].v;
      float ratio = (v - curve[i + 1].v) / span;
      return curve[i + 1].pct + ratio * (curve[i].pct - curve[i + 1].pct);
    }
  }
  return 0.0f;
}

void showBatteryColor() {
  float vbat = readBatteryVoltage();

  // LiPo: ~3.0V empty, ~4.2V full
  // float pct = constrain((vbat - 3.0f) / (4.2f - 3.0f), 0.0f, 1.0f);
  float pct = voltageToPercent(vbat) / 100.0f; // new (more precise) way!

  // Red (empty) → Yellow → Green (full)
  uint8_t r = (uint8_t)((1.0f - pct) * 255);
  uint8_t g = (uint8_t)(pct * 255);

  Serial.print("[BAT] voltage: ");
  Serial.println(vbat);
  Serial.print("[BAT] percent: ");
  Serial.println(pct * 100);

  rgb.setPixelColor(0, rgb.Color(r, g, 0));
  rgb.show();
  delay(BAT_SHOW_MS);

  rgb.clear();
  rgb.show();
}

// ============================================================
// STATE MACHINES
// ============================================================
// ============================================================
// TOUCH STATE MACHINE
// ============================================================
void handleTouch(unsigned long _now) {
  bool touchDetected = isTouched();  // sensor below thresh

  switch (touchState) {

    // default mode: below thresh > check debounce, start timer
    case TS_IDLE:
      if (touchDetected) {
        Serial.println("[TOUCH] touch detected.");
        touchState = TS_DEBOUNCING;
        touchStateTimer = _now;
      }
      break;

    // debounce mode: exceeding decounce delay > pressed
    case TS_DEBOUNCING:
      if (!touchDetected) {
        Serial.println("[TOUCH] glitch!");
        touchState = TS_IDLE;  // under debounce, only glitch
      } else if (_now - touchStateTimer >= DEBOUNCE_MS) {
        touchState = TS_PRESSED;
        touchStateTimer = _now;  // reset for detecting long touch
        Serial.println("[TOUCH] touch validated.");
      }
      break;

    // pressed mode: check long or short and trigger action
    case TS_PRESSED:
      // short touch: released (after debounce) > reset state machine
      if (!touchDetected) {
        Serial.println("[TOUCH] recognized SHORT touch.");
        touchState = TS_IDLE;
        // MAKE MEASUREMENT
        Serial.println("[TOUCH][DEVICE] making measurement.");

        switch (sensorState) {
          case TEMP_SENSOR_DS18B20:
            Serial.println("[TOUCH][DEVICE] measuring DS18B20..");
            if (DS18B20_attached) {
              startDS18B20();
            } else {
              Serial.println("[TOUCH][DEVICE] DS18B20 not attached.");
            }
            break;

          case TEMP_SENSOR_BME280:
            Serial.println("[TOUCH][DEVICE] measuring BME280..");
            if (BME280_attached) {
              startBME280();
            } else {
              Serial.println("[TOUCH][DEVICE] BME280 not attached.");
            }
            break;
        }
      }

      // long touch: held for longer time > long active
      if (_now - touchStateTimer >= LONG_PRESS_MS) {
        Serial.println("[TOUCH] recognized LONG touch, waiting for release or ultra-long.");
        touchState = TS_LONG_ACTIVE;
      }
      break;

    // long touch active: held for longer time, released? > reset state machine
    case TS_LONG_ACTIVE:
      // Ultra-Long after 10s
      if (_now - touchStateTimer >= (ULTRA_LONG_PRESS_MS - LONG_PRESS_MS)) {
        Serial.println("[TOUCH] recognized ULTRA LONG touch → battery display.");
        showBatteryColor();
        touchState = TS_IDLE;
      }

      if (!touchDetected) {
        Serial.println("[TOUCH] LONG touch released → switching sensors.");
        touchStateTimer = 0;
        touchState = TS_IDLE;

        // SWITCH SENSORS
        servo.attach(SERVO_PIN);

        switch (sensorState) {
          case TEMP_SENSOR_DS18B20:
            servo.write(SERVO_ANGLE_MIN);
            delay(500);
            servo.write(SERVO_ANGLE_BME280);
            delay(500);
            servo.write(SERVO_ANGLE_MIN);
            delay(500);
            currentPos = SERVO_ANGLE_MIN;
            sensorState = TEMP_SENSOR_BME280;
            Serial.println("[TOUCH][DEVICE] new sensor: BME280");
            break;

          case TEMP_SENSOR_BME280:
            servo.write(SERVO_ANGLE_MIN);
            delay(500);
            servo.write(SERVO_ANGLE_DS18B20);  // go to start position
            delay(500);
            servo.write(SERVO_ANGLE_MIN);
            delay(500);
            currentPos = SERVO_ANGLE_MIN;
            sensorState = TEMP_SENSOR_DS18B20;
            Serial.println("[TOUCH][DEVICE] new sensor: DS18B20");
            break;
        }
        servo.detach();
      }
      break;
  }
}

// ============================================================
// D18B20 TEMPERATURE STATE MACHINE
// ============================================================

void startDS18B20() {
  if (DS18B20TempState != TEMP_IDLE) return;  // still running

  Serial.println("[DS18B20] Starting measurement..");

  sensors.requestTemperatures();

  DS18B20TempStart = millis();
  DS18B20TempState = TEMP_CONVERTING;
}

void handleDS18B20Temperature(unsigned long _now) {
  switch (DS18B20TempState) {
    case TEMP_IDLE:
      // do nothing
      break;

    case TEMP_CONVERTING:
      if (_now - DS18B20TempStart >= TEMP_CONV_MS) {
        float tempTemp = sensors.getTempCByIndex(0);

        // reset timer and measure again if non-valid measurement, else > update, IDLE
        if (tempTemp == TEMP_ERROR) {
          sensors.requestTemperatures();
          DS18B20TempStart = _now;
        } else {
          latestValidTemp = tempTemp;
          DS18B20TempState = TEMP_IDLE;

          Serial.print("[DS18B20] latest temperature: ");
          Serial.println(latestValidTemp);

          // MOVE SERVO
          float servoAngle =
            (latestValidTemp - LOWEST_TEMP) * (SERVO_ANGLE_MAX - SERVO_ANGLE_MIN) / (HIGHEST_TEMP - LOWEST_TEMP)
            + SERVO_ANGLE_MIN;

          servoAngle = constrain(
            servoAngle,
            SERVO_ANGLE_MIN,
            SERVO_ANGLE_MAX);

          moveServo(int(servoAngle));

          //// *** RANDOM SERVO TEST
          // float testAngle = random(0, 170);
          // Serial.print("[DS18B20] test angle: ");
          // Serial.println(testAngle);
          // moveServo(testAngle);
        }
      }
      break;
  }
}

// ============================================================
// BME280 TEMPERATURE STATE MACHINE
// ============================================================
void startBME280() {
  if (BME280State != BME_IDLE) return;  // still running

  Serial.println("[BME280] Starting measurement..");

  BME280Start = millis();
  BME280State = BME_MEASURING;
}

void handleBME280(unsigned long _now) {
  switch (BME280State) {
    case BME_IDLE:
      //do nothing
      break;

    case BME_MEASURING:
      if (_now - BME280Start >= BME_DELAY) {
        float bmeTemp = bme.readTemperature();

        BME280State = BME_IDLE;

        Serial.print("[BME280] latest temperature: ");
        Serial.println(bmeTemp);

        // MOVE SERVO
        float servoAngle =
          (bmeTemp - LOWEST_BME_TEMP) * (SERVO_ANGLE_MAX - SERVO_ANGLE_MIN) / (HIGHEST_BME_TEMP - LOWEST_BME_TEMP)
          + SERVO_ANGLE_MIN;

        servoAngle = constrain(
          servoAngle,
          SERVO_ANGLE_MIN,
          SERVO_ANGLE_MAX);

        moveServo(int(servoAngle));
      }
      break;
  }
}

// #define BME_DELAY 250
// enum TempState { BME_IDLE,
//                  BME_MEASURING };
// TempState BME280State = TEMP_IDLE;
// unsigned long BME280Start = 0;
// float bmeTemp = bme.readTemperature();


// ============================================================
// SERVO MOTOR STATE MACHINE
// ============================================================
void moveServo(float _newPos) {
  if (_newPos == currentPos) {  // already there
    Serial.println("[SERVO] no changes.");
    return;
  }
  if (servoState != SERVO_IDLE) return;  // still in action, no new target possible

  Serial.print("[SERVO] moving to ");
  Serial.println(_newPos);

  targetPos = _newPos;
  servoStart = millis();
  servo.attach(SERVO_PIN);
  servoState = SERVO_MOVING;
}

void handleServoMotor(unsigned long _now) {
  switch (servoState) {
    case SERVO_IDLE:
      // do nothing
      break;

    case SERVO_MOVING:
      // if arrived, reset servo
      if (currentPos == targetPos) {
        Serial.println("[SERVO] arrived.");
        servoStart = _now;
        servoState = SERVO_SETTLING;
        return;
      }

      // if servo ready, move servo one step
      if (_now - servoStart >= SERVO_SETTLE_MS) {
        // move servo left
        if (currentPos < targetPos) {

          currentPos += CONTINUOUS_MOVEMENT_STEP;

          // arrwived / overshot > arrived (error tolerance)
          if (currentPos > targetPos) {
            currentPos = targetPos;
          }

          servo.write(currentPos);

          // move servo right
        } else if (currentPos > targetPos) {

          currentPos -= CONTINUOUS_MOVEMENT_STEP;
          // arrwived / overshot > arrived (error tolerance)
          if (currentPos < targetPos) {
            currentPos = targetPos;
          }

          servo.write(currentPos);
        }

        // reset timer for next step
        servoStart = _now;
      }
      break;

    // turn off servo after final settle delay
    case SERVO_SETTLING:
      if (_now - servoStart >= SERVO_FINAL_SETTLE_MS) {
        servo.detach();
        servoState = SERVO_IDLE;
      }
      break;
  }
}
