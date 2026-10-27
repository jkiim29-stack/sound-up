
/*
  bench.ino: the Arduino half of a sound-sensor bench pair.

  It reads the microphone and prints one line per reading over USB,
  as name and number pairs separated by spaces:

    sound 42
    sound 57
    sound 31

  The browser-side bench/sense-serial.js can read these lines.
  The OLED also displays the current relative sound level.

  Wired for an Arduino Nano, analog microphone module, and I2C OLED:

  Microphone:
    +  -> 5V
    G  -> GND
    A0 -> A0
    D0 -> not connected

  OLED:
    GND -> GND
    VCC -> module-supported supply voltage
    SCL -> A5
    SDA -> A4

  OLED settings assume a 128x64 SSD1306 at I2C address 0x3C.

  Serial protocol:
    sound <peak-to-peak ADC value>

  The sound value is a relative level, NOT calibrated decibels.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int MIC_PIN = A0;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = -1;
const int OLED_ADDRESS = 0x3C;

const unsigned long SAMPLE_WINDOW_MS = 50;
const unsigned long REPORT_INTERVAL_MS = 50;

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

unsigned long lastReport = 0;

// Measure the microphone's peak-to-peak variation over a short window.
int readSoundLevel() {
  int signalMin = 1023;
  int signalMax = 0;

  unsigned long start = millis();

  while (millis() - start < SAMPLE_WINDOW_MS) {
    int sample = analogRead(MIC_PIN);

    if (sample < signalMin) {
      signalMin = sample;
    }

    if (sample > signalMax) {
      signalMax = sample;
    }
  }

  return signalMax - signalMin;
}

void updateDisplay(int soundLevel) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SOUND LEVEL");

  display.setTextSize(2);
  display.setCursor(0, 15);
  display.println(soundLevel);

  // Relative visual scale; adjust after observing real readings.
  int barWidth = constrain(
    map(soundLevel, 0, 100, 0, 116),
    0,
    116
  );

  display.drawRect(0, 43, 120, 12, SSD1306_WHITE);

  if (barWidth > 0) {
    display.fillRect(2, 45, barWidth, 8, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setCursor(0, 57);
  display.println("Relative level");

  display.display();
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {
    // Stop if the OLED cannot initialize.
    while (true) {
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Sound bench ready");
  display.display();
}

void loop() {
  unsigned long now = millis();

  if (now - lastReport >= REPORT_INTERVAL_MS) {
    lastReport = now;

    int soundLevel = readSoundLevel();

    // USB serial protocol: name followed by numeric value.
    Serial.print("sound ");
    Serial.println(soundLevel);

    updateDisplay(soundLevel);
  }
}
