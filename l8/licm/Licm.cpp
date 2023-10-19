#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

namespace {
    struct LICMPass : public FunctionPass {
        static char ID;
        LICMPass() : FunctionPass(ID) {}

        virtual bool runOnFunction(Function &F) {
            LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

            for (LoopInfo::iterator L = LI.begin(); L != LI.end(); L++) {
                for (Loop::block_iterator BB = *L->block_begin(); BB != *L->block_end(); BB++) {
                    BasicBlock *Preheader = *L->getLoopPreheader();

                    BasicBlock::iterator I = *BB->begin();
                    while (I != *BB->end()) {
                        // Check if the instruction is loop invariant
                        if (isLoopInvariant(I, *L)) {
                            // Move the instruction to the loop preheader
                            I->moveBefore(Preheader->getTerminator());
                            I = (*BB)->erase(I);
                        } else {
                            ++I;
                        }
                    }
                }
            }
            return true;
        }

        bool isLoopInvariant(Instruction *I, Loop *L) {
            for (Value* operand : I->operands()) {
                if (!L->isLoopInvariant(operand)) return false;
            }
            return true;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<LoopInfoWrapperPass>();
        }
    };
}

char LICMPass::ID = 0;

static void registerLICMPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
    PM.add(new LICMPass());
}

static RegisterStandardPasses RegisterLICMPass(PassManagerBuilder::EP_EarlyAsPossible, registerLICMPass);