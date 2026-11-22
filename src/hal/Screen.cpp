#include "hal/Screen.h"

#include <Preferences.h>

#include "hal/board_pins.h"

// Rotacion de la pantalla: 1 = horizontal, 3 = horizontal girada 180 grados.
// Si la imagen sale boca abajo, cambiar a 3 en platformio.ini.
#ifndef SCREEN_ROTATION
#define SCREEN_ROTATION 1
#endif

namespace {

LGFX lcd;

constexpr uint8_t BRIGHTNESS = 255;
constexpr uint32_t RECALIBRATE_WAIT_MS = 2000;
constexpr uint8_t CALIBRATION_MARKER = 24;

void showCentered(const char* text) {
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font4);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawCenterString(text, lcd.width() / 2, lcd.height() / 2 - 13);
}

bool waitForRecalibrateRequest() {
  showCentered("Pulsa BOOT para recalibrar");
  const uint32_t start = millis();
  while (millis() - start < RECALIBRATE_WAIT_MS) {
    if (digitalRead(pins::BOOT_BTN) == LOW) return true;
    delay(10);
  }
  return false;
}

// La calibracion se guarda en NVS (espacio "touch", clave "cal"), con la
// pantalla ya en horizontal: si cambia la rotacion hay que recalibrar.
void setupTouchCalibration() {
  Preferences prefs;
  prefs.begin("touch", false);
  uint16_t cal[8];
  const bool hasCal = prefs.getBytesLength("cal") == sizeof(cal);

  if (hasCal && !waitForRecalibrateRequest()) {
    prefs.getBytes("cal", cal, sizeof(cal));
    lcd.setTouchCalibrate(cal);
    Serial.println("Calibracion tactil cargada.");
  } else {
    showCentered("Toca la punta de cada marca");
    lcd.calibrateTouch(cal, TFT_WHITE, TFT_BLACK, CALIBRATION_MARKER);
    prefs.putBytes("cal", cal, sizeof(cal));
    Serial.print("Calibracion guardada:");
    for (auto v : cal) Serial.printf(" %u", v);
    Serial.println();
  }
  prefs.end();
}

}  // namespace

namespace Screen {

void begin() {
  lcd.init();
  lcd.setRotation(SCREEN_ROTATION);  // horizontal, 480x320
  lcd.invertDisplay(PANEL_INVERT_DEFAULT);
  lcd.setBrightness(BRIGHTNESS);
  Serial.printf("Pantalla %s: %dx%d, rotacion %d\n", panelName(), (int)lcd.width(), (int)lcd.height(),
                SCREEN_ROTATION);

  pinMode(pins::BOOT_BTN, INPUT_PULLUP);
  setupTouchCalibration();
  lcd.fillScreen(TFT_BLACK);
}

LGFX& gfx() { return lcd; }

}  // namespace Screen
