#pragma once

#include "types.hpp"

#include <boost/dynamic_bitset.hpp>

namespace bril {

struct DomInfo {
  // dominated by set
  boost::dynamic_bitset<> dom_by;
  // dominator front
  boost::dynamic_bitset<> dfront;
  // immediate dominator, parent in dom tree
  BasicBlock *idom;
  // direct succers in dom tree
  std::vector<BasicBlock *> succs;

  DomInfo(size_t n) : dom_by(n, true), dfront(n) {}
};

class DomAnalysis {
  BBList &bbs_;
  unsigned int n_;
  BasicBlock **bbsa_;

  void domFrontHelper(BasicBlock &);

public:
  DomAnalysis(BBList &bbs);
  ~DomAnalysis();

  bool verifyDom();
  void computeDomBy();
  void computeDomTree();
  void computeDomFront();
};

void domTreeGV(Func &fn, std::ostream &os);

} // namespace bril