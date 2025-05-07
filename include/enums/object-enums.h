#include <entt.h>
#include <magic-enum.h>
#include <string_view>

enum class ObjectType {
  UNKNOWN,
  TEXT_BEGIN,

  SUBJECT_BEGIN,
#define X(a) a,
#include "defs/subject.def"
#undef X
  SUBJECT_END,

  VERB_BEGIN,
#define X(a) a,
#include "defs/verb.def"
#undef X
  VERB_END,

  PREDICATE_BEGIN,
#define X(a) a,
#include "defs/predicate.def"
#undef X
  PREDICATE_END,

  TEXT_END,

  // every subject (noun) has a icon
  ICON_BEGIN,
#define X(a) ICON_##a,
#include "defs/subject.def"
#undef X
  ICON_END,
};

constexpr bool IsText(ObjectType type) {
  return (type > ObjectType::TEXT_BEGIN) && (type < ObjectType::TEXT_END);
}

constexpr bool IsSubject(ObjectType type) {
  return (type > ObjectType::SUBJECT_BEGIN) && (type < ObjectType::SUBJECT_END);
}

constexpr bool IsVerb(ObjectType type) {
  return (type > ObjectType::VERB_BEGIN) && (type < ObjectType::VERB_END);
}
constexpr bool IsPredicate(ObjectType type) {
  return (type > ObjectType::PREDICATE_BEGIN) &&
         (type < ObjectType::PREDICATE_END);
}

constexpr bool IsIcon(ObjectType type) {
  return (type > ObjectType::ICON_BEGIN) && (type < ObjectType::ICON_END);
}

constexpr std::string_view ToString(ObjectType type) {
  return magic_enum::enum_name(type);
}
