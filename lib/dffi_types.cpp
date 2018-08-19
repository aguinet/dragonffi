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

#include <dffi/config.h>
#include <dffi/dffi.h>
#include <dffi/composite_type.h>
#include <dffi/casting.h>
#include <dffi/ctypes.h>
#include <dffi/mdarray.h>
#include "dffi_impl.h"

using namespace dffi;

Type::Type(details::DFFIImpl& Dffi, TypeKind K):
  Kind_(K),
  Dffi_(Dffi)
{ }

BasicType::BasicType(details::DFFIImpl& Dffi, BasicKind BKind):
  Type(Dffi, TY_Basic),
  BKind_(BKind)
{ }

unsigned BasicType::getAlign() const
{
  switch (BKind_) {
    case Bool:
      return alignof(c_bool);
    case Char:
      return alignof(c_char);
    case SChar:
      return alignof(c_signed_char);
    case Short:
      return alignof(c_short);
    case Int:
      return alignof(c_int);
    case Long:
      return alignof(c_long);
    case LongLong:
      return alignof(c_long_long);
#ifdef DFFI_SUPPORT_I128
    case Int128:
      return alignof(__int128_t);
#endif
    case UChar:
      return alignof(c_unsigned_char);
    case UShort:
      return alignof(c_unsigned_short);
    case UInt:
      return alignof(c_unsigned_int);
    case ULong:
      return alignof(c_unsigned_long);
    case ULongLong:
      return alignof(c_unsigned_long_long);
#ifdef DFFI_SUPPORT_I128
    case UInt128:
      return alignof(__uint128_t);
#endif
    case Float:
      return alignof(c_float);
    case Double:
      return alignof(c_double);
    case LongDouble:
      return alignof(c_long_double);
#ifdef DFFI_SUPPORT_COMPLEX
    case ComplexFloat:
      return alignof(c_complex_float);
    case ComplexDouble:
      return alignof(c_complex_double);
    case ComplexLongDouble:
      return alignof(c_complex_long_double);
#endif
  }
  assert(false && "unhandled basic type!");
}

uint64_t BasicType::getSize() const
{
  switch (BKind_) {
    case Bool:
      return sizeof(c_bool);
    case Char:
      return sizeof(c_char);
    case SChar:
      return sizeof(c_signed_char);
    case Short:
      return sizeof(c_short);
    case Int:
      return sizeof(c_int);
    case Long:
      return sizeof(c_long);
    case LongLong:
      return sizeof(c_long_long);
#ifdef DFFI_SUPPORT_I128
    case Int128:
      return 16;
#endif
    case UChar:
      return sizeof(c_unsigned_char);
    case UShort:
      return sizeof(c_unsigned_short);
    case UInt:
      return sizeof(c_unsigned_int);
    case ULong:
      return sizeof(c_unsigned_long);
    case ULongLong:
      return sizeof(c_unsigned_long_long);
#ifdef DFFI_SUPPORT_I128
    case UInt128:
      return 16;
#endif
    case Float:
      return sizeof(c_float);
    case Double:
      return sizeof(c_double);
    case LongDouble:
      return sizeof(c_long_double);
#ifdef DFFI_SUPPORT_COMPLEX
    case ComplexFloat:
      return sizeof(c_complex_float);
    case ComplexDouble:
      return sizeof(c_complex_double);
    case ComplexLongDouble:
      return sizeof(c_complex_long_double);
#endif
  }
  assert(false && "unhandled basic type!");
}

FunctionType::FunctionType(details::DFFIImpl& Dffi, QualType RetTy, ParamsVecTy ParamsTy, CallingConv CC, bool VarArgs):
  Type(Dffi, TY_Function),
  RetTy_(RetTy),
  ParamsTy_(std::move(ParamsTy))
{
  Flags_.D.CC = CC;
  Flags_.D.VarArgs = VarArgs;
}

NativeFunc FunctionType::getFunction(void* Ptr) const
{
  return getDFFI().getFunction(this, Ptr);
}

NativeFunc FunctionType::getFunction(Type const** VarArgsTys, size_t VarArgsCount, void* Ptr) const
{
  return getDFFI().getFunction(this, llvm::ArrayRef<Type const*>{VarArgsTys, VarArgsCount}, Ptr);
}

bool FunctionType::hasVarArgs() const
{
  return Flags_.D.VarArgs;
}

CallingConv FunctionType::getCC() const
{
  return (CallingConv)Flags_.D.CC;
}

PointerType::PointerType(details::DFFIImpl& Dffi, QualType Pointee):
  Type(Dffi, TY_Pointer),
  Pointee_(Pointee)
{ }

PointerType const* PointerType::get(QualType Ty)
{
  return Ty->getDFFI().getPointerType(Ty);
}

BasicType const* EnumType::getBasicType() const
{
  return getDFFI().getBasicType(BasicType::getKind<IntType>());
}

ArrayType::ArrayType(details::DFFIImpl& Dffi, QualType Ty, uint64_t NElements):
  Type(Dffi, TY_Array),
  Ty_(Ty),
  NElements_(NElements)
{ }

CompositeField::CompositeField(const char* Name, Type const* Ty, unsigned Offset):
  Name_(Name),
  Ty_(Ty),
  Offset_(Offset)
{ }

CanOpaqueType::CanOpaqueType(details::DFFIImpl& Dffi, TypeKind Ty):
  Type(Dffi, Ty),
  IsOpaque_(true)
{
  assert(Ty > TY_CanOpaqueType && Ty < TY_CanOpaqueTypeEnd && "invalid mayopaque type");
}

CompositeType::CompositeType(details::DFFIImpl& Dffi, TypeKind Ty):
  CanOpaqueType(Dffi, Ty),
  Size_(0),
  Align_(0)
{
  assert(Ty > TY_Composite && Ty < TY_CompositeEnd && "invalid composite type");
}

CompositeField const* CompositeType::getField(const char* Name) const
{
  auto It = FieldsMap_.find(Name);
  if (It == FieldsMap_.end()) {
    return nullptr;
  }
  return It->second;
}

void CompositeType::setBody(std::vector<CompositeField>&& Fields, uint64_t Size, unsigned Align) {
  assert(isOpaque() && "composite type isn't opaque!");
  setAsDefined();
  Fields_ = std::move(Fields);
  Size_ = Size;
  Align_ = Align;
  for (CompositeField const& F: Fields_) {
    FieldsMap_[F.getName()] = &F;
  }
}

void UnionType::setBody(std::vector<CompositeField>&& Fields, uint64_t Size, unsigned Align) {
  CompositeType::setBody(std::move(Fields), Size, Align);
#ifndef NDEBUG
  for (auto const& F: Fields_) {
    assert(F.getOffset() == 0 && "offset of an union field must be 0!");
  }
#endif
}

void EnumType::setBody(Fields&& Fields) {
  setAsDefined();
  Fields_ = std::move(Fields);
}

MDArray::MDArray() = default;
MDArray::MDArray(MDArray&&) = default;
MDArray::MDArray(MDArray const&) = default;

MDArray::MDArray(ShapeTy Shape, QualType BTy):
  Shape_(Shape),
  BTy_(BTy)
{ }

MDArray MDArray::fromArrayTy(ArrayType const& ATy)
{
  QualType EltTy = ATy.getElementType();
  ShapeTy Shape;
  Shape.push_back(ATy.getNumElements());
  while (auto* ArEltTy = dyn_cast<ArrayType>(EltTy.getType())) {
    Shape.push_back(ArEltTy->getNumElements());
    EltTy = ArEltTy->getElementType();
  }
  return MDArray{std::move(Shape), EltTy};
}
