#!/usr/bin/env python3
"""
Erzeuge ein PDF mit allen (oder nur PUA-) Icons einer TTF/OTF – jeweils mit Namen & Unicode.
Verwendet fontTools (cmap, Glyphnamen) und PyMuPDF (PDF-Rendering).

Beispiel:
    python font_icons_to_pdf.py ./MaterialDesignIcons.ttf -o icons.pdf --cols 8 --rows 10 --pt 48 --only-pua
"""

import argparse
import sys
import re
import csv
from pathlib import Path

import fitz  # PyMuPDF
from fontTools.ttLib import TTFont


def sanitize_name(name: str) -> str:
    """Glyphnamen 'lesbar' machen."""
    if not name:
        return "glyph"
    s = name.strip()
    # Muster wie uniF0319 / uF0319 entfernen, falls der Name sonst nichtssagend ist
    if re.fullmatch(r'(uni|u)[0-9A-Fa-f]{4,6}', s):
        s = ""
    s = s.replace("_", "-").replace(".", "-").replace(" ", "-")
    s = re.sub(r"-{2,}", "-", s)
    s = s.strip("-").lower()
    return s or "glyph"


def pick_unicode_cmap(tt: TTFont):
    """Bevorzugt Format 12/10, dann 4 – gibt dict: codepoint -> glyphname."""
    cmaps = tt["cmap"].tables
    for pref in (12, 10, 4):
        for t in cmaps:
            if t.isUnicode() and getattr(t, "format", None) == pref:
                return dict(t.cmap)
    # Fallback: irgendeine Unicode-cmap
    for t in cmaps:
        if t.isUnicode():
            return dict(t.cmap)
    return {}


def is_pua(cp: int) -> bool:
    return (0xE000 <= cp <= 0xF8FF) or (0xF0000 <= cp <= 0xFFFFD) or (0x100000 <= cp <= 0x10FFFD)


def layout_grid(page_rect, cols, rows, margin):
    """Erzeugt ein Raster; gibt Zellen-Rects in Lesereihenfolge zurück."""
    x0, y0, x1, y1 = page_rect
    x0 += margin; y0 += margin; x1 -= margin; y1 -= margin
    width = (x1 - x0) / cols
    height = (y1 - y0) / rows
    rects = []
    for r in range(rows):
        for c in range(cols):
            rx0 = x0 + c * width
            ry0 = y0 + r * height
            rects.append(fitz.Rect(rx0, ry0, rx0 + width, ry0 + height))
    return rects


def draw_icon_cell(page, rect, fontfile, codepoint, title, icon_pt=48, text_pt=8):
    """Zeichnet Icon (zentriert) + zwei Zeilen Text (Name, U+xxxx) in die Zelle."""
    # Icon
    icon_text = chr(codepoint)
    # bounding box fürs Icon durch Textmessung grob bestimmen
    page.insert_textbox(
        rect,                    # wir zeichnen in zwei Stufen, erst Icon separat
        "",                      # Platzhalter, damit wir die Box nicht belegen
        fontfile=fontfile,
        fontsize=icon_pt
    )
    # Icon zentriert oben (etwas Puffer)
    icon_box = fitz.Rect(rect.x0, rect.y0, rect.x1, rect.y1 - 2 * text_pt - 8)
    page.insert_textbox(
        icon_box,
        icon_text,
        fontfile=fontfile,
        fontsize=icon_pt,
        align=1,   # center
        color=(0, 0, 0)
    )

    # Beschriftungen unten
    # 1) Name (evtl. kürzen)
    name = title
    max_chars = 30
    if len(name) > max_chars:
        name = name[:max_chars-1] + "…"

    label_box = fitz.Rect(rect.x0 + 4, rect.y1 - (text_pt + 4) * 2.2, rect.x1 - 4, rect.y1 - (text_pt + 4) * 1.2)
    page.insert_textbox(label_box, name, fontsize=text_pt, fontname="helv", align=1, color=(0, 0, 0))

    # 2) Codepoint
    cp_str = f"U+{codepoint:04X}"
    cp_box = fitz.Rect(rect.x0 + 4, rect.y1 - (text_pt + 4) * 1.1, rect.x1 - 4, rect.y1 - 4)
    page.insert_textbox(cp_box, cp_str, fontsize=text_pt, fontname="helv", align=1, color=(0, 0, 0))

    # Rahmen dezent
    page.draw_rect(rect, color=(0.85, 0.85, 0.85), width=0.3)


