#pragma once
// Texto con emojis a color: medida, dibujo, dibujo sin parpadeo, recorte con
// "..." y division en lineas.
//
// Las fuentes solo tienen Latin-1: las comillas tipograficas, las rayas, los
// puntos suspensivos y el euro se cambian por su equivalente sencillo
// (' " - ... EUR), y los emojis se dibujan con ui/Emoji.

#include <Arduino.h>

#include <vector>

#include "hal/Screen.h"

namespace Text {

enum class Align { Left, Center, Right };

// Cambia los caracteres que la fuente no tiene por su equivalente sencillo.
String normalize(const char* text);

// En width(), draw(), fit() y wrap(), miniEmoji usa los emojis de 9 px (para la
// fuente diminuta de las vistas Semana y Mes) en vez de los de 18.

// Ancho en pixeles del texto con esa fuente, contando los emojis.
int width(lgfx::LovyanGFX& g, const lgfx::IFont* font, const char* text, bool miniEmoji = false);

// Dibuja el texto con su lado izquierdo en x y centrado en vertical en cy.
// bg es el color de fondo, para mezclar los bordes de los emojis.
void draw(lgfx::LovyanGFX& g, const lgfx::IFont* font, const char* text, int x, int cy,
          uint16_t fg, uint16_t bg, bool miniEmoji = false);

// Dibuja un texto centrado en vertical dentro de un rectangulo, borrando lo que
// hubiera. Se compone en un sprite del tamaño del rectangulo y se envia de una
// vez, asi el valor no parpadea al cambiar (hora, memoria...).
void box(lgfx::LovyanGFX& g, int x, int y, int w, int h, const char* text,
         const lgfx::IFont* font, uint16_t fg, uint16_t bg, Align align = Align::Left);

// Recorta el texto para que quepa en maxWidth pixeles con esa fuente, acabando
// en "..." si no cabe entero. No parte caracteres UTF-8 ni emojis.
String fit(lgfx::LovyanGFX& g, const lgfx::IFont* font, const char* text, int maxWidth,
           bool miniEmoji = false);

// Parte el texto en lineas de como mucho maxWidth pixeles, por palabras ('\n'
// fuerza un salto). Una palabra que no cabe en una linea se parte por letras.
std::vector<String> wrap(lgfx::LovyanGFX& g, const lgfx::IFont* font, const char* text,
                         int maxWidth, bool miniEmoji = false);

}  // namespace Text
