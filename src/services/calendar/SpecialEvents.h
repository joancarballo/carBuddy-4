#pragma once

#include <Arduino.h>

#include <string>

// Eventos especiales del calendario. Los textos y el emoji se configuran en la
// web (sección "Eventos especiales") y se guardan en Preferences (espacio
// "special"): no van en el código. Basta con que el título contenga el texto,
// sin distinguir mayúsculas (puede llevar emojis alrededor). Un texto vacío
// desactiva su regla.
//   Destacado: hoy no va en la lista del calendario y su emoji se muestra en la
//              cabecera del calendario y encima de la hora (app Hora); si es
//              mañana, va el primero de mañana.
//   Prioritario: va el primero de su día (después del destacado).
namespace SpecialEvents {

const size_t MAX_TEXT = 40;  // caracteres de cada texto

// Lee la configuración guardada.
void begin();

String highlightText();
String priorityText();
// Emoji del evento destacado, en UTF-8 (por defecto ⭐).
String highlightEmoji();

// Guarda la configuración. Un emoji vacío vuelve al de por defecto; quien llama
// comprueba antes que sea uno de los que la placa sabe dibujar (web/SpecialSection).
void save(const String& highlight, const String& emoji, const String& priority);

// Solo se usan desde el loop() (apps y web), nunca desde la tarea de descarga,
// así que no necesitan mutex.
bool isHighlight(const std::string& title);
bool isPriority(const std::string& title);

}  // namespace SpecialEvents
