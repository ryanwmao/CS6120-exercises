#include "types.hpp"

#include "casting.hpp"

#include "cfg.hpp"

namespace bril {

bool is_term(Instr *i) {
  if (auto eff = dyn_cast<Effect>(i))
    return eff->op == "jmp" || eff->op == "br";
  return false;
}

std::list<BasicBlock *> toCFG(std::vector<Instr *> instrs) {
  std::list<BasicBlock *> res;
  res.push_back(new BasicBlock(0, "_bb.0"));
  int bb_cnt = 1;
  BasicBlock *cur = res.back();
  for (auto instr : instrs) {
    if (auto label = dyn_cast<Label>(instr)) {
      res.push_back(new BasicBlock(bb_cnt, std::string(label->name)));
      bb_cnt++;
      cur = res.back();
    }
    cur->code.push_back(instr);
    if (is_term(instr)) {
      res.push_back(new BasicBlock(bb_cnt, "_bb." + std::to_string(bb_cnt)));
      bb_cnt++;
      cur = res.back();
    }
  }
  return res;
}

} // namespace bril
