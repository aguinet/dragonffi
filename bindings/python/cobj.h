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

#ifndef PYDFFI_COBJ_H
#define PYDFFI_COBJ_H

#ifdef _WIN32
#include <malloc.h>
#endif
#include <memory>

#include <pybind11/pybind11.h>

#include <dffi/compat.h>
#include <dffi/dffi.h>
#include <dffi/types.h>
#include <dffi/composite_type.h>
#include <dffi/casting.h>

#include "errors.h"

// Python C types 
//

using dffi::dyn_cast;

static void* alloc_align(size_t Size, size_t Align)
{
#ifdef _WIN32
  return _aligned_malloc(Size, Align);
#else
  void* Ptr;
  if (posix_memalign(&Ptr, Align, Size) != 0) {
    return nullptr;
  }
  return Ptr;
#endif
}

template <class T>
struct Data;

template <>
struct Data<void>
{
  enum Type: uint8_t {
    OwnedFree = 1,
    View = 2,
  };

  template <class U>
  friend struct Data;

  Data():
    Ptr_(nullptr),
    Ty_(View)
  { }

  Data(Data&& O):
    Ty_(O.Ty_)
  {
    Ptr_ = O.release();
  }

  Data(Data const&) = delete;

  Data& operator=(Data&& O)
  {
    if (&O != this) {
      clear();
      Ty_ = O.Ty_;
      Ptr_ = O.release();
    }
    return *this;
  }

  void* release() {
    void* Ret = Ptr_;
    Ty_ = View;
    Ptr_ = nullptr;
    return Ret;
  }

  static Data view(void* Ptr)
  {
    return {Ptr, View};
  }

  static Data owned_free(void* Ptr)
  {
    return {Ptr, OwnedFree};
  }

  ~Data()
  {
    clear();
  }

  void clear()
  {
    switch (Ty_) {
      case OwnedFree:
#ifdef _MSC_VER
        _aligned_free(Ptr_);
#else
        free(Ptr_);
#endif
        break;
      case View:
        break;
    }
  }

  void* dataPtr() { return Ptr_; }
  void const* dataPtr() const { return Ptr_; }

  template <class T>
  operator Data<T>() {
    Data<T> Ret(std::move(*this));
    return Ret;
  }

private:
  Data(void* Ptr, Type Ty):
    Ptr_(Ptr),
    Ty_(Ty)
  { }

  void* Ptr_;
  Type Ty_;
};

template <class T>
struct Data
{
  enum Type: uint8_t {
    Owned = 0,
    OwnedFree = 1,
    View = 2,
  };


  Data():
    Ptr_(nullptr),
    Ty_(View)
  { }

  Data(Data&& O):
    Ty_(O.Ty_)
  {
    if (Ty_ == Owned) {
      new (ptrValue()) T{std::move(*O.ptrValue())};
      Ptr_ = ptrValue();
    }
    else {
      Ptr_ = O.release();
    }
  }

  Data(Data<void>&& O):
    Ty_((Type)(O.Ty_))
  {
    Ptr_ = (T*) O.release();
  }

  Data(Data const&) = delete;

  Data& operator=(Data&& O)
  {
    if (&O != this) {
      clear();
      Ty_ = O.Ty_;
      Ptr_ = O.release();
    }
    return *this;
  }

  T* release() {
    T* Ret = Ptr_;
    Ty_ = View;
    Ptr_ = nullptr;
    return Ret;
  }

  template <class... Args>
  static Data emplace_owned(Args&& ... args) {
    Data Ret(nullptr, Owned);
    new (Ret.ptrValue()) T{std::forward<Args>(args)...};
    Ret.Ptr_ = Ret.ptrValue();
    return Ret;
  }

  static Data view(T* Ptr)
  {
    return {Ptr, View};
  }


  static Data owned_free(T* Ptr)
  {
    return {Ptr, OwnedFree};
  }

  ~Data()
  {
    clear();
  }

