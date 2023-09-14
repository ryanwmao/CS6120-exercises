#include "types.hpp"

namespace bril {
std::vector<Instr *> Func::allInstrs() const {
  std::vector<Instr *> res;
  for (auto &bb : bbs) {
    res.insert(res.end(), bb->code.begin(), bb->code.end());
  }
  return res;
}
}; // namespace bril