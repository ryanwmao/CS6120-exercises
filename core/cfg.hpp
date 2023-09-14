#pragma once

#include "types.hpp"

namespace bril {

std::list<BasicBlock *> toCFG(std::list<Instr *> instrs);

// converts all functions to CFGs
void toCFG(Prog);

} // namespace bril