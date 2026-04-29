#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "ids.h"

struct Rule {
  TextId subject;    // always a Noun
  TextId predicate;  // Noun (transformation, follow-up) or Property
};

// Minimal grid used by the parser: a cell is either empty or holds one text
// block. Non-text entities do not participate in rule formation.
class RuleBoard {
 public:
  RuleBoard(int cols, int rows);

  void Set(int x, int y, TextId id);
  std::optional<TextId> At(int x, int y) const;

  int Cols() const { return cols_; }
  int Rows() const { return rows_; }

 private:
  int cols_;
  int rows_;
  std::vector<std::optional<TextId>> cells_;
};

// Scans the board for NOUN IS (NOUN | PROPERTY) triples horizontally and
// vertically. MVP: no AND, no NOT. Text blocks are always pushable; callers
// should force-tag them separately.
std::vector<Rule> ParseRules(const RuleBoard& board);

// Clears all derived rule tags, then re-applies tags to entities whose
// ObjectBlock matches a rule subject. Text entities are always tagged IsPush.
void ApplyRules(entt::registry& registry, const std::vector<Rule>& rules);

// Mutates ObjectBlock.id for entities whose current id matches the subject of
// a "Noun is Noun" rule (e.g. "wall is rock" turns every wall into a rock).
// Captures the source set up-front so cyclic rules (wall is rock + rock is
// wall) atomically swap. Identity rules (wall is wall) are skipped. If a noun
// is the subject of multiple transformations the first one wins (MVP).
// Returns true if any entity was retagged.
bool ApplyTransformations(entt::registry& registry, const std::vector<Rule>& rules);
