#pragma once
// App Hora: hora en grande (HH:MM, 24 h) y debajo la fecha ("Sábado 10/10/26").
// Si hoy hay un evento destacado en el calendario, su emoji justo encima de la
// hora (ver services/calendar/SpecialEvents.h).
// Sin hora sincronizada, "Sin hora" con la animacion de espera.
// Su seccion de la web ("Hora") esta en web/ClockSection.cpp.

#include "appmanager/App.h"

class ClockApp : public App {
 public:
  const char* name() const override { return "Hora"; }
  void draw(lgfx::LovyanGFX& g) override;
  void update(lgfx::LovyanGFX& g) override;

 private:
  void drawTime(lgfx::LovyanGFX& g, const struct tm& t);
  // Dibuja o borra el emoji del evento destacado si ha cambiado (o siempre, con force).
  void drawHighlight(lgfx::LovyanGFX& g, const struct tm& t, bool force);
  bool synced_ = false;
  int shownMinute_ = -1;
  String highlightShown_;                   // emoji en pantalla ("" si ninguno)
  uint32_t highlightVersion_ = 0xFFFFFFFF;  // version de los datos del calendario consultada
  int64_t highlightDay_ = -1;               // dia consultado
};
