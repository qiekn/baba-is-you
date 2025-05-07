#pragma once

#include <tuple>
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
  Rule(Object obj1, Object obj2, Object obj3);

  bool operator==(const Rule &rhs) const;

  std::tuple<Object, Object, Object> objects_;
};
