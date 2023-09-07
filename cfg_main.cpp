#include "cfg.hpp"
#include <iostream>
#include <list>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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