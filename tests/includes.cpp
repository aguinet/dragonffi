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

// RUN: "%build_dir/compile%exeext" "%S/includes"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

int main(int argc, char** argv)
{
  DFFI::initialize();

  {
    CCOpts Opts;
    Opts.OptLevel = 2;
    Opts.IncludeDirs.emplace_back(argv[1]);

    DFFI Jit(Opts);

    std::string Err;
    auto CU = Jit.compile("#include \"add.h\"", Err);
    if (!CU) {
      std::cerr << "Compile error: " << Err << std::endl;
      return 1;
    }
    int a = 2;
    int b = 4;
    int ret;
    void* Args_[] = {&a, &b};
    CU.getFunction("add").call(&ret, &Args_[0]);
    if (ret != a+b) {
      std::cerr << "Invalid sum!" << std::endl;
      return 1;
    }
  }

  {
    CCOpts Opts;
    Opts.OptLevel = 2;
    DFFI Jit(Opts);

    std::string Err;
    auto CU = Jit.compile("#include \"add.h\"", Err);
    if (CU) {
      std::cerr << "This CU shouldn't compile!" << Err << std::endl;
      return 1;
    }
  }

  return 0;
}
