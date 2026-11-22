#include "apps/calendar/CalendarApp.h"

#include <algorithm>

#include "apps/calendar/CalendarLayout.h"
#include "net/Net.h"
#include "services/calendar/SpecialEvents.h"
#include "services/TimeService.h"
#include "ui/Emoji.h"
#include "ui/Message.h"
#include "ui/Text.h"
#include "ui/Theme.h"

using CivilTime::Minutes;

namespace {

// Debajo de la cabecera (HEADER_H, LIST_TOP en CalendarApp.h), 6 filas de 39 px:
// 34..72, 73..111, 112..150, 151..189, 190..228 y 229..267.
constexpr int ROW_H = 39;
constexpr int VISIBLE_ROWS = 6;
constexpr int LIST_BOTTOM = CalendarApp::LIST_TOP + VISIBLE_ROWS * ROW_H;
static_assert(LIST_BOTTOM <= Theme::CONTENT_H, "las 6 filas tienen que caber");

constexpr int ARROW_HALF_W = 14;  // medio ancho de los triangulos ▲/▼
constexpr int ARROW_H = 14;

// Selector de vistas, a la derecha de la cabecera: tres botones enmarcados.
constexpr int BTN_W = 40;
constexpr int BTN_H = 28;
constexpr int BTN_GAP = 6;
constexpr int BTN_Y = (CalendarApp::HEADER_H - BTN_H) / 2;
constexpr int SELECTOR_X = Screen::WIDTH - 4 - 3 * BTN_W - 2 * BTN_GAP;

const char* const SHORT_DAYS[] = {"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"};
const char* const MONTH_NAMES[] = {"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio",
                                   "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};
const char* const WAITING_TIME_TEXT = "Sin hora\nEsperando a internet";

// Posicion dentro de su dia: los eventos especiales (services/calendar/SpecialEvents.h)
// van delante del resto, que conserva su orden.
int dayPriority(const CalendarEvent& e) {
  if (SpecialEvents::isHighlight(e.title)) {
    return 0;
  }
  if (SpecialEvents::isPriority(e.title)) {
    return 1;
  }
  return 2;
}

}  // namespace

// ---------------------------------------------------------------------------
// Datos y filas
// ---------------------------------------------------------------------------

void CalendarApp::refreshData(int64_t today) {
  const uint32_t v = CalendarService::version();
  if (v != dataVersion_) {
    events_ = CalendarService::events();
    range_ = CalendarService::rangeEvents(rangeFirst_, rangeDays_);
    dataVersion_ = v;
  }
  if (dayMode_) {
    buildDayEvents(today);
  }
  buildRows(today);
  if (top_ > maxTop()) {
    top_ = maxTop();
  }
}

void CalendarApp::buildRows(int64_t today) {
  rows_.clear();
  rowsDay_ = today;
  highlightToday_ = false;

  // Sin nada descargado todavia no hay filas: se dibuja un mensaje de estado.
  if (events_.empty() && CalendarService::status().lastOk == 0) {
    return;
  }

  // Lista de un dia: solo sus eventos, con los especiales delante.
  if (dayMode_) {
    for (size_t i = 0; i < dayEvents_.size(); i++) {
      rows_.push_back(Row{RowType::Event, (int)i, nullptr});
    }
    std::stable_sort(rows_.begin(), rows_.end(), [this](const Row& a, const Row& b) {
      return dayPriority(dayEvents_[a.eventIndex]) < dayPriority(dayEvents_[b.eventIndex]);
    });
    if (rows_.empty()) {
      rows_.push_back(Row{RowType::Empty, -1, "Sin eventos"});
    }
    return;
  }

  // Hoy, el separador "Mañana" y mañana.
  for (int d = 0; d < 2; d++) {
    if (d == 1) {
      rows_.push_back(Row{RowType::Separator, -1, "Mañana"});
    }
    const size_t before = rows_.size();
    for (size_t i = 0; i < events_.size(); i++) {
      if (events_[i].day != today + d) {
        continue;
      }
      if (d == 0 && SpecialEvents::isHighlight(events_[i].title)) {
        highlightToday_ = true;  // hoy no va en la lista: se indica con su emoji en la cabecera
        continue;
      }
      rows_.push_back(Row{RowType::Event, (int)i, nullptr});
    }
    // Los eventos especiales delante; el resto mantiene el orden por hora.
    std::stable_sort(rows_.begin() + before, rows_.end(), [this](const Row& a, const Row& b) {
      return dayPriority(events_[a.eventIndex]) < dayPriority(events_[b.eventIndex]);
    });
    if (rows_.size() == before) {
      rows_.push_back(Row{RowType::Empty, -1, "Sin eventos"});
    }
  }
}

// Fila con la que empieza la lista: el primer evento de hoy que no ha terminado.
// Los de todo el dia van arriba del todo; solo se esconden (quedan encima, con la
// flecha ▲) si ya hay eventos de hoy terminados que tampoco caben.
int CalendarApp::firstUpcomingRow(Minutes now) const {
  bool anyPast = false;
  for (size_t i = 0; i < rows_.size(); i++) {
    if (rows_[i].type != RowType::Event) {
      if (rows_[i].type == RowType::Separator) {
        return anyPast ? (int)i : 0;  // hoy ya ha terminado todo
      }
      continue;
    }
    const CalendarEvent& e = listEvents()[rows_[i].eventIndex];
    if (e.allDay) {
      continue;
    }
    if (e.end > now) {
      return anyPast ? (int)i : 0;
    }
    anyPast = true;
  }
  return 0;
}

// Eventos del dia abierto. Hoy y mañana estan completos en events_; para el
// resto se usan los reducidos del mes (sin notas ni lugar).
void CalendarApp::buildDayEvents(int64_t today) {
  dayEvents_.clear();
  if (dayShown_ == today || dayShown_ == today + 1) {
    for (size_t i = 0; i < events_.size(); i++) {
      if (events_[i].day == dayShown_) {
        dayEvents_.push_back(events_[i]);
      }
    }
    return;
  }

  const std::vector<CalendarService::CalendarInfo> calendars = CalendarService::calendars();
  for (size_t i = 0; i < range_.size(); i++) {
    const RangeEvent& r = range_[i];
    if (!CalendarLayout::coversDay(r, dayShown_)) {
      continue;
    }
    CalendarEvent e;
    e.title = r.title;
    e.start = r.start;
    e.end = r.end;
    e.allDay = r.allDay;
    e.uidHash = r.uidHash;
    e.day = dayShown_;
    if (r.calendar < calendars.size()) {
      e.calendar = calendars[r.calendar].name.c_str();
    }
    dayEvents_.push_back(e);
  }
  std::sort(dayEvents_.begin(), dayEvents_.end(), eventListOrder);
}

void CalendarApp::openDay(lgfx::LovyanGFX& g, int64_t day) {
  dayMode_ = true;
  dayShown_ = day;
  manualScroll_ = false;
  top_ = 0;
  buildDayEvents(rowsDay_);
  buildRows(rowsDay_);
  messageShown_ = false;
  g.fillRect(0, LIST_TOP, Screen::WIDTH, Theme::CONTENT_H - LIST_TOP, Theme::BG);
  drawContent(g);
}

// Vuelve a la vista Semana o Mes; quien la llama redibuja.
void CalendarApp::leaveDay() {
  dayMode_ = false;
  dayEvents_.clear();
  manualScroll_ = false;
  buildRows(rowsDay_);
}

int CalendarApp::maxTop() const {
  const int last = (int)rows_.size() - VISIBLE_ROWS;
  return last > 0 ? last : 0;
}

// ---------------------------------------------------------------------------
// Dibujo
// ---------------------------------------------------------------------------

void CalendarApp::drawContent(lgfx::LovyanGFX& g) {
  struct tm t;
  if (!TimeService::now(t)) {
    return;  // update() lo detecta y muestra "Sin hora"
  }
  if (detailOpen_) {
    drawDetail(g);
    return;
  }
  drawSelector(g);
  if (dayMode_) {
    drawList(g, t);
    return;
  }
  switch (view_) {
    case View::List:
      drawList(g, t);
      break;
    case View::Week:
    case View::Month:
      drawHeader(g, t);
      if (rows_.empty()) {
        drawStatusMessage(g);  // todavia no hay nada descargado
        break;
      }
      // Semana y Mes borran su zona al dibujarse: el mensaje anterior desaparece.
      waitingShown_ = false;
      messageShown_ = false;
      if (view_ == View::Week) {
        drawWeek(g, t);
      } else {
        drawMonth(g, t);
      }
      break;
  }
}

// Texto de la cabecera segun la vista, con la hora siempre al final. Si hoy hay
// un evento destacado, empieza con su emoji.
void CalendarApp::drawHeader(lgfx::LovyanGFX& g, const struct tm& t) {
  char header[48];
  const String emojiText = highlightToday_ ? SpecialEvents::highlightEmoji() + " " : String();
  const char* emoji = emojiText.c_str();
  if (dayMode_) {
    // Lista de un dia: "< fecha" en el color de acento; tocarlo vuelve a la vista.
    int y, m, d;
    CivilTime::civilFromDays(dayShown_, y, m, d);
    snprintf(header, sizeof(header), "< %s %02d/%02d/%02d  %02d:%02d", SHORT_DAYS[CivilTime::weekday(dayShown_)],
             d, m, y % 100, t.tm_hour, t.tm_min);
    Text::box(g, PAD, 0, SELECTOR_X - 2 - PAD, HEADER_H, header, &Theme::FONT_HEADER, Theme::ACCENT,
              Theme::BG);
    g.drawFastHLine(0, HEADER_H, Screen::WIDTH, Theme::DIVIDER);
    return;
  }
  if (view_ == View::Week) {
    const int64_t monday = CivilTime::weekStart(CivilTime::dayOf(CivilTime::fromTm(t)));
    int y1, m1, d1, y2, m2, d2;
    CivilTime::civilFromDays(monday, y1, m1, d1);
    CivilTime::civilFromDays(monday + 6, y2, m2, d2);
    snprintf(header, sizeof(header), "%s%02d/%02d - %02d/%02d  %02d:%02d", emoji, d1, m1, d2, m2, t.tm_hour,
             t.tm_min);
  } else if (view_ == View::Month) {
    snprintf(header, sizeof(header), "%s%s %d  %02d:%02d", emoji, MONTH_NAMES[t.tm_mon], t.tm_year + 1900,
             t.tm_hour, t.tm_min);
  } else {
    snprintf(header, sizeof(header), "%s%s %02d/%02d/%02d  %02d:%02d", emoji, SHORT_DAYS[t.tm_wday], t.tm_mday,
             t.tm_mon + 1, (t.tm_year + 1900) % 100, t.tm_hour, t.tm_min);
  }
  Text::box(g, PAD, 0, SELECTOR_X - 2 - PAD, HEADER_H, header, &Theme::FONT_HEADER, Theme::TEXT, Theme::BG);
  g.drawFastHLine(0, HEADER_H, Screen::WIDTH, Theme::DIVIDER);  // de lado a lado
}

// Tres botones enmarcados: ≡ (Lista), 7 (Semana) y 📅 (Mes). El de la vista
// actual va relleno del color de acento. Solo se dibuja al cambiar de vista o
// al dibujar la app entera (no cada minuto), para que no parpadee.
void CalendarApp::drawSelector(lgfx::LovyanGFX& g) {
  for (int i = 0; i < 3; i++) {
    const int bx = SELECTOR_X + i * (BTN_W + BTN_GAP);
    const bool active = (int)view_ == i;
    const uint16_t fill = active ? Theme::ACCENT : Theme::BG;
    const uint16_t ink = active ? Theme::BG : Theme::TEXT;
    g.fillRoundRect(bx, BTN_Y, BTN_W, BTN_H, 4, fill);
    g.drawRoundRect(bx, BTN_Y, BTN_W, BTN_H, 4, active ? Theme::ACCENT : Theme::TEXT_DIM);
    const int cx = bx + BTN_W / 2;
    const int cy = BTN_Y + BTN_H / 2;
    if (i == 0) {
      for (int k = -1; k <= 1; k++) {
        g.fillRect(cx - 8, cy - 1 + k * 6, 16, 2, ink);
      }
    } else if (i == 1) {
      g.setFont(&Theme::FONT_TAB);
      g.setTextColor(ink);
      g.setTextDatum(textdatum_t::middle_center);
      g.drawString("7", cx, cy + 1);
    } else {
      const Emoji::Image* calendar = nullptr;
      Emoji::parse("\U0001F4C5", &calendar);
      Emoji::draw(g, cx - Emoji::ADVANCE / 2, cy, calendar, fill);
    }
  }
}

// La zona tactil de cada boton es toda la altura de la cabecera y su ancho mas el hueco.
int CalendarApp::selectorHit(int x, int y) {
  if (y >= LIST_TOP || x < SELECTOR_X - BTN_GAP / 2) {
    return -1;
  }
  const int i = (x - SELECTOR_X + BTN_GAP / 2) / (BTN_W + BTN_GAP);
  return i > 2 ? 2 : i;
}

void CalendarApp::setView(lgfx::LovyanGFX& g, View view) {
  if (view == view_ && !dayMode_) {
    return;
  }
  if (dayMode_) {
    leaveDay();
  }
  view_ = view;
  manualScroll_ = false;
  messageShown_ = false;
  g.fillRect(0, LIST_TOP, Screen::WIDTH, Theme::CONTENT_H - LIST_TOP, Theme::BG);
  drawContent(g);
}

// Mientras todavia no hay ningun evento descargado, en lugar de la lista va un
// mensaje centrado. Los de espera llevan la animacion de puntos; los errores, no.
void CalendarApp::drawStatusMessage(lgfx::LovyanGFX& g) {
  const CalendarService::Status st = CalendarService::status();
  String text;
  bool waiting = false;
  switch (st.state) {
    case CalendarService::State::NoUrl:
      text = "Sin calendario\nAñádelo en la web:\nhttp://" + Net::hostName();
      break;
    case CalendarService::State::Waiting:
      text = "Esperando Wi-Fi y hora";
      waiting = true;
      break;
    case CalendarService::State::Downloading:
    case CalendarService::State::Ok:
      text = "Descargando";
      waiting = true;
      break;
    case CalendarService::State::Error:
      text = "Error al descargar\n" +
             Text::fit(g, &Theme::FONT_BODY, st.error.c_str(), Screen::WIDTH - 2 * PAD);
      break;
  }
  shownState_ = st.state;
  messageShown_ = true;
  waitingShown_ = waiting;
  waitingY_ = LIST_TOP;
  waitingLines_ = 1;
  Message::show(g, 0, LIST_TOP, Screen::WIDTH, Theme::CONTENT_H - LIST_TOP, text.c_str(), waiting);
}

// Dibuja la fila en d con su esquina en (x0, y0): d es un sprite del tamaño de
// la fila o, si no hubo memoria para el, la propia pantalla.
void CalendarApp::drawRow(lgfx::LovyanGFX& d, int x0, int y0, int row, Minutes now) {
  d.fillRect(x0, y0, CONTENT_W, ROW_H, Theme::BG);
  const Row& r = rows_[row];
  const int cy = y0 + ROW_H / 2;
  d.setTextDatum(textdatum_t::middle_left);

  if (r.type == RowType::Separator) {
    // ─── Mañana ───
    d.setFont(&Theme::FONT_TAB);
    const int w = d.textWidth(r.text);
    const int x = x0 + (CONTENT_W - w) / 2;
    d.drawFastHLine(x0 + PAD, cy, x - 8 - (x0 + PAD), Theme::DIVIDER);
    d.drawFastHLine(x + w + 8, cy, x0 + CONTENT_W - PAD - (x + w + 8), Theme::DIVIDER);
    d.setTextColor(Theme::ACCENT);
    d.drawString(r.text, x, cy);
    return;
  }

  // "Sin eventos": solo el texto, sin la linea de debajo.
  if (r.type == RowType::Empty) {
    d.setFont(&Theme::FONT_BODY);
    d.setTextColor(Theme::TEXT_DIM);
    d.drawString(r.text, x0 + PAD, cy);
    return;
  }

  d.drawFastHLine(x0 + PAD, y0 + ROW_H - 1, CONTENT_W - 2 * PAD, Theme::DIVIDER);

  const CalendarEvent& e = listEvents()[r.eventIndex];
  const bool past = e.end <= now;
  int titleWidth = CONTENT_W - 2 * PAD;

  // Los eventos de todo el dia solo llevan el titulo, que ocupa toda la fila.
  if (!e.allDay) {
    const std::string times = timeRangeText(e);
    d.setFont(&Theme::FONT_SMALL);
    d.setTextColor(Theme::TEXT_DIM);
    d.setTextDatum(textdatum_t::middle_right);
    d.drawString(times.c_str(), x0 + CONTENT_W - PAD, cy);
    titleWidth -= d.textWidth(times.c_str()) + PAD;
    d.setTextDatum(textdatum_t::middle_left);
  }

  const String title = Text::fit(d, &Theme::FONT_BODY, e.title.c_str(), titleWidth);
  Text::draw(d, &Theme::FONT_BODY, title.c_str(), x0 + PAD, cy, past ? Theme::TEXT_DIM : Theme::TEXT,
             Theme::BG);

  // Tachado si ya ha terminado.
  if (past) {
    d.drawFastHLine(x0 + PAD, cy, CONTENT_W - 2 * PAD, Theme::TEXT_DIM);
  }
}

void CalendarApp::drawArrows(lgfx::LovyanGFX& g, int topY, int bottomY, bool up, bool down) {
  const int cx = CONTENT_W + (Screen::WIDTH - CONTENT_W) / 2;
  const int upY = topY + (bottomY - topY) / 4;
  const int downY = topY + 3 * (bottomY - topY) / 4;
  // Una flecha que no hace falta se dibuja con el color del fondo, para borrarla.
  g.fillTriangle(cx, upY - ARROW_H / 2, cx - ARROW_HALF_W, upY + ARROW_H / 2, cx + ARROW_HALF_W,
                 upY + ARROW_H / 2, up ? Theme::ACCENT : Theme::BG);
  g.fillTriangle(cx - ARROW_HALF_W, downY - ARROW_H / 2, cx + ARROW_HALF_W, downY - ARROW_H / 2, cx,
                 downY + ARROW_H / 2, down ? Theme::ACCENT : Theme::BG);
}

// Cada flecha es una zona tactil de toda la columna derecha (56 px de ancho) y
// media altura de la lista.
int CalendarApp::arrowHit(int x, int y, int topY, int bottomY) {
  if (x < CONTENT_W || y < topY || y >= bottomY) {
    return 0;
  }
  return y < (topY + bottomY) / 2 ? -1 : 1;
}

void CalendarApp::drawList(lgfx::LovyanGFX& g, const struct tm& t) {
  const Minutes now = CivilTime::fromTm(t);
  shownMinute_ = t.tm_min;

  // Sin mover la lista a mano, empieza en el siguiente evento.
  if (!manualScroll_) {
    top_ = firstUpcomingRow(now);
    if (top_ > maxTop()) {
      top_ = maxTop();
    }
  }

  drawHeader(g, t);
  if (rows_.empty()) {
    drawStatusMessage(g);
    return;
  }
  waitingShown_ = false;
  if (messageShown_) {
    // El mensaje ocupaba todo el ancho: se borra tambien la columna de las flechas.
    g.fillRect(CONTENT_W, LIST_TOP, Screen::WIDTH - CONTENT_W, Theme::CONTENT_H - LIST_TOP, Theme::BG);
    messageShown_ = false;
  }

  // Cada fila se compone en un sprite y se envia de una vez, sin parpadeo.
  LGFX_Sprite sprite(&g);
  sprite.setColorDepth(16);
  const bool useSprite = sprite.createSprite(CONTENT_W, ROW_H) != nullptr;
  for (int i = 0; i < VISIBLE_ROWS; i++) {
    const int row = top_ + i;
    const int y = LIST_TOP + i * ROW_H;
    if (row >= (int)rows_.size()) {
      g.fillRect(0, y, CONTENT_W, ROW_H, Theme::BG);
    } else if (useSprite) {
      drawRow(sprite, 0, 0, row, now);
      sprite.pushSprite(0, y);
    } else {
      drawRow(g, 0, y, row, now);
    }
  }
  sprite.deleteSprite();

  drawArrows(g, LIST_TOP, LIST_BOTTOM, top_ > 0, top_ < maxTop());
}

void CalendarApp::draw(lgfx::LovyanGFX& g) {
  struct tm t;
  synced_ = TimeService::now(t);
  messageShown_ = false;  // el gestor (o update()) acaba de borrar la zona de contenido
  if (!synced_) {
    waitingShown_ = true;
    waitingY_ = 0;
    waitingLines_ = 2;
    Message::show(g, 0, 0, Screen::WIDTH, Theme::CONTENT_H, WAITING_TIME_TEXT, true);
    return;
  }
  refreshData(CivilTime::dayOf(CivilTime::fromTm(t)));
  drawContent(g);
}

void CalendarApp::update(lgfx::LovyanGFX& g) {
  struct tm t;
  const bool synced = TimeService::now(t);
  if (synced != synced_) {
    // Acaba de llegar la hora (o se ha perdido): se redibuja todo.
    detailOpen_ = false;
    g.fillRect(0, 0, Screen::WIDTH, Theme::CONTENT_H, Theme::BG);
    draw(g);
    return;
  }
  if (!synced) {
    Message::waitingDots(g, 0, waitingY_, Screen::WIDTH, Theme::CONTENT_H - waitingY_, waitingLines_);
    return;
  }

  // 30 s sin tocar nada: se cierran el detalle y la lista de un dia (se vuelve a
  // Semana o Mes), y la lista vuelve a colocarse sola.
  if (millis() - lastTapMs_ >= IDLE_TIMEOUT_MS) {
    if (dayMode_) {
      detailOpen_ = false;
      detailLines_.clear();
      leaveDay();
      g.fillRect(0, 0, Screen::WIDTH, Theme::CONTENT_H, Theme::BG);
      drawContent(g);
      return;
    }
    if (detailOpen_) {
      manualScroll_ = false;
      closeDetail(g);
      return;
    }
    if (manualScroll_) {
      manualScroll_ = false;
      drawList(g, t);
      return;
    }
  }

  const int64_t today = CivilTime::dayOf(CivilTime::fromTm(t));
  bool redraw = false;
  if (CalendarService::version() != dataVersion_ || today != rowsDay_) {
    refreshData(today);
    redraw = true;
  }
  // El detalle es una copia del evento: no cambia con datos nuevos ni con la hora.
  if (detailOpen_) {
    return;
  }
  // Cada minuto: la hora de la cabecera y, en la Lista, los eventos que acaban
  // de terminar. Semana y Mes solo cambian con datos nuevos o al cambiar de dia.
  if (t.tm_min != shownMinute_) {
    shownMinute_ = t.tm_min;
    if (view_ == View::List || dayMode_) {
      redraw = true;
    } else if (!redraw) {
      drawHeader(g, t);
    }
  }
  if (rows_.empty() && CalendarService::status().state != shownState_) {
    redraw = true;
  }

  if (redraw) {
    if (view_ == View::List || dayMode_) {
      drawList(g, t);
    } else {
      drawContent(g);
    }
  } else if (waitingShown_) {
    Message::waitingDots(g, 0, waitingY_, Screen::WIDTH, Theme::CONTENT_H - waitingY_, waitingLines_);
  }
}

// ---------------------------------------------------------------------------
// Toques
// ---------------------------------------------------------------------------

void CalendarApp::onShow() {
  detailOpen_ = false;
  detailLines_.clear();
  manualScroll_ = false;
  if (dayMode_) {
    leaveDay();  // al volver a la pestaña se ve la vista, no la lista de un dia
  }
}

void CalendarApp::onTap(int x, int y) {
  lastTapMs_ = millis();
  if (!synced_) {
    return;
  }
  if (detailOpen_) {
    onDetailTap(x, y);
    return;
  }

  LGFX& g = Screen::gfx();
  const int button = selectorHit(x, y);
  if (button >= 0) {
    setView(g, (View)button);
    return;
  }
  // Lista de un dia: tocar "< fecha" vuelve a Semana o Mes.
  if (dayMode_ && y < LIST_TOP) {
    leaveDay();
    g.fillRect(0, LIST_TOP, Screen::WIDTH, Theme::CONTENT_H - LIST_TOP, Theme::BG);
    drawContent(g);
    return;
  }
  if (rows_.empty()) {
    return;
  }
  // Semana o Mes: tocar un dia abre su lista.
  if (!dayMode_ && view_ != View::List) {
    struct tm t;
    if (!TimeService::now(t)) {
      return;
    }
    const int64_t day = view_ == View::Week ? weekDayAt(x, y, t) : monthDayAt(x, y, t);
    if (day >= 0) {
      openDay(g, day);
    }
    return;
  }

  const int arrow = arrowHit(x, y, LIST_TOP, LIST_BOTTOM);
  if (arrow != 0) {
    // Una pagina menos una fila, para no perder de vista donde se estaba.
    int newTop = top_ + arrow * (VISIBLE_ROWS - 1);
    if (newTop > maxTop()) newTop = maxTop();
    if (newTop < 0) newTop = 0;
    if (newTop != top_) {
      top_ = newTop;
      manualScroll_ = true;
      drawContent(g);
    }
    return;
  }

  if (x >= CONTENT_W || y < LIST_TOP || y >= LIST_BOTTOM) {
    return;
  }
  const int row = top_ + (y - LIST_TOP) / ROW_H;
  struct tm t;
  if (row < (int)rows_.size() && rows_[row].type == RowType::Event && TimeService::now(t)) {
    openDetail(g, listEvents()[rows_[row].eventIndex], CivilTime::dayOf(CivilTime::fromTm(t)));
  }
}
