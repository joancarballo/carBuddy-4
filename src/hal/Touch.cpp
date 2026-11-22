#include "hal/Touch.h"

#include <Arduino.h>

#include "hal/Screen.h"

namespace {

// El tactil resistivo puede perder el contacto un instante mientras se presiona.
// Solo se cuenta un toque nuevo si antes ha estado este tiempo sin tocar.
constexpr uint32_t RELEASE_MS = 60;

bool pressed = false;
uint32_t lastTouchMs = 0;

}  // namespace

namespace Touch {

bool pollTap(int& x, int& y) {
  int32_t tx = 0, ty = 0;
  const bool touching = Screen::gfx().getTouch(&tx, &ty) > 0;
  const uint32_t now = millis();

  if (touching) {
    lastTouchMs = now;
    if (!pressed) {
      pressed = true;
      x = tx;
      y = ty;
      return true;
    }
    return false;
  }

  if (pressed && now - lastTouchMs >= RELEASE_MS) {
    pressed = false;
  }
  return false;
}

}  // namespace Touch
