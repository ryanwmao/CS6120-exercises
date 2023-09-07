#include "cfg.hpp"

#include <iostream>

int anon_bb_cnt = 0;

std::string next_bb_name() { return "_bb." + std::to_string(++anon_bb_cnt); }

bool is_term(const json &instr) {
  return instr["op"] == "jmp" || instr["op"] == "br";
}

std::list<BasicBlock> form_bbs(std::list<json> &instrs) {
  std::list<BasicBlock> res;
  BasicBlock *cur = &res.emplace_back(next_bb_name(), std::list<json>());
  for (auto &instr : instrs) {
    if (instr.contains("label")) {
      auto &&name = instr["label"].template get<std::string>();
      cur = &res.emplace_back(std::move(name), std::list<json>());
      cur->instrs.emplace_back(std::move(instr));
      continue;
    }
    auto &new_instr = cur->instrs.emplace_back(std::move(instr));
    if (is_term(new_instr)) {
      cur = &res.emplace_back(next_bb_name(), std::list<json>());
    }
  }
  return res;
}

int main() {
  json prog = json::parse(std::cin);
  for (auto &fn : prog["functions"]) {
    auto instrs = fn["instrs"].template get<std::list<json>>();
    auto bbs = form_bbs(instrs);

    for (auto &bb : bbs) {
      std::cout << bb.name << std::endl;
      for (auto &instr : bb.instrs)
        std::cout << "\t" << instr << std::endl;
    }
    std::cout << std::endl;
  }
}
