// Vista Mes del calendario: cuadricula de lunes a domingo con las semanas del
// mes en curso. Cada dia lleva su numero y barras de evento; un evento de varios
// dias es una barra que cruza los dias que ocupa, con el titulo al principio de
// la barra en cada semana. Si en un dia no caben todos, "+N" en su esquina.
// Con 480x320 caben 3, 2 o 2 barras por dia con 4, 5 o 6 semanas.

#include "apps/calendar/CalendarApp.h"
#include "apps/calendar/CalendarLayout.h"
#include "ui/Theme.h"

using CalendarLayout::LINE_H;
using CalendarLayout::columnX;

namespace {

constexpr int WEEKDAYS_H = 12;  // fila con L M X J V S D
constexpr int NUMBER_H = 11;    // linea del numero del dia

const char* const WEEKDAY_LETTERS[] = {"L", "M", "X", "J", "V", "S", "D"};
const uint16_t OTHER_MONTH = lgfx::color565(90, 98, 104);  // numero de los dias de otro mes

}  // namespace

void CalendarApp::drawMonth(lgfx::LovyanGFX& g, const struct tm& t) {
  const int top = LIST_TOP;
  g.fillRect(0, top, Screen::WIDTH, Theme::CONTENT_H - top, Theme::BG);

  const int64_t today = CivilTime::dayOf(CivilTime::fromTm(t));
  int64_t first;
  int dayCount;
  CivilTime::monthGrid(today, first, dayCount);
  const int weeks = dayCount / 7;

  g.setFont(&Theme::FONT_TINY);
  g.setTextDatum(textdatum_t::top_center);
  g.setTextColor(Theme::TEXT_DIM);
  for (int c = 0; c < 7; c++) {
    g.drawString(WEEKDAY_LETTERS[c], (columnX(c) + columnX(c + 1)) / 2, top + 1);
  }

  // El alto se reparte entre las semanas: con 4, 5 o 6 semanas caben 3, 2 o 1
  // barras por dia.
  const int gridTop = top + WEEKDAYS_H;
  const int rowH = (Theme::CONTENT_H - gridTop) / weeks;
  const int lanes = (rowH - NUMBER_H - 1) / LINE_H;

  std::vector<CalendarLayout::Span> spans;
  int hidden[7];
  for (int w = 0; w < weeks; w++) {
    const int rowY = gridTop + w * rowH;
    const int64_t monday = first + w * 7;
    g.drawFastHLine(0, rowY, Screen::WIDTH, Theme::DIVIDER);

    CalendarLayout::layoutWeek(range_, monday, lanes, false, spans, hidden);

    for (int c = 0; c < 7; c++) {
      const int64_t day = monday + c;
      const int x = columnX(c);
      const int cw = columnX(c + 1) - x;
      if (c > 0) g.drawFastVLine(x, rowY, rowH, Theme::DIVIDER);

      int y, m, d;
      CivilTime::civilFromDays(day, y, m, d);
      char num[4];
      snprintf(num, sizeof(num), "%d", d);
      g.setFont(&Theme::FONT_TINY);
      g.setTextDatum(textdatum_t::top_left);
      g.setTextColor(day == today ? Theme::ACCENT : (m == t.tm_mon + 1 ? Theme::TEXT : OTHER_MONTH));
      g.drawString(num, x + 3, rowY + 2);

      if (hidden[c] > 0) {
        char more[6];
        snprintf(more, sizeof(more), "+%d", hidden[c]);
        g.setTextDatum(textdatum_t::top_right);
        g.setTextColor(Theme::TEXT_DIM);
        g.drawString(more, x + cw - 2, rowY + 2);
      }
    }

    for (size_t i = 0; i < spans.size(); i++) {
      CalendarLayout::drawSpan(g, spans[i], rowY + NUMBER_H + 1 + spans[i].lane * LINE_H);
    }
  }

  // Hoy, enmarcado en el color de acento (al final, para que quede por encima).
  if (today >= first && today < first + dayCount) {
    const int index = (int)(today - first);
    const int x = columnX(index % 7);
    const int y = gridTop + (index / 7) * rowH;
    g.drawRect(x, y, columnX(index % 7 + 1) - x + 1, rowH + 1, Theme::ACCENT);
  }
}

// Cada casilla es la zona tactil de su dia (con las mismas medidas que drawMonth()).
int64_t CalendarApp::monthDayAt(int x, int y, const struct tm& t) const {
  const int64_t today = CivilTime::dayOf(CivilTime::fromTm(t));
  int64_t first;
  int dayCount;
  CivilTime::monthGrid(today, first, dayCount);
  const int weeks = dayCount / 7;
  const int gridTop = LIST_TOP + WEEKDAYS_H;
  const int rowH = (Theme::CONTENT_H - gridTop) / weeks;
  if (y < gridTop || y >= gridTop + weeks * rowH) {
    return -1;
  }
  int col = x * 7 / Screen::WIDTH;
  if (col > 6) col = 6;
  return first + ((y - gridTop) / rowH) * 7 + col;
}
