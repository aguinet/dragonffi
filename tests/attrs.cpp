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

// RUN: "%build_dir/attrs%exeext"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

int main(int argc, char** argv)
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;
  Opts.LazyJITWrappers = false;

  DFFI Jit(Opts);

  std::string Err;

  auto CU = Jit.compile(R"(
int __attribute__((ms_abi)) foo(int a, int b) { return a+b; }
)", Err);
  if (!CU) {
    std::cerr << "Error: " << Err << std::endl;
    return 1;
  }

  CU = Jit.cdef(R"(
int __attribute__((ms_abi)) foo(int a, int b); 
)", nullptr, Err);
  if (!CU) {
    std::cerr << "Error: " << Err << std::endl;
    return 1;
  }

  auto F = CU.getFunction("foo");
  if (!F) {
    std::cerr << "unable to get function foo!" << std::endl;
    return 1;
  }
  int Ret;
  int a = 1;
  int b = 10;
  void* Args[] = {&a, &b};
  F.call(&Ret, Args);
  if (Ret != 11) {
    std::cerr << "invalid result!" << std::endl;
    return 1;
  }

  return 0;
}
