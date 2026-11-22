# Hardware de carBuddy-4

Placa: CYD de 4" con ESP32-32E, de la familia **E32R40T** de LCDWiki (hay clones con
otras marcas, por ejemplo diymore). Ficha y esquema del fabricante:
<https://www.lcdwiki.com/4.0inch_ESP32-32E_Display>

- MCU: ESP32-D0WD-V3 (doble núcleo, 240 MHz) en un módulo ESP32-32E
- Flash: 4 MB. RAM: 520 KB de SRAM. **Sin PSRAM**: se trabaja asumiendo que no hay
- Pantalla: TN de 4,0", 320x480 (480x320 en horizontal), controlador **ST7796S** por SPI
- Táctil: **resistivo** XPT2046 (hay que presionar; la placa trae lápiz), en el mismo bus SPI que la pantalla
- USB-C para alimentar y cargar el programa (convertidor serie CH340C)
- Alimentación: 5 V por USB. Consumo aproximado: 230 mA con la pantalla encendida
- **Sin sensor de luz** (el CYD de 2,8" sí lo tiene): el brillo se elige en la web
- Extras que el firmware no usa: lector microSD, altavoz, conector de batería, LED RGB

## Pines

| Función | GPIO |
|---|---|
| Pantalla (HSPI): SCLK / MOSI / MISO / CS / DC | 14 / 13 / 12 / 15 / 2 (RST va al EN de la ESP32) |
| Retroiluminación (PWM, activa a nivel alto) | 27 |
| Táctil: SCLK / MOSI / MISO | los mismos que la pantalla (14 / 13 / 12) |
| Táctil: CS / IRQ | 33 / 36 |
| LED RGB R / G / B (activo a nivel bajo) | 22 / 16 / 17 |
| microSD: CS / MOSI / CLK / MISO | 5 / 23 / 18 / 19 |
| Altavoz: activación (a nivel bajo) / salida DAC | 4 / 26 |
| Batería (lectura de tensión, solo entrada) | 34 |
| I2C externo: SCL / SDA | 25 / 32 |
| Botón BOOT | 0 |
| Libres, solo entrada | 35, 39 |

## Cómo se confirmó y qué falta

Los datos salen de la documentación del fabricante, no de una prueba con la placa. Al
arrancar, el monitor serie muestra el chip, la flash, la PSRAM y el tamaño de la
pantalla; deberían ser ESP32-D0WD-V3, 4 MB, 0 bytes y 480x320.

Si algo no coincide en la pantalla, se corrige con las opciones `-D` de `platformio.ini`:

- Colores invertidos (fondo claro): `PANEL_INVERT_DEFAULT=true`
- Rojo y azul cambiados (el verde menta sale morado): `PANEL_RGB_ORDER=true`
- Imagen boca abajo: `SCREEN_ROTATION=3`
- El táctil se calibra al primer arranque (4 marcas); para repetirlo, pulsa BOOT cuando
  la pantalla lo ofrezca al arrancar.

## Notas

- La pantalla y el táctil comparten bus SPI: LovyanGFX los reparte con `bus_shared`.
- Con USB-C puede que no arranque con cable C a C; probar con A a C. Si la subida se
  queda en "Connecting...", mantener BOOT pulsado al empezar.
- El táctil resistivo necesita calibración y puede dar toques fantasma con el tiempo.
