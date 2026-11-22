#pragma once

#include <stdint.h>

#include <utility>
#include <vector>

#include "services/calendar/CalendarEvent.h"

// Recibe los VEVENT del archivo uno a uno y se queda solo con lo que cae en dos
// ventanas de días:
//   - la de la lista (hoy y mañana): eventos completos, uno por evento y día;
//   - la del rango (la cuadrícula del mes): eventos reducidos (RangeEvent), uno
//     por evento aunque dure varios días.
// Al terminar devuelve las dos listas ordenadas. No depende de Arduino.
class EventCollector {
 public:
  // Máximos por calendario, para no agotar la memoria.
  static const size_t MAX_EVENTS = 30;
  static const size_t MAX_RANGE = 150;

  // Ventanas: dayCount días a partir de firstDay y rangeDayCount días a partir
  // de rangeFirstDay (días desde el 1/1/1970).
  EventCollector(int64_t firstDay, int dayCount, int64_t rangeFirstDay, int rangeDayCount);

  void add(const IcsEvent& ev);

  // Aplica las repeticiones modificadas o canceladas y devuelve las dos listas ordenadas.
  void finish(std::vector<CalendarEvent>& list, std::vector<RangeEvent>& range);

  // Reglas de repetición que no se entienden (el evento sale solo en su
  // primera fecha). Se guardan las primeras para el diagnóstico.
  const std::vector<std::string>& unsupportedRules() const { return unsupportedRules_; }

  // true si se llegó a MAX_EVENTS o a MAX_RANGE y se dejaron eventos fuera.
  bool full() const { return full_; }

  // Para usar como callback de IcsParser.
  static void addCallback(const IcsEvent& ev, void* collector);

 private:
  struct Occurrence {
    uint32_t uidHash;
    CivilTime::Minutes originalStart;
    bool fromRule;  // generada por una RRULE (puede haber sido modificada)
    CalendarEvent event;
  };
  struct RangeOccurrence {
    CivilTime::Minutes originalStart;
    bool fromRule;
    RangeEvent event;  // event.uidHash identifica el evento
  };

  void addOccurrence(const IcsEvent& ev, CivilTime::Minutes start, bool fromRule, const std::string& repeat);
  bool isOverridden(uint32_t uidHash, CivilTime::Minutes originalStart) const;

  int64_t firstDay_;
  int dayCount_;
  CivilTime::Minutes listFrom_, listTo_;
  CivilTime::Minutes rangeFrom_, rangeTo_;
  std::vector<Occurrence> occurrences_;
  std::vector<RangeOccurrence> rangeOccurrences_;
  // Repeticiones que tienen su propio VEVENT con RECURRENCE-ID (movidas,
  // cambiadas o canceladas): la versión generada por la RRULE se descarta.
  std::vector<std::pair<uint32_t, CivilTime::Minutes> > overrides_;
  std::vector<std::string> unsupportedRules_;
  bool full_ = false;
};
