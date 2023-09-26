#pragma once

#include "core/types.hpp"
#include <boost/dynamic_bitset.hpp>

namespace bril {

struct GenericDFValue {
public:
  BasicBlock *bb;
  boost::dynamic_bitset<> in, out;
  void updateOut(std::vector<GenericDFValue> &vals);
};

void initGenericDFValue(GenericDFValue &v);

struct GenericSolver {
public:
  template <typename T> struct Result {
    BasicBlock *bb;
    std::vector<VarRef> df_in;
    std::vector<VarRef> df_out;
    std::unordered_map<VarRef, T> vals;

  public:
    Result(BasicBlock *bb_) : bb(bb_) {}
  };

  template <typename T>
  std::vector<Result<T>>
  analyze(Func &fn,
          std::function<bool(std::vector<GenericDFValue> &)> &analysis);
};
} // namespace bril