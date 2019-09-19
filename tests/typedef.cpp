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

// RUN: "%build_dir/typedef" | "%FileCheck" "%s"

#include <iostream>

#include <dffi/dffi.h>
#include <dffi/composite_type.h>

using namespace dffi;

struct A {
  int a;
  int b;
  short c;
  double d;
};

int main()
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
#include <stdio.h>
typedef int MyInt;
typedef short MyShort;

struct A {
  MyInt a;
  MyInt b;
  MyShort c;
  double d;
};

typedef struct A SA;

void dump(SA a)
{
  printf("a=%d, b=%d, c=%d, d=%f\n", a.a, a.b, a.c, a.d);
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  A obj = {1,2,4,5.};
  void* Args[] = {&obj};
  // CHECK: a=1, b=2, c=4, d=5.000000
  CU.getFunction("dump").call(&Args[0]);

  auto const* STy = CU.getType("SA");
  if (!STy) {
    std::cerr << "unable to find type 'SA'!" << std::endl;
    return 1;
  }
  // CHECK: a
  // CHECK: b
  // CHECK: c
  // CHECK: d
  for (auto const& F: static_cast<StructType const*>(STy)->getOrgFields()) {
    std::cout << F.getName() << std::endl;
  }

  return 0;
}
