#include "core/dom.hpp"
#include "core/parse.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
  json j = json::parse(std::cin);
  auto prog = j.template get<bril::Prog>();

  for (auto &fn : prog.fns) {
    bril::DomAnalysis doma(fn.bbs);
    doma.computeDomBy();
    auto bbsa = new bril::BasicBlock *[fn.bbs.size()];
    bril::BasicBlock **bbsa_it = bbsa;
    for (auto &bb : fn.bbs) {
      *bbsa_it = &bb;
      ++bbsa_it;
    }

    // prints the dom by set for each bb
    std::cout << fn.name << '\n';
    for (auto &bb : fn.bbs) {
      std::cout << bb.name << ": ";
      for (size_t i = 0; i < fn.bbs.size(); i++) {
        if (bb.dom_info->dom_by.test(i)) {
          std::cout << bbsa[i]->name << ", ";
        }
      }
      std::cout << '\n';
    }
    std::cout << '\n';

    doma.computeDomTree();
    bril::domTreeGV(fn, std::cout);
    std::cout << '\n';

    doma.computeDomFront();
    for (auto &bb : fn.bbs) {
      std::cout << bb.name << ": ";
      for (size_t i = 0; i < fn.bbs.size(); i++) {
        if (bb.dom_info->dfront.test(i)) {
          std::cout << bbsa[i]->name << ", ";
        }
      }
      std::cout << '\n';
    }

    std::cout << std::endl;
  }
}