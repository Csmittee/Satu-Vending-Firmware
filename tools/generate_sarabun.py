#!/usr/bin/env python3
"""
generate_sarabun.py — Generate firmware/SarabanSubset.h from Sarabun-Regular.ttf
====================================================================================
Satu Project D-11 Thai language support — font bitmap generation tool.

Requirements:
    pip install freetype-py requests
  OR on Ubuntu/Debian:
    sudo apt-get install python3-freetype python3-requests

Usage (run from repo root):
    python3 tools/generate_sarabun.py

Downloads Sarabun-Regular.ttf from Google Fonts (Apache 2.0 license, free to use),
renders Thai glyphs U+0E01–U+0E4C at 12pt, 18pt, 24pt at 96 DPI,
and overwrites firmware/SarabanSubset.h with real GFX bitmap data.

Glyph indexing: first=0, last=75 (glyph index = codepoint - 0x0E01).
This matches the printThai() custom renderer in ui_strings.h exactly.

After running this script:
1. Review the output file: firmware/SarabanSubset.h
2. Compile and flash — Thai text will be visible.
3. Commit: git add firmware/SarabanSubset.h && git commit -m "D-11: populate SarabanSubset.h with real Sarabun bitmaps"

COMBINING MARKS (xAdvance=0, overlay preceding consonant):
  U+0E31 ั  U+0E34-0E37 ิ ี ึ ื  U+0E38-0E39 ุ ู  U+0E3A ฺ
  U+0E47 ็  U+0E48-0E4C ่ ้ ๊ ๋ ์

UNDEFINED RANGE (U+0E3B-0E3E): rendered as empty glyphs.
"""

import os
import sys
import struct
import urllib.request
import datetime

# ── Thai range ────────────────────────────────────────────────────────────────
THAI_FIRST  = 0x0E01  # ก
THAI_LAST   = 0x0E4C  # ์
THAI_COUNT  = THAI_LAST - THAI_FIRST + 1  # 76

# Combining marks: xAdvance = 0 so cursor does NOT move (overlay on preceding consonant)
COMBINING = frozenset([
    0x0E31,                                    # ั
    0x0E34, 0x0E35, 0x0E36, 0x0E37,           # ิ ี ึ ื
    0x0E38, 0x0E39,                            # ุ ู
    0x0E3A,                                    # ฺ
    0x0E47,                                    # ็
    0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C,  # ่ ้ ๊ ๋ ์
])

# Undefined Unicode slots in 0E3B-0E3E — no character exists
UNDEFINED = frozenset([0x0E3B, 0x0E3C, 0x0E3D, 0x0E3E])

# ── Render sizes ─────────────────────────────────────────────────────────────
# (point_size, struct_name, yAdvance_px)
# yAdvance = total line height in pixels (baseline to next baseline)
SIZES = [
    (12, "SarabanSubset_12pt", 18),
    (18, "SarabanSubset_18pt", 26),
    (24, "SarabanSubset_24pt", 34),
]

DPI = 96  # Must match fontconvert default — Arduino_GFX uses 96 DPI convention

# ── Font download ─────────────────────────────────────────────────────────────
FONT_URL  = "https://github.com/google/fonts/raw/main/ofl/sarabun/Sarabun-Regular.ttf"
FONT_PATH = "/tmp/Sarabun-Regular.ttf"
OUT_PATH  = os.path.join(os.path.dirname(__file__), "..", "firmware", "SarabanSubset.h")


def download_font():
    if os.path.exists(FONT_PATH):
        print(f"[font] Using cached {FONT_PATH}")
        return
    print(f"[font] Downloading Sarabun-Regular.ttf from Google Fonts...")
    try:
        urllib.request.urlretrieve(FONT_URL, FONT_PATH)
        print(f"[font] Saved to {FONT_PATH}")
    except Exception as e:
        print(f"[font] ERROR: {e}")
        print("[font] Download Sarabun-Regular.ttf manually and place at /tmp/Sarabun-Regular.ttf")
        sys.exit(1)