  void clear()
  {
    switch (Ty_) {
      case Owned:
        ptrValue()->~T();
        break;
      case OwnedFree:
#ifdef _MSC_VER
        _aligned_free(Ptr_);
#else
        free(Ptr_);
#endif
        break;
      case View:
        break;
    }
  }

  T* dataPtr() { return Ptr_; }
  T const* dataPtr() const { return Ptr_; }

private:
  Data(T* Ptr, Type Ty):
    Ptr_(Ptr),
    Ty_(Ty)
  { }

  T* ptrValue() { return reinterpret_cast<T*>(&BufValue[0]); }

  T* Ptr_;
  ALIGN(alignof(T)) char BufValue[sizeof(T)];
  Type Ty_;
};

struct CObj
{
  CObj(dffi::QualType Ty):
    Ty_(Ty)
  { }

  virtual ~CObj() { }
  virtual void* dataPtr() = 0;

  inline dffi::QualType getType() const { return Ty_; }

  std::unique_ptr<CObj> cast(dffi::QualType To) const;

  inline size_t getSize() const { return getType()->getSize(); }
  inline size_t getAlign() const { return getType()->getAlign(); }

  inline bool isConst() const { return Ty_.hasConst(); }
  inline void setConst() { Ty_ = Ty_.withConst(); }

  pybind11::buffer_info getBufferInfo();

private:
  virtual std::unique_ptr<CObj> cast_impl(dffi::Type const* To) const = 0;

  dffi::QualType Ty_;
};

namespace {

template <class CBO, class T>
struct CBOOps
{
#define _CAT(a, b) a##b
#define CAT(a, b) _CAT( a, b)

#define OP_NAME(OP, PARAM)\
  CAT(operator, CAT(OP, CAT("(", CAT(PARAM, ")"))))

#define IMPL_BINOP(op, opself)\
  inline CBO operator op(T const V) const {\
    return CBO(*static_cast<CBO const&>(*this).getType(), static_cast<CBO const&>(*this).value() op V);\
  }\
  inline CBO operator op(CBO const& V) const {\
    return static_cast<CBO const&>(*this) op V.value();\
  }\
  inline CBO& operator opself(CBO const& V) {\
    static_cast<CBO&>(*this) opself static_cast<T>(V);\
    return static_cast<CBO&>(*this);\
  }\
  inline CBO& operator opself(T const V) {\
    static_cast<CBO&>(*this).vref() opself V;\
    return static_cast<CBO&>(*this);\
  }

  IMPL_BINOP(+,+=)
  IMPL_BINOP(-,-=)
  IMPL_BINOP(*,*=)
  IMPL_BINOP(/,/=)
  IMPL_BINOP(^,^=)
  IMPL_BINOP(&,&=)
  IMPL_BINOP(|,|=)

#define IMPL_UNOP(op)\
  inline CBO& operator op() {\
    static_cast<CBO&>(*this).vref() = op static_cast<CBO&>(*this).value();\
    return static_cast<CBO&>(*this);\
  }\
  inline CBO operator op() const {\
    return CBO(*static_cast<CBO const&>(*this).getType(), op static_cast<CBO const&>(*this).value());\
  }
  IMPL_UNOP(~)

  // Spaceship FTW...!
#define IMPL_CMP(op)\
  inline bool operator op(T const V) const {\
    return static_cast<CBO const&>(*this).value() op V;\
  }\
  inline bool operator op(CBO const& V) const {\
    return static_cast<CBO const&>(*this) op V.value();\
  }\

  IMPL_CMP(==)
  IMPL_CMP(!=)
  IMPL_CMP(<)
  IMPL_CMP(<=)
  IMPL_CMP(>=)
  IMPL_CMP(>)
};

template <class CBO>
struct CBOOps<CBO, bool>
{
  using T = bool;

  IMPL_BINOP(^,^=)
  IMPL_BINOP(&,&=)
  IMPL_BINOP(|,|=)
  IMPL_CMP(==)
  IMPL_CMP(!=)
  IMPL_CMP(<)
  IMPL_CMP(<=)
  IMPL_CMP(>=)
  IMPL_CMP(>)
};

} // anonymous

