#include <OneWire.h>
#include <DallasTemperature.h>

#define PRINT_DELAY_TIME 1000
#define MEASURE_DELAY_TIME 100  // this has to be lower than print rate and higher than X
#define RESOLUTION_BIT 10       // 12 = max, long measure times of about 750ms!; 9 bit = 94ms; 10 bit 188ms
#define ONE_WIRE_BUS 21

float tempTempC = -127.00;
float validTempC = 0;

unsigned long lastPrintTime = 0;
unsigned long lastMeasurementTime = 0;

int failCounter = 0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);

  sensors.begin();
  sensors.setWaitForConversion(true);
  sensors.setResolution(RESOLUTION_BIT);

  Serial.print("Anzahl Sensoren: ");
  Serial.println(sensors.getDeviceCount());

  if (sensors.getDeviceCount() == 0) {
    Serial.println("Kein DS18B20 gefunden!");
  }
}

void loop() {

  unsigned long currentTime = millis();

  // Print Loop
  if (currentTime - lastPrintTime >= PRINT_DELAY_TIME) {
    Serial.print(validTempC);
    Serial.print("°C");
    Serial.print('\t');
    Serial.print("fails: ");
    Serial.println(failCounter);

    lastPrintTime = currentTime;
    failCounter = 0;
  }

  // Measurement Loop (prevent failures)
  if (currentTime - lastMeasurementTime >= MEASURE_DELAY_TIME) {
    sensors.requestTemperatures();
    tempTempC = sensors.getTempCByIndex(0);

    if (tempTempC != -127.00) {
      validTempC = tempTempC;
    } else {
      failCounter++;
    }

    lastMeasurementTime = currentTime;
  }
}
