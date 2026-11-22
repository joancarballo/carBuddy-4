#include "apps/info/InfoApp.h"

#include <Arduino.h>

#include "net/Net.h"
#include "ui/Text.h"
#include "ui/Theme.h"

namespace {

constexpr uint32_t UPDATE_MS = 1000;

constexpr int MARGIN = 24;
constexpr int TITLE_Y = 28;
constexpr int FIRST_ROW_Y = 56;
constexpr int ROW_H = 32;
constexpr int VALUE_X = 180;
constexpr int LABEL_W = VALUE_X - MARGIN;
constexpr int VALUE_W = Screen::WIDTH - MARGIN - VALUE_X;

constexpr int ROWS = 6;
constexpr int TEXT_LEN = 48;

struct Row {
  char label[TEXT_LEN];
  char value[TEXT_LEN];
};

int rowY(int row) { return FIRST_ROW_Y + row * ROW_H; }

void set(Row& row, const char* label, const char* value) {
  snprintf(row.label, sizeof(row.label), "%s", label);
  snprintf(row.value, sizeof(row.value), "%s", value);
}

void formatKb(char* buf, size_t size, uint32_t bytes) {
  snprintf(buf, size, "%lu KB", (unsigned long)(bytes / 1024));
}

// "2 d 03:04:05" o "03:04:05" si no ha pasado un dia.
void formatUptime(char* buf, size_t size, uint32_t ms) {
  const uint32_t s = ms / 1000;
  const unsigned days = s / 86400;
  const unsigned h = (s / 3600) % 24;
  const unsigned m = (s / 60) % 60;
  const unsigned sec = s % 60;
  if (days > 0) {
    snprintf(buf, size, "%u d %02u:%02u:%02u", days, h, m, sec);
  } else {
    snprintf(buf, size, "%02u:%02u:%02u", h, m, sec);
  }
}

// Las tres primeras filas: la red.
void fillNetworkRows(Row rows[]) {
  const String ip = Net::ip();
  if (Net::isApMode()) {
    // Instrucciones para configurar la Wi-Fi desde el movil o el ordenador.
    set(rows[0], "Wi-Fi", "Sin conexión");
    set(rows[1], "Conéctate a", Net::apName().c_str());
    set(rows[2], "y abre", ("http://" + ip).c_str());
  } else if (Net::isConnected()) {
    set(rows[0], "Wi-Fi", Net::ssid().c_str());
    set(rows[1], "IP", ip.c_str());
    set(rows[2], "Web", ("http://" + Net::hostName()).c_str());
  } else {
    // Wi-Fi perdida: Net reintenta 2 minutos antes de crear la red de configuracion.
    set(rows[0], "Wi-Fi", "Reconectando...");
    set(rows[1], "IP", "-");
    set(rows[2], "Web", ("http://" + Net::hostName()).c_str());
  }
}

void fillSystemRows(Row rows[]) {
  set(rows[3], "Memoria libre", "");
  formatKb(rows[3].value, sizeof(rows[3].value), ESP.getFreeHeap());
  set(rows[4], "Bloque mayor", "");
  formatKb(rows[4].value, sizeof(rows[4].value), ESP.getMaxAllocHeap());
  set(rows[5], "Encendido", "");
  formatUptime(rows[5].value, sizeof(rows[5].value), millis());
}

}  // namespace

void InfoApp::draw(lgfx::LovyanGFX& g) {
  g.setTextDatum(textdatum_t::middle_left);
  g.setFont(&Theme::FONT_TITLE);
  g.setTextColor(Theme::TEXT);
  g.drawString("Info", MARGIN, TITLE_Y);
  drawRows(g);
}

void InfoApp::update(lgfx::LovyanGFX& g) {
  if (millis() - lastUpdateMs_ >= UPDATE_MS) {
    drawRows(g);
  }
}

// Las etiquetas tambien se redibujan: cambian al entrar o salir del modo configuracion.
void InfoApp::drawRows(lgfx::LovyanGFX& g) {
  lastUpdateMs_ = millis();
  Row rows[ROWS];
  fillNetworkRows(rows);
  fillSystemRows(rows);

  for (int i = 0; i < ROWS; i++) {
    Text::box(g, MARGIN, rowY(i), LABEL_W, ROW_H, rows[i].label, &Theme::FONT_BODY,
              Theme::TEXT_DIM, Theme::BG);
    Text::box(g, VALUE_X, rowY(i), VALUE_W, ROW_H, rows[i].value, &Theme::FONT_BODY, Theme::TEXT,
              Theme::BG, Text::Align::Right);
  }
}