template <class T>
struct CBasicObj: public CObj, public CBOOps<CBasicObj<T>, T>
{
  CBasicObj(dffi::QualType Ty, Data<T>&& D):
    CObj(Ty),
    Data_(std::move(D))
  { }

  CBasicObj(dffi::QualType Ty, T const V = T{}):
    CObj(Ty),
    Data_(Data<T>::emplace_owned(V))
  { }

  CBasicObj(CBasicObj&& O):
    CObj(*O.getType()),
    Data_(std::move(O.Data_))
  { }

  ~CBasicObj() override
  { }
  
  void* dataPtr() override { return Data_.dataPtr(); }

  inline T value() const { return vref(); }
  explicit operator T() const { return value(); }

  std::unique_ptr<CObj> cast_impl(dffi::Type const* To) const override;

  inline dffi::BasicType const* getType() const { return dffi::cast<dffi::BasicType>(CObj::getType()); }

protected:
  friend struct CBOOps<CBasicObj<T>, T>;

  inline T& vref() { return *Data_.dataPtr(); }
  inline T const& vref() const { return *Data_.dataPtr(); }

  Data<T> Data_;
};

struct CPointerObj: public CObj
{
  CPointerObj(dffi::QualType Ty, Data<void*>&& D):
    CObj(Ty),
    Data_(std::move(D))
  { }

  CPointerObj(CObj* Pointee):
    CObj(*dffi::PointerType::get(Pointee->getType())),
    Data_(Data<void*>::emplace_owned(Pointee->dataPtr()))
  { }

  CPointerObj(dffi::QualType Ty):
    CObj(Ty),
    Data_(Data<void*>::emplace_owned(nullptr))
  { }

  void* dataPtr() override
  {
    return Data_.dataPtr();
  }

  void* getPtr() const {
    return *Data_.dataPtr();
  }

  inline dffi::QualType getPointeeType() const { return dffi::cast<dffi::PointerType>(getType().getType())->getPointee(); }

  std::unique_ptr<CObj> getObj();

  std::unique_ptr<CObj> cast_impl(dffi::Type const* To) const override;

  pybind11::memoryview getMemoryViewObjects(size_t Len);
  pybind11::memoryview getMemoryViewCStr();

private:

  Data<void*> Data_;
};

struct CArrayObj: public CObj
{
  CArrayObj(dffi::QualType Ty, Data<void>&& D):
    CObj(Ty),
    Data_(std::move(D))
  { }

  CArrayObj(dffi::QualType Ty):
    CObj(Ty)
  {
    size_t Align = std::max(sizeof(void*), (size_t)Ty->getAlign());
    auto Size = Ty->getSize();
    void* Ptr = alloc_align(Size, Align);
    if (!Ptr) {
      throw AllocError{"allocation failure!"};
    }
    Data_ = Data<void>::owned_free(Ptr);
  }

  void* dataPtr() override { return getData(); }
  void* getData() { return Data_.dataPtr(); }
  void const* getData() const { return Data_.dataPtr(); }

  inline dffi::ArrayType const* getArrayType() const { return dffi::cast<dffi::ArrayType>(CObj::getType().getType()); }

  dffi::QualType getElementType() const { return getArrayType()->getElementType(); }
  size_t getNumElements() const { return getArrayType()->getNumElements(); }

  void* GEP(size_t Idx) {
    // TODO: assert alignment
    return reinterpret_cast<uint8_t*>(getData()) + (Idx*getElementType()->getSize());
  }

  void const* GEP(size_t Idx) const {
    // TODO: assert alignment
    return const_cast<const void*>(const_cast<CArrayObj*>(this)->GEP(Idx));
  }

  pybind11::object get(size_t Idx);
  void set(size_t Idx, pybind11::handle Obj);

  std::unique_ptr<CObj> cast_impl(dffi::Type const* To) const override;

  pybind11::buffer_info getBufferInfo();

private:
  Data<void> Data_;
};

