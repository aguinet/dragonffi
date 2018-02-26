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

#include <iostream>
#include <cstdlib>

#include <dffi/config.h>
#include <dffi/dffi.h>
#include <dffi/composite_type.h>
#include <dffi/casting.h>
#include "dffi_impl.h"

#include <llvm/Support/TargetSelect.h>

using namespace llvm;

namespace dffi {

DFFI::DFFI(CCOpts const& Opts):
  Impl_(new details::DFFIImpl{Opts})
{ }

DFFI::~DFFI()
{
  // AG: this empty destructor is necessary, so that the destructor of
  // std::unique_ptr is implemented here, and not inline in the class
  // definition, which would need the definition of details::DFFIImpl in the
  // header file!
}

CompilationUnit DFFI::compile(const char* Code, std::string& Err)
{
  return CompilationUnit{Impl_->compile(Code, llvm::StringRef{}, false, Err)};
}

CompilationUnit DFFI::cdef(const char* Code, const char* CUName, std::string& Err)
{
  return CompilationUnit{Impl_->compile(Code, CUName ? CUName : llvm::StringRef{}, true, Err)};
}

BasicType const* DFFI::getBasicType(BasicType::BasicKind K)
{
  return Impl_->getBasicType(K);
}

PointerType const* DFFI::getPointerType(Type const* Ty)
{
  return Impl_->getPointerType(Ty);
}

ArrayType const* DFFI::getArrayType(Type const* Ty, uint64_t NElements)
{
  return Impl_->getArrayType(Ty, NElements);
}

NativeFunc DFFI::getFunction(FunctionType const* FTy, void* FPtr)
{
  return Impl_->getFunction(FTy, FPtr);
}

NativeFunc DFFI::getFunction(FunctionType const* FTy, Type const** VarArgsTys, size_t VarArgsCount, void* FPtr)
{
  return Impl_->getFunction(FTy, ArrayRef<Type const*>{VarArgsTys, VarArgsCount}, FPtr);
}

void DFFI::initialize()
{
  llvm::InitializeNativeTarget();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
}

bool DFFI::dlopen(const char* Path, std::string* Err)
{
  return !llvm::sys::DynamicLibrary::LoadLibraryPermanently(Path, Err);
}

BasicType const* DFFI::getVoidTy()
{
  return nullptr;
}
BasicType const* DFFI::getBoolTy()
{
  return getBasicType(BasicType::Bool);
}
BasicType const* DFFI::getCharTy()
{
  return getBasicType(BasicType::Char);
}
BasicType const* DFFI::getInt8Ty()
{
  return getBasicType(BasicType::Int8);
}
BasicType const* DFFI::getInt16Ty()
{
  return getBasicType(BasicType::Int16);
}
BasicType const* DFFI::getInt32Ty()
{
  return getBasicType(BasicType::Int32);
}
BasicType const* DFFI::getInt64Ty()
{
  return getBasicType(BasicType::Int64);
}
#ifdef DFFI_SUPPORT_I128
BasicType const* DFFI::getInt128Ty()
{
  return getBasicType(BasicType::Int128);
}
#endif
BasicType const* DFFI::getUInt8Ty()
{
  return getBasicType(BasicType::UInt8);
}
BasicType const* DFFI::getUInt16Ty()
{
  return getBasicType(BasicType::UInt16);
}
BasicType const* DFFI::getUInt32Ty()
{
  return getBasicType(BasicType::UInt32);
}
BasicType const* DFFI::getUInt64Ty()
{
  return getBasicType(BasicType::UInt64);
}
#ifdef DFFI_SUPPORT_I128
BasicType const* DFFI::getUInt128Ty()
{
  return getBasicType(BasicType::UInt128);
}
#endif
BasicType const* DFFI::getFloatTy()
{
  return getBasicType(BasicType::Float);
}
BasicType const* DFFI::getDoubleTy()
{
  return getBasicType(BasicType::Double);
}
BasicType const* DFFI::getLongDoubleTy()
{
  return getBasicType(BasicType::LongDouble);
}
PointerType const* DFFI::getVoidPtrTy()
{
  return getPointerType(getVoidTy());
}
PointerType const* DFFI::getBoolPtrTy()
{
  return getPointerType(getBoolTy());
}
PointerType const* DFFI::getCharPtrTy()
{
  return getPointerType(getCharTy());
}
PointerType const* DFFI::getInt8PtrTy()
{
  return getPointerType(getInt8Ty());
}
PointerType const* DFFI::getInt16PtrTy()
{
  return getPointerType(getInt16Ty());
}
PointerType const* DFFI::getInt32PtrTy()
{
  return getPointerType(getInt32Ty());
}
PointerType const* DFFI::getInt64PtrTy()
{
  return getPointerType(getInt64Ty());
}
#ifdef DFFI_SUPPORT_I128
PointerType const* DFFI::getInt128PtrTy()
{
  return getPointerType(getInt128Ty());
}
#endif
PointerType const* DFFI::getUInt8PtrTy()
{
  return getPointerType(getUInt8Ty());
}
PointerType const* DFFI::getUInt16PtrTy()
{
  return getPointerType(getUInt16Ty());
}
PointerType const* DFFI::getUInt32PtrTy()
{
  return getPointerType(getUInt32Ty());
}
PointerType const* DFFI::getUInt64PtrTy()
{
  return getPointerType(getUInt64Ty());
}
#ifdef DFFI_SUPPORT_I128
PointerType const* DFFI::getUInt128PtrTy()
{
  return getPointerType(getUInt128Ty());
}
#endif
PointerType const* DFFI::getFloatPtrTy()
{
  return getPointerType(getFloatTy());
}
PointerType const* DFFI::getDoublePtrTy()
{
  return getPointerType(getDoubleTy());
}
PointerType const* DFFI::getLongDoublePtrTy()
{
  return getPointerType(getLongDoubleTy());
}

// Compilation unit
//

CompilationUnit::CompilationUnit():
  Impl_(nullptr)
{ }

CompilationUnit::CompilationUnit(details::CUImpl* Impl):
  Impl_(Impl)
{ }

CompilationUnit::CompilationUnit(CompilationUnit const&) = default;
CompilationUnit& CompilationUnit::operator=(CompilationUnit const&) = default;

std::vector<std::string> CompilationUnit::getTypes() const
{
  return Impl_->getTypes();
}

std::vector<std::string> CompilationUnit::getFunctions() const
{
  return Impl_->getFunctions();
}

StructType const* CompilationUnit::getStructType(const char* Name)
{
  assert(isValid());
  return Impl_->getStructType(Name);
}

EnumType const* CompilationUnit::getEnumType(const char* Name)
{
  assert(isValid());
  return Impl_->getEnumType(Name);
}

UnionType const* CompilationUnit::getUnionType(const char* Name)
{
  assert(isValid());
  return Impl_->getUnionType(Name);
}

BasicType const* CompilationUnit::getBasicType(BasicType::BasicKind K)
{
  assert(isValid());
  return Impl_->getBasicType(K);
}

PointerType const* CompilationUnit::getPointerType(Type const* Ty)
{
  assert(isValid());
  return Impl_->getPointerType(Ty);
}

Type const* CompilationUnit::getType(const char* Name)
{
  return Impl_->getType(Name);
}

std::tuple<void*, FunctionType const*> CompilationUnit::getFunctionAddressAndTy(const char* Name)
{
  assert(isValid());
  return Impl_->getFunctionAddressAndTy(Name);
}

NativeFunc CompilationUnit::getFunction(const char* Name)
{
  assert(isValid());
  return Impl_->getFunction(Name);
}

NativeFunc CompilationUnit::getFunction(void* FPtr, FunctionType const* FTy)
{
  assert(isValid());
  return Impl_->getFunction(FPtr, FTy);
}

NativeFunc CompilationUnit::getFunction(const char* Name, Type const** VarArgsTys, size_t VarArgsCount)
{
  assert(isValid());
  return Impl_->getFunction(Name, ArrayRef<Type const*>{VarArgsTys, VarArgsCount});
}

NativeFunc CompilationUnit::getFunction(void* FPtr, FunctionType const* FTy, Type const** VarArgsTys, size_t VarArgsCount)
{
  assert(isValid());
  return Impl_->getFunction(FPtr, FTy, ArrayRef<Type const*>{VarArgsTys, VarArgsCount});
}

bool CompilationUnit::isValid() const
{
  return (bool)Impl_;
}

// NativeFunc
//

NativeFunc::NativeFunc():
  TrampFuncPtr_(nullptr),
  FuncCodePtr_(nullptr),
  FTy_(nullptr)
{ }

dffi::Type const* NativeFunc::getReturnType() const
{ return getType()->getReturnType(); }

NativeFunc::NativeFunc(TrampPtrTy Ptr, void* CodePtr, dffi::FunctionType const* FTy):
  TrampFuncPtr_(Ptr),
  FuncCodePtr_(CodePtr),
  FTy_(FTy)
{
  assert(!((Ptr == nullptr) ^ (FTy == nullptr)) && "function wrapper pointer without function type (or the other way around)!");
}

void NativeFunc::call(void* Ret, void** Args) const
{
  TrampFuncPtr_(FuncCodePtr_, Ret, Args);
}

void NativeFunc::call(void** Args) const
{
  TrampFuncPtr_(FuncCodePtr_, nullptr, Args);
}

void NativeFunc::call() const
{
  TrampFuncPtr_(FuncCodePtr_, nullptr, nullptr);
}

NativeFunc::operator bool() const
{
  return TrampFuncPtr_ != nullptr;
}

const char* CCToClangAttribute(CallingConv CC)
{
  switch (CC) {
    case CC_C:           
      return " __attribute__((cdecl))";
    case CC_X86StdCall:  
      return " __attribute__((stdcall))";
    case CC_X86FastCall: 
      return " __attribute__((fastcall))";
    case CC_X86ThisCall: 
      return " __attribute__((thiscall))";
    case CC_X86VectorCall: 
      return " __attribute__((vectorcall))";
    case CC_X86Pascal:   
      return " __attribute__((pascal))";
    case CC_Win64:       
      return " __attribute__((ms_abi))";
    case CC_X86_64SysV:  
      return " __attribute__((sysv_abi))";
    case CC_X86RegCall: 
      return " __attribute__((regcall))";
    case CC_AAPCS:       
      return " __attribute__((pcs(\"aapcs\")))";
    case CC_AAPCS_VFP:   
      return " __attribute__((pcs(\"aapcs-vfp\")))";
    case CC_IntelOclBicc: 
      return " __attribute__((intel_ocl_bicc))";
    case CC_SpirFunction: 
    case CC_OpenCLKernel: 
      // TODO: we should just not support these?
      return "";
    case CC_Swift:        
      return " __attribute__((swiftcall))";
    case CC_PreserveMost: 
      return " __attribute__((preserve_most))";
    case CC_PreserveAll:  
      return " __attribute__((preserve_all))";
  };
  return "";
}

void unreachable(const char* msg)
{
  std::cerr << "Fatal error: DFFI unreachable: " << msg << std::endl;
  std::exit(1);
}

} // dffi
