#!/usr/bin/env python3
# Build a canonical level-order manifest by walking the original game's
# overworld maps. Output is written to `assets/levels_index.json` as a flat
# ordered list of puzzle levels grouped by area, mirroring the in-game
# progression (1. The Lake -> 10. Mountaintop), plus optional bonus areas.
#
# Usage:
#   C:/Python314/python.exe tools/build_level_index.py
#   C:/Python314/python.exe tools/build_level_index.py --include-bonus

import argparse
import json
import re
from pathlib import Path


# Area progression in canonical order, sourced from leveltype=1 maps in
# original-baba-is-you/Data/Worlds/baba/. Each entry is (map_stem, label).
MAIN_AREAS = [
    ('177level', '1. the lake'),
    ('207level', '2. solitary island'),
    ('206level', '3. temple ruins'),
    ('16level',  '4. forest of fall'),
    ('169level', '5. deep forest'),
    ('87level',  '6. rocket trip'),
    ('180level', '7. flower garden'),
    ('182level', '8. chasm'),
    ('179level', '9. volcanic cavern'),
    ('232level', '10. mountaintop'),
]

# Post-game / bonus maps. Order matches the wiki's Map listing.
BONUS_AREAS = [
    ('264level', 'depths'),
    ('282level', 'abc'),
    ('283level', 'meta'),
    ('304level', 'center'),
    ('338level', 'null'),
]


def parse_levels_section(text):
    """Return [{key: value, ...}] for the [levels] section of an .ld file."""
    idx = text.find('[levels]')
    if idx < 0:
        return []
    end = text.find('\n[', idx + 1)
    body = text[idx:end] if end > 0 else text[idx:]
    entries = {}
    for line in body.splitlines()[1:]:
        m = re.match(r'(\d+)(\w+)=(.*)', line.strip())
        if m:
            entries.setdefault(m.group(1), {})[m.group(2)] = m.group(3)
    return list(entries.values())


def read_general(text):
    """Return dict of [general] section keys."""
    idx = text.find('[general]')
    if idx < 0:
        return {}
    end = text.find('\n[', idx + 1)
    body = text[idx:end] if end > 0 else text[idx:]
    out = {}
    for line in body.splitlines()[1:]:
        if '=' in line:
            k, v = line.split('=', 1)
            out[k.strip()] = v.strip()
    return out


def collect_area(map_stem, area_label, area_idx, root, *, dedupe=True):
    """Return ordered list of level dicts for one area map."""
    ld = root / f'{map_stem}.ld'
    text = ld.read_text(encoding='utf-8', errors='replace')
    entries = parse_levels_section(text)

    # number_in_area gives the puzzle's slot. Sort by number, then by appearance
    # order (preserved via enumerate) for ties (icon vs. dot at same slot).
    items = []
    for order, e in enumerate(entries):
        f = e.get('file', '').strip()
        if not f:
            continue
        try:
            num = int(e.get('number', '99'))
        except ValueError:
            num = 99
        items.append((num, order, f))
    items.sort()

    seen = set()
    out = []
    for num, _, f in items:
        if dedupe and f in seen:
            continue
        seen.add(f)
        # Skip back-links to overworld maps and self-references.
        target_ld = root / f'{f}.ld'
        target_general = read_general(target_ld.read_text(encoding='utf-8', errors='replace'))\
            if target_ld.exists() else {}
        try:
            leveltype = int(target_general.get('leveltype', '0'))
        except ValueError:
            leveltype = 0
        if leveltype != 0:
            continue  # drop nested maps
        out.append({
            'area_idx': area_idx,
            'area': area_label,
            'level_in_area': num,
            'source': f,
            'name': target_general.get('name', ''),
            'subtitle': target_general.get('subtitle', ''),
            'palette': target_general.get('palette', ''),
        })
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', default='original-baba-is-you/Data/Worlds/baba',
                    help='source level directory')
    ap.add_argument('-o', '--output', default='assets/levels_index.json',
                    help='manifest output path')
    ap.add_argument('--include-bonus', action='store_true',
                    help='append depths/abc/meta/center/null bonus areas')
    args = ap.parse_args()

    root = Path(args.root)
    manifest = []
    areas = list(MAIN_AREAS)
    if args.include_bonus:
        areas.extend(BONUS_AREAS)

    for ai, (map_stem, label) in enumerate(areas, 1):
        area = collect_area(map_stem, label, ai, root)
        manifest.extend(area)
        print(f'  area {ai:2d} ({label}): {len(area)} levels')

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(
        json.dumps({'areas': areas_meta(areas), 'levels': manifest},
                   indent=2, ensure_ascii=False),
        encoding='utf-8',
    )
    print(f'\nwrote {len(manifest)} levels to {out_path}')


def areas_meta(areas):
    return [{'index': i + 1, 'map': stem, 'label': label}
            for i, (stem, label) in enumerate(areas)]


if __name__ == '__main__':
    main()