def main():
    ap = argparse.ArgumentParser(description="Icon-Font (TTF/OTF) als PDF-Raster mit Namen + Unicode ausgeben.")
    ap.add_argument("font", help="Pfad zur .ttf/.otf")
    ap.add_argument("-o", "--output", default="icons.pdf", help="PDF-Ausgabe (default: icons.pdf)")
    ap.add_argument("--csv", help="Optional: zusätzlich CSV mit cp,name,glyphname")
    ap.add_argument("--cols", type=int, default=8, help="Spalten pro Seite (default: 8)")
    ap.add_argument("--rows", type=int, default=10, help="Zeilen pro Seite (default: 10)")
    ap.add_argument("--pt", type=int, default=48, help="Icon-Punktgröße (default: 48)")
    ap.add_argument("--text-pt", type=int, default=8, help="Textgröße (default: 8)")
    ap.add_argument("--margin", type=float, default=36, help="Seitenrand in pt (default: 36)")
    ap.add_argument("--page", default="A4", choices=["A4", "LETTER", "A3"], help="Seitengröße (default: A4)")
    ap.add_argument("--only-pua", action="store_true", help="Nur Private Use Area drucken (empf. für Icon-Fonts)")
    ap.add_argument("--sort", choices=["cp", "name"], default="cp", help="Sortierung (cp|name), default=cp")
    args = ap.parse_args()

    font_path = Path(args.font)
    if not font_path.exists():
        print(f"Font nicht gefunden: {font_path}", file=sys.stderr)
        sys.exit(1)

    try:
        tt = TTFont(str(font_path))
    except Exception as e:
        print(f"Fehler beim Laden der Font: {e}", file=sys.stderr)
        sys.exit(2)

    cmap = pick_unicode_cmap(tt)
    if not cmap:
        print("Keine Unicode-cmap gefunden.", file=sys.stderr)
        sys.exit(3)

    # Map zusammenstellen: (cp, display_name, raw_glyph_name)
    items = []
    for cp, gname in cmap.items():
        if args.only_pua and not is_pua(cp):
            continue
        disp = sanitize_name(gname)
        # Falls Name leer und Codepoint benannt ist, notfalls 'uXXXX' als Name
        if not disp:
            disp = f"u{cp:04X}".lower()
        items.append((cp, disp, gname))

    if not items:
        print("Keine passenden Glyphen gefunden (Filter?).", file=sys.stderr)
        sys.exit(4)

    if args.sort == "name":
        items.sort(key=lambda x: (x[1], x[0]))
    else:
        items.sort(key=lambda x: x[0])

    # PDF vorbereiten
    if args.page == "A4":
        page_rect = fitz.paper_rect("a4")   # 595 x 842 pt
    elif args.page == "A3":
        page_rect = fitz.paper_rect("a3")
    else:
        page_rect = fitz.paper_rect("letter")

    doc = fitz.open()
    # Font einbetten: wir nutzen insert_textbox mit fontfile=...
    fontfile = str(font_path)

    cells_per_page = args.cols * args.rows
    grids = layout_grid(page_rect, args.cols, args.rows, args.margin)

    # Seiten rendern
    for start in range(0, len(items), cells_per_page):
        page = doc.new_page(width=page_rect.width, height=page_rect.height)
        # Kopfzeile
        header = f"{font_path.name} — {len(items)} Glyphen"
        header_rect = fitz.Rect(args.margin, 8, page_rect.width - args.margin, args.margin)
        page.insert_textbox(header_rect, header, fontsize=10, fontname="helv", align=0, color=(0, 0, 0))

        chunk = items[start:start + cells_per_page]
        for (rect, entry) in zip(grids, chunk):
            cp, disp_name, raw_name = entry
            draw_icon_cell(page, rect, fontfile, cp, disp_name, icon_pt=args.pt, text_pt=args.text_pt)

        # Seitenfuß
        footer = f"Seite { (start // cells_per_page) + 1 }"
        footer_rect = fitz.Rect(args.margin, page_rect.height - args.margin + 8,
                                page_rect.width - args.margin, page_rect.height - 8)
        page.insert_textbox(footer_rect, footer, fontsize=8, fontname="helv", align=2, color=(0.3, 0.3, 0.3))

    # Speichern
    out_pdf = Path(args.output)
    doc.save(out_pdf)
    doc.close()

    # Optional CSV
    if args.csv:
        with open(args.csv, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(["codepoint_hex", "unicode", "name_display", "glyph_name"])
            for cp, disp, gname in items:
                w.writerow([f"{cp:04X}", f"U+{cp:04X}", disp, gname])

    print(f"Fertig: {out_pdf.resolve()}")
    if args.csv:
        print(f"CSV:   {Path(args.csv).resolve()}")


if __name__ == "__main__":
    main()