def pack_mono_bitmap(ft_bitmap):
    """Pack a FreeType bitmap into GFX 1-bit MSB-first format.

    FreeType may return pixel_mode=1 (FT_PIXEL_MODE_MONO: 1-bit packed, MSB first, pitch bytes/row)
    or pixel_mode=2 (FT_PIXEL_MODE_GRAY: 1 byte per pixel, 0=black, 255=white).
    GFX format: 1 bit per pixel, MSB first, rows padded to byte boundaries.

    pitch can be negative for bottom-up bitmaps — always use abs(pitch).
    Returns (packed_bytes: bytes, width: int, height: int).
    """
    w = ft_bitmap.width
    h = ft_bitmap.rows
    if w == 0 or h == 0:
        return b"", 0, 0

    row_bytes_dst = (w + 7) // 8  # bytes per row in GFX format
    pitch = abs(ft_bitmap.pitch)  # abs(): FreeType may use negative pitch for bottom-up bitmaps
    result = bytearray()

    if ft_bitmap.pixel_mode == 1:  # FT_PIXEL_MODE_MONO: already 1-bit packed, MSB first
        for row in range(h):
            src_start = row * pitch
            for byte_col in range(row_bytes_dst):
                src_idx = src_start + byte_col
                result.append(ft_bitmap.buffer[src_idx] if src_idx < len(ft_bitmap.buffer) else 0)
    else:  # FT_PIXEL_MODE_GRAY (pixel_mode=2): 1 byte per pixel, threshold at 128
        for row in range(h):
            src_start = row * pitch
            packed_byte = 0
            bits_written = 0
            for col in range(w):
                src_idx = src_start + col
                gray = ft_bitmap.buffer[src_idx] if src_idx < len(ft_bitmap.buffer) else 0
                packed_byte = (packed_byte << 1) | (1 if gray >= 128 else 0)
                bits_written += 1
                if bits_written == 8:
                    result.append(packed_byte)
                    packed_byte = 0
                    bits_written = 0
            if bits_written > 0:  # flush partial byte, pad with 0 bits on the right
                result.append(packed_byte << (8 - bits_written))

    return bytes(result), w, h


def render_size(face, size_pt):
    """Render all 76 Thai glyphs at size_pt and return (bitmap_blob, glyph_list).

    glyph_list: list of (bitmapOffset, width, height, xAdvance, xOffset, yOffset)
                — matches GFXglyph struct field order.
    """
    import freetype
    face.set_char_size(size_pt * 64, 0, DPI, 0)

    bitmap_blob = bytearray()
    glyph_list  = []

    for i in range(THAI_COUNT):
        cp = THAI_FIRST + i

        if cp in UNDEFINED:
            # Empty slot — zero dimensions, zero advance
            glyph_list.append((0, 0, 0, 0, 0, 0))
            continue

        face.load_char(cp, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO)
        slot = face.glyph
        bm   = slot.bitmap

        packed, w, h = pack_mono_bitmap(bm)

        offset   = len(bitmap_blob)
        bitmap_blob.extend(packed)

        x_advance = slot.advance.x >> 6  # 26.6 fixed → integer pixels
        x_offset  = slot.bitmap_left     # distance right from cursor to glyph left edge
        y_offset  = -slot.bitmap_top     # GFX yOffset is negative above baseline

        if cp in COMBINING:
            x_advance = 0  # Combining marks: do not advance cursor

        glyph_list.append((offset, w, h, x_advance, x_offset, y_offset))

    return bytes(bitmap_blob), glyph_list


