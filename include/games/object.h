// Copyright (c) 2020-2023 Chris Ohk

#pragma once

#include <enums/game_enums.h>
#include <map>
#include <vector>

/**
 * @class Object
 * @brief This class represents object such as noun, operator and property.
 *
 * A noun is a word that corresponds to any possible in-game sprite. A few
 * nouns like STAR) have multiple corresponding sprites, while VIOLET and
 * FLOWER words have the same corresponding sprites. A noun can be used as a
 * NOUN IS VERB statement e.g. BABA IS YOU to give it a property or as a NOUN
 * IS NOUN statement e.g. WALL IS WATER to turn an object into another object.
 * An operator is a word that goes in between properties and nouns to show the
 * relation between them.
 * A property is something that can be attached to noun words to alter their
 * behavior.
 */
class Object {
public:
  Object() = default;

  explicit Object(std::vector<ObjectType> types);

  bool operator==(const Object &rhs) const;

  void Add(ObjectType type, bool isBoundary);

  void Remove(ObjectType type);

  std::vector<ObjectType> GetTypes() const;

  bool HasType(ObjectType type) const;

  bool HasTextType() const;

  bool HasNounType() const;

  bool HasVerbType() const;

  bool HasPropertyType() const;

  bool is_rule_ = false;

private:
  /**
   * @brief map of types
   *
   * key: type
   * value: count
   */
  std::map<ObjectType, std::size_t> types_;
};
