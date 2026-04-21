#!/usr/bin/env bash
# Regenerate everything that derives from the original game's Data/ folder:
#   - src/ids.{h,cpp} + tools/sprite_manifest.txt  (from values.lua tileslist)
#   - assets/sprites/*.png                          (copied from Data/Sprites)
#   - assets/imported/*.json                        (parsed from Data/Worlds/baba)
#
# Requires original-baba-is-you/ to exist (gitignored local copy of the game).
# Re-run after upgrading the game version or changing gen_ids.py.

set -euo pipefail

cd "$(dirname "$0")"

ORIGINAL=original-baba-is-you/Data
if [ ! -d "$ORIGINAL" ]; then
  echo "error: $ORIGINAL is missing — drop a copy of Baba Is You's Data/ folder there" >&2
  exit 1
fi

python tools/gen_ids.py --values "$ORIGINAL/values.lua"
python tools/copy_sprites.py --src "$ORIGINAL/Sprites" --dst assets/sprites
python tools/import_baba_level.py --all "$ORIGINAL/Worlds/baba" --values "$ORIGINAL/values.lua" -o assets/imported
