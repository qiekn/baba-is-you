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
        h = struct.unpack('<H', sub[4:6])[0]

        main_off = d.find(b'MAIN', cursor)
        if main_off < 0:
            raise RuntimeError(f'{path}: no MAIN for layer {i}')
        main_sz = struct.unpack('<I', d[main_off + 4:main_off + 8])[0]
        main = zlib.decompress(d[main_off + 8:main_off + 8 + main_sz])

        data_off = main_off + 8 + main_sz
        if d[data_off:data_off + 4] != b'DATA':
            raise RuntimeError(f'{path}: expected DATA at {data_off}')
        # DATA: 4-byte sub_count, 1-byte type marker, 4-byte size, zlib
        data_sz = struct.unpack('<I', d[data_off + 9:data_off + 13])[0]
        data = zlib.decompress(d[data_off + 13:data_off + 13 + data_sz])
        cursor = data_off + 13 + data_sz

        layers.append(Layer(w, h, main, data))
    return layers


# ---------------------------------------------------------------------------
# Object classification (matches our src/ids.h enums)
# ---------------------------------------------------------------------------

# Objects we know how to render in the remake. Everything else is dropped.
SUPPORTED_OBJECTS = {
    'baba', 'flag', 'wall', 'rock', 'grass', 'flower', 'tile', 'cloud',
    'star', 'brick', 'water', 'ice', 'hedge', 'fence',
}
SUPPORTED_TEXT = {
    'text_is', 'text_and', 'text_not', 'text_baba', 'text_flag', 'text_wall',
    'text_rock', 'text_you', 'text_win', 'text_stop', 'text_push', 'text_move',
    'text_defeat',
}


def classify(name):
    """Return (kind, short_name) or (None, None) if unsupported."""
    if name in SUPPORTED_OBJECTS:
        return ('object', name)
    if name in SUPPORTED_TEXT:
        return ('text', name[len('text_'):])
    return (None, None)


# ---------------------------------------------------------------------------
# Level extraction + JSON emission
# ---------------------------------------------------------------------------

def extract_tiles(layer, tile_map, unknown=None):
    """Walk the MAIN uint16 stream and emit {x, y, kind, name} records.
    Cells equal to 0 (floor default) or 0xFFFF (empty) are skipped.

    MMF2's tilemap stores data column-major with a fixed storage-row count
    per column — so the visible level occupies the first `h` entries of each
    `stride` block, with the remaining rows holding padding the editor keeps
    around for resizing."""
    w, h = layer.w, layer.h
    count = len(layer.main) // 2
    cells = struct.unpack(f'<{count}H', layer.main)
    stride = count // max(w, 1) if w else h
    tiles = []
    used = 0
    for x in range(w):
        for y in range(h):
            idx = x * stride + y
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

    out = {
        'cols': primary.w,
        'rows': primary.h,
        'tiles': tiles,
    }
    Path(out_path).write_text(json.dumps(out, indent=2))
    return used, unknown, primary.w, primary.h


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
