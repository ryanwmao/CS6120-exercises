#include "df.hpp"

#include "core/types.hpp"
#include <boost/dynamic_bitset.hpp>
#include <deque>

namespace bril {

template <typename T>
std::vector<GenericSolver::Result<T>> GenericSolver::analyze(
    Func &fn, std::function<bool(std::vector<GenericDFValue> &)> &analysis) {

  const auto nvars = fn.vp.nvars();

  // initialize df values and worklist
  std::vector<GenericDFValue> values;
  boost::dynamic_bitset<> in_wl(fn.bbs.size());
  values.reserve(fn.bbs.size());
  std::deque<size_t> wl; // worklist
  for (auto &bb : fn.bbs) {
    values.emplace_back(&bb, nvars);
    wl.push_back(static_cast<size_t>(bb.id));

    // initialize the bitsets for the basic block
    initGenericDFValue(values.back());
  }
  in_wl.set();

  // df analysis
  boost::dynamic_bitset<> temp;
  while (!wl.empty()) {
    auto &n = values[wl.front()];
    wl.pop_front();
    in_wl.reset(static_cast<size_t>(n.bb->id));
    // if true, more work to do
    if (analysis(values)) {
      // add bb's entries to worklist if not already there
      for (auto &e : n.bb->entries) {
        auto e_id = static_cast<size_t>(e->id);
        if (!in_wl.test_set(e_id)) {
          wl.push_back(e_id);
        }
      }
    }
  }

  // convert dataflow values to results
  std::vector<GenericSolver::Result<T>> res;
  res.reserve(fn.bbs.size());
  for (auto &v : values) {
    auto &r = res.emplace_back(v.bb);
    // must reset out since iter clobbers it
    v.updateOut(values);
    for (size_t i = 0; i < nvars; i++) {
      if (v.in.test(i))
        r.df_in.push_back(static_cast<VarRef>(i));
      if (v.out.test(i))
        r.df_out.push_back(static_cast<VarRef>(i));
    }
  }

  return res;
}

} // namespace bril