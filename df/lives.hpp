#pragma once

#include "core/types.hpp"

namespace bril {
struct Lives {
public:
  struct Result {
    BasicBlock *bb;
    std::vector<VarRef> live_in;
    std::vector<VarRef> live_out;

  public:
    Result(BasicBlock *bb_) : bb(bb_) {}
  };

  std::vector<Result> analyze(Func &fn);
};
} // namespace bril
