#pragma once

#include <raylib.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class RuleManager {
public:
  explicit RuleManager(Registry& registry);

  void Initialize();
  void Update();

private:
  void ParseRules();
  void ApplyRules();
  void ClearRulesEffect();
  void RebuildTypesMap();

  ObjectType GetTypeAt(int x, int y);
  void AddRuleEffect(const Rule& rule);
  void RemoveRuleEffect(const Rule& rule);

private:
  Registry& registry_;
  std::vector<std::pair<Vector2Int, ObjectType>> verbs_;
  std::unordered_map<Vector2Int, ObjectType> types_map_;
  std::unordered_set<Rule, RuleHash> active_rules_;
  std::unordered_set<Rule, RuleHash> previous_rules_;
};
