#pragma once

#include <list>
#include <memory>
#include "games/command.h"

class CommandManager {
public:
  CommandManager();
  virtual ~CommandManager();

  void Execute(std::unique_ptr<Command> cmd);
  void Undo();
  void Redo();  // the undo of undo

private:
  using list = std::list<std::unique_ptr<Command>>;
  list history_;
  list::iterator current_it_;
  list::iterator head_;
  list::iterator tail_;
  size_t max_history_count_;

  bool CanUndo() const;
  bool CanRedo() const;
  void LimitMaxHistory();
};
