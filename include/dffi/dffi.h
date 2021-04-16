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

#ifndef DFFI_DFFI_H
#define DFFI_DFFI_H

#include <vector>
#include <string>
#include <map>
#include <memory>

#include <dffi/cc.h>
#include <dffi/exports.h>
#include <dffi/types.h> // for BasicType::BasicKind
#include <dffi/native_func.h>

namespace dffi {

class Type;
class BasicType;
class PointerType;
class ArrayType;
class StructType;
class UnionType;
class EnumType;
class FunctionType;

namespace details {
struct DFFIImpl;
struct CUImpl;
} // details

enum CXXMode: uint8_t {
  NoCXX,
  // Warning: order from here is important!
  Std98,
  Std11,
  Std14,
  Std17,
  Std20,
};

struct CCOpts
{
  unsigned OptLevel;
  std::vector<std::string> IncludeDirs;
  std::string Sysroot;

  CXXMode CXX = CXXMode::NoCXX;
  bool GNUExtensions = true;

  bool LazyJITWrappers = true;

  bool hasCXX() const { return CXX != CXXMode::NoCXX; }

  std::string getSysroot() const;
};

class DFFI;

struct Exception
{
  virtual const char* msg() const = 0;
};

class DFFI_API CompilationUnit
{
  friend class DFFI;

protected:
  CompilationUnit();
  CompilationUnit(details::CUImpl* Impl);

public:
  CompilationUnit(CompilationUnit const&);
  CompilationUnit& operator=(CompilationUnit const&);

  bool isValid() const; 
  operator bool() const { return isValid(); }

  StructType const* getStructType(const char* Name);
  UnionType const* getUnionType(const char* Name);
  EnumType const* getEnumType(const char* Name);
  BasicType const* getBasicType(BasicType::BasicKind K);
  PointerType const* getPointerType(Type const* Ty);
  Type const* getType(const char* Name);

  std::tuple<void*, FunctionType const*> getFunctionAddressAndTy(const char* Name);

  NativeFunc getFunction(const char* Name);
  NativeFunc getFunction(void* FPtr, FunctionType const* FTy);
  NativeFunc getFunction(const char* Name, Type const** VarArgsTys, size_t VarArgsCount);
  NativeFunc getFunction(void* FPtr, FunctionType const* FTy, Type const** VarArgsTys, size_t VarArgsCount);

  std::vector<std::string> getTypes() const;
  std::vector<std::string> getFunctions() const;

private:
  // Owned by DFFIImpl
  details::CUImpl* Impl_;
};

// Based on LLVM's DynamicLibrary
struct DFFI_API DynamicLibrary
{
  DynamicLibrary(void* Data = &Invalid_):
    Data_(Data)
  { }

  bool valid() const {
    return Data_ != &Invalid_;
  }

  void* handle() const { return Data_; }

  void* baseAddress() const;

private:
  // Placeholder whose address represents an invalid library.
  // We use this instead of NULL or a pointer-int pair because the OS library
  // might define 0 or 1 to be "special" handles, such as "search all".
  static char Invalid_;

  // Opaque data used to interface with OS-specific dynamic library handling.
  void *Data_;
};

class DFFI_API DFFI
{
public:
  DFFI(CCOpts const& Opts);
  ~DFFI();

  static void initialize();

  CompilationUnit compile(const char* Code, std::string& Err, bool UseLastError = false);
  CompilationUnit cdef(const char* Code, const char* CUName, std::string& Err, bool UseLastError = false);

  BasicType const* getBasicType(BasicType::BasicKind K);
  template <class T>
  BasicType const* getBasicType()
  {
    return getBasicType(BasicType::getKind<T>());
  }
  PointerType const* getPointerType(Type const* Ty);
  ArrayType const* getArrayType(Type const* Ty, uint64_t NElements);

  NativeFunc getFunction(FunctionType const* FTy, void* FPtr);
  NativeFunc getFunction(FunctionType const* FTy, Type const** VarArgsTys, size_t VarArgsCount, void* FPtr);

  static DynamicLibrary dlopen(const char* Path, std::string* Err = nullptr);
  static void addSymbol(const char* Name, void* Ptr);
  static std::string getNativeTriple();

  // Easy type access
  BasicType const* getVoidTy();
  BasicType const* getBoolTy();
  BasicType const* getCharTy();
  BasicType const* getSCharTy();
  BasicType const* getShortTy();
  BasicType const* getIntTy();
  BasicType const* getLongTy();
  BasicType const* getLongLongTy();
#ifdef DFFI_SUPPORT_I128
  BasicType const* getInt128Ty();
#endif
  BasicType const* getUCharTy();
  BasicType const* getUShortTy();
  BasicType const* getUIntTy();
  BasicType const* getULongTy();
  BasicType const* getULongLongTy();
#ifdef DFFI_SUPPORT_I128
  BasicType const* getUInt128Ty();
#endif
  BasicType const* getFloatTy();
  BasicType const* getDoubleTy();
  BasicType const* getLongDoubleTy();

  PointerType const* getVoidPtrTy();
  PointerType const* getBoolPtrTy();
  PointerType const* getCharPtrTy();
  PointerType const* getSCharPtrTy();
  PointerType const* getShortPtrTy();
  PointerType const* getIntPtrTy();
  PointerType const* getLongPtrTy();
  PointerType const* getLongLongPtrTy();
  PointerType const* getInt128PtrTy();
  PointerType const* getUCharPtrTy();
  PointerType const* getUShortPtrTy();
  PointerType const* getUIntPtrTy();
  PointerType const* getULongPtrTy();
  PointerType const* getULongLongPtrTy();
  PointerType const* getUInt128PtrTy();
  PointerType const* getFloatPtrTy();
  PointerType const* getDoublePtrTy();
  PointerType const* getLongDoublePtrTy();

  BasicType const* getInt8Ty();
  BasicType const* getInt16Ty();
  BasicType const* getInt32Ty();
  BasicType const* getInt64Ty();
  BasicType const* getUInt8Ty();
  BasicType const* getUInt16Ty();
  BasicType const* getUInt32Ty();
  BasicType const* getUInt64Ty();
  PointerType const* getInt8PtrTy();
  PointerType const* getInt16PtrTy();
  PointerType const* getInt32PtrTy();
  PointerType const* getInt64PtrTy();
  PointerType const* getUInt8PtrTy();
  PointerType const* getUInt16PtrTy();
  PointerType const* getUInt32PtrTy();
  PointerType const* getUInt64PtrTy();

private:
  std::unique_ptr<details::DFFIImpl> Impl_;
};

[[noreturn]] DFFI_API void unreachable(const char* msg);

} // dffi

#endif // DFFI_DFFI_H
