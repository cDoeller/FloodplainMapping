#define TOUCH_PIN 4
#define TOUCH_THRESH 30
#define DEBOUNCE_MS 20
#define LONG_PRESS_MS 2000

enum State {
  IDLE,
  DEBOUNCING,
  PRESSED,
};

State state = IDLE;
bool lastRaw = false;
unsigned long stateStart = 0;

bool callibrate = false;

void setup() {
  Serial.begin(115200);

  Serial.println("[TOUCH] Starting touch test..");
}

void loop() {
  if (callibrate) {
    callibrateTouch();

  } else {
    unsigned long now = millis();
    bool raw = isTouched();

    switch (state) {

      case IDLE:
        if (raw) {
          state = DEBOUNCING;
          stateStart = now;
        }
        break;

      case DEBOUNCING:
        if (!raw) {
          state = IDLE;
        } else if (now - stateStart >= DEBOUNCE_MS) {
          state = PRESSED;
          stateStart = now;
        }
        break;

      case PRESSED:
        if (!raw) {
          unsigned long held = now - stateStart;
          if (held < LONG_PRESS_MS) {
            Serial.println("TOUCH");
          } else {
            Serial.println("HOLD");
          }
          state = IDLE;
        }
        break;
    }
  }
}

// FUNC
bool isTouched() {
  return touchRead(TOUCH_PIN) < TOUCH_THRESH;
}

void callibrateTouch() {
  Serial.print("[TOUCH] val: ");
  Serial.println(touchRead(TOUCH_PIN));
  delay(100);
}
