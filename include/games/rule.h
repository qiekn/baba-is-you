#pragma once

#include <tuple>
#include "games/object.h"

/**
 * @class Rule
 * @brief This class represents rule.
 *
 * The game centers around the manipulation of "rules"--represented by tiles
 * with words written on them--in order to allow the titular character Baba (or
 * some other object) to reach a specified goal.
 */
class Rule {
public:
  Rule(Object obj1, Object obj2, Object obj3);

  bool operator==(const Rule &rhs) const;

  std::tuple<Object, Object, Object> objects_;
};
