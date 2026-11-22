#include "apps/clock/ClockApp.h"

#include <time.h>

#include "services/TimeService.h"
#include "services/calendar/CalendarService.h"
#include "services/calendar/SpecialEvents.h"
#include "ui/Emoji.h"
#include "ui/Message.h"
#include "ui/Text.h"
#include "ui/Theme.h"

namespace {

// Zona de contenido de 480x268: el bloque (emoji, hora y fecha) ocupa 194 px, centrado.
constexpr int HIGHLIGHT_ZOOM = 2;   // el emoji del evento destacado se dibuja a 36 px
constexpr int HIGHLIGHT_CY = 55;    // su centro, justo encima de la hora
constexpr int TIME_Y = 77;
constexpr int TIME_H = 102;
constexpr int DATE_Y = 191;
constexpr int DATE_H = 40;
constexpr int BOX_PADDING = 8;

const char* const WAITING_TEXT = "Sin hora\nEsperando a internet";
constexpr int WAITING_LINES = 2;

}  // namespace

void ClockApp::draw(lgfx::LovyanGFX& g) {
  struct tm t;
  synced_ = TimeService::now(t);
  if (!synced_) {
    Message::show(g, 0, 0, Screen::WIDTH, Theme::CONTENT_H, WAITING_TEXT, true);
    return;
  }
  drawTime(g, t);
  drawHighlight(g, t, true);
}

void ClockApp::update(lgfx::LovyanGFX& g) {
  struct tm t;
  const bool synced = TimeService::now(t);
  if (synced != synced_) {
    // Acaba de sincronizarse (o se ha perdido la hora): se redibuja todo.
    g.fillRect(0, 0, Screen::WIDTH, Theme::CONTENT_H, Theme::BG);
    draw(g);
    return;
  }
  if (!synced) {
    Message::waitingDots(g, 0, 0, Screen::WIDTH, Theme::CONTENT_H, WAITING_LINES);
    return;
  }
  if (t.tm_min != shownMinute_) {
    drawTime(g, t);
  }
  drawHighlight(g, t, false);
}

// Emoji del evento destacado encima de la hora, solo si hoy hay uno en el
// calendario. Se consulta al cambiar de dia o de datos (tambien al guardar los
// eventos especiales en la web, que avisa con CalendarService::notifyChanged()).
void ClockApp::drawHighlight(lgfx::LovyanGFX& g, const struct tm& t, bool force) {
  const uint32_t version = CalendarService::version();
  const int64_t today = CivilTime::dayOf(CivilTime::fromTm(t));
  if (!force && version == highlightVersion_ && today == highlightDay_) {
    return;
  }
  highlightVersion_ = version;
  highlightDay_ = today;
  const String show = CalendarService::hasHighlightEvent(today) ? SpecialEvents::highlightEmoji() : String();
  if (!force && show == highlightShown_) {
    return;
  }
  highlightShown_ = show;

  const int advance = Emoji::zoomAdvance(HIGHLIGHT_ZOOM);
  const int x = (Screen::WIDTH - advance) / 2;
  g.fillRect(x, HIGHLIGHT_CY - Emoji::SIZE * HIGHLIGHT_ZOOM / 2, advance, Emoji::SIZE * HIGHLIGHT_ZOOM, Theme::BG);
  if (show.length() > 0) {
    const Emoji::Image* image = nullptr;
    Emoji::parse(show.c_str(), &image);
    Emoji::drawZoom(g, x, HIGHLIGHT_CY, image, Theme::BG, HIGHLIGHT_ZOOM);
  }
}

// La hora y la fecha se redibujan juntas una vez por minuto, sin parpadeo.
void ClockApp::drawTime(lgfx::LovyanGFX& g, const struct tm& t) {
  shownMinute_ = t.tm_min;

  char hhmm[6];
  snprintf(hhmm, sizeof(hhmm), "%02d:%02d", t.tm_hour, t.tm_min);
  // El sprite de la hora se ajusta al ancho de la hora, no a toda la pantalla,
  // para pedir menos memoria (es el mayor de la interfaz).
  g.setFont(&Theme::FONT_CLOCK);
  const int timeW = g.textWidth("00:00") + 2 * BOX_PADDING;
  Text::box(g, (Screen::WIDTH - timeW) / 2, TIME_Y, timeW, TIME_H, hhmm, &Theme::FONT_CLOCK,
            Theme::TEXT, Theme::BG, Text::Align::Center);

  Text::box(g, 0, DATE_Y, Screen::WIDTH, DATE_H, TimeService::dateText().c_str(),
            &Theme::FONT_TITLE, Theme::TEXT_DIM, Theme::BG, Text::Align::Center);
}
