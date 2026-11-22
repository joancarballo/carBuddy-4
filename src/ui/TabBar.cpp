#include "ui/TabBar.h"

#include "ui/Theme.h"

namespace {

constexpr int ACTIVE_LINE_H = 4;  // linea de color encima de la pestaña activa

}  // namespace

namespace TabBar {

void draw(lgfx::LovyanGFX& g, const char* const labels[], int count, int active) {
  const int y = Theme::TABBAR_Y;
  g.fillRect(0, y, Screen::WIDTH, Theme::TABBAR_H, Theme::TABBAR_BG);
  g.drawFastHLine(0, y, Screen::WIDTH, Theme::DIVIDER);

  g.setFont(&Theme::FONT_TAB);
  g.setTextDatum(textdatum_t::middle_center);
  for (int i = 0; i < count; i++) {
    const int x0 = i * Screen::WIDTH / count;
    const int x1 = (i + 1) * Screen::WIDTH / count;
    if (i == active) {
      g.fillRect(x0, y + 1, x1 - x0, ACTIVE_LINE_H, Theme::ACCENT);
    }
    g.setTextColor(i == active ? Theme::ACCENT : Theme::TEXT_DIM);
    g.drawString(labels[i], (x0 + x1) / 2, y + (Theme::TABBAR_H + ACTIVE_LINE_H) / 2);
  }
}

int hitTest(int x, int y, int count) {
  if (y < Theme::TABBAR_Y || count <= 0) return -1;
  int i = x * count / Screen::WIDTH;
  if (i < 0) i = 0;
  if (i >= count) i = count - 1;
  return i;
}

}  // namespace TabBar
