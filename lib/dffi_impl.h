// Copyright 2018 Adrien Guinet <adrien@guinet.me>
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DFFI_IMPL_H
#define DFFI_IMPL_H

#include <memory>
#include <sstream>
#include <unordered_set>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LLVMContext.h>

#include <clang/Frontend/FrontendAction.h>

#include <dffi/dffi.h>
#include <dffi/composite_type.h>

#include "dffictx.h"

namespace llvm {
class Module;
class Function;
class TargetMachine;
class ExecutionEngine;
class DIType;
class DICompositeType;
class DISubroutineType;
namespace vfs {
class FileSystem;
}
} // llvm

namespace clang {
class CompilerInstance;
class DiagnosticIDs;
class DiagnosticOptions;
class TextDiagnosticPrinter;
} // clang

namespace dffi {

// Types
class BasicType;
class PointerType;
class StructType;
class UnionType;
class FunctionType;
struct TypePrinter;

namespace details {

void getFuncTrampolineName(llvm::SmallVectorImpl<char>& Ret, llvm::StringRef const Name);
void getFuncWrapperName(llvm::SmallVectorImpl<char>& Ret, llvm::StringRef const Name);
llvm::StringRef getFuncNameFromWrapper(llvm::StringRef const Name);
bool isWrapperFunction(llvm::StringRef const Name);

typedef llvm::StringMap<dffi::FunctionType const*> FuncTysMap;
typedef llvm::StringMap<std::unique_ptr<dffi::CanOpaqueType>> CompositeTysMap;
typedef llvm::StringMap<dffi::Type const*> AliasTysMap;
typedef llvm::DenseMap<llvm::DICompositeType const*, dffi::Type*> AnonTysMap;
typedef llvm::StringMap<std::string> FuncAliasesMap;

llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> getClangResFileSystem();
const char* getClangResRootDirectory();

struct CUImpl;

struct DFFIImpl
{
  friend struct CUImpl;

  DFFIImpl(CCOpts const& Opts);
  ~DFFIImpl();

  CUImpl* compile(llvm::StringRef const Code, llvm::StringRef CUName, bool IncludeDefs, std::string& Err);

  BasicType const* getBasicType(BasicType::BasicKind K);
  PointerType const* getPointerType(QualType Ty);
  ArrayType const* getArrayType(QualType Ty, uint64_t NElements);
  NativeFunc getFunction(FunctionType const* FTy, void* FPtr);
  NativeFunc getFunction(FunctionType const* FTy, llvm::ArrayRef<Type const*> VarArgs, void* FPtr);

  llvm::Function* getWrapperLLVMFunc(FunctionType const* FTy, llvm::ArrayRef<Type const*> VarArgs);

protected:
  DFFICtx& getContext() { return DCtx_; }
  DFFICtx const& getContext() const { return DCtx_; }
  void* getFunctionAddress(llvm::StringRef Name);

private:
  std::unique_ptr<llvm::Module> compile_llvm_with_decls(llvm::StringRef const Code, llvm::StringRef const CUName, FuncAliasesMap& FuncAliases, std::string& Err);
  std::unique_ptr<llvm::Module> compile_llvm(llvm::StringRef const Code, llvm::StringRef const CUName, std::string& Err);

  std::pair<size_t, bool> getFuncTypeWrapperId(FunctionType const* FTy);
  std::pair<size_t, bool> getFuncTypeWrapperId(FunctionType const* FTy, llvm::ArrayRef<Type const*> VarArgs);
  void genFuncTypeWrapper(TypePrinter& P, size_t WrapperIdx, llvm::raw_string_ostream& ss, FunctionType const* FTy, llvm::ArrayRef<Type const*> VarArgs);
  void getCompileError(std::string& Err);
  void setNewDiagnostics();
  void compileWrappers(TypePrinter& P, std::string const& Wrappers);

