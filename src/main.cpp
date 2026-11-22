// carBuddy-4 - carBuddy-2 adaptado al CYD de 4" (480x320)
//
// Estructura del proyecto:
//   src/hal/        hardware: pines, pantalla y tactil (LovyanGFX), LED
//   src/ui/         tema (colores, fuentes, zonas), texto sin parpadeo, mensajes, pestañas
//   src/net/        Wi-Fi, red de configuracion y mDNS
//   src/web/        web de configuracion, una seccion por tema
//   src/services/   hora (SNTP y huso horario), calendario (descarga del iCal) y
//                   pantalla (brillo y apagado por horario)
//   src/appmanager/ interfaz de las apps y gestor (orden, app activa, toques)
//   src/apps/       una carpeta por app

#include <Arduino.h>

#include "appmanager/AppManager.h"
#include "hal/Led.h"
#include "hal/Screen.h"
#include "hal/Touch.h"
#include "net/Net.h"
#include "services/NightMode.h"
#include "services/TimeService.h"
#include "services/calendar/CalendarService.h"
#include "services/calendar/SpecialEvents.h"
#include "ui/Message.h"
#include "ui/Theme.h"
#include "web/Web.h"

// Mientras Net::begin() espera a la Wi-Fi (hasta 15 s), anima los puntos.
static void animateConnecting() {
  Message::waitingDots(Screen::gfx(), 0, 0, Screen::WIDTH, Screen::HEIGHT);
}

static void printHeap(const char* when) {
  Serial.printf("Heap libre %s: %lu bytes (bloque mayor: %lu)\n", when,
                (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMaxAllocHeap());
}

// Datos de la placa en el monitor serie, para comprobar que es la esperada
// (ESP32-D0WD-V3, 4 MB de flash y sin PSRAM).
static void printBoardInfo() {
  Serial.printf("Chip: %s rev %d, %d MHz, flash %lu MB, PSRAM %lu bytes\n", ESP.getChipModel(),
                (int)ESP.getChipRevision(), (int)ESP.getCpuFreqMHz(),
                (unsigned long)(ESP.getFlashChipSize() / (1024 * 1024)), (unsigned long)ESP.getPsramSize());
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("carBuddy-4");
  printBoardInfo();

  Led::begin();
  Screen::begin();
  printHeap("antes de la wifi");

  LGFX& g = Screen::gfx();
  g.fillScreen(Theme::BG);
  Message::show(g, 0, 0, Screen::WIDTH, Screen::HEIGHT, "Conectando Wi-Fi", true);
  Net::setWaitCallback(animateConnecting);
  Net::begin();

  TimeService::begin();
  NightMode::begin();
  SpecialEvents::begin();
  CalendarService::begin();
  AppManager::begin();
  Web::begin();
  printHeap("con la wifi y la web");
}

void loop() {
  Net::loop();
  Web::loop();

  NightMode::loop();

  int x = 0, y = 0;
  if (Touch::pollTap(x, y)) {
    Serial.printf("Toque: %d, %d\n", x, y);
    // Con la pantalla apagada por horario, el toque solo la enciende.
    if (!NightMode::handleTap()) {
      AppManager::handleTap(x, y);
    }
  }
  AppManager::update();
  delay(5);
}
