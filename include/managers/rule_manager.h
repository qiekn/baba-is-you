#pragma once

#include <vector>
#include "games/rule.h"

/** @brief This class manages a list of rules. */
class RuleManager {
public:
  void Add(const Rule &rule);

  void Remove(const Rule &rule);

  void Clear();

  size_t Count() const;

  /**
   * @brief Gets a list of rules that has specific type.
   *
   * @param type The object type to find a rule.
   * @return A list of rules that has specific type.
   */
  std::vector<Rule> GetRules(ObjectType type) const;

  /**
   * @brief Gets the object type for player.
   *
   * @return The object type for player
   */
  ObjectType FindPlayer() const;

  /**
   * @brief Checks an object has specific property.
   *
   * @param types A list of object types to check it has property.
   * @param property
   * @return
   */
  bool HasProperty(const std::vector<ObjectType> &types, ObjectType property);

private:
  std::vector<Rule> rules_;
};
