#include "entities/prefabs.h"
#include <string>
#include "components/components.h"

Prefabs::Prefabs(Registry& registry) : registry_(registry) {}

Prefabs::~Prefabs() {}

void Prefabs::CreateText(Vector2Int pos, const std::string& name) {
  auto entity = registry_.create();
  auto sprite_renderer = SpriteRenderer{
      .name = name,
  };
  registry_.emplace<Position>(entity, pos);
  registry_.emplace<SpriteRenderer>(entity, sprite_renderer);
}

void Prefabs::CreateNoun(Vector2Int pos) {}

void Prefabs::CreateOperator(Vector2Int pos) {}

void Prefabs::CreateProperty(Vector2Int pos) {}
