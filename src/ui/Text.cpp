#include "ui/Text.h"

#include <string>

#include "ui/Emoji.h"

namespace {

const char* const ELLIPSIS = "...";  // la fuente no tiene el caracter de puntos suspensivos

// Caracteres que la fuente no tiene y su equivalente sencillo.
struct Replacement {
  uint32_t codepoint;
  const char* text;
};
const Replacement REPLACEMENTS[] = {
    {0x2018, "'"}, {0x2019, "'"}, {0x201C, "\""}, {0x201D, "\""}, {0x2013, "-"},
    {0x2014, "-"}, {0x2026, "..."}, {0x20AC, "EUR"}, {0x2022, "\xC2\xB7"},  // • -> ·
};

const char* replacementFor(uint32_t cp) {
  for (const Replacement& r : REPLACEMENTS) {
    if (r.codepoint == cp) return r.text;
  }
  return nullptr;
}

}  // namespace

String Text::normalize(const char* text) {
  String out;
  out.reserve(strlen(text));
  const char* p = text;
  while (*p) {
    uint32_t cp;
    const size_t n = Emoji::decodeUtf8(p, cp);
    const char* r = replacementFor(cp);
    if (r) {
      out += r;
    } else {
      for (size_t i = 0; i < n; i++) out += p[i];
    }
    p += n;
  }
  return out;
}

namespace {

// Bytes del siguiente "trozo indivisible": un emoji entero (con sus modificadores)
// o un caracter UTF-8.
size_t unitLength(const char* p) {
  const Emoji::Image* image;
  const size_t n = Emoji::parse(p, &image);
  if (n > 0) return n;
  uint32_t cp;
  return Emoji::decodeUtf8(p, cp);
}

// Recorre el texto en tramos: texto normal (sin emojis) o un emoji.
// onRun(inicio, bytes, esEmoji, imagen).
template <typename F>
void forEachRun(const char* s, F onRun) {
  const char* runStart = s;
  const char* p = s;
  while (*p) {
    const Emoji::Image* image = nullptr;
    const size_t n = Emoji::parse(p, &image);
    if (n == 0) {
      uint32_t cp;
      p += Emoji::decodeUtf8(p, cp);
      continue;
    }
    if (p > runStart) onRun(runStart, (size_t)(p - runStart), false, nullptr);
    onRun(p, n, true, image);
    p += n;
    runStart = p;
  }
  if (p > runStart) onRun(runStart, (size_t)(p - runStart), false, nullptr);
}

// Ancho con la fuente ya puesta en g.
int measure(lgfx::LovyanGFX& g, const char* s, bool mini) {
  int w = 0;
  forEachRun(s, [&](const char* start, size_t len, bool isEmoji, const Emoji::Image*) {
    w += isEmoji ? Emoji::advance(mini) : g.textWidth(std::string(start, len).c_str());
  });
  return w;
}

// Parte una palabra demasiado larga: va sacando el trozo mas largo que cabe.
void breakLongWord(lgfx::LovyanGFX& g, String& word, int maxWidth, bool mini,
                   std::vector<String>& lines) {
  while (word.length() > 0 && measure(g, word.c_str(), mini) > maxWidth) {
    // Al menos un trozo por linea, aunque no quepa, para no quedarse en bucle.
    size_t cut = unitLength(word.c_str());
    while (cut < word.length()) {
      const size_t next = cut + unitLength(word.c_str() + cut);
      if (measure(g, word.substring(0, next).c_str(), mini) > maxWidth) break;
      cut = next;
    }
    lines.push_back(word.substring(0, cut));
    word = word.substring(cut);
  }
}

}  // namespace

namespace Text {

int width(lgfx::LovyanGFX& g, const lgfx::IFont* font, const char* text, bool miniEmoji) {
  g.setFont(font);
  return measure(g, normalize(text).c_str(), miniEmoji);
}

void draw(lgfx::LovyanGFX& g, const lgfx::IFont* font, const char* text, int x, int cy,
          uint16_t fg, uint16_t bg, bool miniEmoji) {
  g.setFont(font);
  g.setTextColor(fg);
  g.setTextDatum(textdatum_t::middle_left);
  const String s = normalize(text);
  forEachRun(s.c_str(), [&](const char* start, size_t len, bool isEmoji, const Emoji::Image* image) {
    if (isEmoji) {
      Emoji::draw(g, x, cy, image, bg, miniEmoji);
      x += Emoji::advance(miniEmoji);
    } else {
      const std::string run(start, len);
      g.drawString(run.c_str(), x, cy);
      x += g.textWidth(run.c_str());
    }
  });
}

void box(lgfx::LovyanGFX& g, int x, int y, int w, int h, const char* text,
         const lgfx::IFont* font, uint16_t fg, uint16_t bg, Align align) {
  LGFX_Sprite sprite(&g);
  sprite.setColorDepth(16);
  const bool useSprite = sprite.createSprite(w, h) != nullptr;
  // Sin memoria para el sprite se dibuja directamente (puede parpadear).
  lgfx::LovyanGFX& d = useSprite ? (lgfx::LovyanGFX&)sprite : g;
  const int ox = useSprite ? 0 : x;
  const int oy = useSprite ? 0 : y;

  d.fillRect(ox, oy, w, h, bg);
  const int textW = width(d, font, text);
  int tx = ox;
  if (align == Align::Center) tx = ox + (w - textW) / 2;
  if (align == Align::Right) tx = ox + w - 1 - textW;
  draw(d, font, text, tx, oy + h / 2, fg, bg);

  if (useSprite) {
    sprite.pushSprite(x, y);
    sprite.deleteSprite();
  }
}

String fit(lgfx::LovyanGFX& g, const lgfx::IFont* font, const char* text, int maxWidth,
           bool miniEmoji) {
  g.setFont(font);
  const String s = normalize(text);
  if (measure(g, s.c_str(), miniEmoji) <= maxWidth) return s;

  // El trozo mas largo que cabe con los "..." detras.
  size_t cut = 0;
  while (cut < s.length()) {
    const size_t next = cut + unitLength(s.c_str() + cut);
    if (measure(g, (s.substring(0, next) + ELLIPSIS).c_str(), miniEmoji) > maxWidth) break;
    cut = next;
  }
  String out = s.substring(0, cut);
  out.trim();
  return out + ELLIPSIS;
}

std::vector<String> wrap(lgfx::LovyanGFX& g, const lgfx::IFont* font, const char* text,
                         int maxWidth, bool miniEmoji) {
  g.setFont(font);
  const String all = normalize(text);
  std::vector<String> lines;
  int pos = 0;
  while (true) {
    // Un parrafo: hasta el siguiente '\n' o el final.
    const int end = all.indexOf('\n', pos);
    const String paragraph = all.substring(pos, end < 0 ? all.length() : end);

    String line;
    int start = 0;
    while (start <= (int)paragraph.length()) {
      int space = paragraph.indexOf(' ', start);
      if (space < 0) space = paragraph.length();
      String word = paragraph.substring(start, space);
      start = space + 1;
      if (word.length() == 0) continue;

      const String candidate = line.length() == 0 ? word : line + " " + word;
      if (measure(g, candidate.c_str(), miniEmoji) <= maxWidth) {
        line = candidate;
        continue;
      }
      if (line.length() > 0) {
        lines.push_back(line);
      }
      breakLongWord(g, word, maxWidth, miniEmoji, lines);
      line = word;
    }
    lines.push_back(line);  // un parrafo vacio deja una linea en blanco

    if (end < 0) break;
    pos = end + 1;
  }
  return lines;
}

}  // namespace Text
