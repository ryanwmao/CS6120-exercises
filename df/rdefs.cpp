#include "rdefs.hpp"

#include "core/types.hpp"
#include <boost/dynamic_bitset.hpp>
#include <cstddef>
#include <deque>
#include <iostream>

namespace bril {

using Result = Rdefs::Result;

boost::dynamic_bitset<> *args;

// represents the dataflow value for a bb
struct DFValue {
  BasicBlock *bb;
  boost::dynamic_bitset<> in, out;
  boost::dynamic_bitset<> gen, kill;

  DFValue(BasicBlock *bb_, size_t n)
      : bb(bb_), in(n), out(n), gen(n), kill(n) {}

  void updateIn(std::vector<DFValue> &vals) {
    if (bb->id == 0) {
      in = *args;
    } else {
      in.reset();
    }
    for (auto pred : bb->entries)
      in |= vals[static_cast<size_t>(pred->id)].out;
  }

  // iterate the dataflow analysis
  // out = union of successors' ins
  // in = uses U (out - defs)
  // returns true if in was updated
  bool iter(std::vector<DFValue> &vals, boost::dynamic_bitset<> &temp) {
    updateIn(vals);
    // use temp so we don't have to allocate any new bitsets
    temp = gen;
    temp |= (in -= kill);
    if (out == temp)
      return false;
    out.swap(temp);
    return true;
  }
};

size_t toSizeT(VarRef reg) { return static_cast<size_t>(reg); }

std::pair<size_t, size_t *> findNdefs(Func &fn) {
  size_t ndefs = fn.args.size();
  const size_t nvars = fn.vp.nvars();

  size_t *var_ndefs = new size_t[fn.vp.nvars()];
  std::fill(var_ndefs, var_ndefs + nvars, 0);

  for (auto arg : fn.args)
    ++var_ndefs[arg.name];

  for (auto &bb : fn.bbs) {
    for (auto &instr : bb.code) {
      if (auto def = instr.def()) {
        ++var_ndefs[*def];
        ++ndefs;
      }
    }
  }
  return {ndefs, var_ndefs};
}

int *computeVarToDef(size_t *var_ndefs, size_t nvars, size_t ndefs) {
  int *var_to_def = new int[nvars + 1];
  var_to_def[0] = 0;
  for (size_t i = 1; i < nvars; i++) {
    var_to_def[i] = var_to_def[i - 1] + static_cast<int>(var_ndefs[i]);
  }
  var_to_def[nvars] = static_cast<int>(ndefs);
  return var_to_def;
}

void initDFValues(Func &fn, std::vector<DFValue> &values, size_t ndefs,
                  int *var_to_def) {
  const size_t nvars = fn.vp.nvars();
  int *seen_defs = new int[ndefs]; // maps def to number seen so far
  std::fill(seen_defs, seen_defs + ndefs, 0);
  args = new boost::dynamic_bitset<>(ndefs);
  for (auto &arg : fn.args) {
    ++seen_defs[arg.name];
    args->set(static_cast<size_t>(var_to_def[arg.name]));
  }

  size_t i = 0;
  boost::dynamic_bitset seen(nvars);

  // gen is the last definition of a var of a bb
  for (auto &bb : fn.bbs) {
    seen.reset();
    for (auto &instr : bb.code) {
      if (auto def = instr.def()) {
        seen.set(*def);
        ++seen_defs[*def];
      }
    }
    for (size_t j = 0; j < nvars; j++) {
      if (seen.test(j)) {
        auto idx = seen_defs[j] - 1 + var_to_def[j];
        values[i].gen.set(static_cast<size_t>(idx));
        // std::cout << bb.name << " " << fn.vp.strOf(static_cast<VarRef>(j))
        //           << " " << seen_defs[j] - 1 << " " << idx << std::endl;
        for (auto k = var_to_def[j]; k < var_to_def[j + 1]; k++) {
          values[i].kill.set(static_cast<size_t>(k));
        }
        values[i].kill.reset(static_cast<size_t>(idx));
      }
    }

    i++;
  }
}

Result Rdefs::analyze(Func &fn) {
  const auto nvars = fn.vp.nvars();

  auto &&[ndefs, var_ndefs] = findNdefs(fn);
  auto var_to_def = computeVarToDef(var_ndefs, nvars, ndefs);

  // initialize df values and worklist
  std::vector<DFValue> values;
  boost::dynamic_bitset<> in_wl(fn.bbs.size());
  values.reserve(fn.bbs.size());
  std::deque<size_t> wl; // worklist
  for (auto &bb : fn.bbs) {
    values.emplace_back(&bb, ndefs);
    wl.push_back(static_cast<size_t>(bb.id));
  }
  in_wl.set();
  initDFValues(fn, values, ndefs, var_to_def);

  // df analysis
  boost::dynamic_bitset<> temp;
  while (!wl.empty()) {
    auto &n = values[wl.front()];
    wl.pop_front();
    in_wl.reset(static_cast<size_t>(n.bb->id));
    // if true, more work to do
    if (n.iter(values, temp)) {
      // add bb's exits to worklist if not already there
      for (auto e : n.bb->exits) {
        if (e) {
          auto e_id = static_cast<size_t>(e->id);
          if (!in_wl.test_set(e_id)) {
            wl.push_back(e_id);
          }
        }
      }
    }
  }

  // convert dataflow values to results
  Result res;
  res.bb_results.reserve(fn.bbs.size());
  res.var_to_def = var_to_def;
  for (auto &v : values) {
    auto &r = res.bb_results.emplace_back(v.bb);
    // must reset out since iter clobbers it
    v.updateIn(values);
    for (size_t i = 0; i < ndefs; i++) {
      if (v.in.test(i))
        r.rd_in.push_back(static_cast<int>(i));
      if (v.out.test(i))
        r.rd_out.push_back(static_cast<int>(i));
    }
  }

  return res;
}

} // namespace bril