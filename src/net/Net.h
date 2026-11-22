#pragma once

#include <Arduino.h>

// Conexión Wi-Fi. La web de configuración está en web/.
//
// Si hay una Wi-Fi guardada y se puede conectar, la placa funciona como cliente.
// Si no, crea su propia red abierta (apName()) para poder configurarla.
namespace Net {

// Nombre del dispositivo: su direccion en la red local (http://<nombre>.local) y
// la base del nombre de la red que crea para configurarse (<nombre>-setup). Se
// cambia en la web (seccion Wi-Fi) y se guarda en Preferences; si no, vale
// DEFAULT_DEVICE_NAME (platformio.ini). Solo letras minusculas, numeros y guiones.
constexpr size_t MAX_NAME = 24;
String deviceName();

// Red abierta que crea la placa cuando no puede conectarse a ninguna (solo para
// configurar): "<nombre>-setup".
String apName();

// Función que se llama a menudo mientras se espera a la Wi-Fi en begin(),
// para poder animar la pantalla (begin() no vuelve hasta conectar o rendirse).
void setWaitCallback(void (*callback)());

// Conecta a la Wi-Fi guardada (hasta 15 s) o crea la red de configuración.
void begin();

// Vigila la Wi-Fi. Llamar en cada vuelta del loop().
// Si la Wi-Fi guardada no está conectada durante 2 minutos, crea la red de
// configuración; mientras existe, reintenta la Wi-Fi cada 30 s (si nadie está
// conectado a esa red) y la apaga en cuanto vuelve.
void loop();

// true si la placa está creando su propia red de configuración.
bool isApMode();

// true si está conectada como cliente a una Wi-Fi.
bool isConnected();

// true si se ha perdido la Wi-Fi guardada y la placa está intentando recuperarla
// durante los 2 minutos previos a crear la red de configuración.
bool isReconnecting();

// Nombre de la Wi-Fi guardada ("" si no hay ninguna).
String ssid();

// IP de la placa: la de la Wi-Fi, o la de su propia red en modo configuración.
// Devuelve "" si no tiene.
String ip();

// Nombre en la red local, "<nombre>.local".
String hostName();

// Guarda una Wi-Fi nueva y el nombre del dispositivo, y reinicia la placa al cabo
// de un momento para conectarse a ella. Un nombre vacio o sin caracteres validos
// vuelve al de por defecto.
void saveWifi(const String& ssid, const String& pass, const String& newName);

}  // namespace Net
