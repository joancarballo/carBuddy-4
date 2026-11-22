// Sección "Wi-Fi" de la web: estado de la Wi-Fi y botón "Editar", que
// abre una ventana con el formulario para cambiarla. Guardar reinicia la placa.

#include "net/Net.h"
#include "web/Sections.h"
#include "web/Web.h"

namespace {

// Campos de la Wi-Fi y del nombre del dispositivo. El nombre de la red guardada
// aparece rellenado; la contraseña nunca se muestra.
String fields() {
  return "<p>Nombre de la wifi (SSID):<br>"
         "<input name=\"ssid\" maxlength=\"32\" required value=\"" +
         Web::htmlEscape(Net::ssid()) +
         "\"></p>"
         "<p>Contrase&ntilde;a:<br><input name=\"pass\" type=\"password\" maxlength=\"63\"></p>"
         "<p>Nombre del dispositivo:<br>"
         "<input name=\"name\" maxlength=\"" + String(Net::MAX_NAME) + "\" value=\"" +
         Web::htmlEscape(Net::deviceName()) +
         "\"></p>"
         "<p class=\"muted\">Es su direcci&oacute;n en la red local (nombre.local) y la base del nombre de la "
         "red que crea para configurarse (nombre-setup). Solo letras, n&uacute;meros y guiones.</p>";
}

void handleSave() {
  WebServer& s = Web::server();
  String ssid = s.arg("ssid");
  ssid.trim();
  if (ssid.length() == 0) {
    s.send(400, "text/plain; charset=utf-8", "Falta el nombre de la wifi");
    return;
  }

  Net::saveWifi(ssid, s.arg("pass"), s.arg("name"));
  s.send(200, "text/html", Web::page("<p>Guardado. La placa se reinicia y se conecta a la nueva wifi.</p>"));
}

}  // namespace

namespace WifiSection {

String html() {
  String html = "<h2 id=\"wifi\">Wi-Fi</h2>";
  const String ssid = Net::ssid();

  if (ssid.length() == 0) {
    // Todavía no hay nada guardado: se muestra el formulario directamente.
    html += "<p>No hay ninguna wifi guardada.</p>";
    html += "<form method=\"POST\" action=\"/save\">" + fields() +
            "<div class=\"actions\"><button type=\"submit\">Guardar y reiniciar</button></div></form>";
    return html;
  }

  html += "<p>Wifi: " + Web::htmlEscape(ssid) + "</p>";
  if (Net::isConnected()) {
    html += "<p>IP: " + Net::ip() + "</p>";
    html += "<p>Nombre: " + Net::hostName() + "</p>";
  } else if (Net::isApMode()) {
    html += "<p>Sin conexi&oacute;n con esta wifi. La placa lo vuelve a intentar cada 30 s.</p>";
    html += "<p>IP de configuraci&oacute;n: " + Net::ip() + "</p>";
  } else {
    html += "<p>Desconectada. La placa est&aacute; intentando reconectar; si no lo consigue en 2 minutos, "
            "crea la red " + Net::apName() + " para configurarla.</p>";
  }

  // El formulario se abre en una ventana, como los del calendario.
  html += Web::openDialogButton("wifi-edit", "Editar");
  html += Web::dialogForm("wifi-edit", "Editar Wi-Fi", "/save", fields(), "Guardar y reiniciar");
  return html;
}

void registerRoutes() {
  Web::server().on("/save", HTTP_POST, handleSave);
}

}  // namespace WifiSection
