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

dffi::Type const* getTypeFromDIE(DWARFDie const& Die)
{
  auto Tag = Die.getTag();
  switch (Tag) {
    case dwarf::DW_TAG_pointer_type:
      break;
    case dwarf::DW_TAG_base_type:
    {
      auto OptEnc = Die.find(dwarf::DW_AT_encoding);
      if (!OptEnc) {
        report_fatal_error("no encoding");
      }
      auto Enc = OptEnc->getAsUnsignedConstant();
      if (!Enc) {
        report_fatal_error("encoding not integer");
      }
      errs() << "basic type, encoding " << Enc.getValue() << "\n";
      break;
    }
    case dwarf::DW_TAG_subprogram:
    {
      auto OptName = Die.find(dwarf::DW_AT_name);
      if (!OptName) {
        report_fatal_error("no name");
      }
      auto Name = OptName->getAsCString();
      if (!Name) {
        report_fatal_error("function w/ no name!");
      }
      errs() << "function " << Name.getValue() << "\n";
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

  //std::unique_ptr<CUImpl> CU(new CUImpl{*this});

  for (const auto &CU : DICtx->compile_units()) {
    auto DIE = CU->getUnitDIE(false);
    if (!DIE)
      continue;
    assert(DIE.getTag() == dwarf::DW_TAG_compile_unit);
    // TODO: check language?

    //dump_die(DIE);
    DWARFDie Child = DIE.getFirstChild();
    while (Child) {
      //dump_die(Child);
      getTypeFromDIE(Child);
      Child = Child.getSibling();
    }

    //CU->dump(outs(), DIDumpOptions{});
  }
  return nullptr;
}

} // impl
} // dffi
