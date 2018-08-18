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

#include <cstdlib>
#include "cobj.h"
#include "dispatcher.h"
#include "errors.h"

#include <dffi/casting.h>
#include <dffi/types.h>
#include <dffi/mdarray.h>

namespace py = pybind11;
using namespace dffi;

namespace {

struct ValueSetter
{
  // TODO: assert alignment is correct!
  template <class T>
  static void case_basic(BasicType const* Ty, void* Ptr, py::handle Obj)
  {
    T* TPtr = reinterpret_cast<T*>(Ptr);
    *TPtr = Obj.cast<T>();
  }

  static void case_pointer(PointerType const* Ty, void* Ptr, py::handle Obj)
  {
    CPointerObj const& PtrObj = Obj.cast<CPointerObj const&>();
    void** PPtr = reinterpret_cast<void**>(Ptr);
    *PPtr = PtrObj.getPtr();
  }

  template <class T>
  static void case_composite(T const* Ty, void* Ptr, py::handle Obj)
  {
    using CObjTy = typename std::conditional<std::is_same<T, StructType>::value, CStructObj, CUnionObj>::type;
    auto const& CObj = Obj.cast<CObjTy const&>();
    memcpy(Ptr, CObj.getData(), CObj.getSize());
  }

  static void case_enum(EnumType const* Ty, void* Ptr, py::handle Obj)
  {
    return case_basic<EnumType::IntType>(Ty->getBasicType(), Ptr, Obj);
  }

  static void case_array(ArrayType const* Ty, void* Ptr, py::handle Obj)
  {
    CArrayObj const& ArrayObj = Obj.cast<CArrayObj const&>();
    memcpy(Ptr, ArrayObj.getData(), Ty->getSize());
  }

  static void case_func(FunctionType const* Ty, void* Ptr, py::handle Obj)
  {
    // This should never happen, as this is prevented by the C standard!
    // Raise an exception!
    throw TypeError{"unable to set a value to a function!"};
  }
};

struct ValueGetter
{
  // TODO: assert alignment is correct!
  template <class T>
  static py::object case_basic(BasicType const* Ty, void* Ptr)
  {
    return py::cast(*reinterpret_cast<T*>(Ptr));
  }

  static py::object case_enum(EnumType const* Ty, void* Ptr)
  {
    return py::cast(*reinterpret_cast<EnumType::IntType*>(Ptr));
  }

  static py::object case_pointer(PointerType const* Ty, void* Ptr)
  {
    auto* Ret = new CPointerObj{*Ty, Data<void*>::view((void**)Ptr)};
    return py::cast(Ret, py::return_value_policy::take_ownership);
  }

  template <class T, class CT>
  static py::object case_composite_impl(CT const* Ty, void* Ptr)
  {
    auto* Ret = new T{*Ty, Data<void>::view(Ptr)};
    return py::cast(Ret, py::return_value_policy::take_ownership);
  }

  static py::object case_composite(StructType const* Ty, void* Ptr)
  {
    return case_composite_impl<CStructObj>(Ty, Ptr);
  }

  static py::object case_composite(UnionType const* Ty, void* Ptr)
  {
    return case_composite_impl<CUnionObj>(Ty, Ptr);
  }

  static py::object case_array(ArrayType const* Ty, void* Ptr)
  {
    auto* Ret = new CArrayObj{*Ty, Data<void>::view(Ptr)};
    return py::cast(Ret, py::return_value_policy::take_ownership);
  }

  static py::object case_func(FunctionType const* Ty, void* Ptr)
  {
    // This should never happen, as this is prevented by the C standard!
    // Raise an exception!
    throw TypeError{"unable to get a value as a function!"};
  }
};

struct PtrToObjView
{
  template <class T>
  static std::unique_ptr<CObj> case_basic(BasicType const* Ty, void* Ptr)
  {
    auto* TPtr = reinterpret_cast<T*>(Ptr);
    return std::unique_ptr<CObj>{new CBasicObj<T>{*Ty, Data<T>::view(TPtr)}};
  }

  static std::unique_ptr<CObj> case_enum(EnumType const* Ty, void* Ptr)
  {
    return case_basic<EnumType::IntType>(Ty->getBasicType(), Ptr);
  }

  static std::unique_ptr<CObj> case_pointer(PointerType const* Ty, void* Ptr)
  {
    return std::unique_ptr<CObj>{new CPointerObj{*Ty, Data<void*>::view((void**)Ptr)}};
  }

