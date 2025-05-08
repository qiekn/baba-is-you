#include "managers/rule-manager.h"
#include <raylib.h>
#include <vector>
#include "components/basic-comp.h"
#include "components/game-comp.h"
#include "games/object.h"
#include "games/utils.h"
#include "types.h"

RuleManager::RuleManager(Registry& registry) : registry_(registry) {
  TraceLog(LOG_DEBUG, "RuleManager is Ready!");
}

// public

void RuleManager::Initialize() { Update(); }

void RuleManager::Update() {
  TraceLog(LOG_DEBUG, "RuleManager: Update");
  TraceLog(LOG_DEBUG, "previous: %d, active: %d", previous_rules_.size(),
           active_rules_.size());
  RebuildTypesMap();
  ClearRulesEffect();
  ParseRules();
  ApplyRules();
}

// private

void RuleManager::RebuildTypesMap() {
  TraceLog(LOG_DEBUG, "RuleManager: 0. Rebuild Types Map");
  verbs_.clear();
  types_map_.clear();

  auto view = registry_.view<Tile, Position>();
  for (auto entity : view) {
    Position pos = registry_.get<Position>(entity);
    ObjectType type = registry_.get<Tile>(entity).type;
    // gather verbs
    if (IsVerb(type)) {
      TraceLog(LOG_DEBUG, "RuleManager:\t Verb: %s", ToString(type));
      verbs_.emplace_back(pos, type);
    }
    // gather tiletype
    if (IsText(type)) {
      types_map_[pos] = type;
    }
  }
}

void RuleManager::ClearRulesEffect() {
  TraceLog(LOG_DEBUG, "RuleManager: 1. Clear Rules Effect");
  return;
#define X(a) registry_.clear<IS_##a>();
#include "properties.def"
#undef X
}

void RuleManager::ParseRules() {
  TraceLog(LOG_DEBUG, "RuleManager: 2. ParseRules");
  active_rules_.clear();
  // default rules: Text Is Push
  active_rules_.emplace(Object(ObjectType::NOUN_TEXT),
                        Object(ObjectType::VERB_IS),
                        Object(ObjectType::PROP_PUSH));
  // horizontal rules
  for (const auto [pos, verb] : verbs_) {
    auto lhs = GetTypeAt(pos.x - 1, pos.y);
    auto rhs = GetTypeAt(pos.x + 1, pos.y);
    if (!IsNoun(lhs)) continue;
    if (IsVerb(rhs)) continue;
    TraceLog(LOG_DEBUG, "RuleManager:\t Horizontal Rule %s %s %s",
             ToString(lhs).data(), ToString(verb).data(), ToString(rhs).data());

    active_rules_.emplace(Object(lhs), Object(verb), Object(rhs));
  }
  // vertical rules
  for (const auto& [pos, verb] : verbs_) {
    auto lhs = GetTypeAt(pos.x, pos.y - 1);
    auto rhs = GetTypeAt(pos.x, pos.y + 1);
    if (!IsNoun(lhs)) continue;
    if (IsVerb(rhs)) continue;
    TraceLog(LOG_DEBUG, "RuleManager:\t Vertical Rule %s %s %s",
             ToString(lhs).data(), ToString(verb).data(), ToString(rhs).data());
    active_rules_.emplace(Object(lhs), Object(verb), Object(rhs));
  }
}

void RuleManager::ApplyRules() {
  TraceLog(LOG_DEBUG, "RuleManager: 3. ApplyRules");
  std::vector<Rule> added_rules;
  std::vector<Rule> removed_rules;

  // added rules
  // not exist in previous rules set
  for (const auto& rule : active_rules_) {
    if (previous_rules_.find(rule) == previous_rules_.end()) {
      added_rules.push_back(rule);
    }
  }

  // removed rules
  // old rules not exist in new rules set
  for (const auto& rule : previous_rules_) {
    if (active_rules_.find(rule) == active_rules_.end()) {
      removed_rules.push_back(rule);
    }
  }
  TraceLog(LOG_DEBUG, "previous: %d, active: %d", previous_rules_.size(),
           active_rules_.size());
  TraceLog(LOG_DEBUG, "add: %d, remove: %d", added_rules.size(),
           removed_rules.size());

  for (const auto& rule : removed_rules) RemoveRuleEffect(rule);
  for (const auto& rule : added_rules) AddRuleEffect(rule);

  previous_rules_ = active_rules_;
}

