#include "games/rule.h"
#include <format>
#include "magic-enum.h"

Rule::Rule(Object subject, Object verb, Object predicate)
    : subject_(subject), verb_(verb), predicate_(predicate) {}

bool Rule::operator==(const Rule& rhs) const {
  return subject_ == rhs.subject_ && verb_ == rhs.verb_ &&
         predicate_ == rhs.predicate_;
}

bool Rule::operator!=(const Rule& rhs) const { return !(*this == rhs); }

size_t Rule::Hash() const {
  // Combine hashes of the three objects
  size_t h1 = std::hash<int>{}(static_cast<int>(subject_.GetType()));
  size_t h2 = std::hash<int>{}(static_cast<int>(verb_.GetType()));
  size_t h3 = std::hash<int>{}(static_cast<int>(predicate_.GetType()));

  // A simple hash combining technique
  return h1 ^ (h2 << 1) ^ (h3 << 2);
}

std::string Rule::ToString() const {
  using magic_enum::enum_name;
  return std::format("{} {} {}", enum_name(subject_.GetType()),
                     enum_name(verb_.GetType()),
                     enum_name(predicate_.GetType()));
}
