#include "core/leave_ssa.hpp"
#include "core/parse.hpp"
#include "core/ssa.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
  json j = json::parse(std::cin);
  auto prog = j.template get<bril::Prog>();
  for (auto &fn : prog.fns) {
    bril::leaveSSA(fn);
  }
  json out = prog;
  std::cout << out << std::endl;
}