#include "hal/Led.h"

#include <Arduino.h>

#include "hal/board_pins.h"

namespace Led {

void begin() {
  const int ledPins[] = {pins::LED_R, pins::LED_G, pins::LED_B};
  for (int pin : ledPins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);  // HIGH = apagado
  }
  // Amplificador del altavoz apagado (a nivel bajo se activa): sin ruido y sin consumo.
  pinMode(pins::AUDIO_EN, OUTPUT);
  digitalWrite(pins::AUDIO_EN, HIGH);
}

}  // namespace Led
