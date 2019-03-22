# Notes
- Rewrite LocalScope, use the new format for lvar location infos
- Integrate into lvar-rewrite.cpp
- Make sure CodeGenerator understands new lvar location infos
- Refactor IRVarAssignmentInfo
- Refactor IROffsetInfo into generalized format for all possible value locations
- Perform $0, $1, $n checks inside LocalScope resolve_symbol
  - New resolve_symbol overload for symbols in string form
- Check for known self vars inside resolve_symbol
- Register known self vars in LocalScope
