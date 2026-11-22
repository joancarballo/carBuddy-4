#pragma once
// App Calendario: eventos descargados de Google (ver CalendarService), en tres
// vistas que se eligen con los botones de la cabecera: Lista (≡), Semana (7) y Mes (📅).
//
// Lista: cabecera con dia, fecha y hora, y 5 filas con hoy, el separador "Mañana"
// y mañana. Los eventos que ya han terminado salen tachados. Sin tocar nada, la
// lista empieza en el primer evento de hoy que no ha terminado.
//
// Tactil: las flechas ▲/▼ de la derecha mueven la lista; tocar un evento abre su
// detalle, que se cierra con "< Volver". Tras 30 s sin tocar nada, el detalle se
// cierra y la lista vuelve a colocarse sola.
//
// En Semana y Mes, tocar un dia abre su lista (la misma pantalla que Lista, con
// solo ese dia); su cabecera "< fecha" vuelve a la vista. Para hoy y mañana se
// usan los eventos completos; para otros dias, los reducidos (sin notas ni lugar).
//
// El codigo esta repartido en varios archivos:
//   CalendarApp.cpp     cabecera, selector de vistas, lista y toques
//   CalendarDetail.cpp  detalle de un evento
//   CalendarWeek.cpp    vista Semana
//   CalendarMonth.cpp   vista Mes
//   CalendarLayout.cpp  lo comun a Semana y Mes (barras que cruzan dias)
// Su seccion de la web ("Calendario") esta en web/CalendarSection.cpp.

#include <vector>

#include "appmanager/App.h"
#include "services/calendar/CalendarEvent.h"
#include "services/calendar/CalendarService.h"

class CalendarApp : public App {
 public:
  // Medidas comunes a la lista y al detalle: a la derecha del texto queda la
  // columna de las flechas (56 px).
  static constexpr int CONTENT_W = 424;
  static constexpr int PAD = 10;
  // Cabecera de 32 px y la linea que la separa de lo de debajo (y = 32).
  static constexpr int HEADER_H = 32;
  static constexpr int LIST_TOP = HEADER_H + 2;

  const char* name() const override { return "Calendario"; }
  void draw(lgfx::LovyanGFX& g) override;
  void update(lgfx::LovyanGFX& g) override;
  void onShow() override;
  void onTap(int x, int y) override;

 private:
  static const uint32_t IDLE_TIMEOUT_MS = 30000;

  enum class View { List, Week, Month };  // en el orden de los botones

  // Event: un evento. Empty: "Sin eventos" (hoy o mañana).
  // Separator: la linea "Mañana".
  enum class RowType { Event, Empty, Separator };

  struct Row {
    RowType type;
    int eventIndex;    // posicion en events_ (solo para RowType::Event)
    const char* text;  // texto del separador o de la fila sin eventos
  };

  // Lista
  void refreshData(int64_t today);
  void buildRows(int64_t today);
  int firstUpcomingRow(CivilTime::Minutes now) const;
  int maxTop() const;
  void drawContent(lgfx::LovyanGFX& g);
  void setView(lgfx::LovyanGFX& g, View view);
  void drawSelector(lgfx::LovyanGFX& g);
  // Boton del selector que hay en (x, y), o -1.
  static int selectorHit(int x, int y);
  void drawList(lgfx::LovyanGFX& g, const struct tm& t);
  void drawHeader(lgfx::LovyanGFX& g, const struct tm& t);
  void drawRow(lgfx::LovyanGFX& d, int x0, int y0, int row, CivilTime::Minutes now);
  void drawStatusMessage(lgfx::LovyanGFX& g);

  // Flechas ▲/▼ de la columna derecha, en la zona entre topY y bottomY.
  static void drawArrows(lgfx::LovyanGFX& g, int topY, int bottomY, bool up, bool down);
  // -1 si el toque es en la flecha de arriba, 1 en la de abajo, 0 si no es en ninguna.
  static int arrowHit(int x, int y, int topY, int bottomY);

  // Semana y Mes (CalendarWeek.cpp y CalendarMonth.cpp)
  void drawWeek(lgfx::LovyanGFX& g, const struct tm& t);
  void drawMonth(lgfx::LovyanGFX& g, const struct tm& t);
  // Dia que hay en (x, y), o -1 si el toque no cae en ninguno.
  int64_t weekDayAt(int x, int y, const struct tm& t) const;
  int64_t monthDayAt(int x, int y, const struct tm& t) const;

  // Lista de un dia
  void openDay(lgfx::LovyanGFX& g, int64_t day);
  void leaveDay();  // sin redibujar
  void buildDayEvents(int64_t today);
  // Eventos a los que apuntan las filas: los de hoy y mañana o los del dia abierto.
  const std::vector<CalendarEvent>& listEvents() const { return dayMode_ ? dayEvents_ : events_; }

  // Detalle (CalendarDetail.cpp)
  void openDetail(lgfx::LovyanGFX& g, const CalendarEvent& ev, int64_t today);
  void closeDetail(lgfx::LovyanGFX& g);
  void drawDetail(lgfx::LovyanGFX& g);
  void onDetailTap(int x, int y);

  View view_ = View::List;
  bool dayMode_ = false;                  // se esta viendo la lista de un dia
  int64_t dayShown_ = 0;                  // ese dia
  std::vector<CalendarEvent> dayEvents_;  // sus eventos

  std::vector<CalendarEvent> events_;
  std::vector<RangeEvent> range_;  // eventos reducidos del mes (Semana y Mes)
  int64_t rangeFirst_ = 0;         // dias que cubre range_
  int rangeDays_ = 0;
  uint32_t dataVersion_ = 0xFFFFFFFF;  // version de la lista que se tiene copiada
  int64_t rowsDay_ = -1;               // dia para el que se construyeron las filas
  bool highlightToday_ = false;        // hoy hay un evento destacado (su emoji en la cabecera)

  std::vector<Row> rows_;
  int top_ = 0;                // primera fila visible
  bool manualScroll_ = false;  // se ha movido la lista con las flechas
  uint32_t lastTapMs_ = 0;

  // Lo que hay en pantalla, para saber cuando redibujar.
  bool synced_ = false;
  int shownMinute_ = -1;
  CalendarService::State shownState_ = CalendarService::State::NoUrl;
  bool messageShown_ = false;  // en lugar de la lista hay un mensaje de estado
  bool waitingShown_ = false;  // hay un mensaje con la animacion de espera
  int waitingY_ = 0;           // y donde empieza su rectangulo
  int waitingLines_ = 1;       // lineas de texto del mensaje

  struct DetailLine {
    String text;
    bool title;
  };
  bool detailOpen_ = false;
  std::vector<DetailLine> detailLines_;
  int detailTop_ = 0;
};
