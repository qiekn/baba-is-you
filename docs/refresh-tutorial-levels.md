# Refresh Tutorial Levels (000-007)

This note records exactly how `assets/levels/000.json` to `007.json` were refreshed from the original Baba data.

## Goal

Update local tutorial levels to this order:

- `00` Baba Is You
- `01` Where Do I Go?
- `02` Now What Is This?
- `03` Out Of Reach
- `04` Still Out Of Reach
- `05` Volcano
- `06` Off Limits
- `07` Grass Yard

## Source Mapping

The mapping from level title to original `.l` file under `original-baba-is-you/Data/Worlds/baba/`:

- `000.json` <- `0level.l` (`baba is you`)
- `001.json` <- `1level.l` (`where do i go?`)
- `002.json` <- `189level.l` (`now what is this?`)
- `003.json` <- `3level.l` (`out of reach`)
- `004.json` <- `2level.l` (`still out of reach`)
- `005.json` <- `90level.l` (`volcano`)
- `006.json` <- `5level.l` (`off limits`)
- `007.json` <- `6level.l` (`grass yard`)

Level name lookup command:

```powershell
rg -n "^name=" original-baba-is-you/Data/Worlds/baba -g "*.ld"
```

## Conversion Command

Run importer per source level and copy output into target level id:

```powershell
$map = @(
  @{src='0level.l';   dst='000.json'},
  @{src='1level.l';   dst='001.json'},
  @{src='189level.l'; dst='002.json'},
  @{src='3level.l';   dst='003.json'},
  @{src='2level.l';   dst='004.json'},
  @{src='90level.l';  dst='005.json'},
  @{src='5level.l';   dst='006.json'},
  @{src='6level.l';   dst='007.json'}
)

foreach ($it in $map) {
  python tools/import_baba_level.py ("original-baba-is-you/Data/Worlds/baba/" + $it.src) --values original-baba-is-you/Data/values.lua -o .cache/rebuild
  $tmp = ".cache/rebuild/" + [System.IO.Path]::GetFileNameWithoutExtension($it.src) + ".json"
  Copy-Item -LiteralPath $tmp -Destination ("assets/levels/" + $it.dst) -Force
}
```

## Verification

Quick check for name, dimensions, and tile count:

```powershell
@'
import json
from pathlib import Path
for i in range(8):
    p = Path(f'assets/levels/{i:03d}.json')
    d = json.loads(p.read_text(encoding='utf-8'))
    print(f"{i:03d} | {d.get('name','<noname>')} | {d['cols']}x{d['rows']} | tiles={len(d['tiles'])}")
'@ | python -
```

Expected titles:

- `000` `baba is you`
- `001` `where do i go?`
- `002` `now what is this?`
- `003` `out of reach`
- `004` `still out of reach`
- `005` `volcano`
- `006` `off limits`
- `007` `grass yard`

## Commit Scope

Only stage these files when committing this refresh:

- `assets/levels/000.json`
- `assets/levels/001.json`
- `assets/levels/002.json`
- `assets/levels/003.json`
- `assets/levels/004.json`
- `assets/levels/005.json`
- `assets/levels/006.json`
- `assets/levels/007.json`

Do not include unrelated local changes (for example `imgui.ini`).
