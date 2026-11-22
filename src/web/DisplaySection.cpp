// Sección "Pantalla" de la web: estado (encendida o apagada y brillo, que se
// actualiza solo), brillo y apagado por horario.

#include "services/NightMode.h"
#include "web/Sections.h"
#include "web/Web.h"

namespace {

// La hoja de estilo pone width:100% a los input; las casillas, a su tamaño.
const char* const CHECKBOX_STYLE = " style=\"width:auto;margin-right:8px\"";

String hhmm(int minutes) {
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", minutes / 60, minutes % 60);
  return buf;
}

// "HH:MM" a minutos desde las 00:00; fallback si no es válida.
int parseHhmm(const String& s, int fallback) {
  const int colon = s.indexOf(':');
  if (colon < 1) return fallback;
  const int h = s.substring(0, colon).toInt();
  const int m = s.substring(colon + 1).toInt();
  if (h < 0 || h > 23 || m < 0 || m > 59) return fallback;
  return h * 60 + m;
}

int parseBrightness(const String& s, int fallback) {
  if (s.length() == 0) return fallback;
  const int v = s.toInt();
  if (v < NightMode::MIN_BRIGHTNESS) return NightMode::MIN_BRIGHTNESS;
  if (v > NightMode::MAX_BRIGHTNESS) return NightMode::MAX_BRIGHTNESS;
  return v;
}

String stateText() {
  if (NightMode::isOff()) {
    return "Apagada por horario";
  }
  return "Encendida, brillo " + String(NightMode::brightness()) + " de " + String(NightMode::MAX_BRIGHTNESS);
}

String checkbox(const char* name, bool checked, const char* label) {
  return String("<label><input type=\"checkbox\" name=\"") + name + "\"" + CHECKBOX_STYLE +
         (checked ? " checked" : "") + ">" + label + "</label>";
}

}  // namespace

namespace DisplaySection {

String html() {
  const NightMode::Settings s = NightMode::settings();
  String html;
  html.reserve(2560);
  html += "<h2 id=\"pantalla\">Pantalla</h2>";
  html += "<p>Estado: <b id=\"dstate\">" + stateText() + "</b></p>";

  html += "<form method=\"POST\" action=\"/display/save\">";
  html += "<h3>Brillo</h3>";
  html += "<p class=\"muted\">Esta placa no tiene sensor de luz, así que el brillo es fijo. "
          "Con valores bajos la pantalla se ve poco.</p>";
  html += "<p>Brillo (" + String(NightMode::MIN_BRIGHTNESS) + " a " + String(NightMode::MAX_BRIGHTNESS) +
          "):<br><input type=\"range\" name=\"level\" min=\"" + String(NightMode::MIN_BRIGHTNESS) +
          "\" max=\"" + String(NightMode::MAX_BRIGHTNESS) + "\" value=\"" + String(s.brightness) + "\"></p>";

  html += "<h3>Apagado por horario</h3>";
  html += "<p class=\"muted\">Entre estas horas la pantalla se apaga. El rango puede cruzar la medianoche "
          "(por ejemplo, de 23:00 a 07:30). Si la tocas, se enciende un minuto; ese toque no pulsa nada.</p>";
  html += "<p>" + checkbox("sched", s.scheduleOff, "Apagar la pantalla por horario") + "</p>";
  html += "<p>Desde:<br><input type=\"time\" name=\"from\" value=\"" + hhmm(s.offFrom) + "\"></p>";
  html += "<p>Hasta:<br><input type=\"time\" name=\"to\" value=\"" + hhmm(s.offTo) + "\"></p>";
  html += "<div class=\"actions\"><button type=\"submit\">Guardar</button></div></form>";

  // El estado se actualiza cada 2 s.
  html +=
      "<script>setInterval(function(){fetch('/display/state')"
      ".then(function(r){return r.text()})"
      ".then(function(s){document.getElementById('dstate').textContent=s;});},2000);</script>";
  return html;
}

void registerRoutes() {
  Web::server().on("/display/save", HTTP_POST, []() {
    WebServer& srv = Web::server();
    NightMode::Settings s = NightMode::settings();
    s.brightness = parseBrightness(srv.arg("level"), s.brightness);
    s.scheduleOff = srv.hasArg("sched");  // una casilla sin marcar no se envía
    s.offFrom = parseHhmm(srv.arg("from"), s.offFrom);
    s.offTo = parseHhmm(srv.arg("to"), s.offTo);
    NightMode::save(s);
    Web::redirectHome("pantalla");
  });

  Web::server().on("/display/state", HTTP_GET, []() {
    Web::server().send(200, "text/plain; charset=utf-8", stateText());
  });
}

}  // namespace DisplaySection
