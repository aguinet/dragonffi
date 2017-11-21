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

#include <dffi/cc.h>
#include <dffi/types.h>
#include <dffi/casting.h>

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
  // Print the type definition. It supposes <stdint.h> is included!
  std::string print_def(dffi::Type const* Ty, DeclMode DMode, const char* Name = nullptr)
  {
    if (Ty == nullptr) {
      std::string Ret = "void";
      if (Name) {
        Ret += " ";
        Ret += Name;
      }
      return Ret;
    }
    switch (Ty->getKind()) {
    case dffi::Type::TY_Basic:
    {
      auto* BTy = cast<BasicType>(Ty);
      std::stringstream ss;
      switch (BTy->getBasicKind()) {
        case BasicType::Char:
          ss << "char";
          break;
        case BasicType::Int8:
        case BasicType::Int16:
        case BasicType::Int32:
        case BasicType::Int64:
          ss << "int" << BTy->getSize() * 8 << "_t";
          break;
        case BasicType::Int128:
          ss << "__int128_t";
          break;
        case BasicType::UInt8:
        case BasicType::UInt16:
        case BasicType::UInt32:
        case BasicType::UInt64:
          ss << "uint" << BTy->getSize() * 8 << "_t";
          break;
        case BasicType::UInt128:
          ss << "__uint128_t";
          break;
        case BasicType::Float32:
          ss << "float";
          break;
        case BasicType::Float64:
          ss << "double";
          break;
        case BasicType::Float128:
          ss << "long double";
          break;
        case BasicType::ComplexFloat32:
          ss << "_Complex float";
          break;
        case BasicType::ComplexFloat64:
          ss << "_Complex double";
          break;
        case BasicType::ComplexFloat128:
          ss << "_Complex long double";
          break;
      };
      if (Name)
        ss << " " << Name;
      return ss.str();
    }
    case dffi::Type::TY_Pointer:
    {
      auto* PTy = cast<PointerType>(Ty);
      auto* Pointee = PTy->getPointee().getType();
      std::string PtrName = "*";
      if (Name)
        PtrName += Name;
      return print_def(Pointee,Forward,PtrName.c_str());
    }
    case dffi::Type::TY_Function:
    {
      auto* FTy = cast<FunctionType>(Ty);
      std::stringstream ss;

      ss << "(" << CCToClangAttribute(FTy->getCC()) << " " << (Name ? Name:"") << ")";
      ss << "(";
      if (FTy->getParams().size() > 0) {
        auto const& Params = FTy->getParams();
        auto ItLast = --Params.end();
        for (auto It = Params.begin(); It != ItLast; ++It) {
          ss << print_def(*It, Full); 
          ss << ",";
        }
        ss << print_def(*ItLast, Full); 
      }
      ss << ")";
      return print_def(FTy->getReturnType(), Full, ss.str().c_str());
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

      std::stringstream ss;
      if (isa<dffi::StructType>(Ty))
        ss << "struct ";
      else
      if (isa<dffi::UnionType>(Ty))
        ss << "union ";
      else
      if (isa<dffi::EnumType>(Ty))
        ss << "enum ";

      auto It = NamedTys_.find(Ty);
      if (It != NamedTys_.end()) {
        ss << It->second;
      }
      else {
        std::string NameTy = "__dffi_ty_" + std::to_string(NamedTys_.size());
        NamedTys_.insert(std::make_pair(Ty, NameTy));
        ss << NameTy;
      }
      if (Name) {
        ss << " " << Name;
      }
      return ss.str();
    }
    case dffi::Type::TY_Array:
    {
      auto* ArTy = cast<ArrayType>(Ty);
      std::stringstream ss;
      ss << print_def(ArTy->getElementType(), Full, Name);
      ss << "[" << ArTy->getNumElements() << "]";
      return ss.str();
    }
    };
  }

  std::string getDecls() { return Decls_.str(); }

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

    std::stringstream ss;
    if (auto* STy = dyn_cast<dffi::CompositeType>(Ty)) {
      print_decl_impl(ss, STy);
    }
    else
    if (auto* ETy = dyn_cast<dffi::EnumType>(Ty)) {
      print_decl_impl(ss, ETy);
    }
    Decls_ << ss.str() << "\n";
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
    Decls_ << print_def(Ty, None) << ";\n";
  }

private:
  void print_decl_impl(std::stringstream& ss, dffi::CompositeType const* Ty)
  {
    ss << print_def(Ty, None) << " {\n";
    size_t Idx = 0;
    for (auto const& F: Ty->getFields()) {
      std::string Name = "__Field_" + std::to_string(Idx);
      ss << "  " << print_def(F.getType(), Full, Name.c_str()) << ";\n";
      ++Idx;
    }
    ss << "};\n";
  }

  void print_decl_impl(std::stringstream& ss, dffi::EnumType const* Ty)
  {
    ss << print_def(Ty, None) << " {\n";
    for (auto const& F: Ty->getFields()) {
      ss << "  " << F.first << " = " << F.second << ",\n";
    }
    ss << "};\n";
  }

private:
  llvm::DenseMap<dffi::Type const*, std::string> NamedTys_;
  llvm::DenseSet<dffi::Type const*> Declared_;
  llvm::DenseSet<dffi::Type const*> ForwardDeclared_;
  std::stringstream Decls_;
};

} // dffi

#endif
