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

#ifndef DFFI_TYPE_PRINTER_H
#define DFFI_TYPE_PRINTER_H

#include <dffi/config.h>
#include <dffi/cc.h>
#include <dffi/types.h>
#include <dffi/casting.h>
#include "tools.h"

using dffi::cast;
using dffi::dyn_cast;
using dffi::dyn_cast_or_null;
using dffi::isa;

namespace dffi {

struct TypePrinter
{
  enum DeclMode
  {
    Full,
    Forward,
    None
  };

  TypePrinter():
    Decls_(DeclsStr_)
  { }

  // Print the type definition.
  llvm::raw_ostream& print_def(llvm::raw_ostream& OS, dffi::Type const* Ty, DeclMode DMode, const char* Name = nullptr)
  {
    if (Ty == nullptr) {
      OS << "void";
      if (Name) {
        OS << ' ';
        OS << Name;
      }
      return OS;
    }
    switch (Ty->getKind()) {
    case dffi::Type::TY_Basic:
    {
      auto* BTy = cast<BasicType>(Ty);
      switch (BTy->getBasicKind()) {
        case BasicType::Bool:
          OS << "_Bool";
          break;
        case BasicType::Char:
          OS << "char";
          break;
        case BasicType::SChar:
          OS << "signed char";
          break;
        case BasicType::Short:
          OS << "short";
          break;
        case BasicType::Int:
          OS << "int";
          break;
        case BasicType::Long:
          OS << "long";
          break;
        case BasicType::LongLong:
          OS << "long long";
          break;
#ifdef DFFI_SUPPORT_I128
        case BasicType::Int128:
          OS << "__int128_t";
          break;
#endif
        case BasicType::UChar:
          OS << "unsigned char";
          break;
        case BasicType::UShort:
          OS << "unsigned short";
          break;
        case BasicType::UInt:
          OS << "unsigned int";
          break;
        case BasicType::ULong:
          OS << "unsigned long";
          break;
        case BasicType::ULongLong:
          OS << "unsigned long long";
          break;
#ifdef DFFI_SUPPORT_I128
        case BasicType::UInt128:
          OS << "__uint128_t";
          break;
#endif
        case BasicType::Float:
          OS << "float";
          break;
        case BasicType::Double:
          OS << "double";
          break;
        case BasicType::LongDouble:
          OS << "long double";
          break;
#ifdef DFFI_SUPPORT_COMPLEX
        case BasicType::ComplexFloat:
          OS << "_Complex float";
          break;
        case BasicType::ComplexDouble:
          OS << "_Complex double";
          break;
        case BasicType::ComplexLongDouble:
          OS << "_Complex long double";
          break;
#endif
      };
      if (Name)
        OS << ' ' << Name;
      return OS; 
    }
    case dffi::Type::TY_Pointer:
    {
      auto* PTy = cast<PointerType>(Ty);
      auto* Pointee = PTy->getPointee().getType();
      std::string PtrName = "*";
      if (Name)
        PtrName += Name;
      return print_def(OS,Pointee,Forward,PtrName.c_str());
    }
    case dffi::Type::TY_Function:
    {
      auto* FTy = cast<FunctionType>(Ty);
      std::string Buf;
      llvm::raw_string_ostream ss(Buf);

      ss << '(' << CCToClangAttribute(FTy->getCC()) << ' ' << (Name ? Name:"") << ')';
      ss << '(';
      if (FTy->getParams().size() > 0) {
        auto const& Params = FTy->getParams();
        auto ItLast = --Params.end();
        for (auto It = Params.begin(); It != ItLast; ++It) {
          print_def(ss, *It, Full); 
          ss << ',';
        }
        print_def(ss, *ItLast, Full); 
        if (FTy->hasVarArgs()) {
          ss << ",...";
        }
      }
      ss << ')';
      return print_def(OS, FTy->getReturnType(), Full, ss.str().c_str());
    }
    case dffi::Type::TY_Struct:
    case dffi::Type::TY_Union:
    case dffi::Type::TY_Enum:
    {
      if (DMode == Full)
        add_decl(Ty);
      else
      if (DMode == Forward)
        add_forward_decl(Ty);

      if (isa<dffi::StructType>(Ty))
        OS << "struct ";
      else
      if (isa<dffi::UnionType>(Ty))
        OS << "union ";
      else
      if (isa<dffi::EnumType>(Ty))
        OS << "enum ";

      auto It = NamedTys_.find(Ty);
      if (It != NamedTys_.end()) {
        OS << It->second;
      }
      else {
        std::string NameTy = "__dffi_ty_" + std::to_string(NamedTys_.size());
        NamedTys_.insert(std::make_pair(Ty, NameTy));
        OS << NameTy;
      }
      if (Name) {
        OS << ' ' << Name;
      }
      return OS;
    }
    case dffi::Type::TY_Array:
    {
      auto* ArTy = cast<ArrayType>(Ty);
      print_def(OS, ArTy->getElementType(), Full, Name);
      OS << '[' << ArTy->getNumElements() << ']';
      return OS;
    }
    };
  }

  std::string& getDecls() { return Decls_.str(); }

private:
  void add_decl(dffi::Type const* Ty)
  {
    if (!isa<dffi::CanOpaqueType>(Ty)) { 
      return;
    }

    auto It = Declared_.insert(Ty);
    if (!It.second) {
      return;
    }

    if (auto* STy = dyn_cast<dffi::CompositeType>(Ty)) {
      print_decl_impl(Decls_, STy);
    }
    else
    if (auto* ETy = dyn_cast<dffi::EnumType>(Ty)) {
      print_decl_impl(Decls_, ETy);
    }
    Decls_ << '\n';
  }

  void add_forward_decl(dffi::Type const* Ty)
  {
    if (!isa<dffi::CanOpaqueType>(Ty)) {
      return;
    }
    if (Declared_.count(Ty))
      return;

    auto It = ForwardDeclared_.insert(Ty);
    if (!It.second) {
      return;
    }
    print_def(Decls_, Ty, None) << ";\n";
  }

private:
  void print_decl_impl(llvm::raw_string_ostream& ss, dffi::CompositeType const* Ty)
  {
    print_def(ss, Ty, None) << " {\n";
    size_t Idx = 0;
    for (auto const& F: Ty->getFields()) {
      std::string Name = "__Field_" + std::to_string(Idx);
      ss << "  ";
      print_def(ss, F.getType(), Full, Name.c_str());
      unsigned SizeBits = F.getSizeBits();
      if (SizeBits != F.getType()->getSize()*CHAR_BIT) {
        ss << ": " << SizeBits;
      }
      ss << ";\n";
      ++Idx;
    }
    ss << "};\n";
  }

  void print_decl_impl(llvm::raw_string_ostream& ss, dffi::EnumType const* Ty)
  {
    print_def(ss, Ty, None) << " {\n";
    for (auto const& F: Ty->getFields()) {
      ss << "  " << F.first << " = " << F.second << ",\n";
    }
    ss << "};\n";
  }

private:
  llvm::DenseMap<dffi::Type const*, std::string> NamedTys_;
  llvm::DenseSet<dffi::Type const*> Declared_;
  llvm::DenseSet<dffi::Type const*> ForwardDeclared_;
  std::string DeclsStr_;
  llvm::raw_string_ostream Decls_;
};

} // dffi

#endif
