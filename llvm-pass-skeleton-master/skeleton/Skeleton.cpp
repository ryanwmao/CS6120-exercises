#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct SkeletonPass : public PassInfoMixin<SkeletonPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        for (auto &F : M) {
            errs() << "Function " << F.getName() << "\n";
            for (auto& B : F) {
                for (auto& I : B) {
                    if (auto* bop = dyn_cast<BinaryOperator>(&I)) {
                        IRBuilder<> builder(bop);
                        Value *lhs = bop->getOperand(0);
                        Value *rhs = bop->getOperand(1);
                        
                        if (auto* lhsConst = dyn_cast<Constant>(lhs)) {
                            if (lhsConst->isOneValue()) {
                                lhs = builder.CreateNeg(lhs);
                            }
                        }

                        if (auto* rhsConst = dyn_cast<Constant>(rhs)) {
                            if (rhsConst->isOneValue()) {
                                rhs = builder.CreateNeg(rhs);
                            }
                        }

                        bop->setOperand(0, rhs);
                        bop->setOperand(1, lhs);

                        errs() << "     " << *bop << "\n";
                    }
                }
            }
        }
        return PreservedAnalyses::all();
    };
};

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "Skeleton pass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(SkeletonPass());
                });
        }
    };
}
