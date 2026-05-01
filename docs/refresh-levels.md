# Refresh Levels

How the level set under `assets/levels/` and the per-area worlds under
`assets/worlds/` are rebuilt from the original Baba Is You data.

## Pipeline

Three scripts handle the full chain. Run them from the repo root.

```powershell
# 1. Re-extract every .l file into assets/imported/{stem}.json.
C:/Python314/python.exe tools/import_baba_level.py --all `
    original-baba-is-you/Data/Worlds/baba `
    --values original-baba-is-you/Data/values.lua `
    -o assets/imported

# 2. Build the canonical area->level manifest at assets/levels_index.json.
C:/Python314/python.exe tools/build_level_index.py --include-bonus

# 3. Copy imported files into assets/levels/ and emit per-area worlds.
C:/Python314/python.exe tools/apply_level_index.py
```

## Importer Behavior

`tools/import_baba_level.py` parses `ACHTUNG!` chunked `.l` files:

- Layer header gives the canvas size (W, H). Baba levels store the
  playfield wrapped in a 1-cell edge ring at id=0; the importer detects
  this and emits `cols = W-2`, `rows = H-2` with positions shifted by
  `(-1, -1)`. Levels without the ring fall through to the full WxH grid.
- Cell ids encode the tile coordinate `(tx | (ty << 8))` and resolve to
  object names via `Data/values.lua` plus the per-level `[currobjlist]`
  override in the `.ld` companion.
- Background and edge colors come from the palette PNG named by
  `[general] palette=`.

## Canonical Area Order

`tools/build_level_index.py` walks the overworld maps in the in-game
progression order and emits a manifest at `assets/levels_index.json`:

1. `177level` - 1. the lake
2. `207level` - 2. solitary island
3. `206level` - 3. temple ruins
4. `16level`  - 4. forest of fall
5. `169level` - 5. deep forest
6. `87level`  - 6. rocket trip
7. `180level` - 7. flower garden
8. `182level` - 8. chasm
9. `179level` - 9. volcanic cavern
10. `232level` - 10. mountaintop

With `--include-bonus`, the post-game maps are appended:
`264level` (depths), `282level` (abc), `283level` (meta),
`304level` (center), `338level` (null).

For each area, levels are sorted by their `[levels] <n>number=` slot;
overworld back-links and nested maps (`leveltype != 0`) are filtered out.

## Apply Step

`tools/apply_level_index.py` does two things:

- Copies every `assets/imported/<stem>.json` referenced by the manifest
  into `assets/levels/<stem>.json`. The runtime loader resolves levels
  by id, so `id = "211level"` reads `assets/levels/211level.json`.
- Writes one world file per area into `assets/worlds/`, e.g.
  `01_the_lake.json` ... `10_mountaintop.json` (+ bonus areas). Each
  world lists levels in canonical order with `id`, `name`, and the
  in-area `number`.

The pre-existing tutorial set (`000-008.json` and
`assets/worlds/tutorial.json`) is left in place; those ids are aliases
that re-import to the same JSON as the canonical sources.

## Verification

```powershell
C:/Python314/python.exe -c @"
import json
from pathlib import Path
idx = json.loads(Path('assets/levels_index.json').read_text(encoding='utf-8'))
last = 0
for L in idx['levels'][:25]:
    if L['area_idx'] != last:
        print(); last = L['area_idx']
    print(f\"{L['area_idx']:2d}-{L['level_in_area']:02d}  {L['source']:>10}  {L['name']}\")
"@
```
