# Floodplain Mapping

This repository contains the firmware for a set of handheld temperature sensing devices developed as part of the **Auenland** artistic residency and workshop. The tools were designed to explore **microclimates in a river floodplain** through a combination of human perception and technical sensing.

Rather than functioning as scientific measuring instruments, the devices are intended as **artistic research tools**. They invite participants to compare bodily sensations of temperature with sensor-based measurements and to reflect on the different ways landscapes can be perceived, interpreted, and represented.

---

## Project Context

The devices were developed for a two-day workshop within the **Auenland** residency, a project focusing on the ecological restoration and cultural perception of floodplain landscapes.

Participants work in small groups to:

- explore local microclimates
- compare human and technical perception
- investigate spatial temperature differences
- create speculative maps of the landscape based on their observations

The workshop combines methods from artistic research, environmental sensing, speculative cartography and media art.

---

## Hardware

Each handheld device consists of:

- **DFRobot FireBeetle ESP32-E (DFR0654)**
- **DS18B20 waterproof temperature sensor**
  - used for water or soil temperature
  - connected via a 3 m cable
- **BME280 environmental sensor**
  - air temperature
  - relative humidity
  - atmospheric pressure
- **SG90 Servo Motor**
  - analogue output representing measured values
- **Capacitive Touch Input**
  - measurement trigger / interaction
- **18650 Li-Ion battery**
- USB charging via the FireBeetle onboard charging circuit

---

## Interaction

Instead of displaying numerical values on a screen, the devices translate measurements into a simple physical movement using a servo motor.

The goal is not precise numerical readout but an embodied and interpretative form of sensing that leaves room for discussion and comparison between different observations.

---

## Repository Structure

```
/
├── src/                Arduino source code
└── README.md
```

---

## Development Environment / Libraries

- Arduino IDE
- ESP32 Arduino Core
- Wire library
- SPI library
- driver/adc.h library

Install:
- OneWire library
- DallasTemperature library
- Adafruit BME280 library (+ Adafruit Sensor periphery)
- Adafruit Neopixel library
- SimpleServoESP32 by noeFly
