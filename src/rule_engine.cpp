#include "rule_engine.h"

#include <unordered_map>
#include <utility>
#include <vector>

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

// One token in a parse line, plus the (x, y) it came from so we can mark the
// originating cell as rule-active when the line resolves.
struct Tok {
  TextId id;
  int x;
  int y;
};

// Greedily consume a clause of the form: [NOT] X (AND [NOT] X)*
// where X is matched by `is_term`. Returns the parsed terms (with their
// negation flag) and the cell positions that participated; on failure
// (empty clause, trailing AND, etc.) returns nullopt.
struct ClauseTerm {
  TextId id;
  bool negated;
};
struct Clause {
  std::vector<ClauseTerm> terms;
  std::vector<std::pair<int, int>> cells;
};

template <typename TermPred>
std::optional<Clause> ParseClause(const std::vector<Tok>& toks, std::size_t& i,
                                  TermPred is_term) {
  Clause out;
  bool expect_term = true;  // toggled by AND
  bool first = true;
  while (i < toks.size()) {
    const Tok& t = toks[i];
    if (!expect_term) {
      if (t.id != TextId::And) break;
      out.cells.emplace_back(t.x, t.y);
      ++i;
      expect_term = true;
      continue;
    }

    bool negated = false;
    if (t.id == TextId::Not) {
      negated = true;
      out.cells.emplace_back(t.x, t.y);
      ++i;
      if (i >= toks.size()) return first ? std::nullopt : std::optional<Clause>{out};
    }
    const Tok& term = toks[i];
    if (!is_term(term.id)) {
      // A bare NOT with no following term, or a non-term where a term was
      // expected — give up the trailing partial clause.
      if (negated) {
        // Roll back the NOT cell since we didn't consume a term.
        if (!out.cells.empty()) out.cells.pop_back();
      }
      break;
    }
    out.terms.push_back({term.id, negated});
    out.cells.emplace_back(term.x, term.y);
    ++i;
    expect_term = false;
    first = false;
  }
  if (out.terms.empty()) return std::nullopt;
  // A clause cannot end on a trailing AND — drop it if it does.
  if (expect_term && !out.terms.empty()) {
    // Last cell must have been the dangling AND; remove it.
    if (!out.cells.empty()) out.cells.pop_back();
  }
  return out;
}

// Try to parse one or more rules starting from `start` in the token stream.
// On success, appends to `rules` and `active`, and returns the index of the
// token *after* the matched expression. On failure, returns `start`.
std::size_t TryParseExpression(const std::vector<Tok>& toks, std::size_t start,
                               std::vector<Rule>& rules,
                               std::vector<std::pair<int, int>>& active) {
  std::size_t i = start;
  auto subj = ParseClause(toks, i, IsNoun);
  if (!subj) return start;
  if (i >= toks.size() || toks[i].id != TextId::Is) return start;
  const Tok is_tok = toks[i];
  ++i;
  auto pred = ParseClause(toks, i, IsPredicate);
  if (!pred) return start;

  // Cross product of subjects and predicates.
  for (const auto& s : subj->terms) {
    if (s.negated) continue;  // subject-side NOT not yet supported; ignore those rules
    for (const auto& p : pred->terms) {
      rules.push_back({s.id, p.id, p.negated});
    }
  }
  for (auto c : subj->cells) active.push_back(c);
  active.emplace_back(is_tok.x, is_tok.y);
  for (auto c : pred->cells) active.push_back(c);
  return i;
}

// Walk one line of text tokens emitting every rule expression that fits.
// After a successful parse we resume from the end of that expression, so
// chained sentences like `A IS B AND C` consume all 5 tokens at once and
// `A IS B  C IS D` (with a gap) parses both halves independently.
void ParseTokenLine(const std::vector<Tok>& toks, std::vector<Rule>& rules,
                    std::vector<std::pair<int, int>>& active) {
  std::size_t i = 0;
  while (i < toks.size()) {
    const std::size_t next = TryParseExpression(toks, i, rules, active);
    if (next == i) {
      ++i;
    } else {
      i = next;
    }
  }
}

}  // namespace

ParseResult ParseRulesEx(const RuleBoard& board) {
  ParseResult result;

  auto flush_run = [&](std::vector<Tok>& run) {
    if (run.size() >= 3) ParseTokenLine(run, result.rules, result.active_cells);
    run.clear();
  };

  // Horizontal: split each row into runs of contiguous text cells.
  for (int y = 0; y < board.Rows(); ++y) {
    std::vector<Tok> run;
    for (int x = 0; x < board.Cols(); ++x) {
      const auto v = board.At(x, y);
      if (v) {
        run.push_back({*v, x, y});
      } else {
        flush_run(run);
      }
    }
    flush_run(run);
  }

  // Vertical: same for columns.
  for (int x = 0; x < board.Cols(); ++x) {
    std::vector<Tok> run;
    for (int y = 0; y < board.Rows(); ++y) {
      const auto v = board.At(x, y);
      if (v) {
        run.push_back({*v, x, y});
      } else {
        flush_run(run);
      }
    }
    flush_run(run);
  }

  return result;
}

