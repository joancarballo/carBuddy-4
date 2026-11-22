#pragma once
// Configuracion de LovyanGFX para el CYD de 4": pantalla ST7796 de 320x480
// (480x320 en horizontal) y tactil resistivo XPT2046, los dos en el mismo bus SPI.
//
// Ajustes que se pueden cambiar con -D en platformio.ini si algo sale mal:
//   PANEL_INVERT_DEFAULT  true si los colores salen invertidos (fondo claro)
//   PANEL_RGB_ORDER       true si rojo y azul salen cambiados
//   SCREEN_ROTATION       ver hal/Screen.cpp (1 y 3 son horizontales)

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "board_pins.h"

#ifndef PANEL_INVERT_DEFAULT
#define PANEL_INVERT_DEFAULT false
#endif

#ifndef PANEL_RGB_ORDER
#define PANEL_RGB_ORDER false
#endif

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;
  lgfx::Touch_XPT2046 _touch;

 public:
  LGFX() {
    {  // Bus SPI, compartido por la pantalla y el tactil
      auto cfg = _bus.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = pins::LCD_SCLK;
      cfg.pin_mosi = pins::LCD_MOSI;
      cfg.pin_miso = pins::LCD_MISO;
      cfg.pin_dc = pins::LCD_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {  // Panel
      auto cfg = _panel.config();
      cfg.pin_cs = pins::LCD_CS;
      cfg.pin_rst = pins::LCD_RST;
      cfg.pin_busy = -1;
      cfg.panel_width = 320;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = true;
      cfg.invert = false;  // la inversion se controla en runtime con invertDisplay()
      cfg.rgb_order = PANEL_RGB_ORDER;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;  // el tactil usa el mismo bus
      _panel.config(cfg);
    }
    {  // Retroiluminacion
      auto cfg = _light.config();
      cfg.pin_bl = pins::LCD_BL;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    {  // Tactil XPT2046 en el bus de la pantalla
      auto cfg = _touch.config();
      cfg.spi_host = HSPI_HOST;
      cfg.freq = 1000000;  // el XPT2046 es lento: mejor bajo
      cfg.pin_sclk = pins::LCD_SCLK;
      cfg.pin_mosi = pins::LCD_MOSI;
      cfg.pin_miso = pins::LCD_MISO;
      cfg.pin_cs = pins::TOUCH_CS;
      cfg.pin_int = pins::TOUCH_IRQ;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;
      // Valores aproximados; la calibracion del arranque los corrige
      cfg.x_min = 300;
      cfg.x_max = 3900;
      cfg.y_min = 200;
      cfg.y_max = 3700;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};

inline const char* panelName() { return "ST7796"; }
