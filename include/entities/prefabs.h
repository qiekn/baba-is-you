#pragma once

#include "games/object.h"
#include "types.h"

class Prefabs {
public:
  Prefabs(Registry& registry);
  virtual ~Prefabs();

  void CreateText(Vector2Int pos, ObjectType type);

  void CreateNoun(Vector2Int pos);
  void CreateOperator(Vector2Int pos);
  void CreateProperty(Vector2Int pos);

private:
  Registry& registry_;
};
