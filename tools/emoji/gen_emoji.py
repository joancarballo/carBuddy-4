#!/usr/bin/env python3
"""Genera src/ui/EmojiData.cpp con los emojis a color de Twemoji.

Se ejecuta a mano en el ordenador solo al cambiar la lista de emojis; el
firmware no lo necesita. Descarga las imagenes de 72x72 de Twemoji (licencia
CC-BY 4.0, (c) Twitter, Inc. y colaboradores; https://github.com/jdecked/twemoji),
las reduce a SIZE x SIZE y a MINI_SIZE x MINI_SIZE (para las vistas Semana y Mes)
y las guarda como RGB565 mas un canal alfa de 8 bits, para mezclarlas en la
placa con el color de fondo que toque.

Uso (necesita Pillow):
    python3 tools/emoji/gen_emoji.py <carpeta_cache> src/ui/EmojiData.cpp
"""

import os
import sys
import urllib.request

from PIL import Image

TWEMOJI_VERSION = "15.1.0"
URL = "https://cdn.jsdelivr.net/gh/jdecked/twemoji@{v}/assets/72x72/{name}.png"
SIZE = 18       # tienen que coincidir con Emoji::SIZE y Emoji::MINI_SIZE (src/ui/Emoji.h)
MINI_SIZE = 9

# Los de la v1 (core/Emoji.cpp), en el mismo orden.
V1_EMOJIS = [
    0x1F467, 0x1F476, 0x1F30A, 0x1F483, 0x1F57A, 0x1F3EB, 0x1F466, 0x1F468, 0x1F469, 0x1F600,
    0x1F601, 0x1F603, 0x1F604, 0x1F606, 0x1F60A, 0x1F642, 0x263A, 0x1F973, 0x1F622, 0x1F61E,
    0x1F614, 0x1F62D, 0x2639, 0x1F60E, 0x1F3C3, 0x1F6B6, 0x1F3CA, 0x1F4AA, 0x1F3CB, 0x2764,
    0x1F493, 0x1F495, 0x1F496, 0x1F497, 0x1F498, 0x1F499, 0x1F49A, 0x1F49B, 0x1F49C, 0x1F49D,
    0x1F49E, 0x1F5A4, 0x1F90D, 0x1F90E, 0x1F9E1, 0x2B50, 0x1F31F, 0x2728, 0x2705, 0x2714,
    0x2611, 0x274C, 0x274E, 0x2716, 0x2757, 0x2755, 0x203C, 0x2753, 0x2754, 0x26A0,
    0x1F525, 0x1F4B0, 0x1F4B3, 0x1F4B5, 0x1F4B6, 0x1F4B8, 0x2600, 0x1F31E, 0x1F319, 0x2601,
    0x26C5, 0x1F327, 0x2614, 0x2602, 0x2744, 0x26C4, 0x1F328, 0x1F338, 0x1F337, 0x1F339,
    0x1F33A, 0x1F33B, 0x1F33C, 0x1F490, 0x1F332, 0x1F333, 0x1F384, 0x26F0, 0x1F3D4, 0x1F3D5,
    0x1F436, 0x1F415, 0x1F431, 0x1F408, 0x1F382, 0x1F370, 0x1F9C1, 0x1F381, 0x1F389, 0x1F38A,
    0x1F388, 0x2615, 0x1F375, 0x1F374, 0x1F37D, 0x1F355, 0x1F354, 0x1F377, 0x1F378, 0x1F379,
    0x1F942, 0x1F37A, 0x1F37B, 0x2708, 0x1F6E9, 0x1F6EB, 0x1F6EC, 0x1F695, 0x1F697, 0x1F698,
    0x1F699, 0x1F68C, 0x1F68D, 0x1F690, 0x1F682, 0x1F683, 0x1F684, 0x1F685, 0x1F686, 0x1F687,
    0x1F688, 0x1F689, 0x1F68A, 0x1F6B2, 0x1F6B4, 0x1F6B5, 0x1F3E0, 0x1F3E1, 0x1F3D6, 0x26F1,
    0x1F3E5, 0x1F691, 0x1FA7A, 0x2695, 0x1F4BC, 0x1F4BB, 0x1F5A5, 0x1F4F1, 0x1F4F2, 0x1F4DE,
    0x260E, 0x2709, 0x1F4E7, 0x1F4E8, 0x1F4E9, 0x1F48C, 0x1F4C5, 0x1F4C6, 0x1F5D3, 0x1F4DA,
    0x1F4D2, 0x1F4D3, 0x1F4D4, 0x1F4D5, 0x1F4D6, 0x1F4D7, 0x1F4D8, 0x1F4D9, 0x270F, 0x2712,
    0x1F4DD, 0x1F58A, 0x1F58B, 0x1F393, 0x1F4CC, 0x1F4CD, 0x23F0, 0x23F1, 0x23F2, 0x231A,
    0x1F514, 0x1F48A, 0x1F9B7, 0x1F6D2, 0x1F6CD, 0x26BD, 0x1F3C0, 0x1F3B5, 0x1F3B6, 0x1F3BC,
    0x1F3AC, 0x1F3A5, 0x1F37F, 0x1F4FA, 0x1F3AE,
]

