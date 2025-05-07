#pragma once

#include <cstddef>
#include <string>
#include "games/object.h"

/**
 * @class Rule
 * @brief Represents a rule
 *
 * Rules are formed by arranging word tiles in the format "A IS B". These rules
 * dynamically alter the behavior and properties of objects in the game world.
 *
 * There are two main types of rules:
 *
 * 1. **Noun-to-Noun Rules (A IS B)**:
 *   These transform all instances of object A into object B. For example,
 *   "BABA IS ROCK" turns every Baba into a Rock.
 *
 * 2. **Noun-to-Property Rules (A IS Property)**:
 *    These assign behaviors or traits to objects. For example, "BABA IS YOU"
 *    allows the player to control Baba, while "ROCK IS PUSH" makes rocks
 *    pushable.
 *
 */
class Rule {
public:
  Rule(Object subject, Object verb, Object predicate);

  Object GetSubject() const { return subject_; }
  Object GetVerb() const { return verb_; }
  Object GetPredicate() const { return predicate_; }

  bool operator==(const Rule& rhs) const;
  bool operator!=(const Rule& rhs) const;

  size_t Hash() const;

  std::string ToString() const;

private:
  Object subject_;
  Object verb_;
  Object predicate_;
};

// Custom hash function for Rule objects
struct RuleHash {
  size_t operator()(const Rule& rule) const { return rule.Hash(); }
};
