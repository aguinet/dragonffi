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

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ADT/StringSet.h>

#include "dffi_impl.h"


#include <functional>
#include <sstream>
#include <unordered_set>

using namespace clang;
using namespace llvm;

namespace dffi {
namespace details {

namespace {

// This is from clang/AST/TypePrinter.cpp. This function isn't exported :/
// This is used as part of a hack because the "dump" API of FunctionDecl does
// not seem to print these attributes!
void printFunctionAttrs(const clang::FunctionType::ExtInfo &Info,
                           raw_ostream &OS) {
  switch (Info.getCC()) {
  case CC_C:
    // The C calling convention is the default on the vast majority of platforms
    // we support.  If the user wrote it explicitly, it will usually be printed
    // while traversing the AttributedType.  If the type has been desugared, let
    // the canonical spelling be the implicit calling convention.
    // FIXME: It would be better to be explicit in certain contexts, such as a
    // cdecl function typedef used to declare a member function with the
    // Microsoft C++ ABI.
    break;
  case CC_X86StdCall:
    OS << " __attribute__((stdcall))";
    break;
  case CC_X86FastCall:
    OS << " __attribute__((fastcall))";
    break;
  case CC_X86ThisCall:
    OS << " __attribute__((thiscall))";
    break;
  case CC_X86VectorCall:
    OS << " __attribute__((vectorcall))";
    break;
  case CC_X86Pascal:
    OS << " __attribute__((pascal))";
    break;
  case CC_AAPCS:
    OS << " __attribute__((pcs(\"aapcs\")))";
    break;
  case CC_AAPCS_VFP:
    OS << " __attribute__((pcs(\"aapcs-vfp\")))";
    break;
  case CC_IntelOclBicc:
    OS << " __attribute__((intel_ocl_bicc))";
    break;
  case CC_Win64:
    OS << " __attribute__((ms_abi))";
    break;
  case CC_X86_64SysV:
    OS << " __attribute__((sysv_abi))";
    break;
  case CC_X86RegCall:
    OS << " __attribute__((regcall))";
    break;
  case CC_SpirFunction:
  case CC_OpenCLKernel:
    // Do nothing. These CCs are not available as attributes.
    break;
  case CC_Swift:
    OS << " __attribute__((swiftcall))";
    break;
  case CC_PreserveMost:
    OS << " __attribute__((preserve_most))";
    break;
  case CC_PreserveAll:
    OS << " __attribute__((preserve_all))";
    break;
  }

  if (Info.getNoReturn())
    OS << " __attribute__((noreturn))";
  if (Info.getProducesResult())
    OS << " __attribute__((ns_returns_retained))";
  if (Info.getRegParm())
    OS << " __attribute__((regparm ("
       << Info.getRegParm() << ")))";
  if (Info.getNoCallerSavedRegs())
    OS << " __attribute__((no_caller_saved_registers))";
}

struct ASTGenWrappersConsumer: public clang::ASTConsumer
{
  ASTGenWrappersConsumer(raw_string_ostream& ForceDecls, FuncAliasesMap& FuncAliases, LangOptions const& LO):
    ForceDecls_(ForceDecls),
    FuncAliases_(FuncAliases),
    PP_(LO),
    ForceIdx_(0)
  {
    PP_.IncludeTagDefinition = false;
    PP_.AnonymousTagLocations = false;
    PP_.IncludeNewlines = false;
  }

  bool HandleTopLevelDecl(DeclGroupRef DR) override
  {
    for (auto& D: DR) {
      if (auto* FD = llvm::dyn_cast<FunctionDecl>(D)) {
        HandleFD(FD);
      }
      else
      if (auto* TD = llvm::dyn_cast<TypedefDecl>(D)) {
        HandleTypedefDecl(TD);
      }
    }
    return true;
  }

private:
  void HandleTypedefDecl(TypedefDecl* TD)
  {
    // TODO: AFAIK, no use-def chain in clang, see if there is a way to know if
    // it is used somewhere!
    // Force the emission of this type by creating an empty function that takes
    // this type as argument!
    ForceDecls_ << "void __dffi_force_typedef_" << std::to_string(ForceIdx_++) << "(" << TD->getName().str() << " *__Arg) {}\n";
  }

  void HandleFD(FunctionDecl* FD)
  {
    if (FD->hasBody() && !FD->isInlineSpecified()) {
      return;
    }

    if (auto* Prev = FD->getPreviousDecl()) {
      if (!Prev->isImplicit()) {
        return;
      }
    }

    auto* FTy = FD->getType()->getAs<clang::FunctionType>();
    assert(FTy);

    auto const& FExtInfo = FTy->getExtInfo();
    bool NoReturn = FExtInfo.getNoReturn();
    if (NoReturn) {
      return;
    }

    // Function name and params
    std::string FuncName;
    DeclarationName DeclName = FD->getNameInfo().getName();
    if (auto *Attr = FD->getAttr<AsmLabelAttr>()) {
      FuncName = Attr->getLabel().str();
      FuncAliases_[DeclName.getAsString()] = FuncName;
    }
    else {
      FuncName = DeclName.getAsString();
    }

    // Verify that we didn't already visited that function. It can occur with aliases.
    if (!Visited_.insert(FuncName).second) {
      return;
    }

    auto Params = FD->parameters();

    SmallString<128> NewFuncNameBuf;
    StringRef NewFuncName = (Twine{"__dffi_force_decl_"} + FuncName).toStringRef(NewFuncNameBuf);

    auto& ASTCtx = FD->getASTContext();
    IdentifierInfo* NewName = &ASTCtx.Idents.get(NewFuncName);

    // TODO: create a real body within the clang AST, and find a way to feed this back to llvm!
    FunctionDecl* NewFD = FunctionDecl::Create(ASTCtx, ASTCtx.getTranslationUnitDecl(), SourceLocation(), SourceLocation(), NewName, clang::QualType{FTy, 0}, nullptr, SC_None);
    auto* NewFDCtx = NewFD->getDeclContext();
    SmallVector<ParmVarDecl*, 8> NewParams;
    NewParams.reserve(Params.size());
    size_t ArgId = 0;
    for (ParmVarDecl* P: Params) {
      if (P->getName().empty()) {
        std::string Name = "__Arg_" + std::to_string(ArgId++);
        P = ParmVarDecl::Create(ASTCtx, NewFDCtx, P->getBeginLoc(), P->getLocation(),
          &ASTCtx.Idents.get(Name), P->getType(), P->getTypeSourceInfo(), P->getStorageClass(), nullptr);
      }
      NewParams.push_back(P);
    }
    NewFD->setParams(NewParams);

    printFunctionAttrs(FExtInfo, ForceDecls_);
    NewFD->print(ForceDecls_, PP_, 0, true);
    ForceDecls_ << " {}\n";
  }

private:
  raw_string_ostream& ForceDecls_;
  FuncAliasesMap& FuncAliases_;
  StringSet<> Visited_;
  PrintingPolicy PP_;
  unsigned ForceIdx_;
};

} // anonymous


std::unique_ptr<clang::ASTConsumer> ASTGenWrappersAction::CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile)
{
  ASTCtxt_ = llvm::IntrusiveRefCntPtr<clang::ASTContext>{&Compiler.getASTContext()};
  return std::make_unique<ASTGenWrappersConsumer>(ForceDecls_, FuncAliases_, Compiler.getLangOpts());
}

std::string& ASTGenWrappersAction::forceDecls()
{
  return ForceDecls_.str();
}

} // details
} //dffi
