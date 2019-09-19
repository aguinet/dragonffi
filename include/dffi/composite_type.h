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

#ifndef DFFI_COMPOSITE_TYPE_H
#define DFFI_COMPOSITE_TYPE_H

#include <string>
#include <vector>
#include <cassert>
#include <unordered_map>

#include <dffi/exports.h>
#include <dffi/types.h>
#include <dffi/iterator_extras.h>
#include <dffi/iterator_range.h>

namespace dffi {
class CompositeField;
class CompositeType;
}
template struct DFFI_API std::hash<std::string>;
template class DFFI_API std::unordered_map<std::string, dffi::CompositeField const*>;
template class DFFI_API std::unordered_map<std::string, int>;

namespace dffi {

namespace details {
struct CUImpl;
} // details

class DFFI_API CompositeField
{
  friend struct details::CUImpl;
  friend class CompositeType;

public:
  CompositeField(CompositeField&&) = default;

  Type const* getType() const { return Ty_; }
  const char* getName() const { return Name_.c_str(); }
  
  unsigned getOffset() const { return Offset_; }
  bool anonymous() const; 

protected:
  CompositeField(const char* Name, Type const* Ty, unsigned Offset);
  CompositeField(CompositeField const&) = default;

private:
  std::string Name_;
  Type const* Ty_;
  unsigned Offset_;
};

class DFFI_API CanOpaqueType: public Type
{
public:
  CanOpaqueType(details::DFFIImpl& Dffi, TypeKind Ty);

  bool isOpaque() const { return IsOpaque_; }

  static bool classof(Type const* T) {
    const auto Kind = T->getKind();
    return Kind > TY_CanOpaqueType && Kind < TY_CanOpaqueTypeEnd;
  }

protected:
  void setAsDefined() { IsOpaque_ = false; }
private:
  bool IsOpaque_;
};

class DFFI_API CompositeType: public CanOpaqueType
{
  friend struct details::CUImpl;
  friend struct details::DFFICtx;

public:
  uint64_t getSize() const override { return Size_; }
  unsigned getAlign() const override { return Align_; }

  // Original fields as they have been parsed.
  std::vector<CompositeField> const& getOrgFields() const { return Fields_; }

  // Get information about the fields, after inlining.
  auto getFieldsName() const {
    const auto GetName = [](auto const& It) { return It.first.c_str(); };
    return make_range(map_iterator(FieldsMap_.begin(), GetName), map_iterator(FieldsMap_.end(), GetName));
  }
  auto getFields() const {
    const auto GetField = [](auto const& It) -> CompositeField const& { return *It.second; };
    return make_range(map_iterator(FieldsMap_.begin(), GetField), map_iterator(FieldsMap_.end(), GetField));
  }
  size_t getFieldsCount() const { return FieldsMap_.size(); }

  // Get a field from its name. Take into account the inlined names.
  CompositeField const* getField(const char* Name) const;

  static bool classof(Type const* T) {
    const auto Kind = T->getKind();
    return Kind > TY_Composite && Kind < TY_CompositeEnd;
  }

  CompositeType(CompositeType&&) = default;

protected:
  // Generate opaque composite type
  CompositeType(details::DFFIImpl& Dffi, TypeKind Ty);
  CompositeType(CompositeType const&) = delete;

  void setBody(std::vector<CompositeField>&& Fields, uint64_t Size, unsigned Align);
  void inlineAnonymousMembers();

protected:
  using FieldsMapTy = std::unordered_map<std::string, CompositeField const*>;
  std::vector<CompositeField> Fields_;
  std::vector<CompositeField> InlinedFields_;
  // This contains the "view" with inline fields.
  FieldsMapTy FieldsMap_;
  uint64_t Size_;
  unsigned Align_;
};

class DFFI_API StructType: public CompositeType
{
  friend struct details::CUImpl;
  friend struct details::DFFICtx;

public:
  // Generate opaque structure
  StructType(details::DFFIImpl& Dffi):
    CompositeType(Dffi, TY_Struct)
  { }

  static bool classof(Type const* T) {
    return T->getKind() == TY_Struct;
  }

protected:
  StructType(StructType const&) = delete;
  StructType(StructType&&) = default;
};

class DFFI_API UnionType: public CompositeType
{
  friend struct details::CUImpl;
  friend struct details::DFFICtx;

public:
  // Generate opaque structure
  UnionType(details::DFFIImpl& Dffi):
    CompositeType(Dffi, TY_Union)
  { }

  static bool classof(Type const* T) {
    return T->getKind() == TY_Union;
  }

protected:
  UnionType(UnionType const&) = delete;
  UnionType(UnionType&&) = default;

  void setBody(std::vector<CompositeField>&& Fields, uint64_t Size, unsigned Align);
};

class DFFI_API EnumType: public CanOpaqueType
{
public:
  using IntType = int;
  using Fields = std::unordered_map<std::string, IntType>;

  friend struct details::CUImpl;
  friend struct details::DFFICtx;

  // Generate opaque enum
  EnumType(details::DFFIImpl& Dffi):
    CanOpaqueType(Dffi, TY_Enum)
  { }

  static bool classof(Type const* T) {
    return T->getKind() == TY_Enum;
  }

  Fields const& getFields() const { return Fields_; }

  unsigned getAlign() const override { return sizeof(IntType); }
  uint64_t getSize() const override { return sizeof(IntType); }

  BasicType const* getBasicType() const;

protected:
  EnumType(EnumType const&) = delete;
  EnumType(EnumType&&) = default;

  void setBody(Fields&& Fields);

private:
  Fields Fields_;
};

} // dffi

#endif
