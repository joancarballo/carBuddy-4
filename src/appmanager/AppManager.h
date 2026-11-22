#pragma once
// Gestiona la lista de apps, cual esta en pantalla y la barra de pestañas.
// Tocar una pestaña cambia de app; el resto de toques van a la app activa.

namespace AppManager {

// Define el orden de las apps y dibuja la primera: Info si la placa esta en
// modo configuracion, para ver como configurar la Wi-Fi; si no, Hora.
// Llamar despues de Net::begin().
void begin();

void handleTap(int x, int y);

// Llamar en cada vuelta del loop(): la app activa redibuja lo que haya cambiado.
// Si la placa acaba de crear la red de configuracion, se cambia a Info.
void update();

}  // namespace AppManager
