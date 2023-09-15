#include "lives.hpp"

#include "core/types.hpp"
#include <boost/dynamic_bitset.hpp>
#include <deque>

namespace bril {

using Result = Lives::Result;

#define TO_UINT static_cast<unsigned int>

// represents the dataflow value for a bb
struct DFValue {
  BasicBlock *bb;
  boost::dynamic_bitset<> in, out;
  boost::dynamic_bitset<> uses, defs;

  DFValue(BasicBlock *bb_, size_t n)
      : bb(bb_), in(n), out(n), uses(n), defs(n) {}

  void updateIn(std::vector<DFValue> &vals) {
    if (bb->exits[0]) {
      out = vals[TO_UINT(bb->exits[0]->id)].in;
      if (bb->exits[1])
        out |= vals[TO_UINT(bb->exits[1]->id)].in;
    }
  }

  // iterate the dataflow analysis
  // out = union of successors' ins
  // in = uses U (out - defs)
  // returns true if in was updated
  bool iter(std::vector<DFValue> &vals, boost::dynamic_bitset<> &temp) {
    updateIn(vals);
    // use temp so we don't have to allocate any new bitsets
    temp = uses;
    temp |= (out -= defs);
    if (in == temp)
      return false;
    in.swap(temp);
    return true;
  }
};

size_t toSizeT(VarRef reg) { return TO_UINT(reg); }

// initialize the use/def bitsets and the in/out bitsets for a df value
void initDFValue(DFValue &v) {
  auto &bb = *v.bb;
  /* Traverse the instructions backwards. If def, unset use, e.g:

      mov x, y
      add x, y

      since x isn't used before it is defed in this block,
      it shouldn't be in the uses
      use: {y}, def: {x}
  */
  for (auto it = bb.code.rbegin(); it != bb.code.rend(); ++it) {
    // defs processed before uses
    if (auto def = it->def()) {
      v.defs.set(TO_UINT(*def));
      v.uses.reset(TO_UINT(*def));
    }
    if (auto uses = it->uses()) {
      for (auto r : *uses) {
        v.uses.set(TO_UINT(r));
      }
    }
  }
}

std::vector<Result> Lives::analyze(Func &fn) {
  const auto nvars = fn.vp.nvars();

  // initialize df values and worklist
  std::vector<DFValue> values;
  boost::dynamic_bitset<> in_wl(fn.bbs.size());
  values.reserve(fn.bbs.size());
  std::deque<size_t> wl; // worklist
  for (auto &bb : fn.bbs) {
    values.emplace_back(&bb, nvars);
    wl.push_back(TO_UINT(bb.id));

    // initialize the bitsets for the basic block
    initDFValue(values.back());
  }
  in_wl.set();

  // df analysis
  boost::dynamic_bitset<> temp;
  while (!wl.empty()) {
    auto &n = values[wl.front()];
    wl.pop_front();
    in_wl.reset(TO_UINT(n.bb->id));
    // if true, more work to do
    if (n.iter(values, temp)) {
      // add bb's entries to worklist if not already there
      for (auto &e : n.bb->entries) {
        auto e_id = TO_UINT(e->id);
        if (!in_wl.test_set(e_id)) {
          wl.push_back(e_id);
        }
      }
    }
  }

  // convert dataflow values to results
  std::vector<Result> res;
  res.reserve(fn.bbs.size());
  for (auto &v : values) {
    auto &r = res.emplace_back(v.bb);
    // must reset out since iter clobbers it
    v.updateIn(values);
    for (size_t i = 0; i < nvars; i++) {
      if (v.in.test(i))
        r.live_in.push_back(static_cast<VarRef>(i));
      if (v.out.test(i))
        r.live_out.push_back(static_cast<VarRef>(i));
    }
  }

  return res;
}

} // namespace bril