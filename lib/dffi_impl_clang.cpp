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

#include "dffi_impl.h"


#include <functional>
#include <sstream>
#include <unordered_set>

using namespace clang;
using namespace llvm;

namespace dffi {
namespace details {

namespace {

struct ASTGenWrappersConsumer: public clang::ASTConsumer
{
  ASTGenWrappersConsumer(std::stringstream& ForceDecls, FuncAliasesMap& FuncAliases, LangOptions const& LO):
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

    bool NoReturn = FTy->getExtInfo().getNoReturn();
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
        P = ParmVarDecl::Create(ASTCtx, NewFDCtx, P->getLocStart(), P->getLocation(),
          &ASTCtx.Idents.get(Name), P->getType(), P->getTypeSourceInfo(), P->getStorageClass(), nullptr);
      }
      NewParams.push_back(P);
    }
    NewFD->setParams(NewParams);

    std::string NewFStr;
    raw_string_ostream NewF(NewFStr);
    NewFD->print(NewF, PP_, 0, true);
    NewF << " {}\n";

    ForceDecls_ << NewF.str();
  }

private:
  std::stringstream& ForceDecls_;
  FuncAliasesMap& FuncAliases_;
  PrintingPolicy PP_;
  unsigned ForceIdx_;
};

} // anonymous


std::unique_ptr<clang::ASTConsumer> ASTGenWrappersAction::CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile)
{
  ASTCtxt_ = llvm::IntrusiveRefCntPtr<clang::ASTContext>{&Compiler.getASTContext()};
  return llvm::make_unique<ASTGenWrappersConsumer>(ForceDecls_, FuncAliases_, Compiler.getLangOpts());
}

std::string ASTGenWrappersAction::getForceDecls() const
{
  return ForceDecls_.str();
}

} // details
} //dffi
