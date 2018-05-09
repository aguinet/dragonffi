#ifndef DFFI_TYPES_HASH_H
#define DFFI_TYPES_HASH_H

#include <dffi/types.h>
#include <dffi/exports.h>

namespace dffi {

DFFI_API Type::HashType hash_value(Type const*);
Type::HashType hash_value(BasicType const*);
Type::HashType hash_value(QualType);
Type::HashType hash_value(PointerType const*);
Type::HashType hash_value(FunctionType const*);
Type::HashType hash_value(ArrayType const*);
Type::HashType hash_value(CompositeField const&);
Type::HashType hash_value(CanOpaqueType const*);
Type::HashType hash_value(CompositeType const*);
Type::HashType hash_value(EnumType const*);

} // dffi

#endif
