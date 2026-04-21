#!/usr/bin/env python3
# Best-effort importer for Baba Is You level files (.l + .ld).
#
# Usage:
#   python tools/import_baba_level.py original-baba-is-you/Data/Worlds/baba/0level.l
#   python tools/import_baba_level.py --all original-baba-is-you/Data/Worlds/baba -o assets/imported/
#
# Known-incomplete areas (documented here so future work has a head start):
#  * .l chunk framing is understood: ACHTUNG! magic, "MAP " tag, LAYR wrapper,
#    then one MAIN and one DATA chunk per layer. Each MAIN/DATA payload is
#    zlib-deflated. Per-layer "subheader size" field appears at LAYR+10.
#  * MAIN payload is an array of uint16 tile cells. Each cell encodes a
#    TILESET coordinate as (tx | (ty << 8)). The tileset position is matched
#    against the object's `tile = {tx, ty}` entry in values.lua.
#  * Tile stride is inferred as len/w (row-major, w = level width). Trailing
#    rows beyond `h` are padding left by the MMF2 editor — they sometimes
#    contain orphan tile ids that should be ignored. We only emit cells where
#    y < h.
#  * DATA payload appears to be a per-cell byte array (most commonly value 3,
#    rarely 0), likely facing/state info. We ignore it for now.
#  * If a .ld companion file is present, its [currobjlist] gives a reliable
#    per-level tile -> name map. We prefer it when available, then fall back
#    to the global values.lua table.

import argparse
import json
import os
import re
import struct
import sys
import zlib
from pathlib import Path


# ---------------------------------------------------------------------------
# Global tile -> object-name lookup from Data/values.lua
# ---------------------------------------------------------------------------

def build_global_tile_map(values_lua_path):
    """Parse `tileslist = { objectNNN = { ..., tile = {x, y}, ... }, ... }`
    and return {(tx, ty): name}. Later objects overwrite earlier ones if the
    tile coordinate collides, matching the game's own "last wins" behavior."""
    text = Path(values_lua_path).read_text(encoding='utf-8', errors='replace')
    start = text.find('tileslist =')
    if start < 0:
        raise RuntimeError('tileslist not found in values.lua')

    tile_map = {}
    # Rough per-object scan: match "objectNNN = { ... }" blocks at the top level.
    # We don't need a full Lua parser — objects are flat and well-delimited.
    pattern = re.compile(
        r'object(\d{3})\s*=\s*\{([^{}]*(?:\{[^{}]*\}[^{}]*)*)\}',
        re.S,
    )
    for m in pattern.finditer(text, start):
        body = m.group(2)
        name_m = re.search(r'name\s*=\s*"([^"]*)"', body)
        tile_m = re.search(r'tile\s*=\s*\{\s*(-?\d+)\s*,\s*(-?\d+)\s*\}', body)
        if not name_m or not tile_m:
            continue
        name = name_m.group(1)
        tx, ty = int(tile_m.group(1)), int(tile_m.group(2))
        tile_map[(tx, ty)] = name
    return tile_map


def build_per_level_tile_map(ld_path):
    """Read [currobjlist] from the .ld INI file. Returns {(tx, ty): name}.
    Only some levels (tutorial/map) populate this section — callers should
    fall back to the global map when it's empty."""
    if not ld_path.exists():
        return {}
    text = ld_path.read_text(encoding='utf-8', errors='replace')
    idx = text.find('[currobjlist]')
    if idx < 0:
        return {}
    end = text.find('\n[', idx + 1)
    section = text[idx:end] if end > 0 else text[idx:]

    entries = {}
    for line in section.splitlines():
        m = re.match(r'(\d+)(\w+)=(.*)', line.strip())
        if m:
            entries.setdefault(m.group(1), {})[m.group(2)] = m.group(3)

    out = {}
    for e in entries.values():
        name = e.get('name')
        tile = e.get('tile')
        if not name or not tile:
            continue
        try:
            tx, ty = (int(p) for p in tile.split(','))
        except ValueError:
            continue
        out[(tx, ty)] = name
    return out


