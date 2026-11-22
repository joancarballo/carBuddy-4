#include "net/Net.h"

#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ctype.h>

// Nombre por defecto del dispositivo; se puede cambiar con -DDEFAULT_DEVICE_NAME=\"..\".
#ifndef DEFAULT_DEVICE_NAME
#define DEFAULT_DEVICE_NAME "carbuddy"
#endif

namespace {

const uint32_t CONNECT_TIMEOUT_MS = 15000;      // espera al arrancar antes de crear la red de configuración
const uint32_t AP_FALLBACK_MS = 2UL * 60 * 1000;  // sin Wi-Fi durante 2 min: se vuelve a crear
const uint32_t RETRY_MS = 30000;                  // en modo configuración, reintento de la Wi-Fi guardada

Preferences prefs;
bool apMode = false;
bool mdnsStarted = false;
String savedSsid;
String savedPass;
String devName = DEFAULT_DEVICE_NAME;
uint32_t lastConnectedMs = 0;  // última vez que se vio la Wi-Fi conectada
uint32_t lastRetryMs = 0;
bool restartPending = false;
uint32_t restartAt = 0;
void (*waitCallback)() = nullptr;

// Deja solo letras minusculas, numeros y guiones (un espacio o '_' pasa a '-',
// sin repetirlos ni dejarlos al principio o al final), hasta Net::MAX_NAME
// caracteres. Si no queda nada, el nombre por defecto.
String sanitizeName(const String& raw) {
  String out;
  for (size_t i = 0; i < raw.length() && out.length() < Net::MAX_NAME; i++) {
    const char c = (char)tolower((unsigned char)raw[i]);
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      out += c;
    } else if ((c == '-' || c == ' ' || c == '_') && out.length() > 0 && out[out.length() - 1] != '-') {
      out += '-';
    }
  }
  while (out.length() > 0 && out[out.length() - 1] == '-') {
    out.remove(out.length() - 1);
  }
  return out.length() > 0 ? out : String(DEFAULT_DEVICE_NAME);
}

// mDNS (<nombre>.local) solo funciona conectado a una Wi-Fi; se arranca una vez.
void startMdns() {
  if (mdnsStarted) {
    return;
  }
  if (MDNS.begin(devName.c_str())) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
    Serial.printf("Tambien disponible en http://%s.local\n", devName.c_str());
  }
}

// Crea la red abierta de configuración. Si hay una Wi-Fi guardada, la placa
// sigue intentando conectarse a ella a la vez (modo AP + estación).
void startConfigAp() {
  apMode = true;
  if (savedSsid.length() > 0) {
    // Los reintentos los hace loop(), solo cuando nadie usa la red de
    // configuración: cada intento cambia de canal y cortaría al teléfono.
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_AP);
  }
  const String ap = Net::apName();
  WiFi.softAP(ap.c_str());
  lastRetryMs = millis();
  Serial.printf("Modo configuracion: red \"%s\", IP %s\n", ap.c_str(), WiFi.softAPIP().toString().c_str());
}

// La Wi-Fi guardada ha vuelto: se apaga la red de configuración.
void stopConfigAp() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  apMode = false;
  Serial.printf("Wifi recuperada. IP: %s\n", WiFi.localIP().toString().c_str());
  startMdns();
}

// Vigila la Wi-Fi: sin conexión durante AP_FALLBACK_MS crea la red de
// configuración; en modo configuración reintenta y la apaga al conectar.
void watchWifi() {
  if (savedSsid.length() == 0) {
    return;  // sin Wi-Fi guardada solo existe la red de configuración
  }
  const bool connected = WiFi.status() == WL_CONNECTED;

  if (!apMode) {
    if (connected) {
      lastConnectedMs = millis();
    } else if (millis() - lastConnectedMs >= AP_FALLBACK_MS) {
      Serial.println("Sin wifi durante 2 minutos.");
      startConfigAp();
    }
    return;
  }

  if (connected) {
    stopConfigAp();
    lastConnectedMs = millis();
  } else if (WiFi.softAPgetStationNum() == 0 && millis() - lastRetryMs >= RETRY_MS) {
    lastRetryMs = millis();
    WiFi.begin(savedSsid.c_str(), savedPass.c_str());
  }
}

}  // namespace

namespace Net {

void setWaitCallback(void (*callback)()) {
  waitCallback = callback;
}

void begin() {
  prefs.begin("wifi", true);
  savedSsid = prefs.getString("ssid", "");
  savedPass = prefs.getString("pass", "");
  // isKey() antes de leer: este nucleo escribe un error por cada clave que no existe.
  devName = sanitizeName(prefs.isKey("name") ? prefs.getString("name", "") : String(DEFAULT_DEVICE_NAME));
  prefs.end();

  bool connected = false;
  if (savedSsid.length() > 0) {
    Serial.printf("Conectando a la wifi \"%s\"...\n", savedSsid.c_str());
    WiFi.setHostname(devName.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSsid.c_str(), savedPass.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < CONNECT_TIMEOUT_MS) {
      if (waitCallback != nullptr) {
        waitCallback();
      }
      delay(50);
    }
    connected = (WiFi.status() == WL_CONNECTED);
  } else {
    Serial.println("No hay ninguna wifi guardada.");
  }

  if (connected) {
    apMode = false;
    lastConnectedMs = millis();
    WiFi.setAutoReconnect(true);
    Serial.printf("Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
    startMdns();
  } else {
    startConfigAp();
  }

}

void loop() {
  watchWifi();
  if (restartPending && (int32_t)(millis() - restartAt) >= 0) {
    ESP.restart();
  }
}

bool isApMode() {
  return apMode;
}

bool isConnected() {
  return !apMode && WiFi.status() == WL_CONNECTED;
}

bool isReconnecting() {
  return savedSsid.length() > 0 && !apMode && WiFi.status() != WL_CONNECTED;
}

String ssid() {
  return savedSsid;
}

String ip() {
  if (apMode) {
    return WiFi.softAPIP().toString();
  }
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "";
}

String deviceName() {
  return devName;
}

String apName() {
  return devName + "-setup";
}

String hostName() {
  return devName + ".local";
}

void saveWifi(const String& ssid, const String& pass, const String& newName) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("name", sanitizeName(newName));
  prefs.end();

  Serial.printf("Wifi guardada: \"%s\". Reiniciando...\n", ssid.c_str());
  // Se espera un poco para que la web pueda responder antes del reinicio.
  restartPending = true;
  restartAt = millis() + 1500;
}

}  // namespace Net
