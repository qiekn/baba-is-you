#pragma once

#include <variant>
#include <vector>

#include <entt/entt.hpp>

#include "ids.h"

// Persistent per-entity state captured into a snapshot. Rule markers
// (IsYou/IsWin/...) are re-derived every turn, so we only serialise the
// ground-truth fields: coordinates, identity, facing.
struct Snapshot {
  struct Row {
    int x;
    int y;
    std::variant<ObjectId, TextId> id;
    Direction facing;
  };
  std::vector<Row> rows;
};

Snapshot CaptureSnapshot(const entt::registry& registry);

// Clears `registry` and rebuilds it from the snapshot. Component wiring
// matches GameLayer::SpawnObject / SpawnText semantics.
void RestoreSnapshot(entt::registry& registry, const Snapshot& snapshot);

class UndoStack {
 public:
  void Push(Snapshot snapshot);
  bool Pop(Snapshot& out);
  void Clear();
  bool Empty() const { return stack_.empty(); }
  std::size_t Size() const { return stack_.size(); }

 private:
  static constexpr std::size_t kMaxDepth = 256;
  std::vector<Snapshot> stack_;
};
