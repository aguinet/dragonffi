#include <limits>

#include <dffi/composite_type.h>
#include <dffi/types.h>
#include "dffi_types_hash.h"

namespace dffi {

using HashType = Type::HashType;

namespace {

HashType _hash_mix(HashType const A, HashType const B) {
  return A ^ (B + 0x9e3779b9 + (A << 6) + (A >> 2));
}

template <class... Hashes>
HashType hash_mix(HashType Org, Hashes... Hs) {
  auto R = details::reduce(_hash_mix, Org);
  auto Ret = (R << ... << Hs);
  return Ret();
}

template <class Iterator>
HashType hash_mix_it(HashType Org, Iterator Begin, Iterator End) {
  HashType Ret = Org;
  for (; Begin != End; ++Begin) {
    Ret = hash_mix(Ret, hash_value(*Begin));
  }
  return Ret;
}


} // anonymous

HashType hash_value(Type const* O)
{
  if (O == nullptr) {
    return 0xFF;
  }
  switch (O->getKind()) {
#define HANDLE_TY(Kind, Ty)\
    case Type::Kind:\
      return hash_value(cast<Ty>(O));

    HANDLE_TY(TY_Basic, BasicType)
    HANDLE_TY(TY_Pointer, PointerType)
    HANDLE_TY(TY_Function, FunctionType)
    HANDLE_TY(TY_Array, ArrayType)
    HANDLE_TY(TY_Struct, StructType)
    HANDLE_TY(TY_Union, UnionType)
    HANDLE_TY(TY_Enum, EnumType)
    default:
      break;
  }
  assert(false && "unhandled type!");
  return -1U;
}

HashType hash_value(BasicType const* O) 
{
  return hash_mix(static_cast<Type const*>(O)->getKind(), O->getBasicKind());
}

HashType hash_value(QualType O)
{
  return hash_mix(hash_value(O.getType()), O.getQualifiers());
}

HashType hash_value(PointerType const* O)
{
  return hash_mix(O->getKind(), hash_value(O->getPointee()));
}

HashType hash_value(FunctionType const* O)
{
  auto Args = hash_mix_it(0, std::begin(O->getParams()), std::end(O->getParams()));
  return hash_mix(O->getKind(), O->getFlagsRaw(), Args);
}

HashType hash_value(ArrayType const* O)
{
  return hash_mix(O->getKind(), hash_value(O->getElementType()), O->getNumElements());
}

HashType hash_value(CompositeField const& O)
{
  return hash_mix(std::hash<std::string>{}(O.getNameStr()), O.getOffset(), hash_value(O.getType()));
}

HashType hash_value(CanOpaqueType const* O)
{
  const auto Kind = O->getKind();
  if (Kind >= Type::TY_Composite && Kind <= Type::TY_CompositeEnd) {
    return hash_value(cast<CompositeType>(O));
  }
  static_assert((Type::TY_Enum == (Type::TY_CompositeEnd+1)) && (Type::TY_Enum == (Type::TY_CanOpaqueTypeEnd-1)),
    "type structure has changed, CanOpaqueType::hash_value must be updated to reflect this change!");
  return hash_value(cast<EnumType>(O));
}

HashType hash_value(CompositeType const* O)
{
  if (O->isOpaque()) {
    return hash_mix(O->getKind(), 0xDEADBEEF);
  }
  HashType Args = hash_mix_it(0, std::begin(O->getFields()), std::end(O->getFields()));
  for (auto const& F: O->getFields()) {
  }
  return hash_mix(O->getKind(), Args, O->getSize(), O->getAlign());
}

HashType hash_value(EnumType const* O)
{
  HashType Ret = 0;
  for (auto const& It: O->getFields()) {
    Ret = hash_mix(Ret, std::hash<std::string>{}(It.first), hash_mix(It.second));
  }
  return Ret;
}

} // dffi