def fmt_hex_array(data, indent="  ", cols=16):
    """Format bytes as C hex array body (no braces, no declaration)."""
    lines = []
    for i in range(0, len(data), cols):
        chunk = data[i:i+cols]
        lines.append(indent + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    return "\n".join(lines)


def fmt_glyph_array(glyphs, size_pt):
    """Format GFXglyph array entries with Thai character comments."""
    lines = []
    THAI_NAMES = [
        "ก","ข","ฃ","ค","ฅ","ฆ","ง","จ","ฉ","ช","ซ","ฌ","ญ","ฎ","ฏ",
        "ฐ","ฑ","ฒ","ณ","ด","ต","ถ","ท","ธ","น","บ","ป","ผ","ฝ","พ",
        "ฟ","ภ","ม","ย","ร","ฤ","ล","ฦ","ว","ศ","ษ","ส","ห","ฬ","อ",
        "ฮ","ๆ","ะ","ั","า","ำ","ิ","ี","ึ","ื","ุ","ู","ฺ",
        "--","--","--","--",
        "฿","เ","แ","โ","ใ","ไ","ๅ","ๆ","็","่","้","๊","๋","์",
    ]
    for i, (off, w, h, xa, xo, yo) in enumerate(glyphs):
        cp   = THAI_FIRST + i
        name = THAI_NAMES[i] if i < len(THAI_NAMES) else "?"
        tag  = "CA" if cp in COMBINING else ("EM" if cp in UNDEFINED else "NC")
        lines.append(
            f"  {{ {off:5d}, {w:3d}, {h:3d}, {xa:3d}, {xo:4d}, {yo:4d} }},  "
            f"// {i:2d}  U+{cp:04X}  {name}  {tag}"
        )
    return "\n".join(lines)


def generate_header(sizes_data):
    date = datetime.date.today().isoformat()
    lines = []

    lines.append("// ============================================================")
    lines.append("// SarabanSubset.h — Thai GFXfont for Satu D-11")
    lines.append("// ============================================================")
    lines.append(f"// Generated: {date} by tools/generate_sarabun.py")
    lines.append("// Font: Sarabun-Regular.ttf — Apache 2.0 license (Google Fonts)")
    lines.append("// Glyphs: U+0E01 (ก) to U+0E4C (์) = 76 glyphs, 3 sizes")
    lines.append("// Glyph index = codepoint - 0x0E01 (first=0, last=75)")
    lines.append("// Render: 96 DPI, 1-bit monochrome, MSB-first row packing")
    lines.append("// Combining marks (ั ิ ี ึ ื ุ ู ฺ ็ ่ ้ ๊ ๋ ์): xAdvance=0")
    lines.append("// Rendering: use printThai() in ui_strings.h — NOT gfx->print()")
    lines.append("// DO NOT EDIT MANUALLY — regenerate with tools/generate_sarabun.py")
    lines.append("// ============================================================")
    lines.append("")
    lines.append("#pragma once")
    lines.append("#include <Arduino_GFX_Library.h>")
    lines.append("")

    for (size_pt, struct_name, y_advance), (bitmap_blob, glyph_list) in sizes_data:
        lines.append(f"// {'═'*60}")
        lines.append(f"//  {size_pt}pt")
        lines.append(f"// {'═'*60}")
        lines.append("")

        # Bitmap array
        bmp_name = f"_{struct_name}Bitmaps"
        lines.append(f"static const uint8_t {bmp_name}[] PROGMEM = {{")
        if bitmap_blob:
            lines.append(fmt_hex_array(bitmap_blob))
        lines.append("};")
        lines.append("")

        # Glyph array
        gly_name = f"_{struct_name}Glyphs"
        lines.append(f"static const GFXglyph {gly_name}[] PROGMEM = {{")
        lines.append("  // { bitmapOffset, width, height, xAdvance, xOffset, yOffset }")
        lines.append(fmt_glyph_array(glyph_list, size_pt))
        lines.append("};")
        lines.append("")

        # Font struct
        lines.append(f"const GFXfont {struct_name} PROGMEM = {{")
        lines.append(f"  (uint8_t*){bmp_name},")
        lines.append(f"  (GFXglyph*){gly_name},")
        lines.append(f"  0,    // first = glyph index 0 (U+0E01 = ก)")
        lines.append(f"  75,   // last  = glyph index 75 (U+0E4C = ์)")
        lines.append(f"  {y_advance}    // yAdvance")
        lines.append("};")
        lines.append("")

    return "\n".join(lines)


def main():
    try:
        import freetype
    except ImportError:
        print("[ERROR] freetype-py not found. Install with:")
        print("  pip install freetype-py")
        print("  OR: sudo apt-get install python3-freetype")
        sys.exit(1)

    download_font()

    print("[render] Loading Sarabun-Regular.ttf...")
    face = freetype.Face(FONT_PATH)

    sizes_data = []
    for size_spec in SIZES:
        size_pt, struct_name, y_advance = size_spec
        print(f"[render] {size_pt}pt — rendering {THAI_COUNT} glyphs...")
        bitmap_blob, glyph_list = render_size(face, size_pt)
        sizes_data.append((size_spec, (bitmap_blob, glyph_list)))
        print(f"[render] {size_pt}pt — bitmap blob: {len(bitmap_blob)} bytes, {len(glyph_list)} glyphs")

    print(f"[write ] Generating {OUT_PATH} ...")
    header = generate_header(sizes_data)
    out = os.path.normpath(OUT_PATH)
    with open(out, "w", encoding="utf-8") as f:
        f.write(header)
    print(f"[write ] Done. {len(header)} chars written.")
    print()
    print("Next steps:")
    print(f"  1. git add firmware/SarabanSubset.h")
    print(f"  2. git commit -m 'D-11: populate SarabanSubset.h with real Sarabun bitmaps'")
    print(f"  3. git push origin claude/dazzling-fermi-aefx6t")
    print(f"  4. Compile + flash — Thai text now visible on hardware")


if __name__ == "__main__":
    main()
