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

#ifndef DFFI_TYPES_H
#define DFFI_TYPES_H

// RTTI based on LLVM RTTI https://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html

#include <vector>
#include <cstdint>
#include <cassert>

#include <dffi/casting.h>
#include <dffi/config.h>
#include <dffi/cc.h>
#include <dffi/exports.h>
#include <dffi/native_func.h>

namespace dffi {

namespace details {
struct DFFICtx;
struct DFFIImpl;
}

class DFFI_API Type
{
public:
  enum TypeKind : uint8_t {
    TY_Basic,
    TY_Pointer,
    TY_Function,
    TY_Array,
    TY_CanOpaqueType,
    TY_Composite,
    TY_Struct,
    TY_Union,
    TY_CompositeEnd,
    TY_Enum,
    TY_CanOpaqueTypeEnd,
  };

protected:
  Type(details::DFFIImpl& Dffi, TypeKind K);

  const TypeKind Kind_;
  details::DFFIImpl& Dffi_;

public:
  TypeKind getKind() const { return Kind_; }
  virtual unsigned getAlign() const { return 1; }
  virtual uint64_t getSize() const { return 1; }

  details::DFFIImpl& getDFFI() const { return Dffi_; }
};

class DFFI_API BasicType: public Type
{
  friend struct details::DFFICtx;

public:
  enum BasicKind: uint8_t {
    Bool,
    Char,
    Int8,
    Int16,
    Int32,
    Int64,
#ifdef DFFI_SUPPORT_I128
    Int128,
#endif
    UInt8,
    UInt16,
    UInt32,
    UInt64,
#ifdef DFFI_SUPPORT_I128
    UInt128,
#endif
    Float,
    Double,
    LongDouble,
#ifdef DFFI_SUPPORT_COMPLEX
    ComplexFloat,
    ComplexDouble,
    ComplexLongDouble
#endif
  };

  BasicKind getBasicKind() const { return BKind_; }

  static bool classof(dffi::Type const* T) {
    return T->getKind() == TY_Basic;
  }

  unsigned getAlign() const override;
  uint64_t getSize() const override;

  template <class T>
  static constexpr BasicKind getKind() { return getKindDefault<T>(); }

protected:
  BasicType(details::DFFIImpl& Dffi, BasicKind BKind);

private:

  template <class T>
  static constexpr BasicKind getKindDefault();

  BasicKind BKind_;
};

#define BASICTY_GETKIND(Ty, K)\
  template <>\
  constexpr BasicType::BasicKind BasicType::getKind<Ty>() { return BasicType::K; }
BASICTY_GETKIND(bool, Bool)
BASICTY_GETKIND(char, Char)
BASICTY_GETKIND(int8_t, Int8)
BASICTY_GETKIND(int16_t, Int16)
BASICTY_GETKIND(int32_t, Int32)
BASICTY_GETKIND(int64_t, Int64)
#ifdef DFFI_SUPPORT_I128
BASICTY_GETKIND(__int128_t, Int128)
#endif
BASICTY_GETKIND(uint8_t, UInt8)
BASICTY_GETKIND(uint16_t, UInt16)
BASICTY_GETKIND(uint32_t, UInt32)
BASICTY_GETKIND(uint64_t, UInt64)
#ifdef DFFI_SUPPORT_I128
BASICTY_GETKIND(__uint128_t, UInt128)
#endif
BASICTY_GETKIND(float, Float)
BASICTY_GETKIND(double, Double)
BASICTY_GETKIND(long double, LongDouble)
#ifdef DFFI_SUPPORT_COMPLEX
BASICTY_GETKIND(_Complex float, ComplexFloat)
BASICTY_GETKIND(_Complex double, ComplexDouble)
BASICTY_GETKIND(_Complex long double, ComplexLongDouble)
#endif

namespace details {
template <bool, size_t, bool>
struct KindDefault;

#define BASICTY_GETKIND_DEFAULT(Size, Signed, K)\
  template <>\
  struct KindDefault<true, Size, Signed> {\
    static constexpr BasicType::BasicKind Kind = K;\
  };

BASICTY_GETKIND_DEFAULT(1, false, BasicType::Int8)
BASICTY_GETKIND_DEFAULT(1, true, BasicType::UInt8)
BASICTY_GETKIND_DEFAULT(2, false, BasicType::Int16)
BASICTY_GETKIND_DEFAULT(2, true, BasicType::UInt16)
BASICTY_GETKIND_DEFAULT(4, false, BasicType::Int32)
BASICTY_GETKIND_DEFAULT(4, true, BasicType::UInt32)
BASICTY_GETKIND_DEFAULT(8, false, BasicType::Int64)
BASICTY_GETKIND_DEFAULT(8, true, BasicType::UInt64)

} // details

template <class T>
constexpr BasicType::BasicKind BasicType::getKindDefault() {
  return details::KindDefault<std::is_integral<T>::value, sizeof(T), std::is_signed<T>::value>::Kind;
}
    
// The concept of this qualified type is mapped on the clang::QualType one. The
// overall idea is that this object is basically a pointer whose lowest bits
// are used to carry information on the qualifiers of the type.
// This allows a zero-memory overhead for this information storage, while
// preserving the uniqueness of the underlying types provied by the DFFI and CU
// objects.
// For now, the only qualifier we support is const. This is used for instance
// by the python bindings, to know if it can map read-only memoryview to a
// pointer (which thus need to be const). In the same way, CPointerObj which
// points to a const-type will return a read-only memoryview!
class DFFI_API QualType
{
public:
  enum Qualifiers : uintptr_t {
    None = 0,
    Const = 1
  };

