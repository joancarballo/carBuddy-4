#include "appmanager/AppManager.h"

#include <Arduino.h>

#include "appmanager/App.h"
#include "apps/calendar/CalendarApp.h"
#include "apps/clock/ClockApp.h"
#include "apps/info/InfoApp.h"
#include "net/Net.h"
#include "ui/TabBar.h"
#include "ui/Theme.h"

namespace {

constexpr int MAX_APPS = 6;

ClockApp clockApp;
CalendarApp calendarApp;
InfoApp infoApp;

App* apps[MAX_APPS];
const char* labels[MAX_APPS];
int appCount = 0;
int current = 0;
bool wasApMode = false;  // para saber cuando se crea la red de configuracion

void drawCurrent() {
  LGFX& g = Screen::gfx();
  g.fillRect(0, 0, Screen::WIDTH, Theme::CONTENT_H, Theme::BG);
  apps[current]->draw(g);
  TabBar::draw(g, labels, appCount, current);
}

void show(int index) {
  if (index == current) return;
  apps[current]->onHide();
  current = index;
  apps[current]->onShow();
  drawCurrent();
  Serial.printf("App: %s\n", apps[current]->name());
}

int indexOf(const App* app) {
  for (int i = 0; i < appCount; i++) {
    if (apps[i] == app) return i;
  }
  return 0;
}

// Si la placa acaba de crear la red de configuracion (por ejemplo, tras 2 minutos
// sin Wi-Fi), se muestra Info, que explica como conectarse a ella.
void showInfoOnApMode() {
  const bool apMode = Net::isApMode();
  if (apMode && !wasApMode) {
    show(indexOf(&infoApp));
  }
  wasApMode = apMode;
}

}  // namespace

namespace AppManager {

void begin() {
  apps[0] = &clockApp;
  apps[1] = &calendarApp;
  apps[2] = &infoApp;
  appCount = 3;
  for (int i = 0; i < appCount; i++) {
    labels[i] = apps[i]->name();
  }

  wasApMode = Net::isApMode();
  current = wasApMode ? indexOf(&infoApp) : 0;
  apps[current]->onShow();
  drawCurrent();
  Serial.printf("App inicial: %s\n", apps[current]->name());
}

void handleTap(int x, int y) {
  const int tab = TabBar::hitTest(x, y, appCount);
  if (tab >= 0) {
    show(tab);
  } else {
    apps[current]->onTap(x, y);
  }
}

void update() {
  showInfoOnApMode();
  apps[current]->update(Screen::gfx());
}

}  // namespace AppManager
