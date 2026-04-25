#include "rule_engine.h"

#include "components.h"

RuleBoard::RuleBoard(int cols, int rows) : cols_(cols), rows_(rows), cells_(static_cast<size_t>(cols * rows)) {}

void RuleBoard::Set(int x, int y, TextId id) {
  if (x < 0 || x >= cols_ || y < 0 || y >= rows_) return;
  cells_[static_cast<size_t>(y) * cols_ + x] = id;
}

std::optional<TextId> RuleBoard::At(int x, int y) const {
  if (x < 0 || x >= cols_ || y < 0 || y >= rows_) return std::nullopt;
  return cells_[static_cast<size_t>(y) * cols_ + x];
}

namespace {

bool IsNoun(TextId id) { return CategoryOf(id) == TextCategory::Noun; }
bool IsPredicate(TextId id) {
  const auto cat = CategoryOf(id);
  return cat == TextCategory::Noun || cat == TextCategory::Property;
}

void TryEmit(std::vector<Rule>& out, std::optional<TextId> a, std::optional<TextId> b, std::optional<TextId> c) {
  if (!a || !b || !c) return;
  if (!IsNoun(*a)) return;
  if (*b != TextId::Is) return;
  if (!IsPredicate(*c)) return;
  out.push_back({*a, *c});
}

}  // namespace

std::vector<Rule> ParseRules(const RuleBoard& board) {
  std::vector<Rule> rules;

  for (int y = 0; y < board.Rows(); ++y) {
    for (int x = 0; x + 2 < board.Cols(); ++x) {
      TryEmit(rules, board.At(x, y), board.At(x + 1, y), board.At(x + 2, y));
    }
  }
  for (int x = 0; x < board.Cols(); ++x) {
    for (int y = 0; y + 2 < board.Rows(); ++y) {
      TryEmit(rules, board.At(x, y), board.At(x, y + 1), board.At(x, y + 2));
    }
  }

  return rules;
}

namespace {

template <typename Tag>
void ApplyTagToObject(entt::registry& r, ObjectId target) {
  for (auto [e, object] : r.view<const ObjectBlock>().each()) {
    if (object.id == target) r.emplace_or_replace<Tag>(e);
  }
}

}  // namespace

void ApplyRules(entt::registry& registry, const std::vector<Rule>& rules) {
  registry.clear<IsYou>();
  registry.clear<IsWin>();
  registry.clear<IsStop>();
  registry.clear<IsPush>();
  registry.clear<IsMove>();
  registry.clear<IsDefeat>();

  // Text blocks are always pushable, regardless of rules.
  for (auto [e, _] : registry.view<const TextBlock>().each()) {
    registry.emplace_or_replace<IsPush>(e);
  }

  for (const Rule& rule : rules) {
    auto subject = NounToObject(rule.subject);
    if (!subject) continue;

    switch (rule.predicate) {
      case TextId::You:
        ApplyTagToObject<IsYou>(registry, *subject);
        break;
      case TextId::Win:
        ApplyTagToObject<IsWin>(registry, *subject);
        break;
      case TextId::Stop:
        ApplyTagToObject<IsStop>(registry, *subject);
        break;
      case TextId::Push:
        ApplyTagToObject<IsPush>(registry, *subject);
        break;
      case TextId::Move:
        ApplyTagToObject<IsMove>(registry, *subject);
        break;
      case TextId::Defeat:
        ApplyTagToObject<IsDefeat>(registry, *subject);
        break;
      default:
        // Noun predicates (transformation) are follow-up.
        break;
    }
  }
}
