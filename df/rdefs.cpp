#include "rdefs.hpp"

#include "core/types.hpp"
#include <boost/dynamic_bitset.hpp>
#include <cstddef>
#include <iostream>
#include <queue>

namespace bril {

using Result = Rdefs::Result;

boost::dynamic_bitset<> *args;

#define TO_UINT static_cast<unsigned int>

// represents the dataflow value for a bb
struct DFValue {
  BasicBlock *bb;
  boost::dynamic_bitset<> in, out;
  boost::dynamic_bitset<> gen, kill;

  DFValue(BasicBlock *bb_, size_t n)
      : bb(bb_), in(n), out(n), gen(n), kill(n) {}

  void updateIn(DFValue *vals) {
    if (bb->id == 0)
      in = *args;
    else
      in.reset();

    for (auto pred : bb->entries)
      in |= vals[TO_UINT(pred->id)].out;
  }

  // iterate the dataflow analysis; returns true iff there was an update
  bool iter(DFValue *vals, boost::dynamic_bitset<> &temp) {
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

std::pair<unsigned int, std::unique_ptr<unsigned int[]>> findNdefs(Func &fn) {
  unsigned int ndefs = TO_UINT(fn.args.size());
  const unsigned int nvars = TO_UINT(fn.vp.nvars());

  auto var_ndefs = std::make_unique<unsigned int[]>(nvars);
  std::fill(var_ndefs.get(), var_ndefs.get() + nvars, 0);

  for (auto arg : fn.args)
    ++var_ndefs[TO_UINT(arg.name)];

  for (auto &bb : fn.bbs) {
    for (auto &instr : bb.code) {
      if (auto def = instr.def()) {
        ++var_ndefs[TO_UINT(*def)];
        ++ndefs;
      }
    }
  }
  return {ndefs, std::move(var_ndefs)};
}

std::unique_ptr<unsigned int[]> computeVarToDef(unsigned int *var_ndefs,
                                                unsigned int nvars,
                                                unsigned int ndefs) {
  auto var_to_def = new unsigned int[nvars + 1];
  var_to_def[0] = 0;
  for (size_t i = 1; i < nvars; i++)
    var_to_def[i] = var_to_def[i - 1] + var_ndefs[i - 1];

  var_to_def[nvars] = ndefs;
  return std::unique_ptr<unsigned int[]>(var_to_def);
}

void initDFValues(Func &fn, DFValue *values, size_t ndefs,
                  unsigned int *var_to_def) {
  const size_t nvars = fn.vp.nvars();
  // maps def to number seen so far
  auto seen_defs = std::make_unique<unsigned int[]>(ndefs);
  args = new boost::dynamic_bitset<>(ndefs);
  for (auto &arg : fn.args) {
    ++seen_defs[TO_UINT(arg.name)];
    args->set(TO_UINT(var_to_def[arg.name]));
  }

  size_t i = 0;
  boost::dynamic_bitset seen(nvars);

  // gen is the last definition of a var of a bb
  for (auto &bb : fn.bbs) {
    seen.reset();
    // see which vars are def in this bb, and how many times
    for (auto &instr : bb.code) {
      if (auto defp = instr.def()) {
        unsigned int def = TO_UINT(*defp);
        seen.set(def);
        ++seen_defs[def];
      }
    }
    // for vars defined in this block:
    //  * add the last def for each to gen
    //  * kill all defs except for the last def
    for (size_t j = 0; j < nvars; j++) {
      if (seen.test(j)) {
        auto idx = seen_defs[j] - 1 + var_to_def[j];
        values[i].gen.set(idx);

        size_t var_defs = var_to_def[j + 1] - var_to_def[j];
        values[i].kill.set(var_to_def[j], var_defs);
        values[i].kill.reset(idx);
      }
    }

    i++;
  }
}

DFValue *createDFArray(Func &fn, size_t ndefs) {
  size_t n = fn.bbs.size();

  auto values_raw = operator new[](n * sizeof(DFValue));
  auto values = reinterpret_cast<DFValue *>(values_raw);
  auto it = fn.bbs.begin();
  for (size_t i = 0; i < n; i++) {
    new (&values[i]) DFValue(&*it, ndefs);
    ++it;
  }
  return values;
}

void destroyDFArray(DFValue *arr, size_t n) {
  for (size_t i = 0; i < n; i++)
    arr[i].~DFValue();
  operator delete[](reinterpret_cast<void *>(arr));
}

Result Rdefs::analyze(Func &fn) {
  const auto n = fn.bbs.size();
  const auto nvars = TO_UINT(fn.vp.nvars());

  auto [ndefs, var_ndefs] = findNdefs(fn);
  auto var_to_def = computeVarToDef(var_ndefs.get(), nvars, ndefs);

  // initialize df values and worklist
  auto values = createDFArray(fn, ndefs);
  initDFValues(fn, values, ndefs, var_to_def.get());
  // add all blocks to worklist
  std::queue<int> wl;
  for (auto &bb : fn.bbs)
    wl.push(bb.id);
  // keep tracks of bbs currently in wl so we don't do extra work
  boost::dynamic_bitset<> in_wl(n);
  in_wl.set();

  // df analysis
  boost::dynamic_bitset<> temp;
  while (!wl.empty()) {
    auto &next = values[TO_UINT(wl.front())];
    wl.pop();
    in_wl.reset(TO_UINT(next.bb->id));
    // if true, more work to do
    if (next.iter(values, temp)) {
      // add bb's exits to worklist if not already there
      for (auto e : next.bb->exits) {
        if (e && !in_wl.test_set(TO_UINT(e->id))) {
          wl.push(e->id);
        }
      }
    }
  }

  // convert dataflow values to results
  Result res;
  res.bb_results.reserve(n);
  res.var_to_def = std::move(var_to_def);
  for (size_t i = 0; i < n; i++) {
    auto &v = values[i];
    auto &r = res.bb_results.emplace_back(v.bb);
    // must reset out since iter clobbers it
    v.updateIn(values);
    for (unsigned int j = 0; j < ndefs; j++) {
      if (v.in.test(j))
        r.rd_in.push_back(j);
      if (v.out.test(j))
        r.rd_out.push_back(j);
    }
  }

  destroyDFArray(values, n);
  return res;
}

} // namespace bril