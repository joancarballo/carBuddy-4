#pragma once
// Colores, fuentes y zonas de la pantalla, comunes a todas las apps.

#include "hal/Screen.h"

namespace Theme {

// Zonas: contenido de la app arriba y barra de pestañas abajo.
constexpr int TABBAR_H = 52;
constexpr int TABBAR_Y = Screen::HEIGHT - TABBAR_H;
constexpr int CONTENT_H = TABBAR_Y;

// Fondo oscuro y verde menta de acento (el de la web de la v1, #A9DCB4).
constexpr uint16_t BG = lgfx::color565(18, 22, 26);
constexpr uint16_t TEXT = lgfx::color565(235, 240, 236);
constexpr uint16_t TEXT_DIM = lgfx::color565(140, 150, 146);
constexpr uint16_t ACCENT = lgfx::color565(169, 220, 180);
constexpr uint16_t TABBAR_BG = lgfx::color565(30, 36, 42);
constexpr uint16_t DIVIDER = lgfx::color565(60, 68, 74);

// Fuentes de U8g2 con el juego de caracteres completo (acentos, ñ, ¿, ¡).
extern const lgfx::U8g2font FONT_SMALL;
extern const lgfx::U8g2font FONT_BODY;
extern const lgfx::U8g2font FONT_TITLE;
extern const lgfx::U8g2font FONT_TAB;
extern const lgfx::U8g2font FONT_HEADER;
// Pequeña (6x10), para las vistas Semana y Mes del calendario.
extern const lgfx::U8g2font FONT_TINY;
// Solo cifras y ':' (para la hora grande).
extern const lgfx::U8g2font FONT_CLOCK;

}  // namespace Theme
