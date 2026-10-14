
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

namespace {

std::string csvEscape(const std::string &s) {
    std::string result = "\"";

    for (char c : s) {
        if (c == '"')
            result += "\"\"";
        else
            result += c;
    }

    result += '"';
    return result;
}

struct TracePass : PassInfoMixin<TracePass> {
    PreservedAnalyses run(
        Module &M,
        ModuleAnalysisManager &
    ) {
        // Снимок исходных инструкций до инструментирования.
        std::vector<Instruction *> original_instructions;

        for (Function &F : M) {
            if (F.isDeclaration())
                continue;

            for (Instruction &I : instructions(F)) {
                // Отладочные псевдоинструкции не трассируем.
                if (isa<DbgInfoIntrinsic>(I))
                    continue;

                original_instructions.push_back(&I);
            }
        }

        // Назначаем ID инструкциям.
        std::unordered_map<const Instruction *, uint64_t> ids;
        uint64_t nextId = 1;

        for (Instruction *I : original_instructions)
            ids[I] = nextId++;

        // Сохраняем расшифровку ID.
        const char *mapEnv = std::getenv("TRACE_MAP_FILE");
        const std::string mapPath =
            mapEnv ? mapEnv : "original_instructions.csv";

        std::ofstream mapFile(mapPath);

        if (!mapFile) {
            errs() << "TracePass: cannot open map file: "
                   << mapPath << "\n";
            return PreservedAnalyses::all();
        }

        mapFile << "id,function,opcode,instruction\n";

        for (Instruction *I : original_instructions) {
            std::string text;
            raw_string_ostream stream(text);
            I->print(stream);
            stream.flush();

            mapFile << ids[I] << ','
                    << csvEscape(I->getFunction()->getName().str())
                    << ','
                    << csvEscape(I->getOpcodeName())
                    << ','
                    << csvEscape(text)
                    << '\n';
        }

        mapFile.close();

        // Объявляем функцию, которую реализует runtime-компонент.
        LLVMContext &Ctx = M.getContext();
        Type *VoidTy = Type::getVoidTy(Ctx);
        Type *I64Ty = Type::getInt64Ty(Ctx);

        FunctionCallee TraceUse = M.getOrInsertFunction(
            "trace_use",
            FunctionType::get(
                VoidTy,
                {I64Ty, I64Ty},
                false
            )
        );

        // Вставляем событие для каждого операнда-инструкции.
        for (Instruction *User : original_instructions) {
            // По условию пропускаем phi в роли User.
            if (isa<PHINode>(User))
                continue;

            for (Use &U : User->operands()) {
                auto *Operand = dyn_cast<Instruction>(U.get());

                if (!Operand)
                    continue;

                auto It = ids.find(Operand);

                if (It == ids.end())
                    continue;

                IRBuilder<> Builder(User);

                Builder.CreateCall(
                    TraceUse,
                    {
                        ConstantInt::get(I64Ty, ids[User]),
                        ConstantInt::get(I64Ty, It->second)
                    }
                );
            }
        }

        errs() << "TracePass: instrumented module "
               << M.getSourceFileName() << "\n";
        errs() << "TracePass: original_instructions = "
               << original_instructions.size() << "\n";

        return PreservedAnalyses::none();
    }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "TracePass",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                    MPM.addPass(TracePass());
                }
            );
        }
    };
}