  void* getWrapperAddress(FunctionType const* FTy);
  void* getWrapperAddress(FunctionType const* FTy, llvm::ArrayRef<Type const*> VarArgs);

private:
  std::unique_ptr<clang::CompilerInstance> Clang_;
  std::string ErrorMsg_;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID_;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts_;
  llvm::raw_string_ostream ErrorMsgStream_;
  llvm::LLVMContext Ctx_;
  std::unique_ptr<llvm::TargetMachine> TM_;
  std::unique_ptr<llvm::ExecutionEngine> EE_;
  llvm::IntrusiveRefCntPtr<clang::SourceManager> SrcMgr_;
  llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> VFS_;
  llvm::IntrusiveRefCntPtr<clang::FileManager> FileMgr_;
  llvm::SmallVector<std::unique_ptr<CUImpl>, 8> CUs_;
  llvm::DenseMap<dffi::FunctionType const*, size_t> FuncTyWrappers_;
  llvm::DenseMap<std::pair<dffi::FunctionType const*, llvm::ArrayRef<Type const*>>, size_t> VarArgsFuncTyWrappers_;
  size_t WrapperIdx_ = 0;

  DFFICtx DCtx_;

  CCOpts Opts_;

  size_t CUIdx_ = 0;
};

struct CUImpl
{
  
  CUImpl(DFFIImpl& DFFI);

  dffi::Type const* getType(llvm::StringRef Name) const;

  dffi::StructType const* getStructType(llvm::StringRef Name) const;
  dffi::UnionType const* getUnionType(llvm::StringRef Name) const;
  dffi::EnumType const* getEnumType(llvm::StringRef Name) const;

  BasicType const* getBasicType(BasicType::BasicKind K) const {
    return DFFI_.getBasicType(K);
  }
  PointerType const* getPointerType(QualType Ty) const {
    return DFFI_.getPointerType(Ty);
  }

  dffi::FunctionType const* getFunctionType(llvm::DISubroutineType const* Ty);
  dffi::FunctionType const* getFunctionType(llvm::Function& F);

  QualType getQualTypeFromDIType(llvm::DIType const* Ty);
  dffi::Type const* getTypeFromDIType(llvm::DIType const* Ty);

  std::tuple<void*, FunctionType const*> getFunctionAddressAndTy(llvm::StringRef Name);

  NativeFunc getFunction(llvm::StringRef Name);
  NativeFunc getFunction(void* FPtr, FunctionType const* FTy);
  NativeFunc getFunction(llvm::StringRef Name, llvm::ArrayRef<Type const*> VarArgs);
  NativeFunc getFunction(void* FPtr, FunctionType const* FTy, llvm::ArrayRef<Type const*> VarArgs);


  void declareDIComposite(llvm::DICompositeType const* Ty);
  void parseDIComposite(llvm::DICompositeType const* Ty, llvm::Module& M);
  void setAlias(llvm::StringRef Name, dffi::Type const* Ty) { AliasTys_[Name] = Ty; }
  void parseFunctionAlias(llvm::Function& F);

  DFFICtx& getContext() { return DFFI_.getContext(); }
  DFFICtx const& getContext() const { return DFFI_.getContext(); }

  void inlineCompositesAnonymousMembers();

  std::vector<std::string> getTypes() const;
  std::vector<std::string> getFunctions() const;

  DFFIImpl& DFFI_;

  CompositeTysMap CompositeTys_;
  FuncTysMap FuncTys_;
  AliasTysMap AliasTys_;
  FuncAliasesMap FuncAliases_;

  // Temporary map used during debug metadata parsing
  AnonTysMap AnonTys_;

private:
  static void inlineCompositesAnonymousMembersImpl(std::unordered_set<CompositeType*>& Visited, CompositeType* CTy);
};

struct ASTGenWrappersAction: public clang::ASTFrontendAction
{
  ASTGenWrappersAction(FuncAliasesMap& FuncAliases):
    clang::ASTFrontendAction(),
    FuncAliases_(FuncAliases),
    ForceDecls_(ForceDeclsStr_)
  { }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) override;

  std::string& forceDecls(); 

  llvm::IntrusiveRefCntPtr<clang::ASTContext> releaseAST() { return std::move(ASTCtxt_); }

private:
  std::string ForceDeclsStr_;
  llvm::raw_string_ostream ForceDecls_;
  llvm::IntrusiveRefCntPtr<clang::ASTContext> ASTCtxt_;
  FuncAliasesMap& FuncAliases_;
};

} // details
} // dffi

#endif
