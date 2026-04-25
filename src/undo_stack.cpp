#include "undo_stack.h"

#include "components.h"

Snapshot CaptureSnapshot(const entt::registry& registry) {
  Snapshot snap;
  snap.rows.reserve(registry.storage<Cell>() ? registry.storage<Cell>()->size() : 0);

  for (auto [e, cell] : registry.view<const Cell>().each()) {
    Snapshot::Row row;
    row.x = cell.x;
    row.y = cell.y;
    row.facing = Direction::Right;
    if (const auto* facing = registry.try_get<Facing>(e)) {
      row.facing = facing->dir;
    }
    if (const auto* object = registry.try_get<ObjectBlock>(e)) {
      row.id = object->id;
    } else if (const auto* text = registry.try_get<TextBlock>(e)) {
      row.id = text->id;
    } else {
      continue;
    }
    snap.rows.push_back(row);
  }
  return snap;
}

void RestoreSnapshot(entt::registry& registry, const Snapshot& snapshot) {
  registry.clear();

  for (const auto& row : snapshot.rows) {
    auto e = registry.create();
    registry.emplace<Cell>(e, row.x, row.y);
    registry.emplace<Facing>(e, row.facing);
    registry.emplace<AnimFrame>(e);
    if (std::holds_alternative<ObjectId>(row.id)) {
      registry.emplace<ObjectBlock>(e, std::get<ObjectId>(row.id));
    } else {
      registry.emplace<TextBlock>(e, std::get<TextId>(row.id));
    }
  }
}

void UndoStack::Push(Snapshot snapshot) {
  stack_.push_back(std::move(snapshot));
  if (stack_.size() > kMaxDepth) {
    stack_.erase(stack_.begin(), stack_.begin() + (stack_.size() - kMaxDepth));
  }
}

bool UndoStack::Pop(Snapshot& out) {
  if (stack_.empty()) return false;
  out = std::move(stack_.back());
  stack_.pop_back();
  return true;
}

void UndoStack::Clear() { stack_.clear(); }
