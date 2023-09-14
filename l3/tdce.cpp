#include "core/casting.hpp"
#include "core/parse.hpp"
#include "core/types.hpp"
#include <iostream>
#include <list>
#include <nlohmann/json.hpp>
#include <unordered_set>

using json = nlohmann::json;

void tdce_global(bril::Func &fn) {
  bool converged = false;
  while (!converged) {
    converged = true;

    std::unordered_set<std::string> uses;
    for (auto &bb : fn.bbs) {
      for (const auto &instr : bb.code) {
        auto args = instr.uses();
        if (!args)
          continue;
        for (const auto &arg : *args)
          uses.insert(arg);
      }
    }

    for (auto &bb : fn.bbs) {
      for (auto it = bb.code.begin(); it != bb.code.end();) {
        auto &instr = *it;
        auto dest = instr.def();
        if (dest && !uses.count(*dest)) {
          it = bb.code.erase(it);
          converged = false;
        } else {
          ++it;
        }
      }
    }
  }
}

void tdce_bb(bril::BasicBlock &bb) {
  bool converged = false;
  while (!converged) {
    converged = true;
    std::unordered_set<std::string> dead;

    auto it = bb.code.end();
    while (it != bb.code.begin()) {
      --it;
      auto &insn = *it;

      if (auto dest = insn.def()) {
        if (dead.count(*dest)) {
          it = bb.code.erase(it);
          converged = false;
          continue;
        }
        dead.insert(*dest);
      }
      if (auto args = insn.uses()) {
        for (auto &arg : *args)
          dead.erase(arg);
      }
    }
  }
}

void tdce_fn(bril::Func &fn) {
  tdce_global(fn);
  for (auto &bb : fn.bbs)
    tdce_bb(bb);
}

void tdce_prog(bril::Prog &prog) {
  for (auto &fn : prog.fns)
    tdce_fn(fn);
}

int main() {
  json j = json::parse(std::cin);
  bril::Prog prog = j;
  tdce_prog(prog);
  json out = prog;
  std::cout << out << std::endl;
}
