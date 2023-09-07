#pragma once

#include <list>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct BasicBlock {
  std::string name;
  std::list<json> instrs;

  template <class T>
  BasicBlock(T &&name_, std::list<json> &&instrs_)
      : name(std::move(name_)), instrs(std::move(instrs_)) {}
};

std::list<BasicBlock> form_bbs(std::list<json> &instrs);
