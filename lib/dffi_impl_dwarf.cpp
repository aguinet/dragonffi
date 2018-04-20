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

#include <dffi/types.h>
#include "dffi_impl.h"
#include "types_printer.h"

using namespace llvm;

void dump_die(DWARFDie const& D) { D.dump(llvm::outs(), 1); }

namespace dffi {
namespace details {

namespace {

using TypesCacheTy = DenseMap<uint64_t, QualType>;

enum class DWARFError {
  Skip = 0,
  InvalidTag,
  EncodingMissing,
  NameMissing,
  ByteSizeMissing,
  TypeMissing,
  FieldMissing,
  OffsetMissing,
  CountMissing,
  ValueMissing,
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


struct DWARFCUParser
{
  DWARFCUParser(CUImpl& CU):
    CU_(CU),
    Wrappers_(WrappersStr_)
  { }

  std::error_code parseCU(DWARFDie const& Die)
  {
    assert(Die.getTag() == dwarf::DW_TAG_compile_unit);
    // TODO: check language?
    // Cache indexes are CU-related, so clear it for each CU!
    Cache_.clear();

    for (auto Child: Die.children()) {
      auto Err = addTypeFromDIE(Child);
      if (Err) {
        return Err;
      }
    }
    return {};
  }

  TypePrinter& getPrinter() { return Printer_; }
  std::string& getWrappers() { return Wrappers_.str(); }

private:
  std::error_code addTypeFromDIE(DWARFDie const& Die)
  {
    auto ErrOrTy = getTypeFromDIE(Die);
    if (!ErrOrTy) {
      auto Err = ErrOrTy.getError();
      if (Err == make_error_code(DWARFError::Skip)) {
        return {};
      }
      errs() << "error " << Err.value() << " while parsing\n";
      Die.dump(errs(), 1);
      errs() << "========\n";
      return Err;
    }
    auto Ty = ErrOrTy.get();
    if (auto* DFTy = dyn_cast_or_null<dffi::FunctionType>(Ty.getType())) {
      auto Name = getDwarfFieldAsCString(Die, dwarf::DW_AT_name, DWARFError::NameMissing);
      if (!Name) {
        if (Name.getError() == make_error_code(DWARFError::NameMissing))
          return {};
        errs() << "unable to get function name!\n";
        return Name.getError();
      }
      auto& DFFI = getDFFI();
      CU_.FuncTys_[Name.get()] = DFTy;
      auto Id = DFFI.getFuncTypeWrapperId(DFTy);
      if (!Id.second) {
        DFFI.genFuncTypeWrapper(Printer_, Id.first, Wrappers_, DFTy, {});
      }
    }
    return {};
  }

  ErrorOr<dffi::QualType> getTypeFromDIE(DWARFDie const& Die);

  template <class Func, class DV>
  static auto getDwarfField(DWARFDie const& D, dwarf::Attribute Field, Func Transform, DV DefValue, DWARFError BadFormat)
    -> ErrorOr<typename decltype(Transform(DWARFFormValue{}))::value_type>
  {
    auto Opt = D.find(Field);
    if (!Opt) {
      return getDefaultValue(DefValue);
    }
    auto const& V = Opt.getValue();
    auto Ret = Transform(V);
    if (!Ret) {
      return make_error_code(BadFormat);
    }
    return Ret.getValue();
  }

  static std::error_code getDefaultValue(DWARFError Err) { return make_error_code(Err); }
  template <class V>
  static V getDefaultValue(V DefV) { return DefV; }

  template <class DV>
  static ErrorOr<uint64_t> getDwarfFieldAsUnsigned(DWARFDie const& D, dwarf::Attribute Field, DV&& DefValue)
  {
    return getDwarfField(D, Field, [](DWARFFormValue const& V) { return V.getAsUnsignedConstant(); }, std::forward<DV>(DefValue), DWARFError::ValueNotUnsigned);
  }

  template <class DV>
  static ErrorOr<int64_t> getDwarfFieldAsSigned(DWARFDie const& D, dwarf::Attribute Field, DV&& DefValue)
  {
    return getDwarfField(D, Field, [](DWARFFormValue const& V) { return V.getAsSignedConstant(); }, std::forward<DV>(DefValue), DWARFError::ValueNotUnsigned);
  }

  template <class DV>
  static ErrorOr<const char*> getDwarfFieldAsCString(DWARFDie const& D, dwarf::Attribute Field, DV&& DefValue)
  {
    return getDwarfField(D, Field, [](DWARFFormValue const& V) { return V.getAsCString(); }, std::forward<DV>(DefValue), DWARFError::ValueNotString);
  }

  template <class DV>
  ErrorOr<dffi::QualType> getDwarfFieldAsType(DWARFDie const& D, dwarf::Attribute Field, DV DefValue)
  {
    auto NewDie = D.getAttributeValueAsReferencedDie(Field);
    if (!NewDie) {
      return getDefaultValue(DefValue);
    }
    return getTypeFromDIE(NewDie);
  }

