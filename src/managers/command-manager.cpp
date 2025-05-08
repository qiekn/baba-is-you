#include "managers/command-manager.h"
#include <cassert>
#include <iterator>
#include <memory>
#include "constants.h"
#include "games/command.h"

CommandManager::CommandManager() : max_history_count_(kMaxUndoHistry) {
  assert(max_history_count_ > 2);
  history_.push_back(std::make_unique<EmptyCommand>());
  head_ = std::prev(history_.end());

  history_.push_back(std::make_unique<EmptyCommand>());
  tail_ = std::prev(history_.end());

  current_it_ = head_;
}

CommandManager::~CommandManager() {}

void CommandManager::Execute(std::unique_ptr<Command> cmd) {
  // clear redo buffer
  // when you execute a new cmd, you cann't redo anymore
  if (CanRedo()) {
    history_.erase(std::next(current_it_), tail_);
  }
  cmd->Execute();
  history_.insert(tail_, std::move(cmd));
  ++current_it_;
  LimitMaxHistory();
}

void CommandManager::Undo() {
  if (CanUndo()) {
    (*current_it_)->Undo();
    --current_it_;
  }
}

void CommandManager::Redo() {
  if (CanRedo()) {
    ++current_it_;
    (*current_it_)->Execute();
  }
}

bool CommandManager::CanUndo() const { return current_it_ != head_; }

bool CommandManager::CanRedo() const { return std::next(current_it_) != tail_; }

void CommandManager::LimitMaxHistory() {
  while (history_.size() > max_history_count_ + 2) {
    history_.erase(std::next(head_));
  }
}
