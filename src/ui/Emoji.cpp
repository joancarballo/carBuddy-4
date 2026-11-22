#include "ui/Emoji.h"

#include "ui/Theme.h"

namespace {

constexpr uint32_t REGIONAL_A = 0x1F1E6;
constexpr uint32_t REGIONAL_Z = 0x1F1FF;
constexpr uint32_t ZWJ = 0x200D;
constexpr int UNKNOWN_DOT_R = 4;

bool isRegional(uint32_t cp) { return cp >= REGIONAL_A && cp <= REGIONAL_Z; }

// Caracteres que acompañan a un emoji y no se dibujan: selectores de variante,
// tonos de piel y el circulo de las teclas (1️⃣).
bool isModifier(uint32_t cp) {
  return cp == 0xFE0F || cp == 0xFE0E || (cp >= 0x1F3FB && cp <= 0x1F3FF) || cp == 0x20E3;
}

// Bloques de simbolos y emojis. Lo que hay en ellos y no esta en la tabla se
// dibuja como un punto (la fuente tampoco lo tiene).
bool isEmojiRange(uint32_t cp) {
  return (cp >= 0x2190 && cp <= 0x2BFF) || (cp >= 0x1F000 && cp <= 0x1FAFF);
}

const Emoji::Image* find(uint32_t cp, uint32_t second) {
  size_t lo = 0, hi = Emoji::IMAGE_COUNT;
  while (lo < hi) {
    const size_t mid = (lo + hi) / 2;
    const Emoji::Image& img = Emoji::IMAGES[mid];
    if (img.codepoint == cp && img.second == second) return &img;
    if (img.codepoint < cp || (img.codepoint == cp && img.second < second)) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return nullptr;
}

// Salta los modificadores y lo que venga unido con ZWJ; devuelve los bytes saltados.
size_t skipJoined(const char* s) {
  size_t len = 0;
  while (s[len]) {
    uint32_t cp;
    const size_t n = Emoji::decodeUtf8(s + len, cp);
    if (isModifier(cp)) {
      len += n;
    } else if (cp == ZWJ) {
      len += n;
      if (s[len]) {
        uint32_t next;
        len += Emoji::decodeUtf8(s + len, next);  // el emoji unido: no se dibuja
      }
    } else {
      break;
    }
  }
  return len;
}

uint16_t blend(uint16_t fg, uint16_t bg, uint8_t alpha) {
  if (alpha == 255) return fg;
  if (alpha == 0) return bg;
  const uint32_t a = alpha, na = 255 - alpha;
  const uint32_t r = (((fg >> 11) & 0x1F) * a + ((bg >> 11) & 0x1F) * na) / 255;
  const uint32_t g = (((fg >> 5) & 0x3F) * a + ((bg >> 5) & 0x3F) * na) / 255;
  const uint32_t b = ((fg & 0x1F) * a + (bg & 0x1F) * na) / 255;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

}  // namespace

namespace Emoji {

size_t decodeUtf8(const char* s, uint32_t& cp) {
  const uint8_t c = (uint8_t)s[0];
  size_t n = 1;
  if (c < 0x80) {
    cp = c;
    return 1;
  } else if ((c & 0xE0) == 0xC0) {
    cp = c & 0x1F;
    n = 2;
  } else if ((c & 0xF0) == 0xE0) {
    cp = c & 0x0F;
    n = 3;
  } else if ((c & 0xF8) == 0xF0) {
    cp = c & 0x07;
    n = 4;
  } else {
    cp = 0xFFFD;  // byte suelto: se avanza uno
    return 1;
  }
  for (size_t i = 1; i < n; i++) {
    const uint8_t cc = (uint8_t)s[i];
    if ((cc & 0xC0) != 0x80) {
      cp = 0xFFFD;  // caracter cortado
      return i;
    }
    cp = (cp << 6) | (cc & 0x3F);
  }
  return n;
}

size_t parse(const char* s, const Image** image) {
  uint32_t cp;
  const size_t n = decodeUtf8(s, cp);

  // Bandera: dos letras regionales seguidas.
  if (isRegional(cp)) {
    uint32_t second = 0;
    const size_t n2 = s[n] ? decodeUtf8(s + n, second) : 0;
    if (isRegional(second)) {
      *image = find(cp, second);
      return n + n2;
    }
    *image = nullptr;
    return n;
  }

  const Image* found = find(cp, 0);
  if (found == nullptr && !isEmojiRange(cp)) {
    return 0;
  }
  *image = found;
  return n + skipJoined(s + n);
}

void draw(lgfx::LovyanGFX& g, int x, int cy, const Image* image, uint16_t bg, bool mini) {
  const int size = mini ? MINI_SIZE : SIZE;
  const int top = cy - size / 2;
  if (image == nullptr) {
    g.fillCircle(x + 1 + size / 2, cy, mini ? 2 : UNKNOWN_DOT_R, Theme::TEXT_DIM);
    return;
  }
  const uint16_t* rgb = mini ? image->miniRgb : image->rgb;
  const uint8_t* alpha = mini ? image->miniAlpha : image->alpha;
  // Se mezcla con el fondo en un buffer pequeño (648 bytes) y se envia de una vez.
  lgfx::rgb565_t pixels[SIZE * SIZE];
  for (int i = 0; i < size * size; i++) {
    pixels[i].raw = blend(rgb[i], bg, alpha[i]);
  }
  g.pushImage(x + 1, top, size, size, pixels);
}

void drawZoom(lgfx::LovyanGFX& g, int x, int cy, const Image* image, uint16_t bg, int zoom) {
  const int size = SIZE * zoom;
  if (image == nullptr) {
    g.fillCircle(x + 1 + size / 2, cy, UNKNOWN_DOT_R * zoom, Theme::TEXT_DIM);
    return;
  }
  const int top = cy - size / 2;
  // Cada pixel del emoji pequeño es un cuadrado de zoom x zoom, mezclado con el
  // fondo. Los transparentes no se dibujan: quien llama ya ha borrado la zona.
  g.startWrite();
  for (int j = 0; j < SIZE; j++) {
    for (int i = 0; i < SIZE; i++) {
      const int k = j * SIZE + i;
      if (image->alpha[k] == 0) continue;
      g.fillRect(x + 1 + i * zoom, top + j * zoom, zoom, zoom, blend(image->rgb[k], bg, image->alpha[k]));
    }
  }
  g.endWrite();
}

}  // namespace Emoji
