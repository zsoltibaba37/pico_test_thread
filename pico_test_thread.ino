/**
 * @file pico_test_thread.ino
 *
 * @brief Test threads and 'add_repeating_timer_ms' with onboard led.
 *
 * @author Zsolt Peto
 * Contact: https://github.com/zsoltibaba37
 *
 */

// ------------------ version --------------------
constexpr float version{ 1.0 };

// ------------------ I2C Oled -------------------
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint16_t displayRefreshTime{ 1000 / 10 };
uint32_t initDisplay{ millis() };
// ---------------- END I2C Oled -----------------

// -------------------- Leds ---------------------
static const uint8_t LED_ONBOARD{ 25 };
bool ledOnboardState{ false };
int32_t mSecOnboard{ 250 };
constexpr int32_t mSecOnboardMin{50};
constexpr int32_t mSecOnboardMax{500};

static const uint8_t ledY{ 10 };
static const uint8_t ledW{ 12 };
static const uint8_t ledG{ 14 };
static const uint8_t *leds[4]{ &ledY, &ledW, &ledG, &LED_ONBOARD };
bool ledsState{ false };
uint8_t ledsCount{ 0 };

uint16_t mSec{ 200 };
uint32_t initMillisBoardLed{ millis() };
uint32_t initMillisLeds{ millis() };

// ---------------- Analog Stuffs ----------------
#define ANALOG_POT A0
uint64_t potRawValue{};
static const uint8_t sampleNum{ 30 };

float tempRawValue{};
constexpr uint8_t tempSampleNum{ 30 };
constexpr float convFactor{ 3.3 / 10.23 };

// ------------------ Buttons --------------------
static const uint8_t button1{ 20 };
static const uint8_t button2{ 21 };
static const uint8_t *buttons[2]{ &button1, &button2 };
int16_t incr{ 10 };
bool loopState{ true };

// --------------- predefined VOIDS --------------

/** Blink 3 led
*
* Delayed with [mSec] variable.
* 
*/
void blinkLeds();
bool blinkOnboardLed(struct repeating_timer *t);
void displayMessage();
void readPot();
void readTemp();
void sensButton();

// ------------------ SETUP ----------------------
// ------------------ SETUP ----------------------
void setup() {
  Serial.begin(115200);

  // ------------------ I2C Oled ------------------
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {  // Address 0x3D or 0x3C for 128x64
    Serial.println(F("-- SSD1306 allocation failed --"));
    for (;;)
      ;
  }

  // ---------------- Init buttons ---------------
  for (auto a : buttons) {
    gpio_init(*a);
    gpio_set_dir(*a, INPUT);
    gpio_pull_up(*a);
  }

  // ----------------- Init leds -----------------
  for (auto a : leds) {
    gpio_init(*a);
    gpio_set_dir(*a, OUTPUT);
  }

  // --------------- Init screen ----------------
  display.clearDisplay();
  display.setRotation(2);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(44, 2);
  display.println("Pico");
  display.setCursor(40, 19);
  display.println("Test");
  display.setCursor(28, 36);
  display.println("Thread");
  display.setTextSize(1);
  display.setCursor(24, 53);
  display.println("Version: " + String(version));
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.display();
  delay(3000);
}

// ----------------- SETUP 1 ---------------------
// ----------------- SETUP 1 ---------------------
void setup1() {
  pinMode(ANALOG_POT, INPUT);
}

// ------------------- LOOP ----------------------
// ------------------- LOOP ----------------------
void loop() {
  // ---------- Start repeating timer ----------
  struct repeating_timer timer;
  add_repeating_timer_ms(mSecOnboard, blinkOnboardLed, NULL, &timer);

  // ---------- This is the real loop ----------
  while (loopState) {
    blinkLeds();

    displayMessage();

    sensButton();
  }
  // -------------- END LOOP --------------

  // once run this when push one of the buttons(button1, button2) +inc or -inc
  cancel_repeating_timer(&timer);
  mSecOnboard -= incr;
  delay(10);
  mSecOnboard = max(mSecOnboard, mSecOnboardMin);
  mSecOnboard = min(mSecOnboard, mSecOnboardMax);

  loopState = true;
}
// ------------------- LOOP 1 --------------------
// ------------------- LOOP 1 --------------------
void loop1() {

  readPot();
  delay(10);

  readTemp();
  delay(10);
}
// ---------------- END LOOP 1 -------------------

// ------------------ VOIDS ----------------------
// ------------------ VOIDS ----------------------

void blinkLeds() {
  if (millis() - initMillisLeds >= mSec) {
    gpio_put(*leds[ledsCount], ledsState);
    initMillisLeds = millis();
    ledsCount++;
    if (ledsCount > 2) {
      ledsCount = 0;
    }
    ledsState ^= 1;
  }
}
// --------------------------------------------------

/** Blink pico onboard led 
*
* With add_repeating_timer_ms, delay time is [mSecOnboard]
* 
*/
bool blinkOnboardLed(struct repeating_timer *t) {
  ledOnboardState ^= 1;
  gpio_put(LED_ONBOARD, ledOnboardState);

  return true;
}
// --------------------------------------------------

/** Display information on 0.96" oled
* 
* AnalogRaw: 167
* mSec     : 123 ms
* Temp     : 30.02 C
* onBoard  : 320 ms
* version: 
* 
*/
void displayMessage() {
  if (millis() - initDisplay >= displayRefreshTime) {
    display.clearDisplay();
    display.setRotation(2);
    display.setCursor(8, 3);
    display.println("AnalogRaw: " + String(int(potRawValue)));
    display.setCursor(8, 15);
    display.println("mSec     : " + String(int(mSec)) + " ms");
    display.setCursor(8, 27);
    display.println("Temp     : " + String(tempRawValue) + " C");
    display.setCursor(8, 39);
    display.println("onBord   : " + String(mSecOnboard) + " ms");
    display.setCursor(24, 53);
    display.println("Version: " + String(version));
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    for (int i = 1; i != 5; i++) {
      display.drawFastHLine(0, i * 12, SCREEN_WIDTH, WHITE);
    }
    display.display();
    initDisplay = millis();
  }
}
// --------------------------------------------------

/** Read potentiometer analog value
*
* With [sampleNum] times, and set [mSet] variable
* 
*/
void readPot() {
  for (int i = 1; i != sampleNum; i++) {
    potRawValue += analogRead(ANALOG_POT);
  }
  potRawValue = potRawValue / sampleNum;
  mSec = map(potRawValue, 0, 1024, 50, 500);
}
// --------------------------------------------------

/** Read temperature value
*
* [tempRawValue] Calculate with [convFactor]
* 
*/
void readTemp() {
  tempRawValue = analogReadTemp();
  tempRawValue = tempRawValue / convFactor;
}
// --------------------------------------------------

/** Detect the push buttons
*
* Exit from main() loop and, set [incr]
* positive or negative
* 
*/
void sensButton() {
  if (!gpio_get(button1) && loopState) {
    loopState = false;
    incr = 10;
    Serial.println("Increment: " + String(incr));
  }
  if (!gpio_get(button2) && loopState) {
    loopState = false;
    incr = -10;
    Serial.println("Increment: " + String(incr));
  }
}