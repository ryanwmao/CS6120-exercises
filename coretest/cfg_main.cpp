#include "core/cfg.hpp"
#include "core/parse.hpp"
#include <iostream>
#include <list>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
  json j = json::parse(std::cin);
  bril::Prog prog = j;
  for (auto &fn : prog.fns) {
    for (auto &bb : fn.bbs) {
      std::cout << bb->name << std::endl;
      for (auto &instr : bb->code)
        std::cout << "\t" << json{instr} << std::endl;
      std::cout << std::endl;
    }
  }
}