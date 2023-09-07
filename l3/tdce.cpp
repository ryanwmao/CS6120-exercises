#include <iostream>
#include <list>
#include <nlohmann/json.hpp>
#include <unordered_set>
using json = nlohmann::json;

void tdce_fn(json &fn) {
  std::unordered_set<std::string> live;
  auto instrs = fn["instrs"].template get<std::list<json>>();

  auto it = instrs.end();
  while (it != instrs.begin()) {
    --it;
    auto &insn = *it;

    if (insn.contains("dest")) {
      if (!live.count(insn["dest"].template get<std::string>())) {
        it = instrs.erase(it);
        continue;
      }
    }
    if (insn.contains("args")) {
      for (auto &arg : insn["args"])
        live.insert(arg.template get<std::string>());
    }
  }

  fn["instrs"] = instrs;
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
