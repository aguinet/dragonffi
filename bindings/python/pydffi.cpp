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

CFunction cu_getfunction(CompilationUnit& CU, const char* Name)
{
  auto Ret = CU.getFunction(Name);
  if (!Ret) {
    throw UnknownFunctionError(Name);
  }
  return CFunction{Ret};
}


std::unique_ptr<CArrayObj> dffi_view(DFFI& D, py::buffer& B)
{
  auto Info = B.request();
  if (Info.ndim != 1) {
    ThrowError<TypeError>() << "buffer should have only one dimension, got " << Info.ndim << "!";
  }
  // Get type from format
  auto const& Format = Info.format;
  if (Format.size() != 1) {
    ThrowError<TypeError>() << "unsupported format " << Format;
  }
  BasicType const* PteTy = nullptr; 
  switch (Format[0]) {
    #define HANDLE_BTY(Format, CTy)\
      case Format:\
        PteTy = D.getBasicType(BasicType::getKind<CTy>());\
        break;
    HANDLE_BTY('c', char)
    HANDLE_BTY('b', signed char)
    HANDLE_BTY('B', unsigned char)
    HANDLE_BTY('?', bool)
    HANDLE_BTY('h', short)
    HANDLE_BTY('H', unsigned short)
    HANDLE_BTY('i', int)
    HANDLE_BTY('I', unsigned int)
    HANDLE_BTY('l', long)
    HANDLE_BTY('L', unsigned long)
    HANDLE_BTY('q', long long)
    HANDLE_BTY('Q', unsigned long long)
    HANDLE_BTY('f', float)
    HANDLE_BTY('d', double)
    HANDLE_BTY('P', uintptr_t)
    default:
      ThrowError<TypeError>() << "unsupported format character " << Format[0];
  };

  return std::unique_ptr<CArrayObj>{new CArrayObj{
    *D.getArrayType(PteTy, Info.size),
    Data<void>::view(Info.ptr)}};
}

