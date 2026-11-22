#include "services/calendar/EventCollector.h"

#include <string.h>

#include <algorithm>

#include "services/calendar/IcsParser.h"
#include "services/calendar/Recurrence.h"

using CivilTime::Minutes;

namespace {

// Resumen corto del UID (FNV-1a) para comparar sin guardar el texto entero.
uint32_t hashUid(const std::string& uid) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < uid.size(); i++) {
    h ^= (uint8_t)uid[i];
    h *= 16777619u;
  }
  return h;
}

bool overlaps(Minutes start, Minutes end, Minutes from, Minutes to) {
  if (end <= start) {
    return start >= from && start < to;  // evento sin duración
  }
  return start < to && end > from;
}

}  // namespace

EventCollector::EventCollector(int64_t firstDay, int dayCount, int64_t rangeFirstDay, int rangeDayCount)
    : firstDay_(firstDay),
      dayCount_(dayCount),
      listFrom_(firstDay * CivilTime::MINUTES_PER_DAY),
      listTo_((firstDay + dayCount) * CivilTime::MINUTES_PER_DAY),
      rangeFrom_(rangeFirstDay * CivilTime::MINUTES_PER_DAY),
      rangeTo_((rangeFirstDay + rangeDayCount) * CivilTime::MINUTES_PER_DAY) {}

void EventCollector::addCallback(const IcsEvent& ev, void* collector) {
  static_cast<EventCollector*>(collector)->add(ev);
}

void EventCollector::add(const IcsEvent& ev) {
  // Las repeticiones se calculan en la unión de las dos ventanas.
  const Minutes from = std::min(listFrom_, rangeFrom_);
  const Minutes to = std::max(listTo_, rangeTo_);

  // Repetición modificada de otro evento: anula la original, se muestre o no.
  if (ev.recurrenceId >= 0) {
    overrides_.push_back(std::make_pair(hashUid(ev.uid), ev.recurrenceId));
    if (!ev.cancelled && overlaps(ev.start, ev.end, from, to)) {
      addOccurrence(ev, ev.start, false, std::string());
    }
    return;
  }

  if (ev.cancelled) {
    return;
  }

  Recurrence::Rule rule;
  if (!ev.rrule.empty() && Recurrence::parse(ev.rrule, rule)) {
    std::vector<Minutes> starts;
    Recurrence::occurrences(ev, rule, from, to, starts);
    const std::string repeat = starts.empty() ? std::string() : Recurrence::describe(rule);
    for (size_t i = 0; i < starts.size(); i++) {
      addOccurrence(ev, starts[i], true, repeat);
    }
    return;
  }
  if (!ev.rrule.empty() && unsupportedRules_.size() < 5) {
    unsupportedRules_.push_back(ev.rrule);
  }

  if (overlaps(ev.start, ev.end, from, to)) {
    addOccurrence(ev, ev.start, false, std::string());
  }
}

// Cada aparición va a la lista (completa) si cae en hoy o mañana, y al rango
// (reducida) si cae en la cuadrícula del mes; puede ir a las dos.
void EventCollector::addOccurrence(const IcsEvent& ev, Minutes start, bool fromRule, const std::string& repeat) {
  const uint32_t uidHash = hashUid(ev.uid);
  const Minutes end = start + (ev.end - ev.start);
  const std::string title = ev.summary.empty() ? "(Sin título)" : ev.summary;

  if (overlaps(start, end, listFrom_, listTo_)) {
    if (occurrences_.size() < MAX_EVENTS) {
      Occurrence occ;
      occ.uidHash = uidHash;
      occ.originalStart = start;
      occ.fromRule = fromRule;
      occ.event.title = title;
      occ.event.location = ev.location;
      occ.event.notes = ev.description;
      occ.event.repeat = repeat;
      occ.event.uidHash = uidHash;
      occ.event.allDay = ev.allDay;
      occ.event.start = start;
      occ.event.end = end;
      occurrences_.push_back(occ);
    } else {
      full_ = true;
    }
  }

  if (overlaps(start, end, rangeFrom_, rangeTo_)) {
    if (rangeOccurrences_.size() < MAX_RANGE) {
      RangeOccurrence occ;
      occ.originalStart = start;
      occ.fromRule = fromRule;
      const std::string cut = Ics::truncateUtf8(title, RangeEvent::TITLE_SIZE - 1);
      memcpy(occ.event.title, cut.c_str(), cut.size() + 1);
      occ.event.start = start;
      occ.event.end = end;
      occ.event.uidHash = uidHash;
      occ.event.allDay = ev.allDay;
      rangeOccurrences_.push_back(occ);
    } else {
      full_ = true;
    }
  }
}

bool EventCollector::isOverridden(uint32_t uidHash, Minutes originalStart) const {
  for (size_t j = 0; j < overrides_.size(); j++) {
    if (overrides_[j].first == uidHash && overrides_[j].second == originalStart) {
      return true;
    }
  }
  return false;
}

void EventCollector::finish(std::vector<CalendarEvent>& list, std::vector<RangeEvent>& range) {
  list.clear();
  for (size_t i = 0; i < occurrences_.size(); i++) {
    const Occurrence& occ = occurrences_[i];
    if (occ.fromRule && isOverridden(occ.uidHash, occ.originalStart)) {
      continue;
    }

    // Un evento aparece en la lista de cada día que toca.
    for (int d = 0; d < dayCount_; d++) {
      const int64_t day = firstDay_ + d;
      const Minutes dayStart = day * CivilTime::MINUTES_PER_DAY;
      if (overlaps(occ.event.start, occ.event.end, dayStart, dayStart + CivilTime::MINUTES_PER_DAY) &&
          list.size() < MAX_EVENTS) {
        list.push_back(occ.event);
        list.back().day = day;
      }
    }
  }
  std::sort(list.begin(), list.end(), eventListOrder);

  range.clear();
  for (size_t i = 0; i < rangeOccurrences_.size(); i++) {
    const RangeOccurrence& occ = rangeOccurrences_[i];
    if (occ.fromRule && isOverridden(occ.event.uidHash, occ.originalStart)) {
      continue;
    }
    range.push_back(occ.event);
  }
  std::sort(range.begin(), range.end(), rangeOrder);

  occurrences_.clear();
  rangeOccurrences_.clear();
  overrides_.clear();
}