std::vector<Rule> ParseRules(const RuleBoard& board) { return ParseRulesEx(board).rules; }

namespace {

template <typename Tag>
void ApplyTagToObject(entt::registry& r, ObjectId target) {
  for (auto [e, object] : r.view<const ObjectBlock>().each()) {
    if (object.id == target) r.emplace_or_replace<Tag>(e);
  }
}

template <typename Tag>
void RemoveTagFromObject(entt::registry& r, ObjectId target) {
  for (auto [e, object] : r.view<const ObjectBlock>().each()) {
    if (object.id == target) r.remove<Tag>(e);
  }
}

template <typename Tag>
void ApplyTagPair(entt::registry& r, ObjectId target, bool negated) {
  if (negated) {
    RemoveTagFromObject<Tag>(r, target);
  } else {
    ApplyTagToObject<Tag>(r, target);
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
  registry.clear<IsSink>();
  registry.clear<IsHot>();
  registry.clear<IsMelt>();
  registry.clear<IsFloat>();
  registry.clear<IsOpen>();
  registry.clear<IsShut>();
  registry.clear<IsPull>();
  registry.clear<IsWeak>();
  registry.clear<IsShift>();

  // Text blocks are always pushable, regardless of rules.
  for (auto [e, _] : registry.view<const TextBlock>().each()) {
    registry.emplace_or_replace<IsPush>(e);
  }

  // Two passes: first apply all positive rules, then strip tags from any
  // (subject, property) pair that has a matching "X IS NOT Y" rule. This
  // mirrors Baba Is You: a NOT clause cancels matching positive clauses but
  // doesn't suppress unrelated rules.
  auto apply = [&](const Rule& rule, bool negated) {
    auto subject = NounToObject(rule.subject);
    if (!subject) return;
    switch (rule.predicate) {
      case TextId::You: ApplyTagPair<IsYou>(registry, *subject, negated); break;
      case TextId::Win: ApplyTagPair<IsWin>(registry, *subject, negated); break;
      case TextId::Stop: ApplyTagPair<IsStop>(registry, *subject, negated); break;
      case TextId::Push: ApplyTagPair<IsPush>(registry, *subject, negated); break;
      case TextId::Move: ApplyTagPair<IsMove>(registry, *subject, negated); break;
      case TextId::Defeat: ApplyTagPair<IsDefeat>(registry, *subject, negated); break;
      case TextId::Sink: ApplyTagPair<IsSink>(registry, *subject, negated); break;
      case TextId::Hot: ApplyTagPair<IsHot>(registry, *subject, negated); break;
      case TextId::Melt: ApplyTagPair<IsMelt>(registry, *subject, negated); break;
      case TextId::Float: ApplyTagPair<IsFloat>(registry, *subject, negated); break;
      case TextId::Open: ApplyTagPair<IsOpen>(registry, *subject, negated); break;
      case TextId::Shut: ApplyTagPair<IsShut>(registry, *subject, negated); break;
      case TextId::Pull: ApplyTagPair<IsPull>(registry, *subject, negated); break;
      case TextId::Weak: ApplyTagPair<IsWeak>(registry, *subject, negated); break;
      case TextId::Shift: ApplyTagPair<IsShift>(registry, *subject, negated); break;
      default: break;  // noun predicates are transformations, handled elsewhere
    }
  };

  for (const Rule& rule : rules) {
    if (rule.negated) continue;
    apply(rule, false);
  }
  for (const Rule& rule : rules) {
    if (!rule.negated) continue;
    apply(rule, true);
  }
}

bool ApplyTransformations(entt::registry& registry, const std::vector<Rule>& rules) {
  // Skip negated transformations - they don't trigger a transformation.
  std::unordered_map<ObjectId, ObjectId> map;
  for (const Rule& rule : rules) {
    if (rule.negated) continue;
    auto subj = NounToObject(rule.subject);
    if (!subj) continue;
    auto pred = NounToObject(rule.predicate);
    if (!pred) continue;
    if (*subj == *pred) continue;
    map.try_emplace(*subj, *pred);
  }
  if (map.empty()) return false;

  std::vector<std::pair<entt::entity, ObjectId>> changes;
  for (auto [e, ob] : registry.view<const ObjectBlock>().each()) {
    auto it = map.find(ob.id);
    if (it != map.end()) changes.emplace_back(e, it->second);
  }
  for (auto& [e, new_id] : changes) {
    registry.get<ObjectBlock>(e).id = new_id;
  }
  return !changes.empty();
}
