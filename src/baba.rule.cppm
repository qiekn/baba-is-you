module;

#include <cstddef>
#include <string>

export module baba.rule;

export import baba.object;

export class Rule {
public:
  Rule(Object subject, Object verb, Object predicate);

  Object GetSubject() const { return subject_; }
  Object GetVerb() const { return verb_; }
  Object GetPredicate() const { return predicate_; }

  bool operator==(const Rule& rhs) const;
  bool operator!=(const Rule& rhs) const;

  size_t Hash() const;

  std::string ToString() const;

private:
  Object subject_;
  Object verb_;
  Object predicate_;
};

export struct RuleHash {
  size_t operator()(const Rule& rule) const { return rule.Hash(); }
};
