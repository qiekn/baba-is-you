#pragma once

#include "types.h"

class Prefabs {
public:
  Prefabs(Registry& registry);
  virtual ~Prefabs();

  void CreateText(Vector2Int pos, const std::string& name);

  void CreateNoun(Vector2Int pos);
  void CreateOperator(Vector2Int pos);
  void CreateProperty(Vector2Int pos);

private:
  Registry& registry_;
};
