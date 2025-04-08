// Copyright (c) 2020-2023 Chris Ohk

// I am making my contributions/submissions to this project solely in our
// personal capacity and am not conveying any rights to any intellectual
// property of any third parties.

#pragma once

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

constexpr bool IsText(ObjectType type) { return (type < ObjectType::ICON); }

constexpr bool IsNoun(ObjectType type) {
  return (type > ObjectType::NOUN) && (type < ObjectType::OPERATOR);
}

constexpr bool IsOperator(ObjectType type) {
  return (type > ObjectType::OPERATOR) && (type < ObjectType::PROPERTY);
}

constexpr bool IsVerb(ObjectType type) {
  return (type == ObjectType::IS || type == ObjectType::HAS ||
          type == ObjectType::MAKE);
}

constexpr bool IsProperty(ObjectType type) {
  return (type > ObjectType::PROPERTY) && (type < ObjectType::ICON);
}

constexpr ObjectType ConverTextToIcon(ObjectType type) {
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

enum class PlayerState {
  INVALID,
  PLAING,
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