struct CCompositeObj: public CObj
{
  CCompositeObj(dffi::QualType Ty, Data<void>&& D):
    CObj(Ty),
    Data_(std::move(D))
  { }

  CCompositeObj(dffi::QualType Ty):
    CObj(Ty)
  {
    assert(!getCompositeType()->isOpaque() && "can't instantiate an opaque structure/union!");
    size_t Align = std::max(sizeof(void*), (size_t)Ty->getAlign());
    void* Ptr = alloc_align(getSize(), Align);
    if (!Ptr) {
      throw AllocError{"allocation failure!"};
    }
    Data_ = Data<void>::owned_free(Ptr);
  }

  void setZero()
  {
    memset(getData(), 0, getSize());
  }

  dffi::CompositeField const& getField(const char* Name)
  {
    auto* Ret = getCompositeType()->getField(Name);
    if (!Ret) {
      ThrowError<UnknownField>() << "unknown field " << Name;
    }
    return *Ret;
  }

  void setValue(dffi::CompositeField const& Field, pybind11::handle Obj);
  void setValue(const char* Field, pybind11::handle Obj)
  {
    setValue(getField(Field), Obj);
  }

  pybind11::object getValue(dffi::CompositeField const& Field);
  pybind11::object getValue(const char* Field)
  {
    return getValue(getField(Field));
  }

  inline dffi::CompositeType const* getCompositeType() const { return dffi::cast<dffi::CompositeType>(getType().getType()); }

  void* getData() { return Data_.dataPtr(); }
  void const* getData() const { return Data_.dataPtr(); }

  void* dataPtr() override { return getData(); }

  std::unique_ptr<CObj> cast_impl(dffi::Type const* To) const override;

private:
  void* getFieldData(dffi::CompositeField const& F)
  {
    return reinterpret_cast<uint8_t*>(getData()) + F.getOffset();
  }

  void const* getFieldData(dffi::CompositeField const& F) const
  {
    return const_cast<const void*>(const_cast<CCompositeObj*>(this)->getFieldData(F));
  }

  Data<void> Data_;
};

struct CStructObj: public CCompositeObj
{
  CStructObj(dffi::QualType Ty, Data<void>&& D):
    CCompositeObj(Ty, std::move(D))
  { }

  CStructObj(dffi::QualType Ty):
    CCompositeObj(Ty)
  { }
};

struct CUnionObj: public CCompositeObj
{
  CUnionObj(dffi::QualType Ty, Data<void>&& D):
    CCompositeObj(Ty, std::move(D))
  { }

  CUnionObj(dffi::QualType Ty):
    CCompositeObj(Ty)
  { }
};

struct CFunction: public CObj
{
  using TrampPtrTy = dffi::NativeFunc::TrampPtrTy;

  CFunction(dffi::NativeFunc const& NF):
    CObj(*NF.getType()),
    NF_(NF)
  { }

  pybind11::object call(pybind11::args const& Args) const;

  inline dffi::FunctionType const* getFuncType() const { return dffi::cast<dffi::FunctionType>(getType().getType()); }
  
  void* dataPtr() override { return NF_.getFuncCodePtr(); }

  std::unique_ptr<CObj> cast_impl(dffi::Type const* To) const override { return {nullptr}; }

  inline uint64_t getTrampPtr() const { return (uint64_t)NF_.getTrampPtr(); }

private:
  dffi::NativeFunc NF_;
};

struct CVarArgsFunction: public CObj
{
  CVarArgsFunction(void* FuncPtr, dffi::FunctionType const* FTy):
    CObj(*FTy),
    FuncPtr_(FuncPtr)
  {
    assert(FTy->hasVarArgs() && "function must have variadic arguments!");
  }

  pybind11::object call(pybind11::args const& Args) const;

  inline dffi::FunctionType const* getFuncType() const { return dffi::cast<dffi::FunctionType>(getType().getType()); }
  
  void* dataPtr() override { return FuncPtr_; }

  std::unique_ptr<CObj> cast_impl(dffi::Type const* To) const override { return {nullptr}; }

private:
  void* FuncPtr_;
};