  static std::unique_ptr<CObj> case_composite(StructType const* Ty, void* Ptr)
  {
    return std::unique_ptr<CObj>{new CStructObj{*Ty, Data<void>::view(Ptr)}};
  }

  static std::unique_ptr<CObj> case_composite(UnionType const* Ty, void* Ptr)
  {
    return std::unique_ptr<CObj>{new CUnionObj{*Ty, Data<void>::view(Ptr)}};
  }

  static std::unique_ptr<CObj> case_array(ArrayType const* Ty, void* Ptr)
  {
    return std::unique_ptr<CObj>{new CArrayObj{*Ty, Data<void>::view(Ptr)}};
  }

  static std::unique_ptr<CObj> case_func(FunctionType const* Ty, void* Ptr)
  {
    auto NF = Ty->getFunction(Ptr);
    return std::unique_ptr<CObj>{new CFunction{NF}};
  }
};

struct ConvertArgsSwitch
{
  typedef std::vector<std::unique_ptr<CObj>> ObjsHolder;
  typedef std::vector<py::object> PyObjsHolder;

  static CObj* checkType(CObj* O, Type const* Ty)
  {
    /*if (O->getType() != Ty) {
      throw TypeError{O->getType(), Ty};
    }*/
    return O;
  }

  template <class T>
  static CObj* case_basic(BasicType const* Ty, ObjsHolder& H, PyObjsHolder&, py::handle O)
  {
    if (CObj* Ret = O.dyn_cast<CBasicObj<T>>()) {
      return Ret;
    }
    // Create a temporary object with the python value
    auto* Ret = new CBasicObj<T>{*Ty, O.cast<T>()};
    H.emplace_back(std::unique_ptr<CObj>{Ret});
    return Ret;
  }

  static CObj* case_enum(EnumType const* Ty, ObjsHolder& H, PyObjsHolder& PyH, py::handle O)
  {
    return case_basic<EnumType::IntType>(Ty->getBasicType(), H, PyH, O);
  }

  static CObj* case_pointer(PointerType const* Ty, ObjsHolder& H, PyObjsHolder& PyH, py::handle O)
  {
    if (auto* PtrObj = O.dyn_cast<CPointerObj>()) {
      if (!Ty->getPointee().hasConst() && PtrObj->getPointeeType().hasConst()) {
        throw ConstCastError{};
      }
      return PtrObj;
    }

    // Automatic promote arrays to pointers if the underlying type is the same!
    if (auto* ArrObj = O.dyn_cast<CArrayObj>()) {
      QualType PteeTy = Ty->getPointee();
      QualType EltTy = ArrObj->getElementType();
      if (!PteeTy.hasConst() && EltTy.hasConst()) {
        throw ConstCastError{};
      }
      if (PteeTy.getType() == EltTy.getType()) {
        auto* Ret = new CPointerObj{ArrObj};
        H.emplace_back(std::unique_ptr<CObj>{Ret});
        return Ret;
      }
    }

    auto PteTy = Ty->getPointee();
    const bool isWritable = !PteTy.hasConst();
    // If the argument is const char* and we have a py::str, do an automatic conversion using UTF8!
    // TODO: let the user choose if this automatic conversion must happen, and the codec to use!
    if (!isWritable) {
      if (auto* BTy = dyn_cast<BasicType>(PteTy.getType())) {
        if (BTy->getBasicKind() == BasicType::Char) {
          py::handle Tmp = O;
          if (PyUnicode_Check(O.ptr())) {
            py::object Buf = py::reinterpret_steal<py::object>(PyUnicode_AsUTF8String(O.ptr()));
            if (!Buf)
              throw TypeError{"Unable to extract string contents! (encoding issue)"};
            // Keep this object for the call lifetime as we will get its
            // underlying buffer!
            PyH.emplace_back(Buf);
            Tmp = Buf;
          }
          char *Buffer = PYBIND11_BYTES_AS_STRING(Tmp.ptr());
          if (!Buffer)
            throw TypeError{"Unable to extract string contents! (invalid type)"};
          auto* Ret = new CPointerObj{*Ty, Data<void*>::emplace_owned(Buffer)};
          H.emplace_back(std::unique_ptr<CObj>{Ret});
          return Ret;
        }
      }
    }
    // Last resort: cast this as a buffer
    py::buffer B = O.cast<py::buffer>();
    py::buffer_info Info = B.request(isWritable);
    if (Info.ndim != 1) {
      ThrowError<TypeError>() << "buffer should have only one dimension, got " << Info.ndim << "!";
    }
    auto ExpectedFormat = getFormatDescriptor(PteTy);
    if (Info.format != ExpectedFormat) {
      ThrowError<TypeError>() << "buffer doesn't have the good format, got '" << Info.format << "', expected '" << ExpectedFormat << "'";
    }

    auto* Ret = new CPointerObj{*Ty, Data<void*>::emplace_owned(Info.ptr)};
    H.emplace_back(std::unique_ptr<CObj>{Ret});
    return Ret;
  }

