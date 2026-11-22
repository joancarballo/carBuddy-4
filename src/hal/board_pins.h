#pragma once
// Pines del CYD de 4" con ESP32-32E (familia E32R40T de LCDWiki).
// Fuente: https://www.lcdwiki.com/4.0inch_ESP32-32E_Display

namespace pins {

// Pantalla ST7796 (bus HSPI, compartido con el tactil)
constexpr int LCD_SCLK = 14;
constexpr int LCD_MOSI = 13;
constexpr int LCD_MISO = 12;
constexpr int LCD_CS   = 15;
constexpr int LCD_DC   = 2;
constexpr int LCD_RST  = -1;   // el reset de la pantalla es el EN de la ESP32
constexpr int LCD_BL   = 27;   // retroiluminacion (PWM, activa a nivel alto)

// Tactil resistivo XPT2046: mismos SCLK/MOSI/MISO que la pantalla
constexpr int TOUCH_CS  = 33;
constexpr int TOUCH_IRQ = 36;

// LED RGB (activo a nivel bajo: LOW = encendido)
constexpr int LED_R = 22;
constexpr int LED_G = 16;
constexpr int LED_B = 17;

// Otros
constexpr int AUDIO_EN = 4;    // amplificador del altavoz: a nivel bajo se activa
constexpr int SPEAKER  = 26;   // salida DAC al amplificador
constexpr int BATTERY  = 34;   // lectura de la tension de la bateria (solo entrada)
constexpr int BOOT_BTN = 0;

}  // namespace pins
