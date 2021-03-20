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

#ifndef DFFI_NATIVE_FUNC_H
#define DFFI_NATIVE_FUNC_H

#include <dffi/exports.h>
#ifdef _WIN32
#include <IntSafe.h> // For DWORD
#endif

namespace dffi {

class FunctionType;
class Type;

namespace details {
struct DFFIImpl;
} // details

struct DFFI_API NativeFunc
{
  typedef void(*TrampPtrTy)(void*, void*, void**);

#ifdef _WIN32
  using LastErrorTy = DWORD;
#else
  using LastErrorTy = int;
#endif

  NativeFunc();

  NativeFunc(NativeFunc&&) = default;
  NativeFunc(NativeFunc const&) = default;

  void call(void* Ret, void** Args) const;
  void call(void** Args) const;
  void call() const;

  TrampPtrTy getTrampPtr() const { return TrampFuncPtr_; }
  void* getFuncCodePtr() const { return FuncCodePtr_; }
  // TODO!
  //size_t getFuncCodeSize() const;

  operator bool() const;

  dffi::FunctionType const* getType() const { return FTy_; }
  dffi::Type const* getReturnType() const; 

  static LastErrorTy getLastError();
  static void setLastError(LastErrorTy Err);

protected:
  friend struct details::DFFIImpl;

  NativeFunc(TrampPtrTy Ptr, void* CodePtr, dffi::FunctionType const* FTy);

private:
  static void swapLastError();

  TrampPtrTy TrampFuncPtr_;
  void* FuncCodePtr_;
  dffi::FunctionType const* FTy_;
};

} // dffi

#endif
