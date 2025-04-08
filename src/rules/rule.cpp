// Copyright (c) 2020-2023 Chris Ohk

#include "rules/rule.h"

Rule::Rule(Object obj1, Object obj2, Object obj3) {
  objects = {obj1, obj2, obj3};
}

bool Rule::operator==(const Rule &rhs) const { return objects == rhs.objects; }