  QualType(Type const* Ty)
  {
    Ty_ = pointerWithQualifiers(Ty, None);
  }


  QualType(Type const* Ty, Qualifiers Q)
  {
    Ty_ = pointerWithQualifiers(Ty, Q);
  }

  QualType(QualType const&) = default;
  QualType(QualType&&) = default;

  QualType& operator=(QualType const&) = default;
  QualType& operator=(QualType&&) = default;

  Type const* operator->() const { return getType(); }
  operator Type const*() const { return getType(); }

  bool hasConst() const { return getQualifiers() & Const; }
  Qualifiers getQualifiers() const { return (Qualifiers)(Ty_ & QMask); }

  QualType withConst() const {
    return QualType(Ty_ | Const);
  }

  Type const* getType() const { return (Type const*) (Ty_ & (~QMask)); }

  bool operator<(QualType const& O) const { return Ty_ < O.Ty_; }
  bool operator>(QualType const& O) const { return Ty_ > O.Ty_; }
  bool operator==(QualType const& O) const { return Ty_ == O.Ty_; }

  uintptr_t getRawValue() const { return Ty_; }
private:
  static constexpr uintptr_t QMask = 1;

  QualType(uintptr_t Ty):
    Ty_(Ty)
  { }

  static uintptr_t pointerWithQualifiers(Type const* Ty, Qualifiers Q)
  {
    assert(((uintptr_t)Ty & QMask) == 0 && "type isn't aligned on the necessary amount of bytes!");
    return ((uintptr_t)Ty | Q);
  }

  uintptr_t Ty_;
};

template <class T>
T const* cast(dffi::QualType Ty)
{
  return cast<T>(Ty.getType());
}

class DFFI_API PointerType: public Type
{
  friend struct details::DFFICtx;

public:
  static bool classof(Type const* T) {
    return T->getKind() == TY_Pointer;
  }

  QualType getPointee() const { return Pointee_; }

  unsigned getAlign() const override { return sizeof(void*); }
  uint64_t getSize() const override { return sizeof(void*); }

  static PointerType const* get(QualType Ty);

private:
  PointerType(details::DFFIImpl& Dffi, QualType Pointee);

  QualType Pointee_;
};

class DFFI_API FunctionType: public Type
{
  friend struct details::DFFICtx;

public:
  typedef std::vector<QualType> ParamsVecTy;

  static bool classof(Type const* T) {
    return T->getKind() == TY_Function;
  }

  Type const* getReturnType() const { return RetTy_; }
  ParamsVecTy const& getParams() const { return ParamsTy_; }

  bool hasVarArgs() const; 
  CallingConv getCC() const; 

  NativeFunc getFunction(void* Ptr) const;
  NativeFunc getFunction(Type const** VarArgsTys, size_t VarArgsCount, void* Ptr) const;

protected:
  FunctionType(details::DFFIImpl& Dffi, QualType RetTy, ParamsVecTy ParamsTy, CallingConv CC, bool VarArgs);

private:
  QualType RetTy_;
  ParamsVecTy ParamsTy_;
  union {
    struct {
      uint8_t CC: 7;
      uint8_t VarArgs: 1;
    } D;
    uint8_t V;
  } Flags_;
};

class DFFI_API ArrayType: public Type
{
  friend struct details::DFFICtx;

public:
  static bool classof(Type const* T) {
    return T->getKind() == TY_Array;
  }

  Type const* getElementType() const { return Ty_; }
  uint64_t getNumElements() const { return NElements_; }
  uint64_t getSize() const override { return Ty_->getSize()*NElements_; }

protected:
  ArrayType(details::DFFIImpl& Dffi, QualType Ty, uint64_t NElements);

  QualType Ty_;
  uint64_t NElements_;
};

} // dffi

namespace std {

template <>
struct hash<dffi::QualType>
{
  typedef dffi::QualType argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const& QTy) const noexcept
  {
    uintptr_t PtrVal = QTy.getRawValue();
    return (PtrVal >> 4) ^ (PtrVal >> 9);
  }
};

} // std

#endif
