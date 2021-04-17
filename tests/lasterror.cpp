// Copyright 2021 Adrien Guinet <adrien@guinet.me>
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

// RUN: "%build_dir/lasterror%exeext"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

static bool test(DFFI& FFI, bool UseLastError)
{
  // Call open on a non existant file and check we have ENOENT as last error.
  std::string Err;
  auto CU = FFI.compile(
#ifdef _WIN32
      R"(
#include <windows.h>
void seterrno(int val) {
  SetLastError(val);
}
)"
#else
      R"(
#include <errno.h>
void seterrno(int val) {
  errno = val;
}
)"
#endif
    , Err, UseLastError); 
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  NativeFunc::setLastError(0);
  int val = 10;
  void* Args[] = {&val};
  CU.getFunction("seterrno").call(Args);
  // Simulate errno being modified
  errno = 0;
  const int lastError = NativeFunc::getLastError();
  return lastError == 10;
}

int main(int argc, char** argv)
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI FFI(Opts);

  bool Ret = true;
  Ret &= test(FFI, true);
  Ret &= !test(FFI, false);
  return Ret?0:1;
}
