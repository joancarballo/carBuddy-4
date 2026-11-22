// Detalle de un evento del calendario: titulo, dia y horas, calendario (si hay
// varios), lugar, repeticion y notas, en lineas que se recorren con las flechas.
// Arriba a la izquierda, "< Volver" vuelve a la lista.

#include "apps/calendar/CalendarApp.h"

#include "ui/Text.h"
#include "ui/Theme.h"

namespace {

constexpr int TOP_BAR_H = 44;   // barra con "< Volver"
constexpr int BACK_W = 140;     // zona tactil de "< Volver"
constexpr int LINES_TOP = 48;
constexpr int LINE_H = 24;
constexpr int VISIBLE_LINES = (Theme::CONTENT_H - LINES_TOP) / LINE_H;

const char* const SHORT_DAYS[] = {"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"};

// "Jue 30/10".
String dateText(int64_t day) {
  int y, m, d;
  CivilTime::civilFromDays(day, y, m, d);
  char buf[16];
  snprintf(buf, sizeof(buf), "%s %02d/%02d", SHORT_DAYS[CivilTime::weekday(day)], d, m);
  return buf;
}

// "Hoy", "Mañana" o "Jue 30/10".
String dayText(int64_t day, int64_t today) {
  if (day == today) return "Hoy";
  if (day == today + 1) return "Mañana";
  return dateText(day);
}

String clockText(CivilTime::Minutes t) {
  const int m = CivilTime::minuteOfDay(t);
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", m / 60, m % 60);
  return buf;
}

// "Hoy, 10:00 - 11:00", "Jue 30/10, todo el día" o, si dura varios días,
// "Del Lun 20/10 al Jue 23/10" (con las horas si no es de todo el día; siempre
// con fechas, que "Del Lun 20/10 al Hoy" se lee mal).
String whenText(const CalendarEvent& ev, int64_t today) {
  const int64_t first = CivilTime::dayOf(ev.start);
  const int64_t last = ev.end > ev.start ? CivilTime::dayOf(ev.end - 1) : first;
  if (first != last) {
    if (ev.allDay) {
      return "Del " + dateText(first) + " al " + dateText(last);
    }
    return "Del " + dateText(first) + " " + clockText(ev.start) + " al " + dateText(last) + " " +
           clockText(ev.end);
  }
  const String day = dayText(ev.day, today);
  if (ev.allDay) {
    return day + ", todo el día";
  }
  return day + ", " + clockText(ev.start) + " - " + clockText(ev.end);
}

}  // namespace

void CalendarApp::openDetail(lgfx::LovyanGFX& g, const CalendarEvent& ev, int64_t today) {
  const int textW = CONTENT_W - 2 * PAD;
  detailLines_.clear();

  // El titulo va en negrita y en el color de acento para distinguirlo del resto.
  // Las lineas se parten con la misma fuente con la que se dibujan.
  for (const String& line : Text::wrap(g, &Theme::FONT_TAB, ev.title.c_str(), textW)) {
    detailLines_.push_back(DetailLine{line, true});
  }
  std::vector<String> body;
  body.push_back(whenText(ev, today));
  // Con varios calendarios se indica de cual viene.
  if (CalendarService::calendars().size() > 1 && !ev.calendar.empty()) {
    body.push_back(String("Calendario: ") + ev.calendar.c_str());
  }
  if (!ev.location.empty()) {
    body.push_back(String("Lugar: ") + ev.location.c_str());
  }
  if (!ev.repeat.empty()) {
    body.push_back(String("Repite: ") + ev.repeat.c_str());
  }
  if (!ev.notes.empty()) {
    body.push_back("");
    body.push_back(ev.notes.c_str());
  } else if (ev.day != today && ev.day != today + 1) {
    // Para los demas dias solo se guarda una version reducida de los eventos.
    body.push_back("");
    body.push_back("Las notas y el lugar solo se ven en los eventos de hoy y mañana.");
  }
  for (const String& text : body) {
    for (const String& line : Text::wrap(g, &Theme::FONT_BODY, text.c_str(), textW)) {
      detailLines_.push_back(DetailLine{line, false});
    }
  }

  detailTop_ = 0;
  detailOpen_ = true;
  g.fillRect(0, 0, Screen::WIDTH, Theme::CONTENT_H, Theme::BG);
  drawDetail(g);
}

void CalendarApp::closeDetail(lgfx::LovyanGFX& g) {
  detailOpen_ = false;
  detailLines_.clear();
  g.fillRect(0, 0, Screen::WIDTH, Theme::CONTENT_H, Theme::BG);
  drawContent(g);
}

void CalendarApp::drawDetail(lgfx::LovyanGFX& g) {
  // "< Volver": la fuente no tiene el caracter ‹, asi que se usa "<".
  g.setFont(&Theme::FONT_TAB);
  g.setTextColor(Theme::ACCENT);
  g.setTextDatum(textdatum_t::middle_left);
  g.drawString("< Volver", PAD, TOP_BAR_H / 2);
  g.drawFastHLine(0, TOP_BAR_H, Screen::WIDTH, Theme::DIVIDER);

  for (int i = 0; i < VISIBLE_LINES; i++) {
    const int line = detailTop_ + i;
    const bool exists = line < (int)detailLines_.size();
    const bool title = exists && detailLines_[line].title;
    Text::box(g, PAD, LINES_TOP + i * LINE_H, CONTENT_W - 2 * PAD, LINE_H,
              exists ? detailLines_[line].text.c_str() : "",
              title ? &Theme::FONT_TAB : &Theme::FONT_BODY, title ? Theme::ACCENT : Theme::TEXT,
              Theme::BG);
  }

  const int maxTop = (int)detailLines_.size() - VISIBLE_LINES;
  drawArrows(g, LINES_TOP, Theme::CONTENT_H, detailTop_ > 0, detailTop_ < maxTop);
}

void CalendarApp::onDetailTap(int x, int y) {
  LGFX& g = Screen::gfx();
  if (y < TOP_BAR_H && x < BACK_W) {
    closeDetail(g);
    return;
  }

  const int arrow = arrowHit(x, y, LINES_TOP, Theme::CONTENT_H);
  if (arrow == 0) {
    return;
  }
  // Una pagina menos una linea, para no perder el hilo al leer.
  int maxTop = (int)detailLines_.size() - VISIBLE_LINES;
  if (maxTop < 0) maxTop = 0;
  int newTop = detailTop_ + arrow * (VISIBLE_LINES - 1);
  if (newTop > maxTop) newTop = maxTop;
  if (newTop < 0) newTop = 0;
  if (newTop != detailTop_) {
    detailTop_ = newTop;
    drawDetail(g);
  }
}