  static CObj* case_composite(StructType const* Ty, ObjsHolder&, PyObjsHolder&, py::handle O)
  {
    return checkType(O.cast<CStructObj*>(), Ty);
  }

  static CObj* case_composite(UnionType const* Ty, ObjsHolder&, PyObjsHolder&, py::handle O)
  {
    return checkType(O.cast<CUnionObj*>(), Ty);
  }

  static CObj* case_array(ArrayType const* Ty, ObjsHolder&, PyObjsHolder&, py::handle O)
  {
    return checkType(O.cast<CArrayObj*>(), Ty);
  }

  static CObj* case_func(FunctionType const* Ty, ObjsHolder&, PyObjsHolder&, py::handle O)
  {
    return checkType(O.cast<CFunction*>(), Ty);
  }
};
using ConvertArgs = TypeDispatcher<ConvertArgsSwitch>;

struct CreateObjSwitch
{
  typedef std::vector<std::unique_ptr<CObj>> ObjsHolder;

  template <class T, class... Args>
  static std::unique_ptr<CObj> case_basic(BasicType const* Ty, Args&& ... args)
  {
    return std::unique_ptr<CObj>{new CBasicObj<T>{*Ty, std::forward<Args>(args)...}};
  }

  template <class... Args>
  static std::unique_ptr<CObj> case_enum(EnumType const* Ty, Args&& ... args)
  {
    return std::unique_ptr<CObj>{new CBasicObj<EnumType::IntType>{*Ty->getBasicType(), std::forward<Args>(args)...}};
  }

  template <class... Args>
  static std::unique_ptr<CObj> case_pointer(PointerType const* Ty, Args&& ... args)
  {
    return std::unique_ptr<CObj>{new CPointerObj{*Ty, std::forward<Args>(args)...}};
  }

  template <class... Args>
  static std::unique_ptr<CObj> case_composite(StructType const* Ty, Args&& ... args)
  {
    return std::unique_ptr<CObj>{new CStructObj{*Ty, std::forward<Args>(args)...}};
  }

  template <class... Args>
  static std::unique_ptr<CObj> case_composite(UnionType const* Ty, Args&& ... args)
  {
    return std::unique_ptr<CObj>{new CUnionObj{*Ty, std::forward<Args>(args)...}};
  }

  template <class... Args>
  static std::unique_ptr<CObj> case_array(ArrayType const* Ty, Args&& ... args)
  {
    return std::unique_ptr<CObj>{new CArrayObj{*Ty, std::forward<Args>(args)...}};
  }

  template <class... Args>
  static std::unique_ptr<CObj> case_func(FunctionType const* Ty, Args&& ... args)
  {
    return std::unique_ptr<CObj>{new CFunction{Ty->getFunction(nullptr)}};
  }
};
using CreateObj = TypeDispatcher<CreateObjSwitch>;

} // anonymous

std::unique_ptr<CObj> createObj(Type const* Ty, Data<void>&& D)
{
  return CreateObj::switch_(Ty, std::move(D));
}

