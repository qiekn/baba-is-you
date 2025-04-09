// Copyright (c) 2020-2023 Chris Ohk

#pragma once

enum class GameState {
  INVALID,
  PLAYING,
  WON,
  LOST,
};

enum class Direction {
  NONE,
  UP,
  DOWN,
  LEFT,
  RIGHT,
};

enum class ObjectType {
  NOUN,
#define X(a) a,
#include "noun.def"
#undef X
  OPERATOR,
#define X(a) a,
#include "operator.def"
#undef X
  PROPERTY,
#define X(a) a,
#include "property.def"
#undef X
  ICON,
#define X(a) a,
#include "icon.def"
#undef X
};

/** @brief if you are not icon, then you are text */
constexpr bool IsText(ObjectType type) { return (type < ObjectType::ICON); }

/** @brief A noun is a word that corresponds to any possible in-game sprite. */
constexpr bool IsNoun(ObjectType type) {
  return (type > ObjectType::NOUN) && (type < ObjectType::OPERATOR);
}

constexpr bool IsOperator(ObjectType type) {
  return (type > ObjectType::OPERATOR) && (type < ObjectType::PROPERTY);
}

/**
 * @brief Verb is a type of operator
 *
 * operators can be divided into: verbs, conditions, and, not.
 * Verbs: These go between two nouns. Or in case of IS or WRITE, can also be
 * after a noun and before a property.
 */
constexpr bool IsVerb(ObjectType type) {
  return (type == ObjectType::IS || type == ObjectType::HAS ||
          type == ObjectType::MAKE) ||
         type == ObjectType::WRITE;
}

/**
 * @brief Property type
 *
 * A property, internally known as a quality, is something that can be attached
 * to noun words to alter their behavior.
 */
constexpr bool IsProperty(ObjectType type) {
  return (type > ObjectType::PROPERTY) && (type < ObjectType::ICON);
}

constexpr ObjectType ConvertTextToIcon(ObjectType type) {
  const auto typeVal = static_cast<int>(type);
  const auto iconTypeVal = static_cast<int>(ObjectType::ICON);

  if (typeVal > iconTypeVal) {
    return type;
  }

  const int convertedVal = typeVal + iconTypeVal;
  return static_cast<ObjectType>(convertedVal);
}

constexpr ObjectType ConvertIconToText(ObjectType type) {
  const auto typeVal = static_cast<int>(type);
  const auto iconTypeVal = static_cast<int>(ObjectType::ICON);

  if (typeVal <= iconTypeVal) {
    return type;
  }

  const int convertedVal = typeVal - iconTypeVal;
  return static_cast<ObjectType>(convertedVal);
}