  DFFIImpl& getDFFI() { return CU_.DFFI_; }

private:
  CUImpl& CU_;
  std::string WrappersStr_;
  raw_string_ostream Wrappers_;
  TypePrinter Printer_;
  TypesCacheTy Cache_;
  size_t AnonID_ = 0;
};

ErrorOr<dffi::QualType> DWARFCUParser::getTypeFromDIE(DWARFDie const& Die)
{
  const auto Tag = Die.getTag();
  // First, parse tags that do not need caching
  switch (Tag) {
    case dwarf::DW_TAG_const_type:
    {
      auto Ty = getDwarfFieldAsType(Die, dwarf::DW_AT_type, dffi::QualType{nullptr});
      if (!Ty) {
        return Ty.getError();
      }
      return Ty->withConst();
    }
    case dwarf::DW_TAG_typedef:
    case dwarf::DW_TAG_formal_parameter:
    case dwarf::DW_TAG_volatile_type:
    case dwarf::DW_TAG_restrict_type:
      return getDwarfFieldAsType(Die, dwarf::DW_AT_type, dffi::QualType{nullptr});
    default:
      break;
  };

  auto ItCache = Cache_.find(Die.getOffset());
  if (ItCache != Cache_.end()) {
    return ItCache->second;
  }
  QualType QRetTy(nullptr);
  switch (Tag) {
    case dwarf::DW_TAG_array_type:
      {
      auto EltTy = getDwarfFieldAsType(Die, dwarf::DW_AT_type, DWARFError::TypeMissing);
      if (!EltTy) {
        return EltTy.getError();
      }
      auto Child = Die.getFirstChild();
      if (!Child || Child.getTag() != dwarf::DW_TAG_subrange_type) {
        return make_error_code(DWARFError::InvalidTag);
      }
      auto LowerBound = getDwarfFieldAsUnsigned(Child, dwarf::DW_AT_lower_bound, 0);
      if (!LowerBound) {
        return LowerBound.getError();
      }
      size_t Count;
      auto NElts = getDwarfFieldAsUnsigned(Child, dwarf::DW_AT_count, DWARFError::CountMissing);
      if (!NElts && NElts.getError() == make_error_code(DWARFError::CountMissing)) {
        auto HighBound = getDwarfFieldAsUnsigned(Child, dwarf::DW_AT_upper_bound, 0);
        if (!HighBound) {
          return HighBound.getError();
        }
        Count = HighBound.get()-LowerBound.get()+1;
      }
      else {
        Count = NElts.get() + LowerBound.get();
      }
      QRetTy = CU_.getArrayType(EltTy.get(), Count);
      break;
    }
    case dwarf::DW_TAG_pointer_type:
    {
      auto EltTy = getDwarfFieldAsType(Die, dwarf::DW_AT_type, dffi::QualType{nullptr});
      if (!EltTy) {
        return EltTy.getError();
      }
      QRetTy = CU_.getPointerType(EltTy.get());
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
        errs() << "base type size missing\n";
        return ByteSize.getError();
      }
      auto Name = getDwarfFieldAsCString(Die, dwarf::DW_AT_name, DWARFError::NameMissing);
      if (!Name) {
        return Name.getError();
      }
      QRetTy = CU_.getBasicTypeFromDWARF(Enc.get(), ByteSize.get() * CHAR_BIT, Name.get());
      break;
    }
    case dwarf::DW_TAG_subroutine_type:
    case dwarf::DW_TAG_subprogram:
    {
      auto Prototyped = getDwarfFieldAsUnsigned(Die, dwarf::DW_AT_prototyped, 0);
      if (!Prototyped) {
        return Prototyped.getError();
      }
      if (!Prototyped.get()) {
        // TODO: what to really do if not implemented?
        return make_error_code(DWARFError::Skip);
      }

      auto RetTy = getDwarfFieldAsType(Die, dwarf::DW_AT_type, dffi::QualType{nullptr});
      if (!RetTy) {
        return RetTy.getError();
      }
      llvm::SmallVector<QualType, 8> ParamsTy;
      for (auto Child: Die.children()) {
        if (Child.getTag() != dwarf::DW_TAG_formal_parameter) {
          continue;
        }
        auto ErrOrTy = getTypeFromDIE(Child);
        if (!ErrOrTy)
          return ErrOrTy.getError();
        ParamsTy.push_back(ErrOrTy.get());
      }

      // Get calling convention. Defaults to C!
      auto CCOrErr = getDwarfFieldAsUnsigned(Die, dwarf::DW_AT_calling_convention, 0);
      if (!CCOrErr) {
        return CCOrErr.getError();
      }
      const auto CC = dwarfCCToDFFI(CCOrErr.get());
      // TODO: varargs
      QRetTy = CU_.getFunctionType(RetTy.get(), ParamsTy, CC, false);
      break;
    }
    case dwarf::DW_TAG_enumeration_type:
    case dwarf::DW_TAG_union_type:
    case dwarf::DW_TAG_structure_type:
    {
      auto Name_ = getDwarfFieldAsCString(Die, dwarf::DW_AT_name, DWARFError::NameMissing);
      auto IsOpaque = getDwarfFieldAsUnsigned(Die, dwarf::DW_AT_declaration, 0);
      if (!IsOpaque) {
        return IsOpaque.getError();
      }

      std::string Name;
      if (!Name_) {
        if (Name_.getError() == make_error_code(DWARFError::NameMissing)) {
          // Anonymous union/struct
          auto ID = AnonID_++;
          std::stringstream ss;
          ss << "__dffi_anon_" << ID;
          Name = ss.str();
        }
        else
          return Name_.getError();
      }
      else {
        Name = Name_.get();
      }

      CanOpaqueType* Ret;
      auto ItFind = CU_.CompositeTys_.find(Name);

      if (ItFind != CU_.CompositeTys_.end()) {
        Ret = ItFind->second.get();
        if (Ret->isOpaque() && !IsOpaque.get()) {
          // Do the process
        }
        else {
          // TODO: check they are the same!
          return Ret;
        }
      }
      else {
        auto& DFFI = getDFFI();
        std::unique_ptr<CanOpaqueType> NewTy;
        switch (Tag) {
          case dwarf::DW_TAG_union_type:
            NewTy.reset(new UnionType{DFFI});
            break;
          case dwarf::DW_TAG_structure_type:
            NewTy.reset(new StructType{DFFI});
            break;
          case dwarf::DW_TAG_enumeration_type:
            NewTy.reset(new EnumType{DFFI});
            break;
        };
        Ret = NewTy.get();
        CU_.CompositeTys_[Name] = std::move(NewTy);
      }

      Cache_.insert(std::make_pair(Die.getOffset(), Ret));

      if (IsOpaque.get()) {
        return Ret;
      }

      if (Tag == dwarf::DW_TAG_enumeration_type) {
        EnumType::Fields Fields;
        for (auto Field: Die.children()) {
          if (Field.getTag() != dwarf::DW_TAG_enumerator) {
            continue;
          }
          auto Name = getDwarfFieldAsCString(Field, dwarf::DW_AT_name, DWARFError::NameMissing);
          if (!Name) {
            return Name.getError();
          }
          auto Value = getDwarfFieldAsSigned(Field, dwarf::DW_AT_const_value, DWARFError::ValueMissing);
          if (!Value) {
            return Value.getError();
          }
          Fields[Name.get()] = Value.get();
        }
        cast<EnumType>(Ret)->setBody(std::move(Fields));
      }
      else {
        std::vector<CompositeField> Fields;
        unsigned Align = 1;
        // Parse the structure! Every children must be a TAG_member
        for (auto Field: Die.children()) {
          if (Field.getTag() != dwarf::DW_TAG_member) {
            continue;
          }
          auto Name = getDwarfFieldAsCString(Field, dwarf::DW_AT_name, DWARFError::NameMissing);
          if (!Name) {
            return Name.getError();
          }
          auto Offset = getDwarfFieldAsUnsigned(Field, dwarf::DW_AT_data_member_location, DWARFError::OffsetMissing);
          if (!Offset) {
            // No offset in union is fine!
            if (Tag == dwarf::DW_TAG_union_type && Offset.getError() == make_error_code(DWARFError::OffsetMissing)) {
              Offset = 0;
            }
            else {
              return Offset.getError();
            }
          }
          auto FTy = getDwarfFieldAsType(Field, dwarf::DW_AT_type, DWARFError::TypeMissing);
          if (!FTy) {
            return FTy.getError();
          }
          Fields.emplace_back(CompositeField{Name.get(), FTy.get(), static_cast<unsigned>(Offset.get())});

          Align = std::max(Align, FTy->getType()->getAlign());
        }

        auto Size = getDwarfFieldAsUnsigned(Die, dwarf::DW_AT_byte_size, DWARFError::ByteSizeMissing);
        if (!Size) {
          errs() << "byte size missing in\n";
          Die.dump(errs(), 1);
          return Size.getError();
        }
        cast<CompositeType>(Ret)->setBody(std::move(Fields), Size.get(), Align);
      }

      // Return directly as we have already populated the cache!
      return Ret;
    }
    case dwarf::DW_TAG_dwarf_procedure:
    case dwarf::DW_TAG_variable:
      return make_error_code(DWARFError::Skip);
    default:
    {
      errs() << "tag: " << TagString(Tag) << "\n";
      return make_error_code(DWARFError::Skip);
    }
  }
  Cache_.insert(std::make_pair(Die.getOffset(), QRetTy));
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

  DWARFCUParser P(*NewCU);
  for (const auto &CU : DICtx->compile_units()) {
    auto Die = CU->getUnitDIE(false);
    if (!Die)
      continue;
    P.parseCU(Die);
  }

  compileWrappers(P.getPrinter(), P.getWrappers());

  auto* Ret = NewCU.get();
  CUs_.emplace_back(std::move(NewCU));
  return Ret;
}

} // impl
} // dffi
