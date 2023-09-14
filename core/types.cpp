#include "types.hpp"

namespace bril {
std::vector<const Instr *> Func::allInstrs() const {
  std::vector<const Instr *> res;
  for (auto &bb : bbs) {
    for (auto it = bb.code.begin(); it != bb.code.end(); ++it)
      res.push_back(&*it);
  }
  return res;
}
}; // namespace bril