#include "games/rule.h"

Rule::Rule(Object obj1, Object obj2, Object obj3) {
  objects_ = {obj1, obj2, obj3};
}

bool Rule::operator==(const Rule &rhs) const {
  return objects_ == rhs.objects_;
}
