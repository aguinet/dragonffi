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

// RUN: "%build_dir/system_headers%exeext" | "%FileCheck" "%s"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

int main(int argc, char** argv)
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
#include <stdio.h>
void foo() {
  puts("hello world!");
}
)",
  Err);

  if (!CU) {
    std::cerr << "Compile error: " << Err << std::endl;
    return 1;
  }

  // CHECK: hello world!
  CU.getFunction("foo").call();

  CU = Jit.cdef("#include <stdio.h>\n#include <stdlib.h>\n", nullptr, Err);
  if (!CU) {
    std::cerr << "Compile error: " << Err << std::endl;
    return 1;
  }

  const char* Msg = "hello puts!";
  void* Args[] = {&Msg};
  int Ret;
  // CHECK: hello puts!
  CU.getFunction("puts").call(&Ret, Args);
  if (Ret < 0) {
    std::cerr << "error while calling puts!" << std::endl;
    return 1;
  }

  const char* Int = "10";
  Args[0] = &Int;
  Ret = -1;
  CU.getFunction("atoi").call(&Ret, Args);
  if (Ret != 10) {
    std::cerr << "error while calling atoi!" << std::endl;
    return 1;
  }


  return 0;
}
