#include <memory>
#include <string>

#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Error.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/Binary.h>
#include <llvm/DebugInfo/DIContext.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/BinaryFormat/Dwarf.h>

#include "dffi_impl.h"
#include "types_printer.h"

using namespace llvm;

void dump_die(DWARFDie const& D) { D.dump(llvm::outs(), 1); }

namespace dffi {
namespace details {

namespace {

using TypesCacheTy = DenseMap<uint64_t, QualType>;

enum class DWARFError {
  NoError = 0,
  EncodingMissing,
  NameMissing,
  ByteSizeMissing,
  TypeMissing,
  FieldMissing,
  ValueNotUnsigned,
  ValueNotString,
};

class DWARFErrorCategory: public std::error_category
{
  const char* name() const noexcept override { return "DWARF parsing"; }
  std::string message(int ev) const noexcept override {
    return "TODO";
  }
};

const DWARFErrorCategory g_DWARFerrorCategory;

std::error_code make_error_code(DWARFError Err) {
  return {static_cast<int>(Err), g_DWARFerrorCategory};
}

ErrorOr<dffi::QualType> getTypeFromDIE(CUImpl& CU, DWARFDie const& Die, TypesCacheTy& Cache);

template <class Func>
auto getDwarfField(DWARFDie const& D, dwarf::Attribute Field, Func Transform, DWARFError NotFound, DWARFError BadFormat)
  -> ErrorOr<typename decltype(Transform(DWARFFormValue{}))::value_type>
{
  auto Opt = D.find(Field);
  if (!Opt) {
    return make_error_code(NotFound);
  }
  auto const& V = Opt.getValue();
  auto Ret = Transform(V);
  if (!Ret) {
    return make_error_code(BadFormat);
  }
  return Ret.getValue();
}

ErrorOr<uint64_t> getDwarfFieldAsUnsigned(DWARFDie const& D, dwarf::Attribute Field, DWARFError NotFound)
{
  return getDwarfField(D, Field, [](DWARFFormValue const& V) { return V.getAsUnsignedConstant(); }, NotFound, DWARFError::ValueNotUnsigned);
}

ErrorOr<const char*> getDwarfFieldAsCString(DWARFDie const& D, dwarf::Attribute Field, DWARFError NotFound)
{
  return getDwarfField(D, Field, [](DWARFFormValue const& V) { return V.getAsCString(); }, NotFound, DWARFError::ValueNotString);
}

ErrorOr<dffi::QualType> getDwarfFieldAsType(CUImpl& CU, TypesCacheTy& Cache, DWARFDie const& D, dwarf::Attribute Field, DWARFError NotFound)
{
  auto NewDie = D.getAttributeValueAsReferencedDie(Field);
  if (!NewDie) {
    return make_error_code(NotFound);
  }
  return getTypeFromDIE(CU, NewDie, Cache);
}

ErrorOr<dffi::QualType> getTypeFromDIE(CUImpl& CU, DWARFDie const& Die, TypesCacheTy& Cache)
{
  auto ItCache = Cache.find(Die.getOffset());
  if (ItCache != Cache.end()) {
    return ItCache->second;
  }

  auto Tag = Die.getTag();
  QualType QRetTy(nullptr);
  switch (Tag) {
    case dwarf::DW_TAG_pointer_type:
      break;
    case dwarf::DW_TAG_formal_parameter:
    {
      auto Ty = getDwarfFieldAsType(CU, Cache, Die, dwarf::DW_AT_type, DWARFError::TypeMissing);
      if (!Ty) {
        return Ty.getError();
      }
      QRetTy = Ty.get();
      break;
    }
    case dwarf::DW_TAG_base_type:
    {
      auto Enc = getDwarfFieldAsUnsigned(Die, dwarf::DW_AT_encoding, DWARFError::EncodingMissing);
      if (!Enc) {
        return Enc.getError();
      }
      auto ByteSize = getDwarfFieldAsUnsigned(Die, dwarf::DW_AT_byte_size, DWARFError::ByteSizeMissing);
      if (!ByteSize) {
        return ByteSize.getError();
      }
      auto Name = getDwarfFieldAsCString(Die, dwarf::DW_AT_name, DWARFError::NameMissing);
      if (!Name) {
        return Name.getError();
      }
      QRetTy = CU.getBasicTypeFromDWARF(Enc.get(), ByteSize.get() * CHAR_BIT, Name.get());
      break;
    }
    case dwarf::DW_TAG_subprogram:
    {
      // TODO: what to do if not implemented?
#if 0
      auto Prototyped = getDwarfFieldAsUnsigned(dwarf::DW_AT_prototyped, DWARFError::FieldMissing);
      if (!Prototyed) {
        return Prototyped.getError();
      }
      if (!Prototyped.get()) {
        return nullptr;
      }
#endif
      auto RetTy = getDwarfFieldAsType(CU, Cache, Die, dwarf::DW_AT_type, DWARFError::TypeMissing);
      if (!RetTy) {
        return RetTy.getError();
      }
      llvm::SmallVector<QualType, 8> ParamsTy;
      for (auto Child: Die.children()) {
        auto ErrOrTy = getTypeFromDIE(CU, Child, Cache);
        if (!ErrOrTy)
          return ErrOrTy.getError();
        ParamsTy.push_back(ErrOrTy.get());
      }

      // Get calling convention. Defaults to C!
      auto CCOrErr = getDwarfFieldAsUnsigned(Die, dwarf::DW_AT_calling_convention, DWARFError::NoError);
      auto CC = CC_C;
      if (!CCOrErr) {
        auto Err = CCOrErr.getError();
        if (Err != make_error_code(DWARFError::NoError)) {
          return Err;
        }
      }
      else {
        CC = dwarfCCToDFFI(CCOrErr.get());
      }
      // TODO: varargs
      QRetTy = CU.getFunctionType(RetTy.get(), ParamsTy, CC, false);
      break;
    }
    default:
      errs() << "tag: " << TagString(Tag) << "\n";
      return nullptr;
  }
  Cache.insert(std::make_pair(Die.getOffset(), QRetTy));
  return QRetTy;
}

}

CUImpl* DFFIImpl::from_dwarf(StringRef const Path, std::string& Err)
{
  ErrorOr<std::unique_ptr<MemoryBuffer>> BufOrErr =
      MemoryBuffer::getFile(Path);
  if (!BufOrErr) {
    Err = BufOrErr.getError().message();
    return nullptr;
  }
  std::unique_ptr<MemoryBuffer> Buf(std::move(BufOrErr.get()));

  Expected<std::unique_ptr<object::Binary>> BinOrErr =
      object::createBinary(Buf->getMemBufferRef());
  if (!BinOrErr) {
    Err = errorToErrorCode(BinOrErr.takeError()).message();
    return nullptr;
  }

  std::unique_ptr<object::Binary> B(std::move(BinOrErr.get()));
  if (!isa<object::ObjectFile>(B.get())) {
    Err = "invalid file format!";
    return nullptr;
  }
  auto* Obj = cast<object::ObjectFile>(B.get());
  std::unique_ptr<DWARFContextInMemory> DICtx(new DWARFContextInMemory(*Obj));

  std::unique_ptr<CUImpl> NewCU(new CUImpl{*this});

  std::stringstream Wrappers;
  TypePrinter Printer;
  for (const auto &CU : DICtx->compile_units()) {
    TypesCacheTy TypesCache;
    auto Die = CU->getUnitDIE(false);
    if (!Die)
      continue;
    assert(Die.getTag() == dwarf::DW_TAG_compile_unit);
    // TODO: check language?

    for (auto Child: Die.children()) {
      auto ErrOrTy = getTypeFromDIE(*NewCU, Child, TypesCache);
      if (!ErrOrTy) {
        errs() << "error: " << ErrOrTy.getError().value() << "\n";
        continue;
      }
      // Depending on the type, save the associated names!
      // TODO: isSubroutineDie?
      if (Child.isSubprogramDIE()) {
        auto Name = getDwarfFieldAsCString(Child, dwarf::DW_AT_name, DWARFError::NameMissing);
        if (!Name) {
          errs() << "unable to get function name!\n";
          continue;
        }
        auto* DFTy = cast<FunctionType const>(ErrOrTy->getType());
        NewCU->FuncTys_[Name.get()] = DFTy;
        auto Id = getFuncTypeWrapperId(DFTy);
        if (!Id.second) {
          genFuncTypeWrapper(Printer, Id.first, Wrappers, DFTy, {});
        }
      }
    }
  }

  compileWrappers(Printer, Wrappers.str());

  auto* Ret = NewCU.get();
  CUs_.emplace_back(std::move(NewCU));
  return Ret;
}

} // impl
} // dffi
