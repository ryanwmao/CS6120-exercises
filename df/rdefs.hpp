#pragma once

#include "core/types.hpp"

#include <memory>

namespace bril {
struct Rdefs {
public:
  struct BBResult {
    BasicBlock *bb;
    std::vector<unsigned int> rd_in;
    std::vector<unsigned int> rd_out;

  public:
    BBResult(BasicBlock *bb_) : bb(bb_) {}
  };

  struct Result {
    std::vector<BBResult> bb_results;
    std::unique_ptr<unsigned int[]> var_to_def;
  };

  Result analyze(Func &fn);
};
} // namespace bril
