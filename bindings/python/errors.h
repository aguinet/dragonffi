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

#ifndef PYDFFI_ERRORS_H
#define PYDFFI_ERRORS_H

#include <sstream>

namespace dffi {
class Type;
}

struct DFFIError
{
  virtual ~DFFIError() { }
  virtual const char* what() const = 0;
};

struct DFFIErrorStr: public DFFIError
{
  DFFIErrorStr();
  DFFIErrorStr(std::string Err);
  const char* what() const override; 

protected:
  std::string Err_;
};

struct CompileError: public DFFIErrorStr
{
  using DFFIErrorStr::DFFIErrorStr;
};

struct TypeError: public DFFIErrorStr
{
  TypeError(dffi::Type const* Got, dffi::Type const* Expected);
  TypeError(std::string Err):
    DFFIErrorStr(std::move(Err))
  { }
};

struct DLOpenError: public DFFIErrorStr
{
  using DFFIErrorStr::DFFIErrorStr;
};

struct UnknownFunctionError: public DFFIErrorStr
{
  UnknownFunctionError(const char* Name);
};

struct AllocError: public DFFIErrorStr
{
  using DFFIErrorStr::DFFIErrorStr;
};

struct UnknownField: public DFFIErrorStr
{
  using DFFIErrorStr::DFFIErrorStr;
};

template <class T>
struct ThrowError
{
  ~ThrowError() noexcept(false)
  {
    throw T{ss.str()};
  }

  template <class U>
  ThrowError& operator<<(U&& o) {
    ss << std::forward<U>(o);
    return *this;
  }

private:
  std::stringstream ss;
};

#endif
