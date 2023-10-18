#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct LICMPass : public LoopPass {
public:
  static char ID;
  LICMPass() : LoopPass(ID) {}

  bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    bool changed = false;
    LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    for (auto &BB : L->getBlocks()) {
      for (auto &I : *BB) {
        if (isLoopInvariant(I, L, LI)) {
          I.moveBefore(L->getLoopPreheader()->getTerminator());
          changed = true;
        }
      }
    }

    return changed;
  }

  // Check if an instruction is loop-invariant.
  bool isLoopInvariant(Instruction &I, Loop *L, LoopInfo *LI) {
    return !LI->getLoopFor(I.getParent()) || !LI->getLoopFor(I.getParent())->contains(L);
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
  }
};
}

char LICMPass::ID = 0;


static RegisterPass<LICMPass> X("licm", "LICM Pass");