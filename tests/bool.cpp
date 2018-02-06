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

// RUN: "%build_dir/bool"

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
#include <stdbool.h>
bool foo(int a) { return a == 1; }
bool invert(bool v) { return !v; }
)", Err);
  if (!CU) {
    std::cerr << "Compile error: " << Err << std::endl;
    return 1;
  }
  int a = 1;
  bool Ret = false;
  void* Args_[] = {&a};
  CU.getFunction("foo").call(&Ret, &Args_[0]);
  if (Ret != true) {
    std::cerr << "Invalid bool!" << std::endl;
    return 1;
  }

  a = 0;
  CU.getFunction("foo").call(&Ret, &Args_[0]);
  if (Ret != false) {
    std::cerr << "Invalid bool!" << std::endl;
    return 1;
  }

  bool bA = true;
  Args_[0] = &bA;
  CU.getFunction("invert").call(&Ret, &Args_[0]);
  if (Ret != false) {
    std::cerr << "Error on invert!" << std::endl;
    return 1;
  }

  return 0;
}
