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

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include <dffi/dffi.h>
#include <dffi/types.h>
#include <dffi/composite_type.h>
#include <dffi/casting.h>
#include <dffi/mdarray.h>

#include <sstream>

namespace py = pybind11;

#include "cobj.h"
#include "dispatcher.h"
#include "errors.h"

using namespace dffi;

DFFIErrorStr::DFFIErrorStr()
{ }

DFFIErrorStr::DFFIErrorStr(std::string Err):
    Err_(std::move(Err))
{ }

const char* DFFIErrorStr::what() const
{ return Err_.c_str(); }

UnknownFunctionError::UnknownFunctionError(const char* Name)
{
  std::stringstream ss;
  ss << "unknown function '" << Name << "'";
  Err_ = ss.str();
}

TypeError::TypeError(dffi::Type const* Got, dffi::Type const* Expected):
  DFFIErrorStr("invalid type!")
{
  // TODO: use type prettyprinter to create the message!
}

ConstError::ConstError():
  DFFIErrorStr("can't modify a const object!")
{ }

ConstCastError::ConstCastError():
  DFFIErrorStr("const type can't be converted to a non-const type.")
{ }

namespace {

void throwCompileErr(std::string&& Err)
{
  throw CompileError{std::move(Err)};
}

// DFFI wrappers
CompilationUnit dffi_cdef(DFFI& C, const char* Code, const char* Name)
{
  std::string Err;
  auto CU = C.cdef(Code, Name, Err);
  if (!CU) {
    throwCompileErr(std::move(Err));
  }
  return CU;
}

CompilationUnit dffi_cdef_no_name(DFFI& C, const char* Code)
{
  return dffi_cdef(C, Code, nullptr);
}

CompilationUnit dffi_compile(DFFI& C, const char* Code)
{
  std::string Err;
  auto CU = C.compile(Code, Err);
  if (!CU) {
    throwCompileErr(std::move(Err));
  }
  return CU;
}

std::unique_ptr<CObj> cu_getfunction(CompilationUnit& CU, const char* Name)
{
  void* FPtr;
  FunctionType const* FTy;
  std::tie(FPtr, FTy) = CU.getFunctionAddressAndTy(Name);
  if (!FPtr || !FTy) {
    throw UnknownFunctionError{Name};
  }

  CObj* Ret;
  if (FTy->hasVarArgs()) {
    Ret = new CVarArgsFunction{FPtr, FTy};
  }
  else {
    auto NF = CU.getFunction(FPtr, FTy);
    Ret = new CFunction{NF};
  }

  return std::unique_ptr<CObj>{Ret};
}

//std::unique_ptr<CArrayObj> dffi_view(DFFI& D, py::buffer& B)
//{
//  auto Info = B.request();
//  if (Info.ndim != 1) {
//    ThrowError<TypeError>() << "buffer should have only one dimension, got " << Info.ndim << "!";
//  }
//  // Get type from format
//  auto const& Format = Info.format;
//  if (Format.size() != 1) {
//    ThrowError<TypeError>() << "unsupported format " << Format;
//  }
//  BasicType const* PteTy = nullptr; 
//  switch (Format[0]) {
//    #define HANDLE_BTY(Format, CTy)\
//      case Format:\
//        PteTy = D.getBasicType(BasicType::getKind<CTy>());\
//        break;
//    HANDLE_BTY('c', c_char)
//    HANDLE_BTY('b', c_signed_char)
//    HANDLE_BTY('B', c_unsigned_char)
//    HANDLE_BTY('?', c_bool)
//    HANDLE_BTY('h', c_short)
//    HANDLE_BTY('H', c_unsigned_short)
//    HANDLE_BTY('i', c_int)
//    HANDLE_BTY('I', c_unsigned_int)
//    HANDLE_BTY('l', c_long)
//    HANDLE_BTY('L', c_unsigned_long)
//    HANDLE_BTY('q', c_long_long)
//    HANDLE_BTY('Q', c_unsigned_long_long)
//    HANDLE_BTY('f', c_float)
//    HANDLE_BTY('d', c_double)
//    HANDLE_BTY('P', uintptr_t)
//    default:
//      ThrowError<TypeError>() << "unsupported format character " << Format[0];
//  };
//
//  return std::unique_ptr<CArrayObj>{new CArrayObj{
//    *D.getArrayType(PteTy, Info.size),
//    Data<void>::view(Info.ptr)}};
//}

void dffi_dlopen(const char* Path)
{
  std::string Err;
  if (!DFFI::dlopen(Path, &Err)) {
    throw DLOpenError{Err};
  }
}

QualType dffi_const(Type const& Ty)
{
  return QualType{&Ty}.withConst();
}

std::unique_ptr<CObj> dffi_view_from_buffer(QualType Ty, py::buffer& B)
{
  auto BI = B.request(!Ty.hasConst());
  const size_t nbytes = BI.size * BI.itemsize;
  if (Ty->getSize() != nbytes) {
    ThrowError<TypeError>() << "expect a buffer of " << Ty->getSize() << " bytes, got " << nbytes;
  }
  auto Ret = createObj(Ty, Data<void>::view(BI.ptr));
  return Ret;
}

std::unique_ptr<DFFI> default_ctor(unsigned optLevel, py::list includeDirs)
{
  CCOpts Opts;
  Opts.OptLevel = optLevel;
  auto& Dirs = Opts.IncludeDirs;
  Dirs.reserve(py::len(includeDirs));
  for (py::handle O: includeDirs) {
    Dirs.emplace_back(O.cast<std::string>());
  }
  return std::unique_ptr<DFFI>{new DFFI{Opts}};
}

CFunction dffi_getfunction(DFFI& D, FunctionType const& Ty, void* Ptr)
{
  return CFunction{D.getFunction(&Ty, Ptr)};
}

CFunction functiontype_getfunction(FunctionType const& Ty, void* Ptr)
{
  return CFunction{Ty.getFunction(Ptr)};
}

uintptr_t cpointerobj_getptr(CPointerObj& Obj)
{
  return (uintptr_t)Obj.getPtr();
}

template <class T>
std::unique_ptr<CBasicObj<T>> createBasicObj(DFFI& D, T V)
{
  return std::unique_ptr<CBasicObj<T>>{new CBasicObj<T>{*D.getBasicType<T>(), V}};
}

struct CUFuncs
{
  CUFuncs(CompilationUnit& CU):
    CU_(CU)
  { }

