#include "core/parse.hpp"
#include "core/string_pool.hpp"
#include "df/rdefs.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
  json j = json::parse(std::cin);
  bril::Prog prog = j;
  bril::Rdefs lives;
  for (auto &fn : prog.fns) {
    std::cout << fn.name << std::endl;
    auto res = lives.analyze(fn);
    for (auto &r : res.bb_results) {
      std::cout << r.bb->name << std::endl;
      std::cout << "  in :";
      int vi = -1;
      for (auto d : r.rd_in) {
        if (d >= res.var_to_def[vi + 1]) {
          ++vi;
          std::cout << "\n    " << fn.vp.strOf(static_cast<bril::VarRef>(vi))
                    << ": ";
        }
        std::cout << (d - res.var_to_def[vi]) << ", ";
      }
      std::cout << "\n  out:";
      vi = -1;
      for (auto d : r.rd_out) {
        if (d >= res.var_to_def[vi + 1]) {
          ++vi;
          std::cout << "\n    " << fn.vp.strOf(static_cast<bril::VarRef>(vi))
                    << ": ";
        }
        std::cout << (d - res.var_to_def[vi]) << ", ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}
