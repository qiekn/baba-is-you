// Copyright (c) 2020-2023 Chris Ohk
#pragma once

#include "rules/rule_manager.h"
#include "enums/game_enums.h"

#include <algorithm>
#include <tuple>

void RuleManager::Add(const Rule &rule) { rules_.emplace_back(rule); }

void RuleManager::Remove(const Rule &rule) {
  const auto iter = std::find(rules_.begin(), rules_.end(), rule);
  if (iter != rules_.end()) {
    rules_.erase(iter);
  }
}

void RuleManager::Clear() { rules_.clear(); }

std::vector<Rule> RuleManager::GetRules(ObjectType type) const {
  std::vector<Rule> ret;

  for (auto &rule : rules_) {
    if (std::get<0>(rule.objects).HasType(type) ||
        std::get<1>(rule.objects).HasType(type) ||
        std::get<2>(rule.objects).HasType(type)) {
      ret.emplace_back(rule);
    }
  }

  return ret;
}

std::size_t RuleManager::Count() const { return rules_.size(); }

ObjectType RuleManager::FindPlayer() const {
  for (auto &rule : rules_) {
    if (std::get<2>(rule.objects).HasType(ObjectType::YOU)) {
      const ObjectType type = std::get<0>(rule.objects).GetTypes()[0];
      return ConvertTextToIcon(type);
    }
  }

  return ObjectType::ICON_EMPTY;
}

bool RuleManager::HasProperty(const std::vector<ObjectType> &types,
                              ObjectType property) {
  for (auto type : types) {
    type = ConvertIconToText(type);

    for (auto &rule : rules_) {
      if (std::get<0>(rule.objects).HasType(type) &&
          std::get<2>(rule.objects).HasType(property)) {
        return true;
      }
    }
  }

  return false;
}
