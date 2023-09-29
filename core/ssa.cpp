#include "ssa.hpp"
#include "core/dom.hpp"
#include "core/types.hpp"

#include <boost/dynamic_bitset.hpp>
#include <iostream>
#include <queue>

namespace bril {
struct ToSSA::VarInfo {
  boost::dynamic_bitset<> defs;
  boost::dynamic_bitset<> livein;
  Type *type;
};

#define TO_UINT static_cast<unsigned int>
#define TO_INT static_cast<int>

void ToSSA::computeVarInfos() {
  for (unsigned int var = 0; var < nvars_; var++) {
    auto &vi = var_infos_[var];
    vi.defs = boost::dynamic_bitset<>(nbbs_);
    vi.livein = boost::dynamic_bitset<>(nbbs_);
  }

  for (auto &arg : fn_.args) {
    auto &vi = var_infos_[TO_UINT(arg.name)];
    vi.defs.set(0);
    vi.type = arg.type;
  }

  for (auto &bb : bbs_) {
    auto bb_id = TO_UINT(bb.id);
    for (auto &instr : bb.code) {
      // mark uses as livein if they haven't been def yet
      if (auto uses = instr.uses()) {
        for (auto use : *uses) {
          auto uuse = TO_UINT(use);
          auto &vi = var_infos_[uuse];
          if (!vi.defs.test(bb_id)) {
            vi.livein.set(bb_id);
          }
        }
      }
      // mark def
      if (auto def = instr.def()) {
        auto &vi = var_infos_[TO_UINT(*def)];
        vi.defs.set(bb_id);
        vi.type = *instr.type();
      }
    }
  }
}

void ToSSA::idf(unsigned int var, boost::dynamic_bitset<> &temp,
                boost::dynamic_bitset<> &phis) {
  auto &vi = var_infos_[var];
  temp = vi.defs;
  std::queue<unsigned int> wl;
  for (unsigned int bb = 0; bb < nbbs_; bb++)
    if (vi.defs.test(bb))
      wl.push(bb);

  while (!wl.empty()) {
    auto &bb = *fn_.bbsv[wl.front()];
    wl.pop();

    for (unsigned int df = 0; df < nbbs_; df++) {
      if (!bb.dom_info->dfront.test(df) || phis.test(df))
        continue;
      phis.set(df);
      if (!temp.test_set(df))
        wl.push(df);
    }
  }
}

void ToSSA::removeDeadPhis(unsigned int var, boost::dynamic_bitset<> &phis) {
  auto &vi = var_infos_[var];
  // remove phis when there's a def before any use in a bb
  auto kills = vi.defs - vi.livein;
  phis -= kills;

  if (phis.empty())
    return;

  // set to all defs + phis
  kills = vi.defs;
  kills |= phis;

  // now we compute only the live phis by propagating livein information
  // when processing a bb that is livein, we look for its nearest dominating def
  // if this is a phi, it is live, and if we haven't already marked it live,
  // we process its predecessors and mark them as livein
  boost::dynamic_bitset<> live(nbbs_);
  // initialize worklist to bbs that are livein
  std::queue<unsigned int> wl;
  for (unsigned int i = 0; i < nbbs_; i++)
    if (vi.livein.test(i))
      wl.push(i);

  while (!wl.empty()) {
    auto bbi = wl.front();
    auto &bb = *fn_.bbsv[bbi];
    wl.pop();

    // find the dominating def
    unsigned int p;
    if (phis.test(bbi)) {
      p = bbi;
    } else {
      // find first def/phi that dominates this use by traversing dom tree
      auto cur = bb.dom_info->idom;
      while (!kills.test(TO_UINT(cur->id)))
        cur = cur->dom_info->idom;
      p = TO_UINT(cur->id);

      if (!phis.test(p))
        continue;
    }

    if (live.test_set(p))
      continue;

    // set predecessors as livein
    auto &pbb = *fn_.bbsv[p];
    for (auto pred : pbb.entries) {
      auto pid = TO_UINT(pred->id);
      // already added to the wl
      if (vi.livein.test(pid))
        continue;
      // if already deffed, shouldn't check
      if (vi.defs.test(pid))
        continue;

      // add to wl and mark as livein
      vi.livein.set(pid);
      wl.push(pid);
    }
  }

  phis = live;
}

void ToSSA::addPhiNodes() {
  boost::dynamic_bitset<> phis(nbbs_), temp(nbbs_);
  for (unsigned int var = 0; var < nvars_; var++) {
    phis.reset();
    temp.reset();

    // perform iterated dominance frontier to find places to put phis
    idf(var, temp, phis);
    // remove obivously dead phis
    if (remove_dead_phis_)
      removeDeadPhis(var, phis);

    // actually place the phi nodes
    for (unsigned int bbi = 0; bbi < nbbs_; bbi++) {
      if (phis.test(bbi)) {
        auto &bb = *fn_.bbsv[bbi];
        bb.phis.push_back(*(
            new Value(TO_INT(var), var_infos_[var].type, std::string("phi"))));
      }
    }
  }
}

VarRef ToSSA::getNewName(VarRef var) {
  // returns the currently reaching def name for var
  auto &s = name_stacks_[var];
  if (s.empty()) {
    s.push(var);
    return var;
  }
  return s.top();
}

VarRef ToSSA::genNewName(VarRef var) {
  // when there is a def; creates a new ssa name for var and sets it reaching
  auto new_name = fn_.vp.nextVarOf(var);
  name_stacks_[var].push(new_name);
  name_logs_->push_back(var);
  return new_name;
}

void ToSSA::rename(BasicBlock &bb) {
  auto log_pos = name_logs_->size();
  // assign new destination names to the phi nodes
  for (auto &phi : bb.phis) {
    auto def = phi.def();
    *phi.def() = genNewName(*def);
  }

  // update names used in instructions
  for (auto &instr : bb.code) {
    if (auto uses = instr.uses()) {
      for (auto &use : *uses)
        use = getNewName(use);
    }
    if (auto def = instr.def()) {
      *def = genNewName(*def);
    }
  }

  // update successor phi nodes
  for (auto exit : bb.exits) {
    if (!exit)
      break;
    for (auto &instr : exit->phis) {
      auto &phi = cast<Value>(instr);
      phi.args.push_back(getNewName(fn_.vp.origRefOf(phi.dest)));
      phi.labels.push_back(bb.name);
    }
  }

  // recursively call on dominance children
  for (auto succ : bb.dom_info->succs)
    rename(*succ);

  // pop all pushed names (restoring reaching defs before this bb)
  for (auto i = log_pos; i < name_logs_->size(); i++) {
    name_stacks_[(*name_logs_)[i]].pop();
  }
  name_logs_->resize(log_pos);
}

void ToSSA::rename() {
  auto name_stacks = std::make_unique<std::stack<VarRef>[]>(nvars_);
  name_stacks_ = name_stacks.get();
  std::vector<VarRef> name_logs;
  name_logs_ = &name_logs;

  // map ssa names to each argument
  for (auto &arg : fn_.args)
    arg.name = genNewName(arg.name);
  rename(bbs_.front());

  name_stacks_ = nullptr;
  name_logs_ = nullptr;
}

void ToSSA::toSSA() {
  fn_.populateBBsV();
  DomAnalysis doma(bbs_);
  doma.computeDomBy();
  doma.computeDomTree();
  doma.computeDomFront();

  const auto nvars = fn_.vp.nvars();
  var_infos_ = std::make_unique<VarInfo[]>(nvars);
  computeVarInfos();
  addPhiNodes();
  rename();
}

ToSSA::ToSSA(Func &fn, bool remove_dead_phis)
    : fn_(fn), bbs_(fn.bbs), nbbs_(static_cast<unsigned int>(fn.bbs.size())),
      nvars_(static_cast<unsigned int>(fn.vp.nvars())),
      remove_dead_phis_(remove_dead_phis) {}

ToSSA::~ToSSA() {}
} // namespace bril