# Añadidos en la v2: habituales en un calendario.
V2_EMOJIS = [
    0x1F392,  # mochila
    0x1F487,  # peluqueria
    0x1F3A8,  # pintura
    0x1F3AD,  # teatro
    0x1F3B8,  # guitarra
    0x1F3B9,  # piano
    0x1F3BB,  # violin
    0x1F3BE,  # tenis
    0x1F9D1,  # persona
    0x1F474,  # abuelo
    0x1F475,  # abuela
    0x1F43E,  # huellas
    0x1F6C1,  # bañera
    0x1F3C6,  # trofeo
]

# Banderas: pares de letras regionales.
FLAGS = ["ES", "GB"]

REGIONAL_A = 0x1F1E6


def flag_codepoints(letters):
    return [REGIONAL_A + ord(c) - ord("A") for c in letters]


def load(cache_dir, codepoints, size):
    """Imagen RGBA de size x size para la secuencia de codepoints."""
    name = "-".join("%x" % cp for cp in codepoints)
    path = os.path.join(cache_dir, name + ".png")
    if not os.path.exists(path):
        url = URL.format(v=TWEMOJI_VERSION, name=name)
        with urllib.request.urlopen(url) as resp, open(path, "wb") as f:
            f.write(resp.read())
    img = Image.open(path).convert("RGBA")
    # Pillow reduce RGBA teniendo en cuenta el alfa (no oscurece los bordes).
    return img.resize((size, size), Image.LANCZOS)


def encode(img):
    rgb = []
    alpha = []
    for r, g, b, a in img.getdata():
        rgb.append(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
        alpha.append(a)
    return rgb, alpha


def c_array(ctype, name, values, per_line):
    fmt = "0x%04X" if ctype == "uint16_t" else "%3d"
    lines = []
    for i in range(0, len(values), per_line):
        lines.append("    " + ", ".join(fmt % v for v in values[i:i + per_line]) + ",")
    return "const %s %s[] = {\n%s\n};\n" % (ctype, name, "\n".join(lines))


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    cache_dir, out_path = sys.argv[1], sys.argv[2]
    os.makedirs(cache_dir, exist_ok=True)

    # (cp, cp2): cp2 es la segunda letra de una bandera, o 0.
    entries = [(cp, 0) for cp in V1_EMOJIS + V2_EMOJIS]
    entries += [tuple(flag_codepoints(f)) for f in FLAGS]
    entries = sorted(set(entries))  # ordenados para buscar con busqueda binaria

    arrays = []
    table = []
    for cp, cp2 in entries:
        cps = [cp] + ([cp2] if cp2 else [])
        tag = "_".join("%X" % c for c in cps)
        rgb, alpha = encode(load(cache_dir, cps, SIZE))
        arrays.append(c_array("uint16_t", "RGB_" + tag, rgb, 12))
        arrays.append(c_array("uint8_t", "ALPHA_" + tag, alpha, 18))
        rgb, alpha = encode(load(cache_dir, cps, MINI_SIZE))
        arrays.append(c_array("uint16_t", "MINI_RGB_" + tag, rgb, 9))
        arrays.append(c_array("uint8_t", "MINI_ALPHA_" + tag, alpha, 9))
        table.append("    {0x%X, 0x%X, RGB_%s, ALPHA_%s, MINI_RGB_%s, MINI_ALPHA_%s},"
                     % (cp, cp2, tag, tag, tag, tag))

    with open(out_path, "w") as f:
        f.write("// Generado por tools/emoji/gen_emoji.py: no editar a mano.\n")
        f.write("// Emojis de Twemoji %s (CC-BY 4.0, (c) Twitter, Inc. y colaboradores),\n"
                % TWEMOJI_VERSION)
        f.write("// %dx%d y %dx%d, RGB565 y alfa de 8 bits. Van en la flash, no gastan RAM.\n\n"
                % (SIZE, SIZE, MINI_SIZE, MINI_SIZE))
        f.write('#include "ui/Emoji.h"\n\nnamespace {\n\n')
        f.write("\n".join(arrays))
        f.write("\n}  // namespace\n\nnamespace Emoji {\n\n")
        f.write("const Image IMAGES[] = {\n%s\n};\n\n" % "\n".join(table))
        f.write("const size_t IMAGE_COUNT = sizeof(IMAGES) / sizeof(IMAGES[0]);\n\n")
        f.write("}  // namespace Emoji\n")
    print("%d emojis -> %s" % (len(entries), out_path))


if __name__ == "__main__":
    main()
