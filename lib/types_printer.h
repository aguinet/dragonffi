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

#include <string>

#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Support/raw_ostream.h>

#include <dffi/config.h>
#include <dffi/cc.h>
#include <dffi/types.h>
#include <dffi/composite_type.h>
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

  TypePrinter(bool ViewEnumAsBasicType = true):
    Decls_(DeclsStr_),
    ViewEnumAsBasicType_(ViewEnumAsBasicType)
  { }

  // Print the type definition.
  llvm::raw_ostream& print_def(llvm::raw_ostream& OS, dffi::Type const* Ty, DeclMode DMode, const char* Name = nullptr);
  

  std::string& getDecls() { return Decls_.str(); }

private:
  void add_decl(dffi::Type const* Ty);
  void add_forward_decl(dffi::Type const* Ty);
  

private:
  void print_decl_impl(llvm::raw_string_ostream& ss, dffi::CompositeType const* Ty);
  void print_decl_impl(llvm::raw_string_ostream& ss, dffi::EnumType const* Ty);
  

private:
  llvm::DenseMap<dffi::Type const*, std::string> NamedTys_;
  llvm::DenseSet<dffi::Type const*> Declared_;
  llvm::DenseSet<dffi::Type const*> ForwardDeclared_;
  std::string DeclsStr_;
  llvm::raw_string_ostream Decls_;
  bool ViewEnumAsBasicType_;
};

} // dffi

#endif
