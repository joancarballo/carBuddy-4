#pragma once
// Barra de pestañas fija en la parte de abajo: una pestaña por app.

#include "hal/Screen.h"

namespace TabBar {

void draw(lgfx::LovyanGFX& g, const char* const labels[], int count, int active);

// Pestaña que hay en (x, y), o -1 si el punto no esta en la barra.
int hitTest(int x, int y, int count);

}  // namespace TabBar
