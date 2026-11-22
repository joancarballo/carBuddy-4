#include "apps/calendar/CalendarLayout.h"

#include "ui/Text.h"
#include "ui/Theme.h"

namespace {

const uint16_t BAR = lgfx::color565(52, 92, 66);  // fondo de las barras de evento

}  // namespace

namespace CalendarLayout {

int64_t firstDay(const RangeEvent& e) { return CivilTime::dayOf(e.start); }

int64_t lastDay(const RangeEvent& e) {
  return e.end > e.start ? CivilTime::dayOf(e.end - 1) : CivilTime::dayOf(e.start);
}

bool coversDay(const RangeEvent& e, int64_t day) {
  return firstDay(e) <= day && lastDay(e) >= day;
}

bool isBar(const RangeEvent& e) { return e.allDay || firstDay(e) != lastDay(e); }

void layoutWeek(const std::vector<RangeEvent>& range, int64_t monday, int maxLanes, bool onlyBars,
                std::vector<Span>& spans, int hidden[7]) {
  spans.clear();
  for (int c = 0; c < 7; c++) hidden[c] = 0;
  if (maxLanes <= 0) maxLanes = 0;

  // used[linea] = bits de las columnas ocupadas en esa linea.
  std::vector<uint8_t> used(maxLanes, 0);
  // range ya viene ordenado por inicio y, a igual inicio, los mas largos primero.
  for (size_t i = 0; i < range.size(); i++) {
    const RangeEvent& e = range[i];
    if (onlyBars && !isBar(e)) continue;
    const int64_t first = firstDay(e);
    const int64_t last = lastDay(e);
    if (last < monday || first > monday + 6) continue;

    const int colStart = first < monday ? 0 : (int)(first - monday);
    const int colEnd = last > monday + 6 ? 6 : (int)(last - monday);
    uint8_t mask = 0;
    for (int c = colStart; c <= colEnd; c++) mask |= (uint8_t)(1 << c);

    int lane = -1;
    for (int l = 0; l < maxLanes && lane < 0; l++) {
      if ((used[l] & mask) == 0) lane = l;
    }
    if (lane < 0) {
      for (int c = colStart; c <= colEnd; c++) hidden[c]++;
      continue;
    }
    used[lane] |= mask;
    spans.push_back(Span{&e, colStart, colEnd, lane});
  }
}

void drawSpan(lgfx::LovyanGFX& g, const Span& span, int y) {
  const int x = columnX(span.colStart) + 1;
  const int w = columnX(span.colEnd + 1) - 1 - x;
  g.fillRect(x, y, w, LINE_H - 1, BAR);
  g.setClipRect(x + 1, y, w - 2, LINE_H - 1);
  Text::draw(g, &Theme::FONT_TINY, span.event->title, x + 2, y + (LINE_H - 1) / 2, Theme::TEXT, BAR, true);
  g.clearClipRect();
}

}  // namespace CalendarLayout
