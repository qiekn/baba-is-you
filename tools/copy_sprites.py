#!/usr/bin/env python3
"""Copy the sprites listed in tools/sprite_manifest.txt from the original
game's Data/Sprites/ folder into assets/sprites/. Missing files are logged
but ignored so we can still run with partial sprite coverage.

Invocation:
  python tools/copy_sprites.py \
    --src original-baba-is-you/Data/Sprites --dst assets/sprites
"""
import argparse
import shutil
import sys
from pathlib import Path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--src', default='original-baba-is-you/Data/Sprites')
    ap.add_argument('--dst', default='assets/sprites')
    ap.add_argument('--manifest', default='tools/sprite_manifest.txt')
    args = ap.parse_args()

    src = Path(args.src)
    dst = Path(args.dst)
    dst.mkdir(parents=True, exist_ok=True)

    entries = [l.strip() for l in Path(args.manifest).read_text().splitlines() if l.strip()]
    copied = 0
    missing = []
    for name in entries:
        s = src / name
        d = dst / name
        if not s.exists():
            missing.append(name)
            continue
        if d.exists() and d.stat().st_size == s.stat().st_size:
            continue  # already present, skip
        shutil.copy2(s, d)
        copied += 1

    print(f'copied {copied} sprites; {len(missing)} missing (out of {len(entries)})')
    if missing:
        # Group missing by sprite base name for a readable summary.
        bases = {}
        for n in missing:
            base = n.rsplit('_', 2)[0]
            bases[base] = bases.get(base, 0) + 1
        print('top missing sprite bases:')
        for base, count in sorted(bases.items(), key=lambda x: -x[1])[:20]:
            print(f'  {base}: {count}')


if __name__ == '__main__':
    main()
