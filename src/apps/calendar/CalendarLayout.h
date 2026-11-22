#pragma once
// Lo que comparten las vistas Semana y Mes del calendario: dias que ocupa un
// evento, colocacion de las barras en lineas sin que se solapen y dibujo de una barra.

#include <Arduino.h>

#include <vector>

#include "hal/Screen.h"
#include "services/calendar/CalendarEvent.h"

namespace CalendarLayout {

constexpr int TINY_H = 10;  // alto de la fuente FONT_TINY (6x10)
constexpr int LINE_H = 12;  // una linea de barra: la fuente y 2 px

// Primer y ultimo dia que ocupa el evento (el fin de iCal no se incluye).
int64_t firstDay(const RangeEvent& e);
int64_t lastDay(const RangeEvent& e);

bool coversDay(const RangeEvent& e, int64_t day);

// Eventos de todo el dia o de varios dias: van como barras que cruzan columnas.
bool isBar(const RangeEvent& e);

// Una barra dentro de una semana: columnas que ocupa (0 = lunes) y linea.
struct Span {
  const RangeEvent* event;
  int colStart;
  int colEnd;
  int lane;
};

// Coloca en lineas las barras de los eventos que tocan la semana que empieza en
// monday, cada una en la primera linea libre en todos sus dias, como mucho
// maxLanes lineas. Si onlyBars es true solo entran los eventos isBar(). Los que
// no caben se cuentan en hidden[columna].
void layoutWeek(const std::vector<RangeEvent>& range, int64_t monday, int maxLanes, bool onlyBars,
                std::vector<Span>& spans, int hidden[7]);

// Borde izquierdo de la columna col (0..7) de las 7 en que se divide la pantalla.
inline int columnX(int col) { return col * Screen::WIDTH / 7; }

// Dibuja la barra de un Span en la fila que empieza en y, con el titulo (y sus
// emojis mini) al principio: asi, en una barra que cruza semanas, el titulo sale
// al principio de cada semana.
void drawSpan(lgfx::LovyanGFX& g, const Span& span, int y);

}  // namespace CalendarLayout
