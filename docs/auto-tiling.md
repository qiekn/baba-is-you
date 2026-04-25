# Auto-Tiling (Wall Variant Selection)

Connected walls (and other "tiling" surfaces in Baba Is You like `brick`,
`water`, `ice`, `hedge`) pick one of 16 sprite variants at render time based
on which of their 4 cardinal neighbours share the same kind. This doc captures
the exact convention so the system can be extended to more kinds later.

## Sprite naming

On disk:

```
{name}_{variant}_{frame}.png
```

- `name` — the object id, e.g. `wall`
- `variant` — `0..15`, the 4-bit neighbour mask (see below)
- `frame` — `1..3`, the animation frame

`wall_0_1.png` is the standalone (no neighbour) wall on frame 1.
`wall_15_1.png` is the fully surrounded wall on frame 1.

## The bitmask

```
bit 0 (value 1)  = neighbour on the RIGHT  (x + 1, y    )
bit 1 (value 2)  = neighbour ABOVE         (x    , y - 1)
bit 2 (value 4)  = neighbour on the LEFT   (x - 1, y    )
bit 3 (value 8)  = neighbour BELOW         (x    , y + 1)
```

The variant number in the filename equals this mask, which lets us look up the
texture with a direct index — no table needed.

Origin: `original-baba-is-you/Data/dynamictiling.lua` calls `dynamictile()`
which sums `2 ^ (i - 1)` for each of the 4 neighbour directions
(`ndirs = {{1,0},{0,-1},{-1,0},{0,1}}` in `values.lua:2`).

## Implementation points

- **Opt-in flag** — `IsAutoTiled(ObjectId)` in `src/ids.cpp`. Currently only
  `ObjectId::Wall` returns true. Flip another id to `true` once its 16 PNG
  variants are copied into `assets/sprites/`.
- **Sprite storage** — `SpriteSheet` stores objects as
  `[object][variant][frame]` (`src/sprite_sheet.h`). Non-tiled kinds only
  populate `variant = 0`.
- **Loader** — `SpriteSheet::LoadAll` loops variants `0..kVariantCount-1` for
  auto-tiled kinds and stops at `1` otherwise, so missing files never produce
  warnings for non-tiled kinds.
- **Neighbour scan** — `ComputeTileMask` in `src/game_layer.cpp` reads the
  registry every frame and checks the four cardinal cells. A neighbour counts
  as "same" iff another entity with the *same* `ObjectBlock` exists at that cell.
  Board edges do **not** count — walls that sit on the border naturally show
  an edge sprite.
- **Render** — `GameLayer::OnRender` routes through a lambda that passes the
  computed variant to `SpriteSheet::Get(id, frame, variant)`.

## Extending to a new kind

1. Copy `{name}_{v}_{f}.png` for v=1..15, f=1..3 (45 files) into
   `assets/sprites/`. Variant 0 should already be there.
2. In `src/ids.cpp::IsAutoTiled`, add the id to the `return` clause.
3. Rebuild. `ComputeTileMask` is generic; no other code changes needed.

## Why no caching

For the 24×18 board the per-frame cost is a handful of 4-neighbour view
scans, dominated by draw calls. If levels grow, swap the O(entities) neighbour
scan for a prebuilt `std::vector<uint8_t>` occupancy grid rebuilt once per
frame.

## Related

- `src/ids.{h,cpp}` — `IsAutoTiled`
- `src/sprite_sheet.{h,cpp}` — variant-indexed storage / loader / `Get`
- `src/game_layer.cpp` — `ComputeTileMask`, `OnRender` lambda
- `original-baba-is-you/Data/dynamictiling.lua` — reference implementation
