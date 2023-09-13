// this program is an identity: it takes in bril, parses it into C++ types, and
// converts these back into json bril

#include "core/parse.hpp"
#include "core/types.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
  json j = json::parse(std::cin);
  auto prog = j.template get<bril::Prog>();
  json out = prog;
  std::cout << out << std::endl;
}