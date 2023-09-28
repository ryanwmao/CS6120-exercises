#include "leave_ssa.hpp"

#include <iostream>
#include <queue>
#include <string>

namespace bril {
#define TO_UINT static_cast<unsigned int>

void leaveSSA(Func &fn) {
  fn.populateBBsV();

  size_t n = fn.bbs.size(), added = 0;
  for (unsigned int i = 0; i < n; i++) {
    auto &bb = *fn.bbsv[i];

    // std::cout << bb.name << std::endl;
    std::vector<std::string> new_exits;
    for (auto &exit : bb.exits) {
      if (!exit)
        break;
      if (exit->phis.empty()) {
        new_exits.push_back(exit->name);
        continue;
      }

      auto edge_block = new BasicBlock("_bb." + std::to_string(n + added));
      edge_block->label = new Label(std::string(edge_block->name));
      added++;
      fn.bbs.insert(++fn.bbs.iterator_to(bb), *edge_block);
      new_exits.push_back(edge_block->name);

      // add moves at the edge to set the phi destination with the
      // variable defined in bb
      for (auto &phi_ : exit->phis) {
        auto &phi = cast<Value>(phi_);
        auto it = std::find(phi.labels.begin(), phi.labels.end(), bb.name);
        // incoming name that the phi uses
        auto &inc_name =
            phi.args[TO_UINT(std::distance(phi.labels.begin(), it))];
        auto &mov = *new Value(phi.dest, phi.type, "id");
        mov.args.push_back(inc_name);
        edge_block->code.push_back(mov);
      }

      // update entries by removing old bb and adding new one
      exit->entries.erase(
          std::find(exit->entries.begin(), exit->entries.end(), &bb));
      exit->entries.push_back(edge_block);

      // unconditional jump from edge_block to exit
      edge_block->exits[0] = exit;
      auto &jmp = *new Effect("jmp");
      jmp.labels.push_back(exit->name);
      edge_block->code.push_back(jmp);
      // set the exit to edge_block
      exit = edge_block;
      edge_block->entries.push_back(&bb);
    }

    // if terminator is a jump, we need to set its labels
    if (bb.code.back().isJump())
      cast<Effect>(bb.code.back()).labels = new_exits;
  }

  // remove phi nodes from all the bbs
  for (unsigned int i = 0; i < n; i++) {
    fn.bbsv[i]->phis.clear();
  }
}
} // namespace bril