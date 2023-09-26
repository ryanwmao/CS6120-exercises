#include "dom.hpp"

#include "types.hpp"

#include <queue>
#include <unordered_set>

namespace bril {

#define TO_UINT static_cast<unsigned int>

bool DomAnalysis::verifyDom() {
  // check domby
  for (auto &bb : bbs_) {
    // DomInfo *woc = bb.dom_info;
    for (size_t i = 0; i < bbs_.size(); i++) {
      if (bb.dom_info->dom_by.test(i)) {
        BasicBlock **bbsa_it = bbsa_;
        BasicBlock *target = *bbsa_it + i;
        std::queue<BasicBlock *> queue;
        queue.push(*bbsa_it);

        std::unordered_set<BasicBlock *> visited;
        while (queue.size() > 0) {
          BasicBlock *fk = queue.front();
          queue.pop();

          if (visited.find(fk) == visited.end()) {
            visited.insert(fk);
            if (fk == &bb) {
              return false;
            }
            if (fk != target) {
              for (auto &nexts : fk->exits) {
                queue.push(nexts);
              }
            }
          }
        }
      }
    }
  }

  // check idoms
  for (auto &bb : bbs_) {
    unsigned int idom = TO_UINT(bb.dom_info->idom->id);
    for (size_t i = 0; i < bbs_.size(); i++) {
      if (bb.dom_info->dom_by.test(i)) {
        BasicBlock *test = *bbsa_ + i;
        if (test->dom_info->dom_by.test(idom)) {
          return false;
        }
      }
    }
  }

  // check dominator tree
  for (auto &bb : bbs_) {
    int id = bb.id;
    for (auto &succ : bb.dom_info->succs) {
      if (succ->dom_info->idom->id != id) {
        return false;
      }
    }
  }

  // check dom frontier hehe
  for (auto &bb : bbs_) {
    unsigned int id = TO_UINT(bb.id);
    std::vector<int> doms;
    for (auto &bb2 : bbs_) {
      if (&bb2 != &bb) {
        if (!bb2.dom_info->dom_by.test(id)) {
          bool allDom = true;
          for (auto &pred : bb2.entries) {
            if (!pred->dom_info->dom_by.test(id)) {
              allDom = false;
              break;
            }
          }

          if (allDom)
            doms.push_back(bb2.id);
        }
      }
    }
    if (doms.size() != bb.dom_info->dfront.count())
      return false;
    for (int i : doms) {
      if (!bb.dom_info->dfront.test(TO_UINT(i)))
        return false;
    }
  }

  return true;
};

DomAnalysis::DomAnalysis(BBList &bbs)
    : bbs_(bbs), n_(static_cast<unsigned int>(bbs.size())),
      bbsa_(new BasicBlock *[n_]) {
  BasicBlock **bbsa_it = bbsa_;
  for (auto &bb : bbs) {
    *bbsa_it = &bb;
    bb.dom_info = new DomInfo(n_);
    ++bbsa_it;
  }
}

struct DomValue {
  BasicBlock *bb;

  DomValue(BasicBlock *bb_) : bb(bb_) {}