std::string getFormatDescriptor(Type const* Ty)
{
  switch (Ty->getKind()) {
    case Type::TY_Enum:
      Ty = cast<EnumType>(Ty)->getBasicType();
    case Type::TY_Basic: {
      auto* BTy = cast<BasicType>(Ty);
#define HANDLE_BASICTY(DTy, CTy)\
      case BasicType::DTy:\
        return py::format_descriptor<CTy>::format();

      switch (BTy->getBasicKind()) {
        HANDLE_BASICTY(Bool, c_bool);
        HANDLE_BASICTY(Char, c_char);
        HANDLE_BASICTY(UChar, c_unsigned_char);
        HANDLE_BASICTY(UShort, c_unsigned_short);
        HANDLE_BASICTY(UInt, c_unsigned_int);
        HANDLE_BASICTY(ULong, c_unsigned_long);
        HANDLE_BASICTY(ULongLong, c_unsigned_long_long);
        HANDLE_BASICTY(SChar, c_signed_char);
        HANDLE_BASICTY(Short, c_short);
        HANDLE_BASICTY(Int, c_int);
        HANDLE_BASICTY(Long, c_long);
        HANDLE_BASICTY(LongLong, c_long_long);
        HANDLE_BASICTY(Float, c_float);
        HANDLE_BASICTY(Double, c_double);
#undef HANDLE_BASICTY
        default:
          break;
      };
      break;
    }
    case Type::TY_Pointer:
      return "P";
    case Type::TY_Struct:
    {
      std::string Ret;
      auto* STy = cast<StructType>(Ty);
      size_t CurIdx = 0;
      for (auto const& F: STy->getFields()) {
        const size_t Off = F.getOffset();
        const auto* FTy = F.getType();
        if (CurIdx < Off) {
          // Padding
          for (size_t i = 0; i < (Off-CurIdx); ++i) Ret += "x";
        }
        Ret += getFormatDescriptor(FTy);
        CurIdx = F.getOffset() + FTy->getSize();
      }
      // Final padding
      for (size_t i = 0; i < (STy->getSize()-CurIdx); ++i) Ret += "x";
      return Ret;
    }
    default:
      break;
  };

  // We treat every other cases as buffer of bytes.
  return std::to_string(Ty->getSize()) + "B";
}

std::unique_ptr<CObj> CObj::cast(QualType To) const
{
  if (isConst() && !To.hasConst()) {
    return {};
  }
  auto Ret = cast_impl(To.getType());
  if (To.hasConst()) {
    Ret->setConst();
  }
  return Ret;
}

py::buffer_info CObj::getBufferInfo()
{
  const ssize_t Size = getSize();
  py::buffer_info Ret(
    dataPtr(), 1, "B", Size);
  Ret.readonly = isConst();
  return Ret;
}

py::buffer_info CArrayObj::getBufferInfo()
{
  // Check if we have a multi-dimensional array
  auto MDA = MDArray::fromArrayTy(*getArrayType());
  if (MDA.isValid()) {
    auto const& Shape = MDA.getShape();
    const auto Format = getFormatDescriptor(MDA.getElementType());
    const auto EltSize = MDA.getElementType()->getSize();
    const auto NDim = Shape.size();
    std::vector<ssize_t> Strides(NDim);
    ssize_t CurStride = EltSize;
    for (ssize_t i = NDim-1; i >= 0; --i) {
      Strides[i] = CurStride;
      CurStride *= Shape[i];
    }
    py::buffer_info Ret(dataPtr(), static_cast<ssize_t>(EltSize),
      Format, static_cast<ssize_t>(NDim), Shape, Strides);
    Ret.readonly = MDA.getElementType().hasConst();
    return Ret;
  }
  // If not, return the same buffer as view_as_bytes
  return CObj::getBufferInfo();
}

py::object CArrayObj::get(size_t Idx) {
  return TypeDispatcher<ValueGetter>::switch_(getElementType(), GEP(Idx));
}

void CArrayObj::set(size_t Idx, py::handle Obj) {
  if (isConst()) {
    throw ConstError{};
  }
  TypeDispatcher<ValueSetter>::switch_(getElementType(), GEP(Idx), Obj);
}

void CCompositeObj::setValue(CompositeField const& Field, py::handle Obj)
{
  if (isConst()) {
    throw ConstError{};
  }
  void* Ptr = getFieldData(Field);
  TypeDispatcher<ValueSetter>::switch_(Field.getType(), Ptr, Obj);
}

py::object CCompositeObj::getValue(CompositeField const& Field)
{
  void* Ptr = getFieldData(Field);
  return TypeDispatcher<ValueGetter>::switch_(Field.getType(), Ptr);
}

std::unique_ptr<CObj> CPointerObj::getObj() {
  auto PteeTy = getPointeeType();
  auto Ret = TypeDispatcher<PtrToObjView>::switch_(PteeTy, getPtr());
  if (PteeTy.hasConst()) {
    Ret->setConst();
  }
  return Ret;
}

py::memoryview CPointerObj::getMemoryViewObjects(size_t Len)
{
  auto PointeeTy = getPointeeType();
  const size_t PointeeSize = PointeeTy->getSize();
  // TODO: check integer overflow
  // TODO: ssize_t is an issue
  py::buffer_info BI(py::buffer_info{getPtr(), static_cast<ssize_t>(PointeeSize), getFormatDescriptor(PointeeTy), static_cast<ssize_t>(Len)});
  BI.readonly = PointeeTy.hasConst();
  return py::memoryview{BI};
}

