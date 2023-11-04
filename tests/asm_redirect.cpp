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

// RUN: "%build_dir/asm_redirect%exeext" | "%FileCheck" "%s"

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
  // This trick comes directly from stdio.h under Debian! Enjoy' :)
  auto CU = Jit.compile(R"(
#include <stdio.h>
void myfunc() __asm__ ("" "redirected_myfunc");
void myfunc() { puts("toto"); }
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  // CHECK: toto
  CU.getFunction("myfunc").call();
  // CHECK: toto
  CU.getFunction("redirected_myfunc").call();

  CU = Jit.cdef(R"(
void myfunc() __asm__ ("" "redirected_myfunc");
void myfunc();
)", nullptr, Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  // CHECK: toto
  CU.getFunction("myfunc").call();
  // CHECK: toto
  CU.getFunction("redirected_myfunc").call();

  return 0;
}