void dffi_dlopen(const char* Path)
{
  std::string Err;
  if (!DFFI::dlopen(Path, &Err)) {
    throw DLOpenError{Err};
  }
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

__attribute__((constructor)) void init()
{
  DFFI::initialize();
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

  CFunction getAttr(const char* Name)
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
    HANDLE_BASICTY(Bool, bool);
    HANDLE_BASICTY(Char, char);
    HANDLE_BASICTY(UInt8, uint8_t);
    HANDLE_BASICTY(UInt16, uint16_t);
    HANDLE_BASICTY(UInt32, uint32_t);
    HANDLE_BASICTY(UInt64, uint64_t);
#ifdef DFFI_SUPPORT_I128
    HANDLE_BASICTY(UInt128, __uint128_t);
#endif
    HANDLE_BASICTY(Int8, int8_t);
    HANDLE_BASICTY(Int16, int16_t);
    HANDLE_BASICTY(Int32, int32_t);
    HANDLE_BASICTY(Int64, int64_t);
#ifdef DFFI_SUPPORT_I128
    HANDLE_BASICTY(Int128, __int128_t);
#endif
    HANDLE_BASICTY(Float, float);
    HANDLE_BASICTY(Double, double);
    HANDLE_BASICTY(LongDouble, long double);
    HANDLE_BASICTY(ComplexFloat, _Complex float);
    HANDLE_BASICTY(ComplexDouble, _Complex double);
    HANDLE_BASICTY(ComplexLongDouble, _Complex long double);
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

std::unique_ptr<CPointerObj> cpointerobj_new(PointerType const& PTy)
{
  return std::unique_ptr<CPointerObj>{new CPointerObj{PTy, Data<void*>::emplace_owned(nullptr)}};
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
template <class T>
py::tuple list_fields(T const& Fields)
{
  py::tuple Ret(Fields.size());
  size_t I = 0;
  for (auto const& F: Fields) {
    Ret[I++] = py::str(field_get_name(F));
  }
  return Ret;
}

} // anonymous

PYBIND11_MODULE(pydffi, m)
{
  // Types
  py::class_<Type> type(m, "Type");
  type.def_property_readonly("size", &Type::getSize)
      .def_property_readonly("align", &Type::getAlign)
      .def_property_readonly("ptr", [](Type const* Ty) {
        return PointerType::get(Ty);
      }, py::return_value_policy::reference_internal)
    ;

  py::enum_<BasicType::BasicKind>(m, "BasicKind")
    .value("Bool", BasicType::Bool)
    .value("Int8", BasicType::Int8)
    .value("Int16", BasicType::Int16)
    .value("Int32", BasicType::Int32)
    .value("Int64", BasicType::Int64)
    .value("UInt8", BasicType::UInt8)
    .value("UInt16", BasicType::UInt16)
    .value("UInt32", BasicType::UInt32)
    .value("UInt64", BasicType::UInt64)
    .value("Float", BasicType::Float)
    .value("Double", BasicType::Double)
    .value("LongDouble", BasicType::LongDouble)
    .value("ComplexFloat", BasicType::ComplexFloat)
    .value("ComplexDouble", BasicType::ComplexDouble)
    .value("ComplexLongDouble", BasicType::ComplexLongDouble)
    ;

  py::class_<BasicType>(m, "BasicType", type)
    .def_property_readonly("kind", &BasicType::getBasicKind)
    .def("__call__", basictype_new)
    ;

  py::class_<PointerType>(m, "PointerType", type)
    .def("pointee", &PointerType::getPointee, py::return_value_policy::reference_internal)
    .def("__call__", cpointerobj_new, py::keep_alive<0,1>())
    ;

  py::class_<ArrayType>(m, "ArrayType", type)
    .def("elementType", &ArrayType::getElementType, py::return_value_policy::reference_internal)
    ;

  py::class_<FunctionType>(m, "FunctionType", type)
    .def("returnType", &FunctionType::getReturnType, py::return_value_policy::reference_internal)
    .def("params", &FunctionType::getParams, py::return_value_policy::reference_internal)
    .def("getFunction", functiontype_getfunction, py::keep_alive<0,1>());
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
      return py::make_iterator<py::return_value_policy::reference_internal>(CTy.getFields());
     })
    .def("__dir__", [](CompositeType const& Ty) {
        auto const& Fields = Ty.getFields();
        return list_fields(Fields);
        }, py::return_value_policy::copy)
    ;

  py::class_<StructType>(m, "StructType", CompType)
    .def("__call__", structtype_new)
    ;
  py::class_<UnionType>(m, "UnionType", CompType);
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
    ;

  py::class_<CObj> cobj(m, "CObj");
  cobj.def("cast", &CObj::cast, py::keep_alive<0,1>())
      .def("type", &CObj::getType, py::return_value_policy::reference_internal)
      .def("size", &CObj::getSize)
      .def("align", &CObj::getAlign)
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
    
  DECL_CBASICOBJ_INT(uint8_t, "UInt8", "__int__");
  DECL_CBASICOBJ_INT(uint16_t, "UInt16", "__int__");
  DECL_CBASICOBJ_INT(uint32_t, "UInt32", "__int__");
  DECL_CBASICOBJ_INT(uint64_t, "UInt64", sizeof(long) <= sizeof(int64_t) ? "__int__":"__long__");
#ifdef DFFI_SUPPORT_I128
  DECL_CBASICOBJ_INT(__uint128_t, "UInt128", "__long__");
#endif
  DECL_CBASICOBJ_INT(int8_t, "Int8", "__int__");
  DECL_CBASICOBJ_INT(int16_t, "Int16", "__int__");
  DECL_CBASICOBJ_INT(int32_t, "Int32", "__int__");
  DECL_CBASICOBJ_INT(int64_t, "Int64", sizeof(long) <= sizeof(int64_t) ? "__int__":"__long__");
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
    .def("view", &CPointerObj::getMemoryView)
    .def_property_readonly("cstr", &CPointerObj::getMemoryViewCStr)
    ;

  // Composite object
  py::class_<CCompositeObj> PyCompObj(m, "CCompositeObj", py::buffer_protocol(), cobj);
  PyCompObj.def(py::init<CompositeType const&>(), py::keep_alive<1, 2>())
           .def("__setattr__", 
             (void(CCompositeObj::*)(const char*, py::handle)) &CCompositeObj::setValue)
           .def("__getattr__",
             (py::object(CCompositeObj::*)(const char*)) &CCompositeObj::getValue)
           .def("__dir__", [](CCompositeObj const& O) {
             auto const& Fields = O.getType()->getFields();
             return list_fields(Fields);
           }, py::return_value_policy::copy)
//           .def("__iter__", [](CCompositeObj const& O) {
//             return py::make_iterator<py::return_value_policy::reference_internal>(O.getType()->getFields());
//           })
           .def_buffer([](CCompositeObj& O) {
               return py::buffer_info{
                 O.getData(),
                 1,
                 py::format_descriptor<uint8_t>::format(),
                 1,
                 { O.getSize() },
                 { 1 }
               };
             })
           ;

  py::class_<CStructObj>(m, "CStructObj", py::buffer_protocol(), PyCompObj)
    .def(py::init<StructType const&>(), py::keep_alive<1, 2>())
    ;
  py::class_<CUnionObj>(m, "CUnionObj", py::buffer_protocol(), PyCompObj)
    .def(py::init<UnionType const&>(), py::keep_alive<1, 2>())
    ;

  // Array object
  py::class_<CArrayObj>(m, "CArrayObj", py::buffer_protocol(), cobj)
    .def(py::init<ArrayType const&>(), py::keep_alive<1, 2>())
    .def("set", &CArrayObj::set)
    .def("get", &CArrayObj::get)
    .def("elementType", &CArrayObj::getElementType, py::return_value_policy::reference_internal)
    .def_buffer([](CArrayObj& O) {
        auto* EltTy = O.getElementType();
        return py::buffer_info{
          O.getData(),
          // TODO: using ssize_t is a concern here!
          static_cast<ssize_t>(EltTy->getSize()),
          getFormatDescriptor(EltTy),
          static_cast<ssize_t>(O.getType()->getSize())
        };
      })
    ;

  py::class_<CFunction>(m, "CFunction", cobj)
    .def("call", &CFunction::call)
    .def("__call__", &CFunction::call)
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
    .def("getFunction", cu_getfunction, py::keep_alive<0,1>())
    .def("getStructType", &CompilationUnit::getStructType, py::return_value_policy::reference_internal)
    .def("getUnionType", &CompilationUnit::getUnionType, py::return_value_policy::reference_internal)
    .def("getEnumType", &CompilationUnit::getEnumType, py::return_value_policy::reference_internal)
    .def("getType", &CompilationUnit::getType, py::return_value_policy::reference_internal)
    ;

  py::class_<DFFI>(m, "FFI")
    .def(py::init(&default_ctor), py::arg("optLevel") = 2, py::arg("includeDirs") = py::list())
    .def("cdef", dffi_cdef, py::keep_alive<0,1>())
    .def("cdef", dffi_cdef_no_name, py::keep_alive<0,1>())
    .def("compile", dffi_compile, py::keep_alive<0,1>())
    .def("ptr", [](DFFI& D, CObj* O) {
      return std::unique_ptr<CPointerObj>{new CPointerObj{O}};
    }, py::keep_alive<0,1>(), py::keep_alive<0,2>())
    .def("ptr", [](DFFI&, Type const* Ty) {
      return PointerType::get(Ty);
    }, py::return_value_policy::reference_internal)
    .def("typeof", [](DFFI&, CObj const& O) { return O.getType(); }, py::return_value_policy::reference_internal)
    .def("sizeof", [](DFFI&, CObj const& O) { return O.getSize(); })
    .def("alignof", [](DFFI&, CObj const& O) { return O.getAlign(); })
    .def("view", dffi_view, py::keep_alive<0,1>(), py::keep_alive<0,2>())
    .def("basicType", 
      (BasicType const*(DFFI::*)(BasicType::BasicKind)) &DFFI::getBasicType,
      py::return_value_policy::reference_internal)
    .def("arrayType", &DFFI::getArrayType, py::return_value_policy::reference_internal)
    .def("pointerType", &DFFI::getPointerType, py::return_value_policy::reference_internal)
    .def("getFunction", dffi_getfunction, py::keep_alive<0,1>())

    // Basic values
    .def("Int8", createBasicObj<int8_t>, py::keep_alive<0,1>())
    .def("Int16", createBasicObj<int16_t>, py::keep_alive<0,1>())
    .def("Int32", createBasicObj<int32_t>, py::keep_alive<0,1>())
    .def("Int64", createBasicObj<int64_t>, py::keep_alive<0,1>())
#ifdef DFFI_SUPPORT_I128
    .def("Int128", createBasicObj<__int128_t>, py::keep_alive<0,1>())
#endif
    .def("UInt8", createBasicObj<uint8_t>, py::keep_alive<0,1>())
    .def("UInt16", createBasicObj<uint16_t>, py::keep_alive<0,1>())
    .def("UInt32", createBasicObj<uint32_t>, py::keep_alive<0,1>())
    .def("UInt64", createBasicObj<uint64_t>, py::keep_alive<0,1>())
#ifdef DFFI_SUPPORT_I128
    .def("UInt128", createBasicObj<__uint128_t>, py::keep_alive<0,1>())
#endif
    .def("Float", createBasicObj<float>, py::keep_alive<0,1>())
    .def("Double", createBasicObj<double>, py::keep_alive<0,1>())
    .def("LongDouble", createBasicObj<long double>, py::keep_alive<0,1>())

    // Type helpers
    .def_property_readonly("VoidTy", &DFFI::getVoidTy, py::return_value_policy::reference_internal)
    .def_property_readonly("BoolTy", &DFFI::getBoolTy, py::return_value_policy::reference_internal)
    .def_property_readonly("CharTy", &DFFI::getCharTy, py::return_value_policy::reference_internal)
    .def_property_readonly("Int8Ty", &DFFI::getInt8Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("Int16Ty", &DFFI::getInt16Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("Int32Ty", &DFFI::getInt32Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("Int64Ty", &DFFI::getInt64Ty, py::return_value_policy::reference_internal)
#ifdef DFFI_SUPPORT_I128
    .def_property_readonly("Int128Ty", &DFFI::getInt128Ty, py::return_value_policy::reference_internal)
#endif
    .def_property_readonly("UInt8Ty", &DFFI::getUInt8Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt16Ty", &DFFI::getUInt16Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt32Ty", &DFFI::getUInt32Ty, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt64Ty", &DFFI::getUInt64Ty, py::return_value_policy::reference_internal)
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
    .def_property_readonly("Int8PtrTy", &DFFI::getInt8PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("Int16PtrTy", &DFFI::getInt16PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("Int32PtrTy", &DFFI::getInt32PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("Int64PtrTy", &DFFI::getInt64PtrTy, py::return_value_policy::reference_internal)
#ifdef DFFI_SUPPORT_I128
    .def_property_readonly("Int128PtrTy", &DFFI::getInt128PtrTy, py::return_value_policy::reference_internal)
#endif
    .def_property_readonly("UInt8PtrTy", &DFFI::getUInt8PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt16PtrTy", &DFFI::getUInt16PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt32PtrTy", &DFFI::getUInt32PtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("UInt64PtrTy", &DFFI::getUInt64PtrTy, py::return_value_policy::reference_internal)
#ifdef DFFI_SUPPORT_I128
    .def_property_readonly("UInt128PtrTy", &DFFI::getUInt128PtrTy, py::return_value_policy::reference_internal)
#endif
    .def_property_readonly("FloatPtrTy", &DFFI::getFloatPtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("DoublePtrTy", &DFFI::getDoublePtrTy, py::return_value_policy::reference_internal)
    .def_property_readonly("LongDoublePtrTy", &DFFI::getLongDoublePtrTy, py::return_value_policy::reference_internal)
    ;


  m.def("dlopen", dffi_dlopen);

  // Exceptions
  py::register_exception<CompileError>(m, "CompileError");
  py::register_exception<UnknownFunctionError>(m, "UnknownFunctionError");
  py::register_exception<TypeError>(m, "TypeError");
  py::register_exception<DLOpenError>(m, "DLOpenError");
  py::register_exception<UnknownField>(m, "UnknownField");
  py::register_exception<AllocError>(m, "AllocError");
};
