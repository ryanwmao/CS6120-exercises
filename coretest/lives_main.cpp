#include "core/parse.hpp"
#include "df/lives.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
  json j = json::parse(std::cin);
  bril::Prog prog = j;
  bril::Lives lives;
  for (auto &fn : prog.fns) {
    std::cout << fn.name << std::endl;
    auto res = lives.analyze(fn);
    for (auto &r : res) {
      std::cout << r.bb->name << std::endl;
      std::cout << "\tin : ";
      for (auto v : r.live_in)
        std::cout << fn.vp.strOf(v) << ", ";
      std::cout << "\n\tout: ";
      for (auto v : r.live_out)
        std::cout << fn.vp.strOf(v) << ", ";
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}
