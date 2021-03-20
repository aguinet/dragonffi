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

#ifndef DFFI_DFFICTX_H
#define DFFI_DFFICTX_H

#include <memory>
#include <unordered_map>
#include <map>

#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/SmallVector.h>

#include <dffi/cc.h>
#include <dffi/casting.h>
#include <dffi/types.h>

namespace llvm {

template <>
struct DenseMapInfo<dffi::QualType>: public DenseMapInfo<dffi::Type*>
{
  static unsigned getHashValue(const dffi::QualType& QTy) {
    return std::hash<dffi::QualType>{}(QTy);
  }

  static bool isEqual(const dffi::QualType& LHS, const dffi::QualType& RHS)
  { return LHS == RHS; }
};

} // llvm

namespace dffi {

namespace details {

struct FunctionTypeKeyInfo
{
  struct KeyTy {
    QualType RetTy_;
    llvm::ArrayRef<QualType> ParamsTy_;
    union {
      struct {
        uint8_t CC: 6;
        uint8_t VarArgs: 1;
        uint8_t UseLastError: 1;
      } D;
      uint8_t V;
    } Flags_;

    KeyTy(QualType RetTy, llvm::ArrayRef<QualType> ParamsTy, CallingConv CC, bool hasVarArgs, bool useLastError):
      RetTy_(RetTy),
      ParamsTy_(ParamsTy)
    {
      Flags_.D.CC = CC;
      Flags_.D.VarArgs = hasVarArgs;
      Flags_.D.UseLastError = useLastError;
    }

    KeyTy(FunctionType const* FT):
      KeyTy(FT->getReturnType(), FT->getParams(), FT->getCC(), FT->hasVarArgs(), FT->useLastError())
    { }

    bool operator==(KeyTy const& O) const {
      if (Flags_.V != O.Flags_.V) 
        return false;
      if (RetTy_ != O.RetTy_) 
        return false;
      return ParamsTy_ == O.ParamsTy_;
    }

    bool operator!=(KeyTy const& O) const {
      return !(*this == O);
    }
  };

  static inline dffi::FunctionType* getEmptyKey() {
    return llvm::DenseMapInfo<dffi::FunctionType*>::getEmptyKey();
  }

  static inline dffi::FunctionType* getTombstoneKey() {
    return llvm::DenseMapInfo<dffi::FunctionType*>::getTombstoneKey();
  }

  static unsigned getHashValue(const KeyTy& Key) {
    unsigned Hash = std::hash<QualType>{}(Key.RetTy_);
    for (auto const& PTy: Key.ParamsTy_) {
      Hash = (Hash << 11) | (Hash >> (sizeof(unsigned)*8-11));
      Hash ^= std::hash<QualType>{}(PTy);
    }
    Hash ^= Key.Flags_.V;
    return Hash;
  }

  static unsigned getHashValue(const FunctionType *FT) {
    return getHashValue(KeyTy(FT));
  }

  static bool isEqual(const KeyTy& LHS, const FunctionType *RHS) {
    if (RHS == getEmptyKey() || RHS == getTombstoneKey())
      return false;
    return LHS == KeyTy(RHS);
  }

  static bool isEqual(const FunctionType *LHS, const FunctionType *RHS) {
    return LHS == RHS;
  }
};

struct ArrayTypeKeyInfo
{
  struct KeyTy {
    QualType EltTy_;
    uint64_t NumElts_;

    KeyTy(QualType EltTy, uint64_t NumElts):
      EltTy_(EltTy),
      NumElts_(NumElts)
    { }

    KeyTy(ArrayType const* AT):
      KeyTy(AT->getElementType(), AT->getNumElements())
    { }

    bool operator==(KeyTy const& O) const {
      return (EltTy_ == O.EltTy_) && (NumElts_ == O.NumElts_);
    }

    bool operator!=(KeyTy const& O) const {
      return !(*this == O);
    }
  };

  static inline dffi::ArrayType* getEmptyKey() {
    return llvm::DenseMapInfo<dffi::ArrayType*>::getEmptyKey();
  }

  static inline dffi::ArrayType* getTombstoneKey() {
    return llvm::DenseMapInfo<dffi::ArrayType*>::getTombstoneKey();
  }

  static unsigned getHashValue(const KeyTy& Key) {
    unsigned Hash = std::hash<QualType>{}(Key.EltTy_);
    Hash ^= Key.NumElts_ ^ (Key.NumElts_ >> 32);
    return Hash;
  }

  static unsigned getHashValue(const ArrayType *FT) {
    return getHashValue(KeyTy(FT));
  }

  static bool isEqual(const KeyTy& LHS, const ArrayType *RHS) {
    if (RHS == getEmptyKey() || RHS == getTombstoneKey())
      return false;
    return LHS == KeyTy(RHS);
  }

  static bool isEqual(const ArrayType *LHS, const ArrayType *RHS) {
    return LHS == RHS;
  }
};

struct ArrayKey
{
  QualType EltTy_;
  uint64_t NElts_;

  bool operator<(ArrayKey const& O) const {
    if (EltTy_ < O.EltTy_) {
      return true;
    }
    if (EltTy_ > O.EltTy_) {
      return false;
    }
    return NElts_ < O.NElts_;
  }
};

struct DFFICtx
{
  DFFICtx();
  DFFICtx(DFFICtx const&) = delete;

  ~DFFICtx();

  BasicType* getBasicType(DFFIImpl& Dffi, BasicType::BasicKind Kind);
  PointerType* getPtrType(DFFIImpl& Dffi, QualType Pointee);
  FunctionType* getFunctionType(DFFIImpl& Dffi, QualType RetTy, llvm::ArrayRef<QualType> ParamsTy, CallingConv CC, bool VarArgs, bool UseLastError);
  ArrayType* getArrayType(DFFIImpl& Dffi, QualType EltTy, uint64_t NElements);

private:
  std::map<BasicType::BasicKind, BasicType> BasicTys_;
  llvm::DenseMap<QualType, std::unique_ptr<PointerType>> PointerTys_;
  llvm::DenseSet<FunctionType*, FunctionTypeKeyInfo> FunctionTys_;
  llvm::DenseSet<ArrayType*, ArrayTypeKeyInfo> ArrayTys_;
};

} // details
} // dffi

#endif