  std::unique_ptr<CObj> getAttr(const char* Name)
  {
    return cu_getfunction(CU_, Name);
  }

  std::vector<std::string> getList() const
  {
    return CU_.getFunctions();
  }

private:
  CompilationUnit& CU_;
};

CUFuncs cu_funcs(CompilationUnit& CU)
{
  return CUFuncs{CU};
}

struct CUTypes
{
  CUTypes(CompilationUnit& CU):
    CU_(CU)
  { }

  dffi::Type const* getAttr(const char* Name)
  {
    return CU_.getType(Name);
  }

  std::vector<std::string> getList() const
  {
    return CU_.getTypes();
  }

private:
  CompilationUnit& CU_;
};

CUTypes cu_types(CompilationUnit& CU)
{
  return CUTypes{CU};
}

template <class T>
py::object basictype_new_impl(BasicType const& BTy, py::handle O)
{
  T V;
  if (auto* CBO = O.dyn_cast<CBasicObj<T>>()) {
    V = CBO->value();
  }
  else {
    V = O.cast<T>();
  }
  return py::cast(new CBasicObj<T>{BTy, Data<T>::emplace_owned(V)},
    py::return_value_policy::take_ownership);
}

py::object basictype_new(BasicType const& BTy, py::handle O)
{
#define HANDLE_BASICTY(K, CTy)\
  case BasicType::K:\
    return basictype_new_impl<CTy>(BTy, O);
  switch (BTy.getBasicKind()) {
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
  };
  return py::none();
}

std::unique_ptr<CStructObj> structtype_new(StructType const& STy, py::kwargs KW)
{
  std::unique_ptr<CStructObj> Ret(new CStructObj{STy});
  if (KW) {
    for (auto& It: KW) {
      Ret->setValue(It.first.cast<std::string>().c_str(), It.second);
    }
  }

  return Ret;
} 

std::unique_ptr<CUnionObj> uniontype_new(UnionType const& UTy)
{
  return std::unique_ptr<CUnionObj>{new CUnionObj{UTy}};
} 

std::unique_ptr<CUnionObj> uniontype_new(UnionType const& UTy, const char* Field, py::object O)
{
  auto Ret = uniontype_new(UTy);
  Ret->setValue(Field, O);
  return Ret;
} 

std::unique_ptr<CPointerObj> cpointerobj_new(PointerType const& PTy)
{
  return std::unique_ptr<CPointerObj>{new CPointerObj{PTy, Data<void*>::emplace_owned(nullptr)}};
}

std::unique_ptr<CArrayObj> carraytype_new(ArrayType const& ATy)
{
  return std::unique_ptr<CArrayObj>{new CArrayObj{ATy}};
}

std::unique_ptr<CBasicObj<int>> enumtype_get(EnumType const& ETy, const char* Name)
{
  auto const& Fields = ETy.getFields();

  auto It = Fields.find(Name);
  if (It == Fields.end()) {
    throw UnknownField{Name};
  }
  return std::unique_ptr<CBasicObj<int>>{new CBasicObj<int>{*ETy.getBasicType(), It->second}};
}

const char* field_get_name(CompositeField const& F) {
  return F.getName();
}
template <class It>
std::string const& field_get_name(It const& I) {
  return I.first;
}

py::list list_fields(CompositeType const& CF)
{
  py::list Ret(CF.getFieldsCount());
  size_t I = 0;
  for (const char* Name: CF.getFieldsName()) {
    Ret[I++] = py::str(Name);
  }
  return Ret;
}

template <class T>
py::list list_fields(T const& Fields)
{
  py::list Ret(Fields.size());
  size_t I = 0;
  for (auto const& F: Fields) {
    Ret[I++] = py::str(field_get_name(F));
  }
  return Ret;
}

struct CArrayObjPyIterator {
  CArrayObjPyIterator(CArrayObj& Obj, py::object Ref):
    Obj_(Obj),
    Ref_(Ref)
  { }

