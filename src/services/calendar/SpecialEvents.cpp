#include "services/calendar/SpecialEvents.h"

#include <ctype.h>

#include <Preferences.h>

namespace {

const char* const DEFAULT_EMOJI = "\u2B50";  // ⭐ (uno de los de la tabla de ui/EmojiData.cpp)

String highlight;
String priority;
String emoji = DEFAULT_EMOJI;
// Los textos en minúsculas, para comparar con los títulos.
std::string highlightLower;
std::string priorityLower;

std::string toLower(const String& s) {
  std::string out(s.c_str());
  for (size_t i = 0; i < out.size(); i++) {
    out[i] = (char)tolower((unsigned char)out[i]);
  }
  return out;
}

bool titleContains(const std::string& title, const std::string& lowerText) {
  if (lowerText.empty()) {
    return false;  // regla desactivada
  }
  std::string lower = title;
  for (size_t i = 0; i < lower.size(); i++) {
    lower[i] = (char)tolower((unsigned char)lower[i]);
  }
  return lower.find(lowerText) != std::string::npos;
}

void apply(const String& h, const String& e, const String& p) {
  highlight = h;
  priority = p;
  emoji = e.length() > 0 ? e : String(DEFAULT_EMOJI);
  highlightLower = toLower(highlight);
  priorityLower = toLower(priority);
}

}  // namespace

namespace SpecialEvents {

void begin() {
  Preferences prefs;
  prefs.begin("special", true);
  // isKey() antes de leer: este núcleo escribe un error por cada clave que no existe.
  const String h = prefs.isKey("hl") ? prefs.getString("hl", "") : String();
  const String e = prefs.isKey("emoji") ? prefs.getString("emoji", DEFAULT_EMOJI) : String(DEFAULT_EMOJI);
  const String p = prefs.isKey("pr") ? prefs.getString("pr", "") : String();
  prefs.end();
  apply(h, e, p);
}

String highlightText() { return highlight; }

String priorityText() { return priority; }

String highlightEmoji() { return emoji; }

void save(const String& h, const String& e, const String& p) {
  apply(h, e, p);
  Preferences prefs;
  prefs.begin("special", false);
  prefs.putString("hl", highlight);
  prefs.putString("emoji", emoji);
  prefs.putString("pr", priority);
  prefs.end();
  Serial.println("Eventos especiales guardados");  // sin los textos: son privados
}

bool isHighlight(const std::string& title) { return titleContains(title, highlightLower); }

bool isPriority(const std::string& title) { return titleContains(title, priorityLower); }

}  // namespace SpecialEvents
