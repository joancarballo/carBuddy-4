#pragma once
// Brillo de la pantalla y apagado por horario. Se configura en la web (seccion
// "Pantalla") y se guarda en Preferences (espacio "display").
//
// Esta placa no tiene sensor de luz (el CYD de 2,8" si lo tenia), asi que el
// brillo es un valor fijo que se elige en la web. Los cambios de brillo son
// graduales, sin saltos.
//
// Apagado por horario: dentro del rango, la retroiluminacion se apaga. Un toque
// la enciende durante WAKE_MS, y ese toque no llega a las apps. Sin hora
// sincronizada no se apaga.

#include <stdint.h>

namespace NightMode {

constexpr uint8_t MIN_BRIGHTNESS = 10;  // con menos, la pantalla casi no se ve
constexpr uint8_t MAX_BRIGHTNESS = 255;
constexpr uint32_t WAKE_MS = 60000;     // encendida tras un toque en horas de apagado

struct Settings {
  int brightness;     // entre MIN_BRIGHTNESS y MAX_BRIGHTNESS
  bool scheduleOff;
  int offFrom;        // minutos desde las 00:00
  int offTo;          // puede ser menor que offFrom: el rango cruza la medianoche
};

// Lee la configuracion. Llamar despues de Screen::begin().
void begin();

// Aplica el brillo y el apagado por horario. Llamar en cada vuelta del loop().
void loop();

// Llamar con cada toque antes de pasarlo a las apps. Devuelve true si la
// pantalla estaba apagada: el toque la enciende y no debe hacer nada mas.
bool handleTap();

Settings settings();
void save(const Settings& s);

// Para la web: brillo actual y si esta apagada.
uint8_t brightness();
bool isOff();

}  // namespace NightMode
