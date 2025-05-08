#include <entt.h>
#include <magic-enum.h>
#include <string_view>

// In C++, the ## operator is called the token-pasting or concatenation
// operator. It is used in macros to combine two tokens into a single token.
enum class ObjectType {
  DEFAULT,
  TEXT_BEGIN,

  NOUN_BEGIN,
#define X(a) NOUN_##a,
#include "noun.def"
#undef X
  NOUN_END,

  VERB_BEGIN,
#define X(a) VERB_##a,
#include "verb.def"
#undef X
  VERB_END,

  PROPERTY_BEGIN,
#define X(a) PROP_##a,
#include "properties.def"
#undef X
  PROPERTY_END,

  TEXT_END,

  // every subject (noun) has a icon
  ICON_BEGIN,
#define X(a) ICON_##a,
#include "noun.def"
#undef X
  ICON_END,
};

constexpr bool IsText(ObjectType type) {
  return (type > ObjectType::TEXT_BEGIN) && (type < ObjectType::TEXT_END);
}

constexpr bool IsNoun(ObjectType type) {
  return (type > ObjectType::NOUN_BEGIN) && (type < ObjectType::NOUN_END);
}

constexpr bool IsVerb(ObjectType type) {
  return (type > ObjectType::VERB_BEGIN) && (type < ObjectType::VERB_END);
}
constexpr bool IsProperty(ObjectType type) {
  return (type > ObjectType::PROPERTY_BEGIN) &&
         (type < ObjectType::PROPERTY_END);
}

constexpr bool IsIcon(ObjectType type) {
  return (type > ObjectType::ICON_BEGIN) && (type < ObjectType::ICON_END);
}

constexpr ObjectType NounToIcon(ObjectType type) {
  int res = static_cast<int>(type) - static_cast<int>(ObjectType::NOUN_BEGIN) +
            static_cast<int>(ObjectType::ICON_BEGIN);
  return static_cast<ObjectType>(res);
}

constexpr std::string_view ToString(ObjectType type) {
  return magic_enum::enum_name(type).substr(5);
  // TODO: just for debugging , remove it soom <2025-05-08 13:36, @qiekn> //
  return magic_enum::enum_name(type);
}
