#pragma once

#include <memory>
#include "games/command.h"
#include "managers/command-manager.h"
#include "types.h"

class InputManager {
public:
  InputManager(Registry& registry, CommandManager& command_manager);
  virtual ~InputManager();

  std::unique_ptr<Command> HandleInput();
  void Update();

private:
  Registry& registry_;
  CommandManager& command_manager_;

  float last_trigger_time_ = 0.0f;
  const float cooldown_ = 0.12f;

  Vector2Int GetMoveInput();
};
