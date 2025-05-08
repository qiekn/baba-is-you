#pragma once

#include <vector>
#include "types.h"

using MoveBuffer = std::vector<std::pair<Entity, Vector2Int>>;

class MoveSystem {
public:
  MoveSystem(Registry& registry) : registry_(registry) {}

  /**
   * @brief can entities move one step
   *
   * @param entities
   * @param dir
   *
   */
  MoveBuffer Move(std::vector<Entity> entities, Vector2Int dir);

private:
  void InitMaps();

  /**
   * @brief gather allowable moves
   *
   * @param entity
   * @param direction
   * @param count (how many items can be move at once)
   */
  bool TryMove(MoveBuffer& to_move, Entity entity, Vector2Int dir, int count);

  /**
   * @brief helper function used to update push_map_ & stop_map_
   * @tparam component
   * @param map
   */
  template <typename T>
  void UpdateMap(Map& map);

private:
  Registry& registry_;
  Map push_map_;
  Map stop_map_;
};
