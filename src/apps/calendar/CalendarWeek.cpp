// Vista Semana del calendario: 7 columnas, de lunes a domingo de la semana en
// curso. Arriba, los eventos de todo el dia o de varios dias como barras que
// cruzan las columnas. Debajo, en cada columna, los eventos con hora en orden:
// la hora de inicio y el titulo (hasta 3 lineas). Los que ya han terminado van
// apagados; si no caben todos, "+N" al final de la columna.

#include "apps/calendar/CalendarApp.h"
#include "apps/calendar/CalendarLayout.h"
#include "ui/Text.h"
#include "ui/Theme.h"

using CalendarLayout::LINE_H;
using CalendarLayout::columnX;

namespace {

constexpr int DAYS_H = 14;      // fila con "Lun 20"
constexpr int MAX_BAR_LANES = 4;
constexpr int TEXT_LINE_H = 11; // linea de texto con FONT_TINY
constexpr int EVENT_GAP = 4;    // espacio entre eventos de una columna
constexpr int MAX_TITLE_LINES = 3;

const char* const SHORT_DAYS[] = {"Lun", "Mar", "Mié", "Jue", "Vie", "Sáb", "Dom"};

// Titulo (con emojis mini) en como mucho MAX_TITLE_LINES lineas del ancho de la
// columna; la ultima acaba en "..." si no cabe todo.
std::vector<String> titleLines(lgfx::LovyanGFX& g, const char* title, int width) {
  std::vector<String> lines = Text::wrap(g, &Theme::FONT_TINY, title, width, true);
  if ((int)lines.size() > MAX_TITLE_LINES) {
    String rest = lines[MAX_TITLE_LINES - 1];
    for (size_t i = MAX_TITLE_LINES; i < lines.size(); i++) rest += " " + lines[i];
    lines.resize(MAX_TITLE_LINES);
    lines.back() = Text::fit(g, &Theme::FONT_TINY, rest.c_str(), width, true);
  }
  return lines;
}

}  // namespace

void CalendarApp::drawWeek(lgfx::LovyanGFX& g, const struct tm& t) {
  const int top = LIST_TOP;
  g.fillRect(0, top, Screen::WIDTH, Theme::CONTENT_H - top, Theme::BG);

  const CivilTime::Minutes now = CivilTime::fromTm(t);
  const int64_t today = CivilTime::dayOf(now);
  const int64_t monday = CivilTime::weekStart(today);

  // Dias de la semana, con hoy en el color de acento.
  g.setFont(&Theme::FONT_TINY);
  g.setTextDatum(textdatum_t::top_center);
  for (int c = 0; c < 7; c++) {
    int y, m, d;
    CivilTime::civilFromDays(monday + c, y, m, d);
    char label[12];
    snprintf(label, sizeof(label), "%s %d", SHORT_DAYS[c], d);
    g.setTextColor(monday + c == today ? Theme::ACCENT : Theme::TEXT_DIM);
    g.drawString(label, (columnX(c) + columnX(c + 1)) / 2, top + 2);
    if (c > 0) g.drawFastVLine(columnX(c), top, Theme::CONTENT_H - top, Theme::DIVIDER);
  }
  g.drawFastHLine(0, top + DAYS_H, Screen::WIDTH, Theme::DIVIDER);

  // Barras de todo el dia y de varios dias.
  std::vector<CalendarLayout::Span> spans;
  int hidden[7];
  CalendarLayout::layoutWeek(range_, monday, MAX_BAR_LANES, true, spans, hidden);
  const int barsTop = top + DAYS_H + 2;
  int lanesUsed = 0;
  for (size_t i = 0; i < spans.size(); i++) {
    CalendarLayout::drawSpan(g, spans[i], barsTop + spans[i].lane * LINE_H);
    if (spans[i].lane + 1 > lanesUsed) lanesUsed = spans[i].lane + 1;
  }

  // Eventos con hora, apilados en cada columna. range_ ya viene por orden de inicio.
  const int eventsTop = barsTop + lanesUsed * LINE_H + (lanesUsed > 0 ? 2 : 0);
  for (int c = 0; c < 7; c++) {
    const int64_t day = monday + c;
    const int x = columnX(c) + 2;
    const int w = columnX(c + 1) - x - 1;
    int y = eventsTop;
    int more = hidden[c];  // barras que no cupieron arriba
    bool full = false;     // a partir del primero que no cabe, todos van al "+N"

    for (size_t i = 0; i < range_.size(); i++) {
      const RangeEvent& e = range_[i];
      if (CalendarLayout::isBar(e) || !CalendarLayout::coversDay(e, day)) continue;

      const std::vector<String> lines = titleLines(g, e.title, w);
      const int blockH = TEXT_LINE_H * (1 + (int)lines.size());
      // Se reserva una linea al final para el "+N".
      if (full || y + blockH > Theme::CONTENT_H - TEXT_LINE_H) {
        full = true;
        more++;
        continue;
      }

      const bool past = e.end <= now;
      const int minute = CivilTime::minuteOfDay(e.start);
      char hhmm[6];
      snprintf(hhmm, sizeof(hhmm), "%02d:%02d", minute / 60, minute % 60);
      g.setFont(&Theme::FONT_TINY);
      g.setTextDatum(textdatum_t::top_left);
      g.setTextColor(past ? Theme::DIVIDER : Theme::ACCENT);
      g.drawString(hhmm, x, y);
      for (size_t l = 0; l < lines.size(); l++) {
        Text::draw(g, &Theme::FONT_TINY, lines[l].c_str(), x, y + TEXT_LINE_H * (1 + (int)l) + TEXT_LINE_H / 2,
                   past ? Theme::TEXT_DIM : Theme::TEXT, Theme::BG, true);
      }
      y += blockH + EVENT_GAP;
    }

    if (more > 0) {
      char text[6];
      snprintf(text, sizeof(text), "+%d", more);
      g.setTextColor(Theme::TEXT_DIM);
      g.setTextDatum(textdatum_t::top_left);
      g.drawString(text, x, Theme::CONTENT_H - TEXT_LINE_H);
    }
  }
}

// Toda la columna (debajo de la cabecera) es la zona tactil de su dia.
int64_t CalendarApp::weekDayAt(int x, int y, const struct tm& t) const {
  if (y < LIST_TOP) {
    return -1;
  }
  int col = x * 7 / Screen::WIDTH;
  if (col > 6) col = 6;
  return CivilTime::weekStart(CivilTime::dayOf(CivilTime::fromTm(t))) + col;
}