py::memoryview CPointerObj::getMemoryViewCStr()
{
  static constexpr auto CharKind = BasicType::getKind<char>();
  auto* PteeType = getPointeeType().getType();
  if (!isa<BasicType>(PteeType) || static_cast<BasicType const*>(PteeType)->getBasicKind() != CharKind) {
    throw TypeError{"pointer must be a pointer to char*!"};
  }
  const size_t Len = strlen((const char*)getPtr());
  return getMemoryViewObjects(Len);
}

py::object CVarArgsFunction::call(py::args const& Args) const
{
  FunctionType const* FTy = getFuncType();
  auto const& Params = FTy->getParams();
  const size_t NParams = Params.size();
  const size_t VarArgsCount = Args.size() - Params.size();
  std::vector<Type const*> VarArgs;
  VarArgs.resize(VarArgsCount);
  for (size_t i = 0; i < VarArgsCount; ++i) {
    VarArgs[i] = py::cast<CObj*>(Args[NParams+i])->getType();
  }
  NativeFunc NF = FTy->getFunction(&VarArgs[0],VarArgs.size(),FuncPtr_);
  return CFunction{NF}.call(Args);
}

py::object CFunction::call(py::args const& Args) const
{
  ConvertArgsSwitch::ObjsHolder Holders;
  ConvertArgsSwitch::PyObjsHolder PyHolders;

  const auto Len = py::len(Args);
  FunctionType const* FTy = getFuncType();

  const auto NArgs = FTy->getParams().size();
  if (Len != NArgs) {
    ThrowError<BadFunctionCall>() << "invalid number of arguments: expected " << NArgs << ", got " << Len << ".";
  }

  std::vector<void*> Ptrs;
  Ptrs.reserve(Len);

  size_t I = 0;
  for (auto& A: Args) {
    QualType ATy = FTy->getParams()[I];
    auto* AObj = ConvertArgs::switch_(ATy, Holders, PyHolders, A);
    Ptrs.push_back(AObj->dataPtr());
    ++I;
  }

  auto* RetTy = FTy->getReturnType();
  std::unique_ptr<CObj> RetObj;
  if (RetTy) {
    RetObj = CreateObj::switch_(RetTy);
  }
  NF_.call(RetTy ? RetObj->dataPtr() : nullptr, &Ptrs[0]);
  if (RetObj) {
    return py::cast(RetObj.release(), py::return_value_policy::take_ownership);
  }
  return py::none();
}

// Cast
std::unique_ptr<CObj> CPointerObj::cast_impl(Type const* To) const
{
  CObj* Ret = nullptr;
  if (auto const* BTy = dyn_cast<BasicType>(To)) {
    if (BTy->getSize() == sizeof(uintptr_t)) {
      Ret = new CBasicObj<uintptr_t>{*BTy, Data<uintptr_t>::emplace_owned(reinterpret_cast<uintptr_t>(getPtr()))};\
    }
  }
  else
  if (auto const* PTy = dyn_cast<PointerType>(To)) {
    Ret = new CPointerObj{*PTy, Data<void*>::emplace_owned(getPtr())};
  }
  return std::unique_ptr<CObj>{Ret};
}

std::unique_ptr<CObj> CArrayObj::cast_impl(Type const* To) const
{
  CObj* Ret = nullptr;
  if (auto* ATy = dyn_cast<ArrayType>(To)) {
    auto* Ptr = getData();
    if (ATy->getSize() == getType()->getSize() && (((uintptr_t)Ptr) % ATy->getAlign() == 0)) {
      Ret = new CArrayObj{*ATy, Data<void>::view((void*)getData())};
    }
  }
  return std::unique_ptr<CObj>{Ret};
}

std::unique_ptr<CObj> CCompositeObj::cast_impl(Type const* To) const
{
  CObj* Ret = nullptr;
  if (auto* PTy = dyn_cast<PointerType>(To)) {
    Ret = new CPointerObj{*PTy, Data<void*>::emplace_owned((void*)getData())};
  }
  else
  if (auto* CTy = dyn_cast<CompositeType>(To)) {
    auto* Ptr = getData();
    if (CTy->getSize() == getType()->getSize() && (((uintptr_t)Ptr) % CTy->getAlign() == 0)) {
      if (auto* STy = dyn_cast<StructType>(To))
        Ret = new CStructObj{*STy, Data<void>::view((void*)Ptr)};
      else
        Ret = new CUnionObj{*dffi::cast<UnionType>(CTy), Data<void>::view((void*)Ptr)};
    }
  }
  return std::unique_ptr<CObj>{Ret};
}