// helpers

ObjectType RuleManager::GetTypeAt(int x, int y) {
  auto it = types_map_.find(Vector2Int{x, y});
  if (it != types_map_.end()) {
    return it->second;
  }
  return ObjectType::DEFAULT;
}

void RuleManager::AddRuleEffect(const Rule& rule) {
  ObjectType subject = rule.GetSubject().GetType();
  ObjectType verb = rule.GetVerb().GetType();
  ObjectType predicate = rule.GetPredicate().GetType();

  // gloabl rules
  if (subject == ObjectType::NOUN_TEXT) {
    if (verb == ObjectType::VERB_IS && predicate == ObjectType::PROP_PUSH) {
      auto view = registry_.view<Tile>();
      for (auto entity : view) {
        if (IsText(view.get<Tile>(entity).type))
          registry_.emplace<IS_PUSH>(entity);
      }
    }
  }

  // normal process
  switch (verb) {
    case ObjectType::VERB_IS:
      // noun transorm (e.g. Wall Is Flag)
      if (IsNoun(predicate)) {
        auto view = registry_.view<Tile>();
        for (auto entity : view) {
          if (view.get<Tile>(entity).type == NounToIcon(subject)) {
            auto& tile = registry_.get<Tile>(entity);
            auto& render = registry_.get<SpriteRenderer>(entity);
            switch (predicate) {
#define X(a)                                      \
  case ObjectType::NOUN_##a:                      \
    tile.type = NounToIcon(ObjectType::NOUN_##a); \
    render.name = GetSpritePrefixName(tile.type); \
    break;
#include "noun.def"
#undef X
              default:
                break;
            }
          }
        }
      }
      // behavior modification (e.g. Wall Is Stop)
      else if (IsProperty(predicate)) {
        auto view = registry_.view<Tile>();
        for (auto entity : view) {
          if (view.get<Tile>(entity).type == NounToIcon(subject)) {
            switch (predicate) {
#define X(a)                           \
  case ObjectType::PROP_##a:           \
    registry_.emplace<IS_##a>(entity); \
    break;
#include "properties.def"
#undef X
              default:
                break;
            }
          }
        }  // for
      }
      break;
    case ObjectType::VERB_AND:
      // TODO: maybe processed by paser <2025-05-08 09:53, @qiekn> //
      break;
    case ObjectType::VERB_WRITE:
      // TODO: I haven't played this level myself yet.
      break;
    default:
      break;
  }
}

void RuleManager::RemoveRuleEffect(const Rule& rule) {
  ObjectType subject = rule.GetSubject().GetType();
  ObjectType verb = rule.GetVerb().GetType();
  ObjectType predicate = rule.GetPredicate().GetType();

  if (verb == ObjectType::VERB_IS) {
    if (IsProperty(predicate)) {
      auto view = registry_.view<Tile>();
      for (auto entity : view) {
        if (view.get<Tile>(entity).type == NounToIcon(subject)) {
          switch (predicate) {
#define X(a)                                                      \
  case ObjectType::PROP_##a:                                      \
    if (!registry_.all_of<IS_##a>(entity)) {                      \
      TraceLog(LOG_ERROR, "RuleManager: RemoveRuleEffect Error"); \
    }                                                             \
    registry_.remove<IS_##a>(entity);                             \
    break;
#include "properties.def"
#undef X
            default:
              break;
          }
        }
      }
    }
  }
}
