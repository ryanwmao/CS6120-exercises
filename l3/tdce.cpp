#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

int main() {
  json prog = json::parse(std::cin);
  std::cout << prog << std::endl;
}
