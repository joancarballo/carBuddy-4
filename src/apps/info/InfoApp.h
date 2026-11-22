#pragma once
// Pantalla Info: Wi-Fi, IP y direccion de la web de configuracion, memoria libre
// y tiempo encendido. Todo se actualiza cada segundo. En modo configuracion
// (sin Wi-Fi) las filas de la red explican como conectarse a la placa.

#include "appmanager/App.h"

class InfoApp : public App {
 public:
  const char* name() const override { return "Info"; }
  void draw(lgfx::LovyanGFX& g) override;
  void update(lgfx::LovyanGFX& g) override;

 private:
  void drawRows(lgfx::LovyanGFX& g);
  uint32_t lastUpdateMs_ = 0;
};
