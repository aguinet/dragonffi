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

using namespace llvm;

void dump_die(DWARFDie const& D) { D.dump(llvm::outs(), 1); }

namespace dffi {
namespace details {

namespace {

enum class DWARFError {
  EncodingMissing,
  NameMissing,
  ByteSizeMissing,
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

ErrorOr<dffi::QualType> getTypeFromDIE(CUImpl& CU, DWARFDie const& Die)
{
  auto Tag = Die.getTag();
  switch (Tag) {
    case dwarf::DW_TAG_pointer_type:
      break;
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
      return CU.getBasicTypeFromDWARF(Enc.get(), ByteSize.get(), Name.get());
    }
    case dwarf::DW_TAG_subprogram:
    {
      auto Name = getDwarfFieldAsCString(Die, dwarf::DW_AT_name, DWARFError::NameMissing);
      if (!Name) {
        return Name.getError();
      }
      errs() << "function " << Name.get() << "\n";
      break;
    }
    default:
      errs() << "tag: " << TagString(Tag) << "\n";
      break;
  }
  return nullptr;
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

  for (const auto &CU : DICtx->compile_units()) {
    auto DIE = CU->getUnitDIE(false);
    if (!DIE)
      continue;
    assert(DIE.getTag() == dwarf::DW_TAG_compile_unit);
    // TODO: check language?

    DWARFDie Child = DIE.getFirstChild();
    while (Child) {
      auto ErrOrTy = getTypeFromDIE(*NewCU, Child);
      if (!ErrOrTy)
        continue;
      // TODO: add function type!
      Child = Child.getSibling();
    }

    //CU->dump(outs(), DIDumpOptions{});
  }
  return nullptr;
}

} // impl
} // dffi
