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

#include "dffictx.h"

#include <dffi/types.h>
#include <dffi/composite_type.h>

using namespace dffi;

details::DFFICtx::DFFICtx()
{ }

details::DFFICtx::~DFFICtx()
{
  for (FunctionType* FTy: FunctionTys_) {
    delete FTy;
  }
  for (ArrayType* ATy: ArrayTys_) {
    delete ATy;
  }
}

BasicType* details::DFFICtx::getBasicType(DFFIImpl& Dffi, BasicType::BasicKind Kind)
{
  auto It = BasicTys_.emplace(std::piecewise_construct,
    std::forward_as_tuple(Kind),
    std::forward_as_tuple(BasicType{Dffi, Kind}));
  return &It.first->second;
}

PointerType* details::DFFICtx::getPtrType(DFFIImpl& Dffi, QualType Pointee)
{
  auto It = PointerTys_.find(Pointee);
  if (It != PointerTys_.end()) {
    return It->second.get();
  }
  std::unique_ptr<PointerType> Obj(new PointerType{Dffi, Pointee});
  auto* Ret = Obj.get();
  PointerTys_[Pointee] = std::move(Obj);
  return Ret;
}

ArrayType* details::DFFICtx::getArrayType(DFFIImpl& Dffi, QualType EltTy, uint64_t NElements)
{
  ArrayTypeKeyInfo::KeyTy K(EltTy, NElements);
  auto It = ArrayTys_.find_as(K);
  if (It != ArrayTys_.end()) {
    return *It;
  }
  auto* Ret = new ArrayType{Dffi, EltTy, NElements};
  ArrayTys_.insert(Ret);
  return Ret;
}

FunctionType* details::DFFICtx::getFunctionType(DFFIImpl& Dffi, QualType RetTy, llvm::ArrayRef<QualType> ParamsTy, CallingConv CC, bool VarArgs)
{
  FunctionTypeKeyInfo::KeyTy K(RetTy, ParamsTy, CC, VarArgs);
  auto It = FunctionTys_.find_as(K);
  if (It != FunctionTys_.end()) {
    return *It;
  }
  auto* Ret = new FunctionType{Dffi, RetTy, ParamsTy, CC, VarArgs};
  FunctionTys_.insert(Ret);
  return Ret;
}
