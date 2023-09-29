#include "core/parse.hpp"
#include "core/ssa.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char *argv[]) {
  bool remove_dead_phis = true;
  if (argc >= 2 && std::string(argv[1]) == "-O")
    remove_dead_phis = false;
  json j = json::parse(std::cin);
  auto prog = j.template get<bril::Prog>();
  for (auto &fn : prog.fns) {
    bril::ToSSA(fn, remove_dead_phis).toSSA();
  }
  json out = prog;
  std::cout << out << std::endl;
}