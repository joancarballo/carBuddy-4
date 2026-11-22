// Sección "Eventos especiales" de la web: qué hacen, cómo están configurados y
// botón "Editar", que abre una ventana con los dos textos y el emoji del evento
// destacado (un desplegable con los emojis que la placa sabe dibujar).

#include "services/calendar/CalendarService.h"
#include "services/calendar/SpecialEvents.h"
#include "ui/Emoji.h"
#include "web/Sections.h"
#include "web/Web.h"

namespace {

const char* const HOW_IT_WORKS =
    "<p class=\"muted\">Sirven para que algunos eventos del calendario se vean de forma distinta. "
    "Basta con que el título del evento contenga el texto, sin distinguir mayúsculas. "
    "Deja un texto vacío para desactivarlo.</p>"
    "<ul>"
    "<li><b>Evento destacado</b>: si hoy hay un evento con este texto, su emoji aparece encima de la "
    "hora y en la cabecera del calendario, y el evento no sale en la lista de hoy. Si es mañana, va el "
    "primero de la lista de mañana.</li>"
    "<li><b>Evento prioritario</b>: los eventos con este texto van los primeros de su día (detrás del "
    "destacado).</li>"
    "</ul>";

void appendUtf8(String& out, uint32_t cp) {
  if (cp < 0x80) {
    out += (char)cp;
  } else if (cp < 0x800) {
    out += (char)(0xC0 | (cp >> 6));
    out += (char)(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += (char)(0xE0 | (cp >> 12));
    out += (char)(0x80 | ((cp >> 6) & 0x3F));
    out += (char)(0x80 | (cp & 0x3F));
  } else {
    out += (char)(0xF0 | (cp >> 18));
    out += (char)(0x80 | ((cp >> 12) & 0x3F));
    out += (char)(0x80 | ((cp >> 6) & 0x3F));
    out += (char)(0x80 | (cp & 0x3F));
  }
}

// Desplegable con todos los emojis de la tabla, con el actual elegido.
String emojiSelect() {
  const String current = SpecialEvents::highlightEmoji();
  String html = "<select name=\"emoji\">";
  for (size_t i = 0; i < Emoji::IMAGE_COUNT; i++) {
    String e;
    appendUtf8(e, Emoji::IMAGES[i].codepoint);
    if (Emoji::IMAGES[i].second != 0) {
      appendUtf8(e, Emoji::IMAGES[i].second);  // bandera
    }
    html += "<option";
    if (e == current) {
      html += " selected";
    }
    html += ">" + e + "</option>";
  }
  return html + "</select>";
}

String textOrOff(const String& text) {
  return text.length() > 0 ? "<b>" + Web::htmlEscape(text) + "</b>" : String("<span class=\"muted\">desactivado</span>");
}

// true si text es exactamente un emoji de los que la placa sabe dibujar.
bool isKnownEmoji(const String& text) {
  const Emoji::Image* image = nullptr;
  const size_t n = Emoji::parse(text.c_str(), &image);
  return n > 0 && n == text.length() && image != nullptr;
}

// Recorta a maxBytes sin partir un carácter UTF-8 (maxlength del formulario
// cuenta caracteres, pero no hay que fiarse de lo que llega).
String cut(String s, size_t maxBytes) {
  s.trim();
  if (s.length() <= maxBytes) return s;
  size_t n = maxBytes;
  while (n > 0 && ((uint8_t)s[n] & 0xC0) == 0x80) n--;
  return s.substring(0, n);
}

}  // namespace

namespace SpecialSection {

String html() {
  String html;
  html.reserve(8192);
  html += "<h2 id=\"especiales\">Eventos especiales</h2>";
  html += HOW_IT_WORKS;
  html += "<p>Destacado: " + textOrOff(SpecialEvents::highlightText());
  if (SpecialEvents::highlightText().length() > 0) {
    html += " " + SpecialEvents::highlightEmoji();
  }
  html += "<br>Prioritario: " + textOrOff(SpecialEvents::priorityText()) + "</p>";

  const String maxLen = String(SpecialEvents::MAX_TEXT);
  const String fields =
      "<p>Texto del evento destacado:<br><input name=\"hl\" maxlength=\"" + maxLen + "\" value=\"" +
      Web::htmlEscape(SpecialEvents::highlightText()) + "\"></p>"
      "<p>Emoji del evento destacado:<br>" + emojiSelect() + "</p>"
      "<p>Texto del evento prioritario:<br><input name=\"pr\" maxlength=\"" + maxLen + "\" value=\"" +
      Web::htmlEscape(SpecialEvents::priorityText()) + "\"></p>";
  html += Web::openDialogButton("special-edit", "Editar");
  html += Web::dialogForm("special-edit", "Editar eventos especiales", "/special/save", fields);
  return html;
}

void registerRoutes() {
  Web::server().on("/special/save", HTTP_POST, []() {
    WebServer& s = Web::server();
    // Hasta 4 bytes por carácter en UTF-8.
    const size_t maxBytes = SpecialEvents::MAX_TEXT * 4;
    const String emoji = s.arg("emoji");
    SpecialEvents::save(cut(s.arg("hl"), maxBytes), isKnownEmoji(emoji) ? emoji : String(),
                        cut(s.arg("pr"), maxBytes));
    CalendarService::notifyChanged();  // que Hora y Calendario lo apliquen ya
    Web::redirectHome("especiales");
  });
}

}  // namespace SpecialSection
