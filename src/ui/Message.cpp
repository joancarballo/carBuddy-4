#include "ui/Message.h"

#include <Arduino.h>

#include "ui/Theme.h"

namespace {

constexpr int DOTS = 5;
constexpr int DOT_R = 5;
constexpr int DOT_GAP = 20;          // distancia entre centros
constexpr int DOTS_H = 32;  // espacio para los puntos debajo del texto
constexpr uint32_t DOT_STEP_MS = 250;
constexpr int LINE_H = 26;
constexpr int MAX_LINE = 64;

// El texto y, si los hay, los puntos forman un bloque centrado en el rectangulo.
int blockTop(int y, int h, int lines, bool waiting) {
  return y + (h - lines * LINE_H - (waiting ? DOTS_H : 0)) / 2;
}

}  // namespace

namespace Message {

void show(lgfx::LovyanGFX& g, int x, int y, int w, int h, const char* text, bool waiting) {
  g.fillRect(x, y, w, h, Theme::BG);
  g.setFont(&Theme::FONT_BODY);
  g.setTextColor(Theme::TEXT);
  g.setTextDatum(textdatum_t::middle_center);

  int lines = 1;
  for (const char* c = text; *c; c++) {
    if (*c == '\n') lines++;
  }
  int lineY = blockTop(y, h, lines, waiting) + LINE_H / 2;
  const char* start = text;
  while (true) {
    const char* end = strchr(start, '\n');
    const size_t len = end ? (size_t)(end - start) : strlen(start);
    char line[MAX_LINE];
    snprintf(line, sizeof(line), "%.*s", (int)len, start);
    g.drawString(line, x + w / 2, lineY);
    if (!end) break;
    start = end + 1;
    lineY += LINE_H;
  }
}

void waitingDots(lgfx::LovyanGFX& g, int x, int y, int w, int h, int lines) {
  const int cy = blockTop(y, h, lines, true) + lines * LINE_H + DOTS_H / 2;
  const int x0 = x + w / 2 - (DOTS - 1) * DOT_GAP / 2;
  const int lit = (millis() / DOT_STEP_MS) % DOTS;
  for (int i = 0; i < DOTS; i++) {
    g.fillCircle(x0 + i * DOT_GAP, cy, DOT_R, i == lit ? Theme::ACCENT : Theme::DIVIDER);
  }
}

}  // namespace Message
