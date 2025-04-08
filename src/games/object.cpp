#include "games/object.h"

Object::Object(std::vector<ObjectType> types) {
  for (const auto &type : types) {
    types_.emplace(type, 1);
  }
}

bool Object::operator==(const Object &rhs) const {
  return types_ == rhs.types_;
}

void Object::Add(ObjectType type, bool isBoundary) {
  if (types_.find(type) != types_.end()) {
    if (isBoundary) {
      types_[type] = 1;
    } else {
      types_[type] += 1;
    }
  } else {
    types_.emplace(type, 1);
  }
}

void Object::Remove(ObjectType type) {
  if (auto iter = types_.find(type); iter != types_.end()) {
    if (types_[type] == 1) {
      types_.erase(iter);
    } else {
      types_[type] -= 1;
    }
  }

  if (types_.empty()) {
    types_.emplace(ObjectType::ICON_EMPTY, 1);
  }
}

std::vector<ObjectType> Object::GetTypes() const {
  std::vector<ObjectType> ret;

  for (const auto &type : types_) {
    ret.emplace_back(type.first);
  }

  return ret;
}

bool Object::HasType(ObjectType type) const {
  return types_.find(type) != types_.end();
}

bool Object::HasTextType() const {
  for (const auto &type : types_) {
    if (static_cast<int>(type.first) <= static_cast<int>(ObjectType::ICON)) {
      return true;
    }
  }

  return false;
}

bool Object::HasNounType() const {
  for (auto &type : types_) {
    if (IsNoun(type.first)) {
      return true;
    }
  }

  return false;
}

bool Object::HasVerbType() const {
  for (auto &type : types_) {
    if (IsVerb(type.first)) {
      return true;
    }
  }

  return false;
}

bool Object::HasPropertyType() const {
  for (auto &type : types_) {
    if (IsProperty(type.first)) {
      return true;
    }
  }

  return false;
}
