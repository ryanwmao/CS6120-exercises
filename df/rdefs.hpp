#pragma once

#include "core/types.hpp"

namespace bril {
struct Rdefs {
public:
  struct BBResult {
    BasicBlock *bb;
    std::vector<int> rd_in;
    std::vector<int> rd_out;

  public:
    BBResult(BasicBlock *bb_) : bb(bb_) {}
  };

  struct Result {
    std::vector<BBResult> bb_results;
    int *var_to_def;
  };

  Result analyze(Func &fn);
};
} // namespace bril
