#pragma once
// Pantalla y tactil (LovyanGFX), en horizontal: 480x320.

#include "hal/display.h"

namespace Screen {

constexpr int WIDTH = 480;
constexpr int HEIGHT = 320;

// Inicia la pantalla y carga la calibracion del tactil. Si no hay calibracion
// guardada, o se pulsa BOOT en los 2 s que se ofrece, se calibra de nuevo.
void begin();

LGFX& gfx();

}  // namespace Screen