  py::object next() {
    if (Index_ == Obj_.getNumElements())
      throw py::stop_iteration();
    return Obj_.get(Index_++);
  }

private:
  CArrayObj& Obj_;
  py::object Ref_; // keep a reference
  size_t Index_ = 0;
};

} // anonymous

#ifndef PYDFFI_EXT_NAME
#define PYDFFI_EXT_NAME pydffi
#endif

PYBIND11_MODULE(PYDFFI_EXT_NAME, m)
{
  DFFI::initialize();

  // Types
  py::class_<Type> type(m, "Type");
  type.def_property_readonly("size", &Type::getSize)
      .def_property_readonly("align", &Type::getAlign)
      .def_property_readonly("ptr", [](Type const* Ty) {
        return PointerType::get(Ty);
      }, py::return_value_policy::reference_internal)
      .def_property_readonly("format", &getFormatDescriptor)
      .def_property_readonly("portable_format", &getPortableFormatDescriptor)
      .def_property_readonly("names", [](Type const* Ty) {
          return py::make_iterator<py::return_value_policy::reference_internal>(Ty->getNames());
        });
    ;

  py::class_<QualType>(m, "QualType")
    .def(py::init<Type const*>(), py::keep_alive<1, 2>())
    .def(py::init<Type const&>(), py::keep_alive<1, 2>())
    .def_property_readonly("hasConst", &QualType::hasConst)
    .def_property_readonly("withConst", &QualType::withConst, py::keep_alive<1,2>())
    .def_property_readonly("type", &QualType::getType, py::keep_alive<1,2>())
    .def("__iter__", [](py::object& o) { return py::iter(py::getattr(o, "type")); })
    .def("__getattr__", [](py::object& o, py::object& attr) { return py::getattr(py::getattr(o, "type"), attr); })
    .def("__dir__", [](py::object& o) { return py::dir(py::getattr(o, "type")); })
    ;
 
  py::implicitly_convertible<Type,QualType>();

  py::enum_<BasicType::BasicKind>(m, "BasicKind")
    .value("Bool", BasicType::Bool)
    .value("Char", BasicType::Char)
    .value("SChar", BasicType::SChar)
    .value("Short", BasicType::Short)
    .value("Int", BasicType::Int)
    .value("Long", BasicType::Long)
    .value("LongLong", BasicType::LongLong)
    .value("UChar", BasicType::UChar)
    .value("UShort", BasicType::UShort)
    .value("UInt", BasicType::UInt)
    .value("ULong", BasicType::ULong)
    .value("ULongLong", BasicType::ULongLong)
    .value("Float", BasicType::Float)
    .value("Double", BasicType::Double)
    .value("LongDouble", BasicType::LongDouble)
#ifdef DFFI_SUPPORT_COMPLEX
    .value("ComplexFloat", BasicType::ComplexFloat)
    .value("ComplexDouble", BasicType::ComplexDouble)
    .value("ComplexLongDouble", BasicType::ComplexLongDouble)
#endif
    ;

  py::class_<BasicType>(m, "BasicType", type)
    .def_property_readonly("kind", &BasicType::getBasicKind)
    .def("__call__", basictype_new, py::keep_alive<0,1>())
    ;

  py::class_<PointerType>(m, "PointerType", type)
    .def("pointee", &PointerType::getPointee, py::return_value_policy::reference_internal)
    .def("__call__", cpointerobj_new, py::keep_alive<0,1>())
    ;

  py::class_<ArrayType>(m, "ArrayType", type)
    .def("elementType", &ArrayType::getElementType, py::return_value_policy::reference_internal)
    .def("__call__", carraytype_new, py::keep_alive<0,1>())
    .def("count", &ArrayType::getNumElements)
    ;

  py::class_<FunctionType>(m, "FunctionType", type)
    .def_property_readonly("returnType", &FunctionType::getReturnType, py::return_value_policy::reference_internal)
    .def_property_readonly("params", &FunctionType::getParams, py::return_value_policy::reference_internal)
    .def_property_readonly("varArgs", &FunctionType::hasVarArgs)
    .def("getWrapperLLVMStr", &FunctionType::getWrapperLLVMStr)
    .def("__call__", functiontype_getfunction, py::keep_alive<0,1>())
    ;

  py::class_<CompositeField>(m, "CompositeField")
    .def_property_readonly("name", &CompositeField::getName)
    .def_property_readonly("offset", &CompositeField::getOffset)
    .def_property_readonly("type", &CompositeField::getType, py::return_value_policy::reference_internal)
    ;

  py::class_<CompositeType> CompType(m, "CompositeType", type);
  CompType.def_property_readonly("fields", &CompositeType::getFields, py::return_value_policy::reference_internal)
    .def("__getattr__", &CompositeType::getField, py::return_value_policy::reference_internal)
    .def("__iter__", [](CompositeType const& CTy) {
      auto Range = CTy.getFields();
      return py::make_iterator<py::return_value_policy::reference_internal>(Range.begin(), Range.end());
     })
    .def("__dir__", (py::list(*)(CompositeType const&)) &list_fields, py::return_value_policy::copy)
    ;

  py::class_<StructType>(m, "StructType", CompType)
    .def("__call__", structtype_new, py::keep_alive<0,1>())
    ;
  py::class_<UnionType>(m, "UnionType", CompType)
    .def("__call__", (std::unique_ptr<CUnionObj>(*)(UnionType const&)) uniontype_new, py::keep_alive<0,1>())
    .def("__call__", (std::unique_ptr<CUnionObj>(*)(UnionType const&, const char*, py::object)) uniontype_new, py::keep_alive<0,1>())
    ;

  py::class_<EnumType>(m, "EnumType", type)
    .def("__getattr__", enumtype_get)
    .def("__iter__", [](EnumType const& ETy) {
      return py::make_iterator<py::return_value_policy::reference_internal>(ETy.getFields());
     })
    .def("__dir__", [](EnumType const& Ty) {
        auto const& Fields = Ty.getFields();
        return list_fields(Fields);
        }, py::return_value_policy::copy)
    ;

  py::class_<CObj> cobj(m, "CObj")
    ;

#define DECL_CBASICOBJ_BASE(CTy, Name)\
  py::class_<CBasicObj<CTy>>(m, Name, cobj)\
    .def(py::init(&createBasicObj<CTy>), py::keep_alive<1, 2>())\
    .def_property_readonly("value", &CBasicObj<CTy>::value)\

#define DECL_CBASICOBJ(CTy, Name, Conv)\
    DECL_CBASICOBJ_BASE(CTy, Name)\
    .def(Conv, &CBasicObj<CTy>::value)\
    .def(py::self + py::self)\
    .def(py::self + typename std::remove_cv<CTy>::type{})\
    .def(py::self += py::self)\
    .def(py::self += typename std::remove_cv<CTy>::type{})\
    .def(py::self - py::self)\
    .def(py::self - typename std::remove_cv<CTy>::type{})\
    .def(py::self -= py::self)\
    .def(py::self -= typename std::remove_cv<CTy>::type{})\
    .def(py::self * py::self)\
    .def(py::self * typename std::remove_cv<CTy>::type{})\
    .def(py::self *= py::self)\
    .def(py::self *= typename std::remove_cv<CTy>::type{})\
    .def(py::self / py::self)\
    .def(py::self / typename std::remove_cv<CTy>::type{})\
    .def(py::self /= py::self)\
    .def(py::self /= typename std::remove_cv<CTy>::type{})\
    .def(py::self == py::self)\
    .def(py::self == typename std::remove_cv<CTy>::type{})\
    .def(py::self != py::self)\
    .def(py::self != typename std::remove_cv<CTy>::type{})\
    .def(py::self < py::self)\
    .def(py::self < typename std::remove_cv<CTy>::type{})\
    .def(py::self > py::self)\
    .def(py::self > typename std::remove_cv<CTy>::type{})\
    .def(py::self <= py::self)\
    .def(py::self <= typename std::remove_cv<CTy>::type{})\
    .def(py::self >= py::self)\
    .def(py::self >= typename std::remove_cv<CTy>::type{})

#define DECL_CBASICOBJ_INT(CTy, Name, Conv)\
  DECL_CBASICOBJ(CTy, Name, Conv)\
    .def(py::self ^ py::self)\
    .def(py::self ^ typename std::remove_cv<CTy>::type{})\
    .def(py::self ^= py::self)\
    .def(py::self ^= typename std::remove_cv<CTy>::type{})\
    .def(py::self | py::self)\
    .def(py::self | typename std::remove_cv<CTy>::type{})\
    .def(py::self |= py::self)\
    .def(py::self |= typename std::remove_cv<CTy>::type{})\
    .def(py::self & py::self)\
    .def(py::self & typename std::remove_cv<CTy>::type{})\
    .def(py::self &= py::self)\
    .def(py::self &= typename std::remove_cv<CTy>::type{})\
    .def(~py::self)

  DECL_CBASICOBJ_BASE(bool, "Bool")
    .def("__nonzero__", &CBasicObj<bool>::value)
    .def("__bool__", &CBasicObj<bool>::value)
    .def(py::self ^ py::self)
    .def(py::self ^ bool())
    .def(py::self ^= py::self)
    .def(py::self ^= bool())
    .def(py::self | py::self)
    .def(py::self | bool())
    .def(py::self |= py::self)
    .def(py::self |= bool())
    .def(py::self & py::self)
    .def(py::self & bool())
    .def(py::self &= py::self)
    .def(py::self &= bool())
    .def(py::self == py::self)
    .def(py::self == bool())
    .def(py::self != py::self)
    .def(py::self != bool())
    .def(py::self < py::self)
    .def(py::self < bool())
    .def(py::self > py::self)
    .def(py::self > bool())
    .def(py::self <= py::self)
    .def(py::self <= bool())
    .def(py::self >= py::self)
    .def(py::self >= bool())
    ;
    
  DECL_CBASICOBJ_INT(uint8_t, "UChar", "__int__");
  DECL_CBASICOBJ_INT(uint16_t, "UShort", "__int__");
  DECL_CBASICOBJ_INT(uint32_t, "UInt", "__int__");
  DECL_CBASICOBJ_INT(uint64_t, "ULongLong", sizeof(long) <= sizeof(int64_t) ? "__int__":"__long__");
#ifdef DFFI_SUPPORT_I128
  DECL_CBASICOBJ_INT(__uint128_t, "UInt128", "__long__");
#endif
  DECL_CBASICOBJ_INT(int8_t, "SChar", "__int__");
  DECL_CBASICOBJ_INT(int16_t, "Short", "__int__");
  DECL_CBASICOBJ_INT(int32_t, "Int", "__int__");
  DECL_CBASICOBJ_INT(int64_t, "LongLong", sizeof(long) <= sizeof(int64_t) ? "__int__":"__long__");
#ifdef DFFI_SUPPORT_I128
  DECL_CBASICOBJ_INT(__int128_t, "Int128", "__long__");
#endif
  DECL_CBASICOBJ(float, "Float", "__float__");
  DECL_CBASICOBJ(double, "Double", "__float__");
  DECL_CBASICOBJ(long double, "LongDouble", "__float__");

  py::class_<CPointerObj>(m, "CPointerObj", cobj)
    .def(py::init<PointerType const&>(), py::keep_alive<1, 2>())
    .def_property_readonly("pointeeType", &CPointerObj::getPointeeType, py::return_value_policy::reference_internal)
    .def_property_readonly("obj", &CPointerObj::getObj)
    .def_property_readonly("value", cpointerobj_getptr)
    .def("__int__", cpointerobj_getptr)
    .def("__long__", cpointerobj_getptr)
    .def(PYBIND11_BOOL_ATTR, [](CPointerObj const& O) -> bool { return O.getPtr() != nullptr; })
    .def("viewObjects", &CPointerObj::getMemoryViewObjects)
    .def_property_readonly("cstr", &CPointerObj::getMemoryViewCStr)
    ;

  // Composite object
  py::class_<CCompositeObj> PyCompObj(m, "CCompositeObj", cobj);
  PyCompObj.def(py::init<CompositeType const&>(), py::keep_alive<1, 2>())
           .def("__setattr__", 
             (void(CCompositeObj::*)(const char*, py::handle)) &CCompositeObj::setValue)
           .def("__getattr__",
             (py::object(CCompositeObj::*)(const char*)) &CCompositeObj::getValue)
           .def("__dir__", [](CCompositeObj const& O) {
             return list_fields(*O.getCompositeType());
           }, py::return_value_policy::copy)
     ;

  py::class_<CStructObj>(m, "CStructObj", PyCompObj)
    .def(py::init<StructType const&>(), py::keep_alive<1, 2>())
    ;
  py::class_<CUnionObj>(m, "CUnionObj", PyCompObj)
    .def(py::init<UnionType const&>(), py::keep_alive<1, 2>())
    ;

  py::class_<CArrayObjPyIterator>(m, "CArrayObjPyIterator")
    .def("__iter__", [](CArrayObjPyIterator& It) { return It; })
    .def("__next__", &CArrayObjPyIterator::next)
    ;

  py::class_<CArrayObj>(m, "CArrayObj", py::buffer_protocol(), cobj)
    .def(py::init<ArrayType const&>(), py::keep_alive<1, 2>())
    .def("__setitem__", &CArrayObj::set)
    .def("__getitem__", &CArrayObj::get)
    .def("__len__", &CArrayObj::getNumElements)
    .def("__iter__", [](py::object O) { return CArrayObjPyIterator{O.cast<CArrayObj&>(), O}; })
    .def("set", &CArrayObj::set)
    .def("get", &CArrayObj::get)
    .def("elementType", &CArrayObj::getElementType, py::return_value_policy::reference_internal)
    .def_buffer(&CArrayObj::getBufferInfo)
  ;

  py::class_<CFunction>(m, "CFunction", cobj)
    .def("__call__", &CFunction::call)
    ;

  py::class_<CVarArgsFunction>(m, "CVarArgsFunction", cobj)
    .def("__call__", &CVarArgsFunction::call)
    ;

  py::class_<CUTypes>(m, "CUTypes")
    .def("__getattr__", &CUTypes::getAttr, py::return_value_policy::reference_internal)
    .def("__dir__", &CUTypes::getList)
    ;
  py::class_<CUFuncs>(m, "CUFuncs")
    .def("__getattr__", &CUFuncs::getAttr, py::keep_alive<0,1>())
    .def("__dir__", &CUFuncs::getList)
    ;

  py::class_<CompilationUnit>(m, "CompilationUnit")
    .def_property_readonly("funcs", py::cpp_function(cu_funcs, py::keep_alive<0,1>()))
    .def_property_readonly("types", py::cpp_function(cu_types, py::keep_alive<0,1>()))
    ;

  py::class_<DFFI>(m, "FFI")
    .def(py::init(&default_ctor), py::arg("optLevel") = 2, py::arg("includeDirs") = py::list())
    .def("cdef", dffi_cdef, py::keep_alive<0,1>())
    .def("cdef", dffi_cdef_no_name, py::keep_alive<0,1>())
    .def("compile", dffi_compile, py::keep_alive<0,1>())
    //.def("view", dffi_view, py::keep_alive<0,1>(), py::keep_alive<0,2>())
    .def("basicType", 
      (BasicType const*(DFFI::*)(BasicType::BasicKind)) &DFFI::getBasicType,
      py::return_value_policy::reference_internal)
    .def("arrayType", &DFFI::getArrayType, py::return_value_policy::reference_internal)
    .def("pointerType", &DFFI::getPointerType, py::return_value_policy::reference_internal)
    .def("getFunction", dffi_getfunction, py::keep_alive<0,1>())

    // Basic values
    .def("SChar", createBasicObj<c_signed_char>, py::keep_alive<0,1>())
    .def("Short", createBasicObj<c_short>, py::keep_alive<0,1>())
    .def("Int", createBasicObj<c_int>, py::keep_alive<0,1>())
    .def("Long", createBasicObj<c_long>, py::keep_alive<0,1>())
    .def("LongLong", createBasicObj<c_long_long>, py::keep_alive<0,1>())
#ifdef DFFI_SUPPORT_I128
    .def("Int128", createBasicObj<__int128_t>, py::keep_alive<0,1>())
#endif
    .def("UChar", createBasicObj<c_unsigned_char>, py::keep_alive<0,1>())
    .def("UShort", createBasicObj<c_unsigned_short>, py::keep_alive<0,1>())
    .def("UInt", createBasicObj<c_unsigned_int>, py::keep_alive<0,1>())
    .def("ULong", createBasicObj<c_unsigned_long>, py::keep_alive<0,1>())
    .def("ULongLong", createBasicObj<c_unsigned_long_long>, py::keep_alive<0,1>())
#ifdef DFFI_SUPPORT_I128
    .def("UInt128", createBasicObj<__uint128_t>, py::keep_alive<0,1>())
#endif
    .def("Float", createBasicObj<c_float>, py::keep_alive<0,1>())
    .def("Double", createBasicObj<c_double>, py::keep_alive<0,1>())
    .def("LongDouble", createBasicObj<c_long_double>, py::keep_alive<0,1>())

    .def("Int8", createBasicObj<int8_t>, py::keep_alive<0,1>())
    .def("UInt8", createBasicObj<uint8_t>, py::keep_alive<0,1>())
    .def("Int16", createBasicObj<int16_t>, py::keep_alive<0,1>())
    .def("UInt16", createBasicObj<uint16_t>, py::keep_alive<0,1>())
    .def("Int32", createBasicObj<int32_t>, py::keep_alive<0,1>())
    .def("UInt32", createBasicObj<uint32_t>, py::keep_alive<0,1>())
    .def("Int64", createBasicObj<int64_t>, py::keep_alive<0,1>())
    .def("UInt64", createBasicObj<uint64_t>, py::keep_alive<0,1>())

    // Type helpers
    .def_property_readonly("VoidTy", &DFFI::getVoidTy, py::return_value_policy::reference_internal)
    .def_property_readonly("BoolTy", &DFFI::getBoolTy, py::return_value_policy::reference_internal)
    .def_property_readonly("CharTy", &DFFI::getCharTy, py::return_value_policy::reference_internal)
    .def_property_readonly("SCharTy", &DFFI::getSCharTy, py::return_value_policy::reference_internal)
    .def_property_readonly("ShortTy", &DFFI::getShortTy, py::return_value_policy::reference_internal)
    .def_property_readonly("IntTy", &DFFI::getIntTy, py::return_value_policy::reference_internal)
    .def_property_readonly("LongTy", &DFFI::getLongTy, py::return_value_policy::reference_internal)
    .def_property_readonly("LongLongTy", &DFFI::getLongLongTy, py::return_value_policy::reference_internal)
#ifdef DFFI_SUPPORT_I128
    .def_property_readonly("Int128Ty", &DFFI::getInt128Ty, py::return_value_policy::reference_internal)
#endif
    .def_property_readonly("UCharTy", &DFFI::getUCharTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UShortTy", &DFFI::getUShortTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UIntTy", &DFFI::getUIntTy, py::return_value_policy::reference_internal)
    .def_property_readonly("ULongTy", &DFFI::getULongTy, py::return_value_policy::reference_internal)
    .def_property_readonly("ULongLongTy", &DFFI::getULongLongTy, py::return_value_policy::reference_internal)
#ifdef DFFI_SUPPORT_I128
    .def_property_readonly("UInt128Ty", &DFFI::getUInt128Ty, py::return_value_policy::reference_internal)
#endif
    .def_property_readonly("FloatTy", &DFFI::getFloatTy, py::return_value_policy::reference_internal)
    .def_property_readonly("DoubleTy", &DFFI::getDoubleTy, py::return_value_policy::reference_internal)
    .def_property_readonly("LongDoubleTy", &DFFI::getLongDoubleTy, py::return_value_policy::reference_internal)

    // Pointer type helpers
    .def_property_readonly("VoidPtrTy", &DFFI::getVoidPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("BoolPtrTy", &DFFI::getBoolPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("CharPtrTy", &DFFI::getCharPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("SCharPtrTy", &DFFI::getSCharPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("ShortPtrTy", &DFFI::getShortPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("IntPtrTy", &DFFI::getIntPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("LongPtrTy", &DFFI::getLongPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("LongLongPtrTy", &DFFI::getLongLongPtrTy, py::return_value_policy::reference_internal)
#ifdef DFFI_SUPPORT_I128
    .def_property_readonly("Int128PtrTy", &DFFI::getInt128PtrTy, py::return_value_policy::reference_internal)
#endif
    .def_property_readonly("UCharPtrTy", &DFFI::getUCharPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UShortPtrTy", &DFFI::getUShortPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UIntPtrTy", &DFFI::getUIntPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("ULongPtrTy", &DFFI::getULongPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("ULongLongPtrTy", &DFFI::getULongLongPtrTy, py::return_value_policy::reference_internal)
#ifdef DFFI_SUPPORT_I128
    .def_property_readonly("UInt128PtrTy", &DFFI::getUInt128PtrTy, py::return_value_policy::reference_internal)
#endif
    .def_property_readonly("FloatPtrTy", &DFFI::getFloatPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("DoublePtrTy", &DFFI::getDoublePtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("LongDoublePtrTy", &DFFI::getLongDoublePtrTy, py::return_value_policy::reference_internal)

    .def_property_readonly("Int8Ty", &DFFI::getInt8Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt8Ty", &DFFI::getUInt8Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("Int8PtrTy", &DFFI::getInt8PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt8PtrTy", &DFFI::getUInt8PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("Int16Ty", &DFFI::getInt16Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt16Ty", &DFFI::getUInt16Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("Int16PtrTy", &DFFI::getInt16PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt16PtrTy", &DFFI::getUInt16PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("Int32Ty", &DFFI::getInt32Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt32Ty", &DFFI::getUInt32Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("Int32PtrTy", &DFFI::getInt32PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt32PtrTy", &DFFI::getUInt32PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("Int64Ty", &DFFI::getInt64Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt64Ty", &DFFI::getUInt64Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("Int64PtrTy", &DFFI::getInt64PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt64PtrTy", &DFFI::getUInt64PtrTy, py::return_value_policy::reference_internal)
    ;

  m.def("dlopen", dffi_dlopen);

  // CObj-related functions
  m.def("cast", &CObj::cast, py::keep_alive<0,1>());
  m.def("view_as", dffi_view_from_buffer, py::keep_alive<0,1>(), py::keep_alive<0,2>(),
    "View a Python buffer object as a specified C type. The number of bytes within the buffer must be equal to the size of the specified C type");
  m.def("view_as_bytes", [](CObj& O) -> py::memoryview { return py::memoryview{O.getBufferInfo()}; }, py::keep_alive<1,0>(), "Generate a view of a C object as a contiguous buffer of bytes");
  m.def("typeof", [](CObj const& O) { return O.getType(); }, py::return_value_policy::reference, py::keep_alive<0,1>());
  m.def("sizeof", [](CObj const& O) { return O.getSize(); });
  m.def("alignof", [](CObj const& O) { return O.getAlign(); });
  m.def("const", dffi_const, py::keep_alive<0,1>());

  // GEP functions
  m.def("ptr", [](CObj* O) {
    return std::unique_ptr<CPointerObj>{new CPointerObj{O}};
  }, py::return_value_policy::reference, py::keep_alive<0,1>());
  m.def("ptr", [](Type const* Ty) {
    return PointerType::get(Ty);
  }, py::return_value_policy::reference, py::keep_alive<0,1>());

  m.def("native_triple", []() { return DFFI::getNativeTriple(); });

  // Types
  m.def("format", &getFormatDescriptor);
  m.def("portable_format", &getPortableFormatDescriptor);

  // Exceptions
  py::register_exception<CompileError>(m, "CompileError");
  py::register_exception<UnknownFunctionError>(m, "UnknownFunctionError");
  py::register_exception<TypeError>(m, "TypeError");
  py::register_exception<DLOpenError>(m, "DLOpenError");
  py::register_exception<UnknownField>(m, "UnknownField");
  py::register_exception<AllocError>(m, "AllocError");
  py::register_exception<BadFunctionCall>(m, "BadFunctionCall");
  py::register_exception<ConstError>(m, "ConstError");
  py::register_exception<ConstCastError>(m, "ConstCastError");
};
