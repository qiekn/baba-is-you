#include "systems/move-system.h"
#include <raylib.h>
#include "components/basic-comp.h"
#include "components/game-comp.h"
#include "constants.h"
#include "types.h"

bool MoveSystem::TryMove(MoveBuffer& to_move, Entity entity, Vector2Int dir,
                         int push_ability) {
  // get player original position and destination
  auto& pos = registry_.get<Position>(entity);
  Vector2Int dest = pos + dir;
  TraceLog(LOG_DEBUG, "TryMove: %d %d, dir: %d %d", pos.x, pos.y, dir.x, dir.y);
  to_move.emplace_back(entity, dir);  // register self

  // out of map
  if (dest.x < 0 || dest.x >= kCols || dest.y < 0 || dest.y >= kRows) {
    TraceLog(LOG_DEBUG, "TryMove: out of map");
    return false;
  }

  // hit obstacle
  if (stop_map_.find(dest) != stop_map_.end()) {
    TraceLog(LOG_DEBUG, "TryMove: hit obstacle");
    return false;
  }

  // hit movable
  bool hit_movable = push_map_.find(dest) != push_map_.end();
  if (hit_movable) {
    if (push_ability > 0) {
      TraceLog(LOG_DEBUG, "TryMove: hit movable");
      Entity other = push_map_[dest];
      return TryMove(to_move, other, dir, --push_ability);
    }
    return false;
  }

  // hit nothing
  TraceLog(LOG_DEBUG, "TryMove: hit nothing");
  return true;
}

MoveBuffer MoveSystem::Move(std::vector<Entity> entities, Vector2Int dir) {
  InitMaps();
  MoveBuffer to_move;
  TraceLog(LOG_DEBUG, "MoveSystem: start");
  if (entities.size() == 0) {
    TraceLog(LOG_ERROR, "Player Count: 0");
  }
  // check move for each entity
  for (const auto entity : entities) {
    // now this method only used by player entities
    // skip non-player entity
    if (!registry_.all_of<IS_YOU, Position>(entity)) {
      continue;
    }
    // 999 magic number means you can push pushing the box continuously.
    // (in most cases)
    if (!TryMove(to_move, entity, dir, 999)) {
      TraceLog(LOG_DEBUG, "MoveSystem: failed");
      return {};
    }
  }
  TraceLog(LOG_DEBUG, "MoveSystem: succeed");
  return to_move;
}

template <typename T>
void MoveSystem::UpdateMap(Map& map) {
  map.clear();
  auto view = registry_.view<Position, T>(entt::exclude<Hide>);
  for (auto e : view) {
    const auto& pos = registry_.get<Position>(e);
    map[pos] = e;
  }
}

void MoveSystem::InitMaps() {
  UpdateMap<IS_PUSH>(push_map_);
  UpdateMap<IS_STOP>(stop_map_);
}