def read_ld_general(ld_path):
    """Extract the [general] INI section as a dict. Missing file -> empty."""
    if not ld_path.exists():
        return {}
    text = ld_path.read_text(encoding='utf-8', errors='replace')
    idx = text.find('[general]')
    if idx < 0:
        return {}
    end = text.find('\n[', idx + 1)
    body = text[idx:end] if end > 0 else text[idx:]
    out = {}
    for line in body.splitlines()[1:]:  # skip [general] line
        if '=' in line:
            k, v = line.split('=', 1)
            out[k.strip()] = v.strip()
    return out


# ---------------------------------------------------------------------------
# Palette PNG reader (stdlib only; handles PNG filters 0-4)
# ---------------------------------------------------------------------------

def _paeth(a, b, c):
    p = a + b - c
    pa = abs(p - a); pb = abs(p - b); pc = abs(p - c)
    if pa <= pb and pa <= pc: return a
    if pb <= pc: return b
    return c


def read_palette_pixel(png_path, x, y):
    """Return (r, g, b) at (x, y) from a tiny palette PNG. Returns None on
    any decode error — callers should just skip the background emit."""
    try:
        d = Path(png_path).read_bytes()
        if d[:8] != b'\x89PNG\r\n\x1a\n':
            return None
        i = 8
        idat = b''
        plte = None
        w = h = ct = None
        while i < len(d):
            ln = struct.unpack('>I', d[i:i + 4])[0]
            tag = d[i + 4:i + 8]
            payload = d[i + 8:i + 8 + ln]
            i += 12 + ln
            if tag == b'IHDR':
                w, h = struct.unpack('>II', payload[:8])
                ct = payload[9]
            elif tag == b'PLTE':
                plte = payload
            elif tag == b'IDAT':
                idat += payload
            elif tag == b'IEND':
                break
        if w is None or y >= h or x >= w:
            return None
        bpp = {2: 3, 6: 4, 3: 1}.get(ct)
        if bpp is None:
            return None
        raw = zlib.decompress(idat)
        stride = w * bpp
        rows = []
        prev = bytes(stride)
        off = 0
        for _ in range(h):
            filt = raw[off]
            line = bytearray(raw[off + 1:off + 1 + stride])
            off += 1 + stride
            if filt == 1:
                for k in range(bpp, stride):
                    line[k] = (line[k] + line[k - bpp]) & 0xFF
            elif filt == 2:
                for k in range(stride):
                    line[k] = (line[k] + prev[k]) & 0xFF
            elif filt == 3:
                for k in range(stride):
                    a = line[k - bpp] if k >= bpp else 0
                    line[k] = (line[k] + (a + prev[k]) // 2) & 0xFF
            elif filt == 4:
                for k in range(stride):
                    a = line[k - bpp] if k >= bpp else 0
                    b = prev[k]
                    c = prev[k - bpp] if k >= bpp else 0
                    line[k] = (line[k] + _paeth(a, b, c)) & 0xFF
            rows.append(bytes(line))
            prev = line
        if ct == 2:
            s = rows[y][x * 3:x * 3 + 3]
            return (s[0], s[1], s[2])
        if ct == 6:
            s = rows[y][x * 4:x * 4 + 4]
            return (s[0], s[1], s[2])
        if ct == 3 and plte is not None:
            idx = rows[y][x]
            return (plte[idx * 3], plte[idx * 3 + 1], plte[idx * 3 + 2])
    except Exception:
        return None
    return None


# ---------------------------------------------------------------------------
# .l chunk parsing
# ---------------------------------------------------------------------------

class Layer:
    def __init__(self, w, h, main, data):
        self.w = w
        self.h = h
        self.main = main  # bytes, uint16 cells
        self.data = data  # bytes, per-cell state


def parse_l(path):
    d = Path(path).read_bytes()
    if d[:8] != b'ACHTUNG!':
        raise RuntimeError(f'{path}: missing ACHTUNG! magic')

    layr = d.find(b'LAYR')
    if layr < 0:
        raise RuntimeError(f'{path}: no LAYR chunk')

    # nlayers = uint16 at layr+8, subheader_size = uint32 at layr+10.
    # For levels with sub_sz < 35 there are 9 trailing bytes + a 2-byte tail
    # (0xff 0x02) before MAIN, so scan ahead rather than computing the offset.
    nlayers = struct.unpack('<H', d[layr + 8:layr + 10])[0]

    layers = []
    cursor = layr + 14  # subheader start
    for i in range(nlayers):
        if i == 0:
            sub_sz = struct.unpack('<I', d[layr + 10:layr + 14])[0]
        else:
            sub_sz = struct.unpack('<I', d[cursor:cursor + 4])[0]
            cursor += 4
        sub = d[cursor:cursor + sub_sz]
        cursor += sub_sz
        w = struct.unpack('<H', sub[0:2])[0]
        # Subheader offset 4 holds tile pixel size, NOT grid row count. The
        # true row count falls out of the MAIN payload length: it's column-
        # major with stride `rows`, so `rows = (len(main)//2) / w` (computed
        # below once MAIN is decompressed).

        main_off = d.find(b'MAIN', cursor)
        if main_off < 0:
            raise RuntimeError(f'{path}: no MAIN for layer {i}')
        main_sz = struct.unpack('<I', d[main_off + 4:main_off + 8])[0]
        main = zlib.decompress(d[main_off + 8:main_off + 8 + main_sz])
        rows = (len(main) // 2) // max(w, 1)

        data_off = main_off + 8 + main_sz
        if d[data_off:data_off + 4] != b'DATA':
            raise RuntimeError(f'{path}: expected DATA at {data_off}')
        # DATA: 4-byte sub_count, 1-byte type marker, 4-byte size, zlib
        data_sz = struct.unpack('<I', d[data_off + 9:data_off + 13])[0]
        data = zlib.decompress(d[data_off + 13:data_off + 13 + data_sz])
        cursor = data_off + 13 + data_sz

        layers.append(Layer(w, rows, main, data))
    return layers


# ---------------------------------------------------------------------------
# Object classification
# ---------------------------------------------------------------------------

def classify(name):
    """Return (kind, short_name). 'text_*' names become ('text', '<rest>'),
    everything else is treated as a gameplay object. We no longer drop
    anything — the game's catalog (src/ids.cpp) decides what it can render."""
    if name.startswith('text_'):
        return ('text', name[len('text_'):])
    return ('object', name)


# ---------------------------------------------------------------------------
# Level extraction + JSON emission
# ---------------------------------------------------------------------------

def extract_tiles(layer, tile_map, unknown=None):
    """Walk the MAIN uint16 stream and emit {x, y, kind, name} records.
    Cells equal to 0 (floor default) or 0xFFFF (empty) are skipped.

    MMF2's tilemap is stored column-major at stride = layer.h, so cell
    (x, y) = main[x * layer.h + y]."""
    w, h = layer.w, layer.h
    count = len(layer.main) // 2
    cells = struct.unpack(f'<{count}H', layer.main)
    tiles = []
    used = 0
    for x in range(w):
        for y in range(h):
            idx = x * h + y
            if idx >= count:
                continue
            v = cells[idx]
            if v == 0 or v == 0xFFFF:
                continue
            tx, ty = v & 0xFF, (v >> 8) & 0xFF
            name = tile_map.get((tx, ty))
            if name is None:
                if unknown is not None:
                    unknown.setdefault((tx, ty), 0)
                    unknown[(tx, ty)] += 1
                continue
            kind, short = classify(name)
            if kind is None:
                continue
            tiles.append({'x': x, 'y': y, 'kind': kind, 'name': short})
            used += 1
    return tiles, used


def trim_to_bbox(tiles, w, h, pad=1):
    """Shrink the grid around its content so levels with huge empty margins
    (e.g. the overworld, which stores 20x35 cells but only uses rows 12-22)
    don't render as tall vertical boards. Returns a possibly-smaller (tiles,
    w, h). `pad` controls how many empty cells we keep on each side."""
    if not tiles:
        return tiles, w, h
    xs = [t['x'] for t in tiles]
    ys = [t['y'] for t in tiles]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)
    # Only trim if we can shave at least ~4 cells off either side.
    # Otherwise leave the stored dimensions alone so playable levels don't
    # lose their perimeter framing.
    lead_x = max(0, min_x - pad)
    trail_x = max(0, (w - 1) - (max_x + pad))
    lead_y = max(0, min_y - pad)
    trail_y = max(0, (h - 1) - (max_y + pad))
    if lead_x + trail_x < 4 and lead_y + trail_y < 4:
        return tiles, w, h
    new_tiles = [{**t, 'x': t['x'] - lead_x, 'y': t['y'] - lead_y} for t in tiles]
    return new_tiles, w - lead_x - trail_x, h - lead_y - trail_y


def convert(l_path, values_lua, out_path):
    layers = parse_l(l_path)
    ld_path = Path(str(l_path)[:-2] + '.ld')
    level_map = build_per_level_tile_map(ld_path)
    global_map = build_global_tile_map(values_lua)

    # Merge: per-level wins where present, global fills the rest.
    tile_map = dict(global_map)
    tile_map.update(level_map)

    # Only the first layer holds playfield tiles in practice; subsequent
    # layers are decorative overlays that we can't map cleanly yet.
    primary = layers[0]
    unknown = {}
    tiles, used = extract_tiles(primary, tile_map, unknown)
    tiles, cols, rows = trim_to_bbox(tiles, primary.w, primary.h)

    out = {
        'cols': cols,
        'rows': rows,
        'tiles': tiles,
    }

    # Background (palette cell 0,4) = walkable interior.
    # Edge (palette cell 1,0) = the fill around the playfield.
    general = read_ld_general(ld_path)
    palette_name = general.get('palette')
    if palette_name:
        palettes_dir = Path(values_lua).parent / 'Palettes'
        palette_path = palettes_dir / palette_name
        bg = read_palette_pixel(palette_path, 0, 4)
        if bg is not None:
            out['background'] = list(bg)
        edge = read_palette_pixel(palette_path, 1, 0)
        if edge is not None:
            out['edge'] = list(edge)
        name = general.get('name')
        if name:
            out['name'] = name

    Path(out_path).write_text(json.dumps(out, indent=2))
    return used, unknown, cols, rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('input', help='.l file or directory (with --all)')
    ap.add_argument('-o', '--output', default='assets/imported',
                    help='output directory (JSON per level)')
    ap.add_argument('--all', action='store_true',
                    help='treat input as a directory and convert every .l inside')
    ap.add_argument('--values',
                    default='original-baba-is-you/Data/values.lua',
                    help='path to Baba Is You Data/values.lua')
    args = ap.parse_args()

    out_dir = Path(args.output)
    out_dir.mkdir(parents=True, exist_ok=True)

    targets = []
    if args.all:
        targets = sorted(Path(args.input).glob('*.l'))
    else:
        targets = [Path(args.input)]

    if not targets:
        print('no input files', file=sys.stderr)
        sys.exit(1)

    all_unknown = {}
    for t in targets:
        out_path = out_dir / (t.stem + '.json')
        try:
            used, unknown, w, h = convert(t, args.values, out_path)
        except Exception as e:
            print(f'[skip] {t.name}: {e}')
            continue
        for k, v in unknown.items():
            all_unknown[k] = all_unknown.get(k, 0) + v
        print(f'{t.name}: {w}x{h}, {used} tiles -> {out_path}')

    if all_unknown:
        top = sorted(all_unknown.items(), key=lambda kv: -kv[1])[:10]
        print('\nTop unknown/unsupported tile coords (count):')
        for (tx, ty), n in top:
            print(f'  ({tx},{ty}): {n}')


if __name__ == '__main__':
    main()
