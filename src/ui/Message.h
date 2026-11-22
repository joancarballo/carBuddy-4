#pragma once
// Mensajes centrados (espera, conexion, error), como Display::message() de la v1.

#include "hal/Screen.h"

namespace Message {

// Borra el rectangulo y escribe el texto centrado; '\n' separa lineas. Si waiting
// es true, deja debajo el sitio para la animacion de espera (ver waitingDots()).
void show(lgfx::LovyanGFX& g, int x, int y, int w, int h, const char* text, bool waiting);

// Animacion de espera: 5 puntos que se encienden por turnos. Llamar a menudo
// mientras se espera, con el mismo rectangulo y numero de lineas que el texto de
// show(); solo redibuja los puntos.
void waitingDots(lgfx::LovyanGFX& g, int x, int y, int w, int h, int lines = 1);

}  // namespace Message
