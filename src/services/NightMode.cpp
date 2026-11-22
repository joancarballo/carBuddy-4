#include "services/NightMode.h"

#include <Arduino.h>
#include <Preferences.h>

#include "hal/Screen.h"
#include "services/TimeService.h"

namespace {

constexpr uint32_t STEP_MS = 30;       // el brillo cambia poco a poco...
constexpr int STEP = 4;                // ...de 4 en 4

// Por defecto: brillo al maximo y sin apagado.
NightMode::Settings cfg = {NightMode::MAX_BRIGHTNESS, false, 23 * 60, 7 * 60};

int current = NightMode::MAX_BRIGHTNESS;  // brillo aplicado
bool off = false;
uint32_t wakeUntil = 0;  // encendida por un toque hasta este momento
bool woken = false;
uint32_t lastStep = 0;

void apply(int value) {
  current = value;
  Screen::gfx().setBrightness((uint8_t)value);
}

int readInt(Preferences& prefs, const char* key, int fallback) {
  return prefs.isKey(key) ? prefs.getInt(key, fallback) : fallback;
}

bool readBool(Preferences& prefs, const char* key, bool fallback) {
  return prefs.isKey(key) ? prefs.getBool(key, fallback) : fallback;
}

int clampBrightness(int value) {
  if (value < NightMode::MIN_BRIGHTNESS) return NightMode::MIN_BRIGHTNESS;
  if (value > NightMode::MAX_BRIGHTNESS) return NightMode::MAX_BRIGHTNESS;
  return value;
}

// true si ahora toca tener la pantalla apagada por horario.
bool inOffHours() {
  if (!cfg.scheduleOff || cfg.offFrom == cfg.offTo) {
    return false;
  }
  struct tm t;
  if (!TimeService::now(t)) {
    return false;  // sin hora no se apaga
  }
  const int m = t.tm_hour * 60 + t.tm_min;
  if (cfg.offFrom < cfg.offTo) {
    return m >= cfg.offFrom && m < cfg.offTo;
  }
  return m >= cfg.offFrom || m < cfg.offTo;  // cruza la medianoche
}

bool awakeByTap(uint32_t now) {
  if (woken && (int32_t)(now - wakeUntil) >= 0) {
    woken = false;
  }
  return woken;
}

}  // namespace

namespace NightMode {

void begin() {
  Preferences prefs;
  prefs.begin("display", true);
  // isKey() antes de leer: este nucleo escribe un error por cada clave que no existe.
  cfg.brightness = clampBrightness(readInt(prefs, "level", cfg.brightness));
  cfg.scheduleOff = readBool(prefs, "sched", cfg.scheduleOff);
  cfg.offFrom = readInt(prefs, "from", cfg.offFrom);
  cfg.offTo = readInt(prefs, "to", cfg.offTo);
  prefs.end();
  apply(cfg.brightness);
}

void loop() {
  const uint32_t now = millis();

  off = inOffHours() && !awakeByTap(now);
  if (off) {
    if (current != 0) {
      apply(0);  // apagar, de golpe
    }
    return;
  }

  const int target = cfg.brightness;
  if (current == 0) {
    apply(target);  // al encenderse, directamente al brillo que toca
    return;
  }
  const int diff = target - current;
  if (diff != 0 && now - lastStep >= STEP_MS) {
    lastStep = now;
    const int step = diff > 0 ? (diff < STEP ? diff : STEP) : (diff > -STEP ? diff : -STEP);
    apply(current + step);
  }
}

bool handleTap() {
  const bool wasOff = off;
  if (inOffHours()) {
    // En horas de apagado cada toque alarga el minuto encendida.
    woken = true;
    wakeUntil = millis() + WAKE_MS;
  }
  if (wasOff) {
    off = false;
    apply(cfg.brightness);
    Serial.println("Pantalla encendida por un toque");
  }
  return wasOff;
}

Settings settings() { return cfg; }

void save(const Settings& s) {
  cfg = s;
  cfg.brightness = clampBrightness(cfg.brightness);
  Preferences prefs;
  prefs.begin("display", false);
  prefs.putInt("level", cfg.brightness);
  prefs.putBool("sched", cfg.scheduleOff);
  prefs.putInt("from", cfg.offFrom);
  prefs.putInt("to", cfg.offTo);
  prefs.end();
  Serial.printf("Pantalla: brillo %d, apagado %s %02d:%02d-%02d:%02d\n", cfg.brightness,
                cfg.scheduleOff ? "SI" : "NO", cfg.offFrom / 60, cfg.offFrom % 60, cfg.offTo / 60,
                cfg.offTo % 60);
}

uint8_t brightness() { return (uint8_t)current; }

bool isOff() { return off; }

}  // namespace NightMode
