#pragma once

#include <Arduino.h>

// Secciones de la página principal. Cada una devuelve su HTML con html() y
// registra sus rutas (formularios y consultas) con registerRoutes().

namespace WifiSection {
String html();
void registerRoutes();
}  // namespace WifiSection

namespace ClockSection {
String html();
void registerRoutes();
}  // namespace ClockSection

namespace CalendarSection {
String html();
void registerRoutes();
}  // namespace CalendarSection

namespace SpecialSection {
String html();
void registerRoutes();
}  // namespace SpecialSection

namespace DisplaySection {
String html();
void registerRoutes();
}  // namespace DisplaySection
