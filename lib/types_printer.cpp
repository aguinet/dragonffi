#include "types_printer.h"

namespace dffi {

llvm::raw_ostream& TypePrinter::print_def(llvm::raw_ostream& OS, dffi::Type const* Ty, DeclMode DMode, const char* Name)
{
  if (Ty == nullptr) {
    OS << "void";
    if (Name) {
      OS << ' ';
      OS << Name;
    }
    return OS;
  }

  if (ViewEnumAsBasicType_ && isa<EnumType>(Ty)) {
    Ty = cast<EnumType>(Ty)->getBasicType();
  }
  switch (Ty->getKind()) {
  case dffi::Type::TY_Basic:
  {
    auto* BTy = cast<BasicType>(Ty);
    switch (BTy->getBasicKind()) {
      case BasicType::Bool:
        OS << "_Bool";
        break;
      case BasicType::Char:
        OS << "char";
        break;
      case BasicType::SChar:
        OS << "signed char";
        break;
      case BasicType::Short:
        OS << "short";
        break;
      case BasicType::Int:
        OS << "int";
        break;
      case BasicType::Long:
        OS << "long";
        break;
      case BasicType::LongLong:
        OS << "long long";
        break;
#ifdef DFFI_SUPPORT_I128
      case BasicType::Int128:
        OS << "__int128_t";
        break;
#endif
      case BasicType::UChar:
        OS << "unsigned char";
        break;
      case BasicType::UShort:
        OS << "unsigned short";
        break;
      case BasicType::UInt:
        OS << "unsigned int";
        break;
      case BasicType::ULong:
        OS << "unsigned long";
        break;
      case BasicType::ULongLong:
        OS << "unsigned long long";
        break;
#ifdef DFFI_SUPPORT_I128
      case BasicType::UInt128:
        OS << "__uint128_t";
        break;
#endif
      case BasicType::Float:
        OS << "float";
        break;
      case BasicType::Double:
        OS << "double";
        break;
      case BasicType::LongDouble:
        OS << "long double";
        break;
#ifdef DFFI_SUPPORT_COMPLEX
      case BasicType::ComplexFloat:
        OS << "_Complex float";
        break;
      case BasicType::ComplexDouble:
        OS << "_Complex double";
        break;
      case BasicType::ComplexLongDouble:
        OS << "_Complex long double";
        break;
#endif
    };
    if (Name)
      OS << ' ' << Name;
    return OS; 
  }
  case dffi::Type::TY_Pointer:
  {
    auto* PTy = cast<PointerType>(Ty);
    auto* Pointee = PTy->getPointee().getType();
    std::string PtrName = "(*";
    if (Name)
      PtrName += Name;
    PtrName += ")";
    return print_def(OS,Pointee,Forward,PtrName.c_str());
  }
  case dffi::Type::TY_Function:
  {
    auto* FTy = cast<FunctionType>(Ty);
    std::string Buf;
    llvm::raw_string_ostream ss(Buf);

    ss << '(' << CCToClangAttribute(FTy->getCC()) << ' ' << (Name ? Name:"") << ')';
    ss << '(';
    if (FTy->getParams().size() > 0) {
      auto const& Params = FTy->getParams();
      auto ItLast = --Params.end();
      for (auto It = Params.begin(); It != ItLast; ++It) {
        print_def(ss, *It, Full); 
        ss << ',';
      }
      print_def(ss, *ItLast, Full); 
      if (FTy->hasVarArgs()) {
        ss << ",...";
      }
    }
    ss << ')';
    return print_def(OS, FTy->getReturnType(), Full, ss.str().c_str());
  }
  case dffi::Type::TY_Struct:
  case dffi::Type::TY_Union:
  case dffi::Type::TY_Enum:
  {
    if (DMode == Full)
      add_decl(Ty);
    else
    if (DMode == Forward)
      add_forward_decl(Ty);

    if (isa<dffi::StructType>(Ty))
      OS << "struct ";
    else
    if (isa<dffi::UnionType>(Ty))
      OS << "union ";
    else
    if (isa<dffi::EnumType>(Ty))
      OS << "enum ";

    auto It = NamedTys_.find(Ty);
    if (It != NamedTys_.end()) {
      OS << It->second;
    }
    else {
      std::string NameTy = "__dffi_ty_" + std::to_string(NamedTys_.size());
      NamedTys_.insert(std::make_pair(Ty, NameTy));
      OS << NameTy;
    }
    if (Name) {
      OS << ' ' << Name;
    }
    return OS;
  }
  case dffi::Type::TY_Array:
  {
    auto* ArTy = cast<ArrayType>(Ty);
    // TODO: use Twine?
    std::string ArName;
    if (Name)
      ArName = Name;
    ArName += '[';
    ArName += std::to_string(ArTy->getNumElements());
    ArName += ']';
    print_def(OS, ArTy->getElementType(), Full, ArName.c_str());
    return OS;
  }
  default:
    break;
  };
  return OS;
}

void TypePrinter::add_decl(dffi::Type const* Ty)
{
  if (!isa<dffi::CanOpaqueType>(Ty)) { 
    return;
  }

  auto It = Declared_.insert(Ty);
  if (!It.second) {
    return;
  }

  std::string TmpDecl;
  llvm::raw_string_ostream SS(TmpDecl);
  if (auto* STy = dyn_cast<dffi::CompositeType>(Ty)) {
    print_decl_impl(SS, STy);
  }
  else
  if (auto* ETy = dyn_cast<dffi::EnumType>(Ty)) {
    print_decl_impl(SS, ETy);
  }
  Decls_ << SS.str() << '\n';
}

void TypePrinter::add_forward_decl(dffi::Type const* Ty)
{
  if (!isa<dffi::CanOpaqueType>(Ty)) {
    return;
  }
  if (Declared_.count(Ty))
    return;

  auto It = ForwardDeclared_.insert(Ty);
  if (!It.second) {
    return;
  }
  print_def(Decls_, Ty, None) << ";\n";
}

void TypePrinter::print_decl_impl(llvm::raw_string_ostream& ss, dffi::CompositeType const* Ty)
{
  print_def(ss, Ty, None) << " {\n";
  size_t Idx = 0;
  for (auto const& F: Ty->getOrgFields()) {
    std::string Name = "__Field_" + std::to_string(Idx);
    ss << "  ";
    print_def(ss, F.getType(), Full, Name.c_str()) << ";\n";
    ++Idx;
  }
  ss << "};\n";
}

void TypePrinter::print_decl_impl(llvm::raw_string_ostream& ss, dffi::EnumType const* Ty)
{
  print_def(ss, Ty, None) << " {\n";
  for (auto const& F: Ty->getFields()) {
    ss << "  " << F.first << " = " << F.second << ",\n";
  }
  ss << "};\n";
}


} // dffi
