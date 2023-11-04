// This file need to be in an external CU, because it uses Clang/LLVM
// structures, which are potentially compiled w/o RTTI. In such a case, this
// file is compiled w/o rtti, in order not to link with inexistant typeinfo
// structures.
// This is kind of a hack waiting for making pybind11 RTTI-free.

#include <dffi/composite_type.h>
#include <dffi/casting.h>
#include <dffi/ctypes.h>
#include "dffi_impl.h"

#include <llvm/IR/Function.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Bitcode/BitcodeWriter.h>

using namespace dffi;

namespace {

std::unique_ptr<llvm::Module> isolateFunc(llvm::Function* F, const char* FuncName)
{
  assert(F && "LLVM wrapper should exist!");
  std::unique_ptr<llvm::Module> Ret(new llvm::Module{"", F->getContext()});
  Ret->setTargetTriple(F->getParent()->getTargetTriple());
  Ret->setDataLayout(F->getParent()->getDataLayout());
  auto* NewF = llvm::Function::Create(F->getFunctionType(), F->getLinkage(), FuncName, Ret.get());
  llvm::ValueToValueMapTy VMap;
  auto NewFArgsIt = NewF->arg_begin();
  auto FArgsIt = F->arg_begin();
  for (auto FArgsEnd = F->arg_end(); FArgsIt != FArgsEnd; ++NewFArgsIt, ++FArgsIt) {
    VMap[&*FArgsIt] = &*NewFArgsIt;
  }
  llvm::SmallVector<llvm::ReturnInst*, 1> Returns;
  llvm::CloneFunctionInto(NewF, F, VMap, llvm::CloneFunctionChangeType::GlobalChanges, Returns);
  return Ret;
}

} // anonymous

std::string FunctionType::getWrapperLLVM(const char* FuncName) const
{
  llvm::Function* F = getDFFI().getWrapperLLVMFunc(this, std::nullopt);
  auto M = isolateFunc(F, FuncName);
  std::string Ret;
  llvm::raw_string_ostream ss(Ret);
  llvm::WriteBitcodeToFile(*M, ss);
  return Ret;
}

std::string FunctionType::getWrapperLLVMStr(const char* FuncName) const
{
  llvm::Function* F = getDFFI().getWrapperLLVMFunc(this, std::nullopt);
  auto M = isolateFunc(F, FuncName);
  std::string Ret;
  llvm::raw_string_ostream ss(Ret);
  ss << *M;
  return Ret;
}
