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

// RUN: "%build_dir/enum" | "%FileCheck" "%s"

#include <iostream>
#include <dffi/dffi.h>
#include <dffi/composite_type.h>

namespace dffi {

Type::HashType hash_value(Type const*);

} // dffi

using namespace dffi;


int main()
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI F(Opts);
  std::cout << hash_value(F.getVoidTy()) << std::endl;
  std::cout << hash_value(F.getBoolTy()) << std::endl;
  std::cout << hash_value(F.getCharTy()) << std::endl;
  std::cout << hash_value(F.getSCharTy()) << std::endl;
  std::cout << hash_value(F.getUCharTy()) << std::endl;
  std::cout << hash_value(F.getIntTy()) << std::endl;
  std::cout << hash_value(F.getLongTy()) << std::endl;
  std::cout << hash_value(F.getFloatTy()) << std::endl;
  std::cout << hash_value(F.getDoubleTy()) << std::endl;
  std::cout << hash_value(F.getLongDoubleTy()) << std::endl;
  std::cout << std::endl;
  std::cout << hash_value(F.getVoidPtrTy()) << std::endl;
  std::cout << hash_value(F.getBoolPtrTy()) << std::endl;
  std::cout << hash_value(F.getIntPtrTy()) << std::endl;
  std::cout << hash_value(F.getLongPtrTy()) << std::endl;
  std::cout << hash_value(F.getLongDoublePtrTy()) << std::endl;
  std::cout << std::endl;
  std::cout << hash_value(F.getArrayType(F.getInt8Ty(), 10)) << std::endl;
  std::cout << hash_value(F.getArrayType(F.getInt8Ty(), 11)) << std::endl;
  std::cout << hash_value(F.getArrayType(F.getInt16Ty(), 10)) << std::endl;
  std::cout << hash_value(F.getArrayType(F.getInt16Ty(), 11)) << std::endl;

  std::string Err;
  auto CU = F.cdef("struct A { int a; short b; }; struct A get();", nullptr, Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }
  auto* A = CU.getStructType("A");
  if (!A) {
    std::cerr << "unable to get A!" << std::endl;
    return 1;
  }
  std::cout << hash_value(A) << std::endl;
  return 0;
}
