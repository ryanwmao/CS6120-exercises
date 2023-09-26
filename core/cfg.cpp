#include "cfg.hpp"

#include "casting.hpp"
#include "types.hpp"

#include <unordered_map>

namespace bril {

bool is_term(const Instr *i) {
  if (auto eff = dyn_cast<Effect>(i))
    return eff->op == "jmp" || eff->op == "br";
  return false;
}

BBList *cur_bbs = nullptr;
std::unordered_map<std::string, BasicBlock *> *bb_map = nullptr;

void formBBs(std::vector<Instr *> &instrs) {
  auto cur = new BasicBlock(0, "_bb.0");
  cur->label = new Label("_bb.0");
  cur_bbs->push_back(*cur);
  int bb_cnt = 1;
  for (auto &instr : instrs) {
    if (auto label = dyn_cast<Label>(instr)) {
      cur = new BasicBlock(bb_cnt, std::string(label->name));
      cur->label = label;
      cur_bbs->push_back(*cur);
      bb_map->emplace(std::make_pair(label->name, &cur_bbs->back()));
      bb_cnt++;
      continue;
    }
    cur->code.push_back(*instr);
    if (is_term(instr)) {
      cur = new BasicBlock(bb_cnt, "_bb." + std::to_string(bb_cnt));
      cur_bbs->push_back(*cur);
      bb_cnt++;
    }
  }
}

void deleteEmptyBBs() {
  for (auto it = cur_bbs->begin(); it != cur_bbs->end();) {
    auto &bb = *it;
    if (bb.code.empty() && !bb.label) {
      it = cur_bbs->erase(it);
      continue;
    }
    if (!bb.label)
      bb.label = new Label(std::string(bb.name));
    ++it;
  }
}

void renumberBBs() {
  int i = -1;
  for (auto &bb : *cur_bbs)
    bb.id = ++i;
}

void connectBBs() {
  for (auto it = cur_bbs->begin(); it != cur_bbs->end(); ++it) {
    auto &bb = *it;
    if (auto eff = dyn_cast<Effect>(&bb.code.back())) {
      if (eff->op == "jmp") {
        bb.exits[0] = (*bb_map)[eff->labels[0]];
        bb.exits[0]->entries.push_back(&bb);
        continue;
      } else if (eff->op == "br") {
        bb.exits[0] = (*bb_map)[eff->labels[0]];
        bb.exits[0]->entries.push_back(&bb);
        bb.exits[1] = (*bb_map)[eff->labels[1]];
        bb.exits[1]->entries.push_back(&bb);
        continue;
      }
    }
    auto c = it;
    if (++c == cur_bbs->end())
      break;

    bb.exits[0] = &*c;
    bb.exits[0]->entries.push_back(&bb);
  }
}

BBList toCFG(std::vector<Instr *> &instrs) {
  BBList res;
  std::unordered_map<std::string, BasicBlock *> map;
  bb_map = &map;
  cur_bbs = &res;

  formBBs(instrs);
  deleteEmptyBBs();
  connectBBs();
  renumberBBs();

  bb_map = nullptr;
  cur_bbs = nullptr;
  return res;
}

} // namespace bril
