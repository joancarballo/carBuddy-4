# carBuddy-4

Dispositivo de escritorio con un CYD de 4": una placa ESP32 con pantalla táctil a color de **4,0" (320x480, 480x320 en horizontal)**. Un único firmware con varias apps (Hora, Calendario e Info) que se configura desde una web servida por la propia placa. Todo corre en la ESP32, sin servidor ni programa externo.

- **Hora**: hora grande y fecha, con huso horario configurable.
- **Calendario**: eventos de Google Calendar (por su dirección iCal privada), en vistas de Lista, Semana y Mes, con detalle de cada evento.
- **Info**: estado de la Wi-Fi, dirección de la web, memoria y tiempo encendido.
- **Pantalla**: brillo fijo ajustable y apagado por horario.

Es la adaptación a la pantalla de 4" de **carBuddy-2** (CYD de 2,8").

> Estado: pendiente de probar en la placa.

## Hardware

| | |
|---|---|
| Placa | CYD de 4" con ESP32-32E, familia E32R40T de LCDWiki (hay clones con otras marcas) |
| Pantalla | 4,0", 320x480 (480x320 en horizontal), controlador **ST7796S** por SPI |
| Táctil | Resistivo XPT2046, en el **mismo bus SPI** que la pantalla |
| MCU | ESP32-D0WD-V3, 4 MB de flash, **sin PSRAM** |
| Sensor de luz | **No tiene** |

**Dónde comprarlo:** [Amazon](https://link.amazon/B0gBrUYv1) <!-- Ejemplo: [Nombre de la tienda](https://...) -->

Pines y detalles: [docs/hardware.md](docs/hardware.md).

## Peculiaridades de esta placa

- **Sin sensor de luz** (el CYD de 2,8" sí lo tiene): no hay brillo automático; el brillo es fijo y se elige en la web.
- **Pantalla y táctil comparten bus SPI.** La retroiluminación va en el GPIO 27 y el LED rojo en el 22.
- **Sin PSRAM, memoria justa.** Un buffer de pantalla completo serían 300 KB: se dibuja por zonas con sprites pequeños, y el iCal y las respuestas HTTPS se procesan en streaming, una descarga cada vez. Las particiones son `huge_app.csv` (3 MB para el programa, sin actualización OTA).
- **Colores y orientación según la unidad.** Si los colores salen invertidos, pon `PANEL_INVERT_DEFAULT=true`; si el rojo y el azul salen cambiados, `PANEL_RGB_ORDER=true`; si la imagen sale boca abajo, `SCREEN_ROTATION=3` (en `platformio.ini`).
- **Táctil resistivo:** hay que presionar (la placa trae lápiz). Se calibra en el primer arranque, y para repetirlo se pulsa BOOT cuando la pantalla lo ofrece.
- **USB-C:** puede que no arranque con un cable C a C; usa A a C.

## Compilar y cargar

1. Abre la carpeta en VS Code con PlatformIO.
2. Con el entorno `cyd4` (el único), pulsa Upload y después Monitor.
3. Si la subida se queda en "Connecting...", mantén pulsado BOOT hasta que empiece.
4. Al primer arranque se pide calibrar el táctil: toca la punta de cada marca.

Los ajustes de la pantalla y los valores por defecto están en `platformio.ini`.

## Configuración

Sin Wi-Fi guardada, la placa crea una red abierta llamada `<nombre>-setup` (`carbuddy4-setup` por defecto): conéctate a ella y abre la dirección que indica la pantalla Info. Con Wi-Fi, la web está en `http://<nombre>.local` (o la IP de la pantalla Info).

Todo lo personal se configura en la web y se guarda en la memoria de la placa, no en el código:

- **Wi-Fi** y **nombre del dispositivo** (su dirección en la red local).
- **Calendarios**: hasta 5 direcciones iCal, con nombre.
- **Eventos especiales**: un texto que, si aparece en el título de un evento, lo destaca (su emoji se muestra encima de la hora y en el calendario) y otro que lo hace prioritario en la lista. Vacíos por defecto.
- **Hora**: huso horario.
- **Pantalla**: brillo y apagado por horario.

El código no contiene nombres, direcciones, claves ni calendarios de nadie.

## Emojis

Los emojis a color se generan con `tools/emoji/gen_emoji.py` (necesita Pillow) en `src/ui/EmojiData.cpp`. Solo hace falta al cambiar la lista de emojis.

## Créditos

Emojis de [Twemoji](https://github.com/jdecked/twemoji) (© Twitter, Inc. y colaboradores), con licencia [CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/).
