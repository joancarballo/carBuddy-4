#pragma once
// Emojis a color dentro del texto. Las fuentes de U8g2 no tienen emojis: cada uno
// es una imagen de Twemoji en la flash, en dos tamaños: SIZE x SIZE y, para la
// letra diminuta de las vistas Semana y Mes, MINI_SIZE x MINI_SIZE
// (EmojiData.cpp, generado con tools/emoji/gen_emoji.py).
//
// Como en la v1: los tonos de piel y los selectores de variante se ignoran, de
// una secuencia con ZWJ (👨‍👩‍👧) solo se dibuja el primero, y un emoji que no
// esta en la tabla se dibuja como un punto.

#include <stddef.h>
#include <stdint.h>

#include "hal/Screen.h"

namespace Emoji {

constexpr int SIZE = 18;
constexpr int MINI_SIZE = 9;

// Ancho que ocupa en el texto, con 1 px a cada lado.
inline int advance(bool mini) { return (mini ? MINI_SIZE : SIZE) + 2; }
constexpr int ADVANCE = SIZE + 2;

struct Image {
  uint32_t codepoint;
  uint32_t second;  // segunda letra regional de una bandera, o 0
  const uint16_t* rgb;
  const uint8_t* alpha;
  const uint16_t* miniRgb;
  const uint8_t* miniAlpha;
};

// Tabla generada, ordenada por (codepoint, second).
extern const Image IMAGES[];
extern const size_t IMAGE_COUNT;

// Si s empieza por un emoji (con sus modificadores y lo que le siga unido con
// ZWJ), devuelve cuantos bytes ocupa y deja en image su dibujo, o nullptr si no
// esta en la tabla. Devuelve 0 si s no empieza por un emoji.
size_t parse(const char* s, const Image** image);

// Dibuja el emoji con su lado izquierdo en x y centrado en vertical en cy,
// mezclando los bordes con el color de fondo bg. mini: el de MINI_SIZE.
void draw(lgfx::LovyanGFX& g, int x, int cy, const Image* image, uint16_t bg, bool mini = false);

// Como draw(), pero cada pixel del emoji de SIZE x SIZE se dibuja como un cuadrado
// de zoom x zoom (zoom = 2 da 36 px). Sin buffer: solo para emojis sueltos y grandes,
// que se dibujan de tarde en tarde. Ocupa advance(zoom) de ancho (un px a cada lado).
inline int zoomAdvance(int zoom) { return SIZE * zoom + 2; }
void drawZoom(lgfx::LovyanGFX& g, int x, int cy, const Image* image, uint16_t bg, int zoom);

// Lee un caracter UTF-8 de s: devuelve sus bytes (al menos 1) y su punto de codigo.
size_t decodeUtf8(const char* s, uint32_t& codepoint);

}  // namespace Emoji
