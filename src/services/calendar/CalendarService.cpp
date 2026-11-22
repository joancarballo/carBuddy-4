#include "services/calendar/CalendarService.h"

#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

#include <algorithm>

#include "certs.h"
#include "services/TimeService.h"
#include "services/calendar/EventCollector.h"
#include "services/calendar/IcsParser.h"
#include "services/calendar/SpecialEvents.h"

namespace {

const uint32_t REFRESH_MS = 15UL * 60 * 1000;   // descarga normal cada 15 min
const uint32_t RETRY_MS = 2UL * 60 * 1000;      // tras un error, reintento a los 2 min
const uint32_t WAKE_MS = 30UL * 1000;           // la tarea mira si toca descargar cada 30 s
const uint32_t READ_TIMEOUT_MS = 20000;         // sin recibir datos durante este tiempo: error
const int DAYS_SHOWN = 2;                       // hoy y mañana

// Todo lo que se comparte entre la tarea de descarga y el resto del programa
// está protegido por este mutex.
SemaphoreHandle_t mutex = nullptr;
TaskHandle_t taskHandle = nullptr;

const size_t MAX_LIST = 50;    // máximo de elementos en la lista conjunta
const size_t MAX_RANGE = 300;  // máximo de eventos del mes, todos los calendarios juntos

struct Calendar {
  String name;
  String url;
  String error;                       // vacío si la última descarga fue bien
  std::vector<CalendarEvent> events;  // últimos eventos descargados (se conservan si falla)
  std::vector<RangeEvent> range;      // eventos reducidos del mes (también se conservan)
};

std::vector<Calendar> calendarList;
uint32_t configVersion = 0;  // cambia al añadir o quitar un calendario
std::vector<CalendarEvent> eventList;  // todos los calendarios juntos
std::vector<RangeEvent> rangeList;     // ídem, eventos reducidos del mes
int64_t rangeFirstDay = 0;             // cuadrícula del mes de la última descarga
int rangeDayCount = 0;
uint32_t listVersion = 0;
CalendarService::Status currentStatus = {CalendarService::State::NoUrl, "", 0, 0};

// Solo los usa la tarea de descarga.
volatile bool requested = false;  // se escribe desde la web (otro núcleo)
bool lastFailed = false;
uint32_t lastAttemptMs = 0;
int64_t lastDay = -1;  // día del último intento de descarga

void lock() {
  xSemaphoreTake(mutex, portMAX_DELAY);
}

void unlock() {
  xSemaphoreGive(mutex);
}

void setState(CalendarService::State state, const String& error = String()) {
  lock();
  currentStatus.state = state;
  currentStatus.error = error;
  unlock();
}

bool eventPtrOrder(const CalendarEvent* a, const CalendarEvent* b) {
  return eventListOrder(*a, *b);
}

struct RangeRef {
  const RangeEvent* event;
  uint8_t calendar;
};

bool rangeRefOrder(const RangeRef& a, const RangeRef& b) {
  return rangeOrder(*a.event, *b.event);
}

// Lo mismo que mergeLists() para los eventos reducidos del mes. Cada uno lleva
// la posición de su calendario. Llamar con el mutex cogido.
void mergeRanges() {
  std::vector<RangeRef> all;
  for (size_t i = 0; i < calendarList.size(); i++) {
    for (size_t j = 0; j < calendarList[i].range.size(); j++) {
      all.push_back(RangeRef{&calendarList[i].range[j], (uint8_t)i});
    }
  }
  std::sort(all.begin(), all.end(), rangeRefOrder);

  rangeList.clear();
  for (size_t i = 0; i < all.size() && rangeList.size() < MAX_RANGE; i++) {
    bool repeated = false;
    for (size_t j = 0; j < rangeList.size() && !repeated; j++) {
      repeated = rangeList[j].uidHash == all[i].event->uidHash && rangeList[j].start == all[i].event->start;
    }
    if (!repeated) {
      rangeList.push_back(*all[i].event);
      rangeList.back().calendar = all[i].calendar;
    }
  }
}

// Junta los eventos de todos los calendarios en una sola lista ordenada.
// Un evento que está en dos calendarios (por ejemplo, una invitación) sale una vez.
// Se ordenan punteros y solo se copia lo que entra en la lista: esta placa no
// tiene PSRAM y cada evento puede ocupar más de 1 KB con sus notas.
// Llamar con el mutex cogido.
void mergeLists() {
  std::vector<const CalendarEvent*> all;
  for (size_t i = 0; i < calendarList.size(); i++) {
    for (size_t j = 0; j < calendarList[i].events.size(); j++) {
      all.push_back(&calendarList[i].events[j]);
    }
  }
  std::sort(all.begin(), all.end(), eventPtrOrder);

  eventList.clear();
  for (size_t i = 0; i < all.size() && eventList.size() < MAX_LIST; i++) {
    bool repeated = false;
    for (size_t j = 0; j < eventList.size() && !repeated; j++) {
      repeated = eventList[j].uidHash == all[i]->uidHash && eventList[j].start == all[i]->start &&
                 eventList[j].day == all[i]->day;
    }
    if (!repeated) {
      eventList.push_back(*all[i]);
    }
  }
  mergeRanges();
  listVersion++;
  currentStatus.eventCount = eventList.size();
}

// Estado general a partir de los errores de cada calendario. Llamar con el mutex cogido.
void updateErrorState() {
  currentStatus.error = "";
  for (size_t i = 0; i < calendarList.size(); i++) {
    if (calendarList[i].error.length() > 0) {
      currentStatus.error = calendarList[i].name + ": " + calendarList[i].error;
      break;
    }
  }
  if (calendarList.empty()) {
    currentStatus.state = CalendarService::State::NoUrl;
  } else {
    currentStatus.state = currentStatus.error.length() > 0 ? CalendarService::State::Error : CalendarService::State::Ok;
  }
}

// Guarda los calendarios en Preferences como url0..url4 y name0..name4.
// Solo se borran las claves que existen: este núcleo de Arduino escribe un error
// en el monitor serie al borrar o leer una clave que no está.
// Llamar con el mutex cogido.
void save() {
  Preferences prefs;
  prefs.begin("cal", false);
  for (int i = 0; i < CalendarService::MAX_CALENDARS; i++) {
    const String urlKey = "url" + String(i);
    const String nameKey = "name" + String(i);
    if (i < (int)calendarList.size()) {
      prefs.putString(urlKey.c_str(), calendarList[i].url);
      prefs.putString(nameKey.c_str(), calendarList[i].name);
    } else {
      if (prefs.isKey(urlKey.c_str())) prefs.remove(urlKey.c_str());
      if (prefs.isKey(nameKey.c_str())) prefs.remove(nameKey.c_str());
    }
  }
  prefs.end();
}

void load() {
  Preferences prefs;
  prefs.begin("cal", false);
  // (La v1 pasaba aquí la antigua clave "url" a "url0"; en la v2 nunca ha existido.)
  for (int i = 0; i < CalendarService::MAX_CALENDARS; i++) {
    const String urlKey = "url" + String(i);
    if (!prefs.isKey(urlKey.c_str())) {
      break;
    }
    const String url = prefs.getString(urlKey.c_str(), "");
    if (url.length() == 0) {
      break;
    }
    Calendar cal;
    cal.url = url;
    cal.name = prefs.getString(("name" + String(i)).c_str(), "Calendario " + String(i + 1));
    calendarList.push_back(cal);
  }
  prefs.end();
}

// Resumen de la descarga en el monitor serie, para comprobar que no falta nada:
// tamaño, eventos leídos, cada evento de hoy y mañana y las repeticiones no admitidas.
void logResult(const String& name, size_t bytes, const IcsParser& parser, const EventCollector& collector,
               const std::vector<CalendarEvent>& list, const std::vector<RangeEvent>& range) {
  Serial.printf("Calendario \"%s\": %u bytes, %d eventos en el archivo, %u hoy y mañana, %u en el mes\n",
                name.c_str(), (unsigned)bytes, parser.eventsRead(), (unsigned)list.size(),
                (unsigned)range.size());
  for (size_t i = 0; i < list.size(); i++) {
    int y, m, d;
    CivilTime::civilFromDays(list[i].day, y, m, d);
    Serial.printf("  %02d/%02d %s %s\n", d, m, timeRangeText(list[i]).c_str(), list[i].title.c_str());
  }
  const std::vector<std::string>& rules = collector.unsupportedRules();
  for (size_t i = 0; i < rules.size(); i++) {
    Serial.printf("  Repetición no admitida (solo se usa la primera fecha): %s\n", rules[i].c_str());
  }
  if (collector.full()) {
    Serial.printf("  Lista llena: se han quedado eventos fuera (máximo %u hoy y mañana, %u en el mes)\n",
                  (unsigned)EventCollector::MAX_EVENTS, (unsigned)EventCollector::MAX_RANGE);
  }
}

// Memoria tras cada ronda de descargas. El mínimo desde el arranque incluye el
// pico de la conexión HTTPS; la pila libre dice si sobra o falta pila a la tarea.
void logMemory() {
  Serial.printf("Memoria: heap libre %lu, minimo desde el arranque %lu, bloque mayor %lu, "
                "pila libre de la tarea %u bytes\n",
                (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
                (unsigned long)ESP.getMaxAllocHeap(), (unsigned)uxTaskGetStackHighWaterMark(nullptr));
}

// Descarga el archivo y lo procesa según llega: out con hoy y mañana, range con
// la cuadrícula del mes. Devuelve false y rellena error si falla.
bool download(const String& name, const String& address, int64_t today, int64_t gridFirst, int gridDays,
              std::vector<CalendarEvent>& out, std::vector<RangeEvent>& range, String& error) {
  WiFiClientSecure client;
  client.setCACert(GOOGLE_ROOT_CAS);

  HTTPClient http;
  http.useHTTP10(true);  // sin "chunked": así el cuerpo se lee tal cual llega
  http.setTimeout(READ_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, address)) {
    error = "URL no válida";
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    error = code < 0 ? HTTPClient::errorToString(code) : "respuesta HTTP " + String(code);
    http.end();
    return false;
  }

  EventCollector collector(today, DAYS_SHOWN, gridFirst, gridDays);
  IcsParser parser(EventCollector::addCallback, &collector);

  WiFiClient* stream = http.getStreamPtr();
  int remaining = http.getSize();  // -1 si el servidor no dice el tamaño
  uint8_t buffer[1024];
  size_t total = 0;
  uint32_t lastData = millis();

  while (remaining != 0) {
    const size_t available = stream->available();
    if (available > 0) {
      const int n = stream->read(buffer, available < sizeof(buffer) ? available : sizeof(buffer));
      for (int i = 0; i < n; i++) {
        parser.feed((char)buffer[i]);
      }
      total += n;
      if (remaining > 0) {
        remaining -= n;
      }
      lastData = millis();
    } else if (!stream->connected()) {
      break;
    } else if (millis() - lastData > READ_TIMEOUT_MS) {
      error = "se cortó la descarga";
      http.end();
      return false;
    } else {
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  }
  http.end();

  parser.finish();
  collector.finish(out, range);
  for (size_t i = 0; i < out.size(); i++) {
    out[i].calendar = name.c_str();
  }
  logResult(name, total, parser, collector, out, range);
  return true;
}

// Decide si toca descargar: hay calendarios, Wi-Fi y hora, y se ha pedido,
// ha pasado el tiempo de refresco o ha cambiado el día.
bool shouldDownload(int64_t today) {
  lock();
  const bool empty = calendarList.empty();
  unlock();

  if (empty) {
    setState(CalendarService::State::NoUrl);
    return false;
  }
  struct tm now;
  if (WiFi.status() != WL_CONNECTED || !TimeService::now(now)) {
    setState(CalendarService::State::Waiting);
    return false;
  }
  if (requested || lastAttemptMs == 0 || today != lastDay) {
    return true;
  }
  return millis() - lastAttemptMs >= (lastFailed ? RETRY_MS : REFRESH_MS);
}

void task(void*) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(WAKE_MS));

