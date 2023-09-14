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
    std::cout << fn.name << std::endl;
    for (auto &bb : fn.bbs) {
      std::cout << bb.name << " " << bb.id << std::endl;

      std::cout << "  preds: ";
      for (auto e : bb.entries)
        std::cout << e->name << ", ";
      std::cout << std::endl;

      for (auto &instr : bb.code)
        std::cout << "\t" << json{instr} << std::endl;

      std::cout << "  succs: ";
      for (auto e : bb.exits)
        if (e)
          std::cout << e->name << ", ";
      std::cout << std::endl;

      std::cout << std::endl;
    }
  }
}