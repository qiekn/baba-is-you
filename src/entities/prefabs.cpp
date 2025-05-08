#include "entities/prefabs.h"
#include "components/basic-comp.h"
#include "components/game-comp.h"
#include "games/object.h"
#include "games/utils.h"

Prefabs::Prefabs(Registry& registry) : registry_(registry) {}

Prefabs::~Prefabs() {}

void Prefabs::CreateText(Vector2Int pos, ObjectType type) {
  auto entity = registry_.create();
  auto sprite_renderer = SpriteRenderer{.name = GetSpritePrefixName(type)};
  registry_.emplace<Tile>(entity, Tile{type});
  registry_.emplace<Position>(entity, pos);
  registry_.emplace<SpriteRenderer>(entity, sprite_renderer);
}

void Prefabs::CreateNoun(Vector2Int pos) {}

void Prefabs::CreateOperator(Vector2Int pos) {}

void Prefabs::CreateProperty(Vector2Int pos) {}