    struct tm now;
    const int64_t today = TimeService::now(now) ? CivilTime::dayOf(CivilTime::fromTm(now)) : -1;
    if (!shouldDownload(today)) {
      continue;
    }

    // Copia de los calendarios: la descarga es larga y no se puede tener el mutex cogido.
    lock();
    const uint32_t startVersion = configVersion;
    std::vector<String> names;
    std::vector<String> urls;
    for (size_t i = 0; i < calendarList.size(); i++) {
      names.push_back(calendarList[i].name);
      urls.push_back(calendarList[i].url);
    }
    unlock();

    requested = false;
    lastAttemptMs = millis();
    lastDay = today;
    setState(CalendarService::State::Downloading);

    int64_t gridFirst;
    int gridDays;
    CivilTime::monthGrid(today, gridFirst, gridDays);

    // Uno detrás de otro: dos conexiones HTTPS a la vez gastarían demasiada memoria.
    // Cada resultado se guarda en cuanto llega (la v1 esperaba al final), para no
    // tener a la vez en memoria las listas nuevas de todos los calendarios y las anteriores.
    bool anyOk = false;
    bool anyFailed = false;
    bool changed = false;  // se añadió o quitó un calendario durante la descarga
    for (size_t i = 0; i < urls.size() && !changed; i++) {
      std::vector<CalendarEvent> list;
      std::vector<RangeEvent> range;
      String error;
      const bool ok = download(names[i], urls[i], today, gridFirst, gridDays, list, range, error);
      anyOk = anyOk || ok;
      anyFailed = anyFailed || !ok;
      if (!ok) {
        Serial.printf("Calendario \"%s\": error al descargar (%s)\n", names[i].c_str(), error.c_str());
      }

      lock();
      changed = configVersion != startVersion;
      if (!changed) {
        Calendar& cal = calendarList[i];
        if (ok) {
          cal.events.swap(list);  // los eventos anteriores se liberan al acabar esta vuelta
          cal.range.swap(range);
          cal.error = "";
          // Por si se cambió el nombre durante la descarga.
          for (size_t j = 0; j < cal.events.size(); j++) {
            cal.events[j].calendar = cal.name.c_str();
          }
        } else {
          cal.error = error;  // se conservan sus eventos anteriores
        }
      }
      unlock();
    }
    lastFailed = anyFailed;

