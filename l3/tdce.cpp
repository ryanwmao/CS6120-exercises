#include "cfg.hpp"
#include <iostream>
#include <list>
#include <nlohmann/json.hpp>
#include <unordered_set>

using json = nlohmann::json;

void tdce_global(std::list<json> &instrs) {
  bool converged = false;
  while (!converged) {
    converged = true;

    std::unordered_set<std::string> uses;
    for (const auto &instr : instrs) {
      if (!instr.contains("args"))
        continue;
      for (const auto &arg : instr["args"]) {
        uses.insert(arg);
      }
    }

    for (auto it = instrs.begin(); it != instrs.end();) {
      auto &instr = *it;
      if (instr.contains("dest") &&
          !uses.count(instr["dest"].template get<std::string>())) {
        it = instrs.erase(it);
        converged = false;
      } else {
        ++it;
      }
    }
  }
}

void tdce_bb(BasicBlock &bb) {
  bool converged = false;
  while (!converged) {
    converged = true;
    std::unordered_set<std::string> dead;

    auto it = bb.instrs.end();
    while (it != bb.instrs.begin()) {
      --it;
      auto &insn = *it;

      if (insn.contains("dest")) {
        auto dest = insn["dest"].template get<std::string>();
        if (dead.count(dest)) {
          it = bb.instrs.erase(it);
          converged = false;
          continue;
        }
        dead.insert(dest);
      }
      if (insn.contains("args")) {
        for (auto &arg : insn["args"])
          dead.erase(arg.template get<std::string>());
      }
    }
  }
}

void tdce_fn(json &fn) {
  auto instrs = fn["instrs"].template get<std::list<json>>();
  tdce_global(instrs);
  auto bbs = form_bbs(instrs);
  for (auto &bb : bbs)
    tdce_bb(bb);

  std::list<json> new_instrs;
  for (auto &bb : bbs)
    new_instrs.insert(new_instrs.end(), bb.instrs.begin(), bb.instrs.end());

  fn["instrs"] = new_instrs;
}

void tdce_prog(json &prog) {
  for (auto &fn : prog["functions"])
    tdce_fn(fn);
}

int main() {
  json prog = json::parse(std::cin);
  tdce_prog(prog);
  std::cout << prog << std::endl;
}
