#pragma once
// Interfaz de una app. Cada app ocupa la zona de contenido (Theme::CONTENT_H
// pixeles de alto, encima de la barra de pestañas) y no debe dibujar fuera.

#include "hal/Screen.h"

class App {
 public:
  virtual ~App() {}

  // Nombre de la app: es el texto de su pestaña y sale en el monitor serie.
  virtual const char* name() const = 0;

  // Dibuja la app entera. El gestor ya ha borrado la zona de contenido.
  virtual void draw(lgfx::LovyanGFX& g) = 0;

  // Se llama en cada vuelta del loop() mientras la app esta en pantalla, para
  // redibujar solo lo que haya cambiado (por ejemplo, la hora).
  virtual void update(lgfx::LovyanGFX& g) { (void)g; }

  // Al mostrar la app (antes de draw()) y al cambiar a otra.
  virtual void onShow() {}
  virtual void onHide() {}

  // Toque dentro de la zona de contenido.
  virtual void onTap(int x, int y) {
    (void)x;
    (void)y;
  }
};
