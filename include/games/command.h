#pragma once

#include <vector>
#include "entt.h"
#include "systems/move-system.h"
#include "types.h"

class Command {
public:
  virtual ~Command() {}
  virtual void Execute() = 0;
  virtual void Undo() = 0;
  bool IsEmpty();

protected:
  bool is_empty_command_ = false;
};

/**
 * @class EmptyCommand
 * @brief Do Nothing. Just Return
 */
class EmptyCommand : public Command {
public:
  EmptyCommand() { is_empty_command_ = true; }
  void Execute() override { return; }
  void Undo() override { return; }
};

/**
 * @class DirectMoveCommand
 * @brief Force Move: never ask move system, just move!
 */
class DirectMoveCommand : public Command {
public:
  DirectMoveCommand(Registry& registry, const std::vector<Entity>& entities,
                    int dx, int dy)
      : registry_(registry), entities_(entities), dx_(dx), dy_(dy) {}

  DirectMoveCommand(Registry& registry, Entity entity, int dx, int dy)
      : registry_(registry), dx_(dx), dy_(dy) {
    entities_.push_back(entity);
  }

  virtual void Execute() override;
  virtual void Undo() override;

private:
  Registry& registry_;
  std::vector<Entity> entities_;
  int dx_;
  int dy_;
};

/**
 * @class ChainMoveCommand
 * @brief e.g. if you move a player, it will move other adjcent movable items
 *
 */
class ChainMoveCommand : public Command {
public:
  ChainMoveCommand(Registry& registry, MoveBuffer& to_move)
      : registry_(registry), to_move_(to_move) {}
  virtual ~ChainMoveCommand() {}

  virtual void Execute() override;
  virtual void Undo() override;

private:
  Registry& registry_;
  MoveBuffer to_move_;
};

class ResetCommand : public Command {
public:
  ResetCommand(Registry& registry) : registry_(registry) {}

  virtual void Execute() override;
  virtual void Undo() override;

private:
  Registry& registry_;
};