    lock();
    if (changed || configVersion != startVersion) {
      // Se añadió o quitó un calendario durante la descarga: se repite.
      requested = true;
      xTaskNotifyGive(taskHandle);
    } else {
      rangeFirstDay = gridFirst;
      rangeDayCount = gridDays;
      mergeLists();
      if (anyOk) {
        currentStatus.lastOk = time(nullptr);
      }
      updateErrorState();
    }
    unlock();
    logMemory();
  }
}

}  // namespace

namespace CalendarService {

void begin() {
  mutex = xSemaphoreCreateMutex();
  load();
  currentStatus.state = calendarList.empty() ? State::NoUrl : State::Waiting;

  // Núcleo 0: el loop() de Arduino (pantalla y web) va en el núcleo 1.
  // La pila es grande porque la conexión HTTPS la necesita.
  xTaskCreatePinnedToCore(task, "calendar", 16384, nullptr, 1, &taskHandle, 0);
}

bool hasCalendars() {
  lock();
  const bool has = !calendarList.empty();
  unlock();
  return has;
}

std::vector<CalendarInfo> calendars() {
  std::vector<CalendarInfo> out;
  lock();
  for (size_t i = 0; i < calendarList.size(); i++) {
    CalendarInfo info;
    info.name = calendarList[i].name;
    info.error = calendarList[i].error;
    info.eventCount = calendarList[i].events.size();
    out.push_back(info);
  }
  unlock();
  return out;
}

bool addCalendar(const String& name, const String& url) {
  lock();
  if ((int)calendarList.size() >= MAX_CALENDARS) {
    unlock();
    return false;
  }
  Calendar cal;
  cal.name = name.length() > 0 ? name : "Calendario " + String(calendarList.size() + 1);
  cal.url = url;
  calendarList.push_back(cal);
  configVersion++;
  save();
  currentStatus.state = State::Waiting;
  unlock();

  Serial.printf("Calendario \"%s\" añadido\n", cal.name.c_str());
  refreshNow();
  return true;
}

void renameCalendar(int index, const String& name) {
  lock();
  if (index >= 0 && index < (int)calendarList.size()) {
    Calendar& cal = calendarList[index];
    Serial.printf("Calendario \"%s\" ahora se llama \"%s\"\n", cal.name.c_str(), name.c_str());
    cal.name = name;
    // Los eventos ya descargados llevan el nombre del calendario: se actualiza también.
    for (size_t i = 0; i < cal.events.size(); i++) {
      cal.events[i].calendar = name.c_str();
    }
    save();
    mergeLists();
    updateErrorState();
  }
  unlock();
}

void removeCalendar(int index) {
  lock();
  if (index >= 0 && index < (int)calendarList.size()) {
    Serial.printf("Calendario \"%s\" quitado\n", calendarList[index].name.c_str());
    calendarList.erase(calendarList.begin() + index);
    configVersion++;
    save();
    mergeLists();
    updateErrorState();
    if (calendarList.empty()) {
      currentStatus.lastOk = 0;
    }
  }
  unlock();
}

void refreshNow() {
  requested = true;
  // Se marca ya como "descargando" para que la web espere al resultado.
  lock();
  if (currentStatus.state == State::Ok || currentStatus.state == State::Error) {
    currentStatus.state = State::Downloading;
  }
  unlock();
  if (taskHandle != nullptr) {
    xTaskNotifyGive(taskHandle);
  }
}

Status status() {
  lock();
  Status s = currentStatus;
  unlock();
  return s;
}

std::vector<CalendarEvent> events() {
  lock();
  std::vector<CalendarEvent> copy = eventList;
  unlock();
  return copy;
}

std::vector<RangeEvent> rangeEvents(int64_t& firstDay, int& dayCount) {
  lock();
  std::vector<RangeEvent> copy = rangeList;
  firstDay = rangeFirstDay;
  dayCount = rangeDayCount;
  unlock();
  return copy;
}

bool hasHighlightEvent(int64_t day) {
  lock();
  bool found = false;
  for (size_t i = 0; i < eventList.size() && !found; i++) {
    found = eventList[i].day == day && SpecialEvents::isHighlight(eventList[i].title);
  }
  unlock();
  return found;
}

void notifyChanged() {
  lock();
  listVersion++;
  unlock();
}

uint32_t version() {
  lock();
  const uint32_t v = listVersion;
  unlock();
  return v;
}

}  // namespace CalendarService
