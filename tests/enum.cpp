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

using namespace dffi;

int main(int argc, char** argv)
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
enum A
{
  V0 = 0,
  V1 = 1,
  V4 = 4,
  V10 = 10
};

int get(enum A a) {
  return a;
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  auto* Enum = CU.getEnumType("A");
  // CHECK-DAG: V0 = 0
  // CHECK-DAG: V1 = 1
  // CHECK-DAG: V4 = 4
  // CHECK-DAG: V10 = 10
  for (auto const& F: Enum->getFields()) {
    std::cout << F.first << " = " << F.second << std::endl;
  }

  return 0;
}
