#include <Adafruit_NeoPixel.h>

#define LED_PIN   5      // GPIO5 = onboard WS2812
#define NUM_LEDS  1

Adafruit_NeoPixel rgb(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  rgb.begin();
  rgb.setBrightness(50); // 0-255, nicht zu hell zum Testen
}

void loop() {
  // Rot
  rgb.setPixelColor(0, rgb.Color(255, 0, 0));
  rgb.show();
  delay(500);

  // Grün
  rgb.setPixelColor(0, rgb.Color(0, 255, 0));
  rgb.show();
  delay(500);

  // Blau
  rgb.setPixelColor(0, rgb.Color(0, 0, 255));
  rgb.show();
  delay(500);

  // Aus
  rgb.setPixelColor(0, rgb.Color(0, 0, 0));
  rgb.show();
  delay(300);
}