  // iterate the dataflow analysis
  // out = {n} union (intersect of n'.out for n' in preds)
  // returns true updated
  bool iter(DomValue *vals, boost::dynamic_bitset<> &temp);
};

bool DomValue::iter(DomValue *vals, boost::dynamic_bitset<> &temp) {
  auto &out = bb->dom_info->dom_by;
  // use temp so we don't have to allocate any new bitsets
  auto &preds = bb->entries;
  temp = vals[preds[0]->id].bb->dom_info->dom_by;
  for (auto it = ++preds.begin(); it != preds.end(); it++)
    temp &= vals[(*it)->id].bb->dom_info->dom_by;

  temp.set(TO_UINT(bb->id));
  if (out == temp)
    return false;
  out.swap(temp);

  return true;
}

DomValue *createDVArray(BBList &bbs) {
  size_t n = bbs.size();

  auto values_raw = operator new[](n * sizeof(DomValue));
  auto values = reinterpret_cast<DomValue *>(values_raw);
  auto it = bbs.begin();
  for (size_t i = 0; i < n; i++) {
    new (&values[i]) DomValue(&*it);
    ++it;
  }
  return values;
}

void destroyDFArray(DomValue *arr, size_t n) {
  for (size_t i = 0; i < n; i++)
    arr[i].~DomValue();
  operator delete[](reinterpret_cast<void *>(arr));
}

// computes dominated_by set for each of the basic blocks.
void DomAnalysis::computeDomBy() {
  // initialize dataflow values
  size_t n = bbs_.size();
  auto vals = createDVArray(bbs_);

  // initialize data flow worklist
  std::queue<unsigned int> wl;
  boost::dynamic_bitset<> in_wl(n);

  auto add_to_wl = [&in_wl, &wl](BasicBlock &bb) {
    for (auto exit : bb.exits) {
      if (!exit)
        break;
      auto e_id = TO_UINT(exit->id);
      if (!in_wl.test(e_id)) {
        in_wl.set(e_id);
        wl.push(e_id);
      }
    }
  };

  // set value for first bb
  bbsa_[0]->dom_info->dom_by.reset();
  bbsa_[0]->dom_info->dom_by.set(0);
  in_wl.set(0); // don't allow first bb to be processed
  add_to_wl(*bbsa_[0]);

  // df analysis
  boost::dynamic_bitset<> temp(n);
  while (!wl.empty()) {
    auto &next = vals[wl.front()];
    auto &next_bb = next.bb;
    in_wl.reset(wl.front());
    wl.pop();
    // if true, more work to do
    if (next.iter(vals, temp)) {
      // if this node changed, add all successors to worklist
      add_to_wl(*next_bb);
    }
  }
}

void DomAnalysis::computeDomTree() {
  boost::dynamic_bitset<> temp(n_);
  for (size_t i = 0; i < n_; i++) {
    auto bb = bbsa_[i];
    for (unsigned int j = 0; j < n_; j++) {
      if (bb->dom_info->dom_by.test(j) && TO_UINT(bb->id) != j) {
        auto dom = bbsa_[j];
        temp.reset();
        // dom is bb's idom iff
        // dom's dominators are equal to bb's dominators - bb
        temp |= dom->dom_info->dom_by;
        temp.set(TO_UINT(bb->id));
        if (temp == bb->dom_info->dom_by) {
          bb->dom_info->idom = dom;
          dom->dom_info->succs.push_back(bb);
          break;
        }
      }
    }
  }
}

void domTreeGV(Func &fn, std::ostream &os) {
  os << "digraph ";
  os << fn.name;
  os << " {\nnode[shape=rectangle]\n";
  os << "label=";
  os << '"' << fn.name << '"';
  os << ";\n";
  for (auto &bb : fn.bbs) {
    os << '"' << bb.name << '"';
    os << " [label=\"";
    os << bb.name;
    os << "\"];\n";
  }

  for (auto &bb : fn.bbs) {
    for (auto dom : bb.dom_info->succs) {
      os << '"' << bb.name << '"';
      os << "->";
      os << '"' << dom->name << '"';
      os << ";\n";
    }
  }
  os << "}\n";
}

void DomAnalysis::computeDomFront() { domFrontHelper(*bbsa_[0]); }

void DomAnalysis::domFrontHelper(BasicBlock &bb) {
  auto &df_n = bb.dom_info->dfront;

  // {n' | n ≻ n'}
  for (auto exit : bb.exits) {
    if (!exit)
      break;
    df_n.set(TO_UINT(exit->id));
  }

  // U_{n idom c} DF[c]
  for (auto bb2 : bb.dom_info->succs) {
    domFrontHelper(*bb2);
    df_n |= bb2->dom_info->dfront;
  }

  // {n' | n dom n'}
  for (unsigned int i = 0; i < n_; i++) {
    if (!df_n.test(i) || i == TO_UINT(bb.id))
      continue;
    auto front = bbsa_[i];
    if (front->dom_info->dom_by.test(TO_UINT(bb.id)))
      df_n.reset(i);
  }
}

DomAnalysis::~DomAnalysis() { delete[] bbsa_; }

} // namespace bril
