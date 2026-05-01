#!/usr/bin/env python3
# Apply the canonical level index to the project:
#   1. Copy every level from assets/imported/ into assets/levels/ using its
#      source-file name (e.g. 211level.json) so the existing loader can find
#      them by id without code changes.
#   2. Emit per-area world files in assets/worlds/ in canonical progression
#      order (01_the_lake.json ... 10_mountaintop.json + bonus areas).
#
# The pre-existing curated tutorial set (000-008.json + worlds/tutorial.json)
# is left untouched - those levels are aliased copies, not the canonical store.

import argparse
import json
import re
import shutil
from pathlib import Path


def slugify(label):
    """1. the lake -> 01_the_lake"""
    m = re.match(r'(\d+)\.\s*(.*)', label)
    if m:
        n = int(m.group(1))
        rest = m.group(2)
    else:
        n = None
        rest = label
    rest = re.sub(r'[^a-z0-9]+', '_', rest.lower()).strip('_')
    return f'{n:02d}_{rest}' if n is not None else rest


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--index', default='assets/levels_index.json')
    ap.add_argument('--imported-dir', default='assets/imported')
    ap.add_argument('--levels-dir', default='assets/levels')
    ap.add_argument('--worlds-dir', default='assets/worlds')
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    idx = json.loads(Path(args.index).read_text(encoding='utf-8'))
    imported = Path(args.imported_dir)
    levels = Path(args.levels_dir)
    worlds = Path(args.worlds_dir)
    levels.mkdir(parents=True, exist_ok=True)
    worlds.mkdir(parents=True, exist_ok=True)

    copied = skipped = 0
    needed = {L['source'] for L in idx['levels']}
    for src in sorted(needed):
        s = imported / f'{src}.json'
        d = levels / f'{src}.json'
        if not s.exists():
            print(f'  miss: {src} (no import)')
            skipped += 1
            continue
        if args.dry_run:
            print(f'  copy: {src} -> {d}')
        else:
            shutil.copyfile(s, d)
        copied += 1
    print(f'copied {copied} level files into {levels} (skipped {skipped})')

    by_area = {}
    for L in idx['levels']:
        by_area.setdefault(L['area_idx'], []).append(L)

    for ai, levels_in in sorted(by_area.items()):
        meta = next(a for a in idx['areas'] if a['index'] == ai)
        slug = slugify(meta['label'])
        out = {
            'title': meta['label'],
            'area_index': ai,
            'source_map': meta['map'],
            'levels': [
                {'id': L['source'], 'name': L['name'], 'number': L['level_in_area']}
                for L in levels_in
            ],
        }
        wp = worlds / f'{slug}.json'
        if args.dry_run:
            print(f'  world: {wp} ({len(levels_in)} levels)')
        else:
            wp.write_text(json.dumps(out, indent=2, ensure_ascii=False), encoding='utf-8')
    print(f'wrote {len(by_area)} world files into {worlds}')


if __name__ == '__main__':
    main()
