#pragma once

#include "enums/object-enums.h"

class Object {
public:
  Object() : type_(ObjectType::DEFAULT) {}
  explicit Object(ObjectType type) : type_(type) {}

  ObjectType GetType() const { return type_; }

  bool operator==(const Object &rhs) const { return type_ == rhs.type_; }

private:
  ObjectType type_;
};
