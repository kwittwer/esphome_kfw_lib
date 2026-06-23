#!/usr/bin/env python3
"""
Dump Unicode code points and glyph names from a TTF/OTF
as lines like:
    "\\U000F0319", # lan-disconnect
Usage:
    python dump_unicode_lines.py path/to/font.ttf [-o output.txt] [--only-pua]
"""

import argparse
import sys
import re
from fontTools.ttLib import TTFont

def sanitize_name(name: str) -> str:
    """
    Make glyph names more comment-friendly:
    - lower-case
    - replace spaces and dots with hyphens
    - strip leading 'uniXXXX'/'uXXXX' if there's a better name alongside
    """
    s = name or ""
    s = s.strip()
    # Common auto-generated patterns like 'uniF0319' or 'uF0319'
    s = re.sub(r'^(uni|u)[0-9A-Fa-f]{4,6}$', '', s)
    # Replace separators with hyphens, collapse repeats
    s = re.sub(r'[.\s_/]+', '-', s)
    s = re.sub(r'-{2,}', '-', s)
    s = s.strip('-').lower()
    return s or "glyph"

def main():
    ap = argparse.ArgumentParser(description="Dump Unicode escape + glyph name lines from a font.")
    ap.add_argument("font", help="Path to .ttf/.otf file")
    ap.add_argument("-o", "--output", help="Optional output file (default: stdout)")
    ap.add_argument("--only-pua", action="store_true",
                    help="Only list Private Use Area codepoints (U+E000–U+F8FF, U+F0000–U+FFFFD, U+100000–U+10FFFD)")
    args = ap.parse_args()

    try:
        font = TTFont(args.font)
    except Exception as e:
        print(f"Error opening font: {e}", file=sys.stderr)
        sys.exit(1)

    # Select a Unicode cmap subtable (prefer 12/10 over 4 if available)
    cmap_table = font.get("cmap")
    if not cmap_table:
        print("No cmap table found.", file=sys.stderr)
        sys.exit(2)

    chosen = None
    # Prefer format 12 (UCS-4), then 10, then 4
    formats_pref = [12, 10, 4]
    for fmt in formats_pref:
        for sub in cmap_table.tables:
            if sub.isUnicode() and getattr(sub, "format", None) == fmt:
                chosen = sub
                break
        if chosen:
            break
    # Fallback to any Unicode subtable
    if not chosen:
        for sub in cmap_table.tables:
            if sub.isUnicode():
                chosen = sub
                break

    if not chosen or not getattr(chosen, "cmap", None):
        print("No Unicode cmap subtable found.", file=sys.stderr)
        sys.exit(3)

    items = []
    for cp, glyph_name in chosen.cmap.items():
        # Optionally restrict to PUA ranges (häufig bei Icon-Fonts)
        if args.only_pua:
            in_bmp_pua = 0xE000 <= cp <= 0xF8FF
            in_plane15 = 0xF0000 <= cp <= 0xFFFFD
            in_plane16 = 0x100000 <= cp <= 0x10FFFD
            if not (in_bmp_pua or in_plane15 or in_plane16):
                continue

        # Build escaped sequence with 8 hex digits
        escaped = f"\\U{cp:08X}"
        name = sanitize_name(glyph_name)
        items.append((cp, escaped, name))

    items.sort(key=lambda t: t[0])

    out = sys.stdout
    close_out = False
    if args.output:
        out = open(args.output, "w", encoding="utf-8")
        close_out = True

    for _, esc, name in items:
        print(f"\"{esc}\", # {name}", file=out)

    if close_out:
        out.close()

if __name__ == "__main__":
    main()