std::string getFormatDescriptor(dffi::Type const* Ty);
std::string getPortableFormatDescriptor(dffi::Type const* Ty);

namespace {
template <class T, bool isConvertibleToPtr>
struct BasicObjConvertor;

template <class From, class To, bool isConvertible>
struct BasicObjConvertorImpl;

template <class From, class To>
struct BasicObjConvertorImpl<From, To, false>
{
  static std::unique_ptr<CObj> cast(dffi::BasicType const*, From)
  {
    return {};
  }
};

template <class From, class To>
struct BasicObjConvertorImpl<From, To, true>
{
  static std::unique_ptr<CObj> cast(dffi::BasicType const* BTy, From V)
  {
    CObj* Ret = new CBasicObj<To>(*BTy, Data<To>::emplace_owned(static_cast<To>(V)));
    return std::unique_ptr<CObj>{Ret};
  }
};

template <class T>
struct BasicObjConvertor<T, false>
{
  static std::unique_ptr<CObj> cast(CBasicObj<T> const& Obj, dffi::Type const* To)
  {
    if (auto const* BTy = dyn_cast<dffi::BasicType>(To)) {
      auto const V = Obj.value();
#define HANDLE_BASICTY(K, T_)\
      case dffi::BasicType::K:\
        return BasicObjConvertorImpl<T, T_, std::is_convertible<T, T_>::value>::cast(BTy, V);
      switch (BTy->getBasicKind()) {
        HANDLE_BASICTY(Bool, c_bool);
        HANDLE_BASICTY(Char, c_char);
        HANDLE_BASICTY(UChar, c_unsigned_char);
        HANDLE_BASICTY(UShort, c_unsigned_short);
        HANDLE_BASICTY(UInt, c_unsigned_int);
        HANDLE_BASICTY(ULong, c_unsigned_long);
        HANDLE_BASICTY(ULongLong, c_unsigned_long_long);
#ifdef DFFI_SUPPORT_I128
        HANDLE_BASICTY(UInt128, __uint128_t);
#endif
        HANDLE_BASICTY(SChar, c_signed_char);
        HANDLE_BASICTY(Short, c_short);
        HANDLE_BASICTY(Int, c_int);
        HANDLE_BASICTY(Long, c_long);
        HANDLE_BASICTY(LongLong, c_long_long);
#ifdef DFFI_SUPPORT_I128
        HANDLE_BASICTY(Int128, __int128_t);
#endif
        HANDLE_BASICTY(Float, c_float);
        HANDLE_BASICTY(Double, c_double);
        HANDLE_BASICTY(LongDouble, c_long_double);
#ifdef DFFI_SUPPORT_COMPLEX
        HANDLE_BASICTY(ComplexFloat, c_complex_float);
        HANDLE_BASICTY(ComplexDouble, c_complex_double);
        HANDLE_BASICTY(ComplexLongDouble, c_complex_long_double);
#endif
#undef HANDLE_BASICTY
        default:
          break;
      };
    }
    return {};
  }
};

template <class T>
struct BasicObjConvertor<T, true>
{
  static std::unique_ptr<CObj> cast(CBasicObj<T> const& Obj, dffi::Type const* To)
  {
    if (auto Ret = BasicObjConvertor<T, false>::cast(Obj, To)) {
      return Ret;
    }
    if (auto const* PTy = dyn_cast<dffi::PointerType>(To)) {
      return std::unique_ptr<CObj>{new CPointerObj{*PTy, Data<void*>::emplace_owned((void*)(Obj.value()))}};
    }
    return {nullptr};
  }
};

} // namespace

template <class T>
std::unique_ptr<CObj> CBasicObj<T>::cast_impl(dffi::Type const* To) const
{
  return BasicObjConvertor<T, std::is_convertible<T, void*>::value>::cast(*this, To);
}

std::unique_ptr<CObj> createObj(dffi::Type const* Ty, Data<void>&& D);

pybind11::memoryview memoryview_from_buffer_info(pybind11::buffer_info const&);

#endif
