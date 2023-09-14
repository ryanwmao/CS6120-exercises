#pragma once

#include "types.hpp"

namespace bril {

std::list<BasicBlock *> toCFG(std::vector<Instr *> instrs);

} // namespace bril