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

#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/DriverDiagnostic.h>
#include <clang/Driver/Options.h>
#include <clang/Driver/ToolChain.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Frontend/TextDiagnosticBuffer.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/Utils.h>
#include <clang/FrontendTool/Utils.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/Option/Arg.h>
#include <llvm/Option/ArgList.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Object/ObjectFile.h>

#include <dffi/config.h>
#include <dffi/ctypes.h>
#include <dffi/dffi.h>
#include <dffi/types.h>
#include <dffi/composite_type.h>
#include <dffi/casting.h>
#include "dffi_impl.h"
#include "types_printer.h"

using namespace llvm;
using namespace clang;


namespace dffi {

static CallingConv dwarfCCToDFFI(uint8_t DwCC)
{
  // This is the inverse of the getDwarfCC function in clang/lib/CodeGen/CGDebugInfo.cpp!
  switch (DwCC) {
    case 0:
      return CC_C;
    case dwarf::DW_CC_BORLAND_stdcall:
      return CC_X86StdCall;
    case dwarf::DW_CC_BORLAND_msfastcall:
      return CC_X86FastCall;
    case dwarf::DW_CC_BORLAND_thiscall:
      return CC_X86ThisCall;
    case dwarf::DW_CC_LLVM_vectorcall:
      return CC_X86VectorCall;
    case dwarf::DW_CC_BORLAND_pascal:
      return CC_X86Pascal;
    case dwarf::DW_CC_LLVM_Win64:
      return CC_Win64;
    case dwarf::DW_CC_LLVM_X86_64SysV:
      return CC_X86_64SysV;
    case dwarf::DW_CC_LLVM_AAPCS:
      return CC_AAPCS;
    case dwarf::DW_CC_LLVM_AAPCS_VFP:
      return CC_AAPCS_VFP;
    case dwarf::DW_CC_LLVM_IntelOclBicc:
      return CC_IntelOclBicc;
    case dwarf::DW_CC_LLVM_SpirFunction:
      return CC_SpirFunction;
    case dwarf::DW_CC_LLVM_OpenCLKernel:
      return CC_OpenCLKernel;
    case dwarf::DW_CC_LLVM_Swift:
      return CC_Swift;
    case dwarf::DW_CC_LLVM_PreserveMost:
      return CC_PreserveMost;
    case dwarf::DW_CC_LLVM_PreserveAll:
      return CC_PreserveAll;
    case dwarf::DW_CC_LLVM_X86RegCall:
      return CC_X86RegCall;
  }
  assert(false && "unknown calling convention!");
  return CC_C;
}

namespace details {

namespace {
const char* WrapperPrefix = "__dffi_wrapper_";

llvm::DIType const* getCanonicalDIType(llvm::DIType const* Ty)
{
  // Go throught every typedef and returns the "original" type!
  if (auto* DTy = llvm::dyn_cast_or_null<DIDerivedType>(Ty)) {
    switch (DTy->getTag()) {
      case dwarf::DW_TAG_typedef:
      case dwarf::DW_TAG_const_type:
      case dwarf::DW_TAG_volatile_type:
      case dwarf::DW_TAG_restrict_type:
        return getCanonicalDIType(DTy->getBaseType().resolve());
      default:
        break;
    };
  }
  return Ty;
}

llvm::DIType const* stripDITypePtr(llvm::DIType const* Ty)
{
  auto* PtrATy = llvm::cast<DIDerivedType>(Ty);
  assert(PtrATy->getTag() == llvm::dwarf::DW_TAG_pointer_type && "must be a pointer type!");
  return PtrATy->getBaseType().resolve();
}

// Inspired by work from Juan Manuel Martinez!
void InitHeaderSearchFlags(std::string const& TripleStr,
                          CCOpts const& Opts,
                          HeaderSearchOptions &HSO) {

  using namespace llvm::sys;

  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts(new DiagnosticOptions());
  auto *DiagsBuffer = new IgnoringDiagConsumer();
  std::unique_ptr<DiagnosticsEngine> Diags(new DiagnosticsEngine(DiagID, &*DiagOpts, DiagsBuffer));

  SmallVector<const char*, 3> Args = {"dummy.c", 
    "-target", TripleStr.c_str(), "-resource-dir", getClangResRootDirectory()};

  // Build a dummy compilation to obtain the current toolchain.
  // Indeed, the BuildToolchain function of clang::driver::Driver is private :/
  driver::Driver D("dummy", TripleStr, *Diags);
  std::unique_ptr<driver::Compilation> C(D.BuildCompilation(Args));

  driver::ToolChain const &TC = C->getDefaultToolChain();

  llvm::opt::ArgStringList IncludeArgs;
  TC.AddClangSystemIncludeArgs(C->getArgs(), IncludeArgs);

  //HSO.Sysroot = ParentHSO.Sysroot;
  HSO.ResourceDir = getClangResRootDirectory();

  // organized in pairs "-<flag> <directory>"
  assert(((IncludeArgs.size() & 1) == 0) && "even number of IncludeArgs");
  HSO.UserEntries.reserve(IncludeArgs.size()/2 + Opts.IncludeDirs.size());
  for (size_t i = 0; i != IncludeArgs.size(); i += 2) {
    auto &Directory = IncludeArgs[i+1];

    auto IncludeType = frontend::System;
    if (IncludeArgs[i] == StringRef("-internal-externc-isystem"))
      IncludeType = frontend::ExternCSystem;

    HSO.UserEntries.emplace_back(Directory, IncludeType, false, false);
  }

  for (auto const& D: Opts.IncludeDirs) {
    HSO.UserEntries.emplace_back(D, frontend::System, false, false);
  }
}

std::string getWrapperName(size_t Idx)
{
  return std::string{WrapperPrefix} + std::to_string(Idx);
}

} // anonymous

DFFIImpl::DFFIImpl(CCOpts const& Opts):
    Clang_(new CompilerInstance()),
    DiagID_(new DiagnosticIDs()),
    DiagOpts_(new DiagnosticOptions()),
    ErrorMsgStream_(ErrorMsg_),
    VFS_(new llvm::vfs::InMemoryFileSystem{}),
    Opts_(Opts)
{
  // Add an overleay with our virtual file system on top of the system!
  vfs::OverlayFileSystem* Overlay = new llvm::vfs::OverlayFileSystem{vfs::getRealFileSystem()};
  Overlay->pushOverlay(VFS_);
  // Finally add clang's ressources
  Overlay->pushOverlay(getClangResFileSystem());
  FileMgr_ = new FileManager{FileSystemOptions{}, Overlay};

  setNewDiagnostics();
  Clang_->setFileManager(FileMgr_.get());
  auto& CI = Clang_->getInvocation();


  auto& TO = CI.getTargetOpts();
  TO.Triple = llvm::sys::getProcessTriple();
  // We create it by hand to have a minimal user-friendly API!
  // From Juan's code!
  auto& CGO = CI.getCodeGenOpts();
  CGO.OptimizeSize = false;
  CGO.OptimizationLevel = Opts.OptLevel;
  CGO.CodeModel = "default";
  CGO.RelocationModel = llvm::Reloc::PIC_;
  CGO.ThreadModel = "posix";
  // We use debug info for type recognition!
  CGO.setDebugInfo(codegenoptions::FullDebugInfo);

  CI.getDiagnosticOpts().ShowCarets = false;

  CI.getLangOpts()->LineComment = true;
  CI.getLangOpts()->Optimize = true;
  CI.getLangOpts()->CPlusPlus = false;
  CI.getLangOpts()->C99 = true;
  CI.getLangOpts()->C11 = true;
  CI.getLangOpts()->CPlusPlus11 = false;
  CI.getLangOpts()->CPlusPlus14 = false;
  CI.getLangOpts()->CXXExceptions = false;
  CI.getLangOpts()->CXXOperatorNames = false;
  CI.getLangOpts()->Bool = false;
  CI.getLangOpts()->WChar = false; // builtin in C++, typedef in C (stddef.h)
  CI.getLangOpts()->EmitAllDecls = true;

  const bool IsWinMSVC = Triple{TO.Triple}.isWindowsMSVCEnvironment();
  CI.getLangOpts()->MSVCCompat = IsWinMSVC;
  CI.getLangOpts()->MicrosoftExt = IsWinMSVC;
  CI.getLangOpts()->AsmBlocks = IsWinMSVC;
  CI.getLangOpts()->DeclSpecKeyword = IsWinMSVC;
  CI.getLangOpts()->MSBitfields = IsWinMSVC;

  // gnu compatibility
  CI.getLangOpts()->GNUMode = true;
  CI.getLangOpts()->GNUKeywords = true;
  CI.getLangOpts()->GNUAsm = true;

  CI.getFrontendOpts().ProgramAction = frontend::EmitLLVMOnly;

  auto& HSO = CI.getHeaderSearchOpts();
  // WARNING: this logic isn't used anymore for lots of targets (see
  // clang/Frontend/InitHeaderSearch.cpp:438). We need to use a clang driver
  // to get everything right!
  HSO.UseStandardSystemIncludes = true;

  // TODO: a big hack is happening here!
  InitHeaderSearchFlags(TO.Triple, Opts, HSO);

  // Intialize execution engine!
  std::string Error;
  const llvm::Target *Tgt = TargetRegistry::lookupTarget(TO.Triple, Error);
  if (!Tgt) {
    std::stringstream ss;
    ss << "unable to find native target: " << Error << "!";
    unreachable(ss.str().c_str());
  }

  std::unique_ptr<llvm::Module> DummyM(new llvm::Module{"DummyM",Ctx_});
  DummyM->setTargetTriple(TO.Triple);
  EngineBuilder EB(std::move(DummyM));
  EB.setEngineKind(EngineKind::JIT)
    .setErrorStr(&Error)
    .setOptLevel(CodeGenOpt::Default)
    .setRelocationModel(Reloc::Static);

  SmallVector<std::string, 1> Attrs;
  // TODO: get the target machine from clang?
  EE_.reset(EB.create());
  if (!EE_) {
    std::stringstream ss;
    ss << "error creating jit: " << Error; 
    unreachable(ss.str().c_str());
  }
}

void DFFIImpl::setNewDiagnostics()
{
  auto* DiagsBuffer = new TextDiagnosticPrinter{ErrorMsgStream_, &*DiagOpts_};
  auto* Diags = new DiagnosticsEngine(DiagID_, &*DiagOpts_, DiagsBuffer);
  auto* SrcMgr = new SourceManager(*Diags, *FileMgr_);
  //Diags->setWarningsAsErrors(Opts_.WarningsAsErrors);
  Clang_->setSourceManager(SrcMgr);
  Clang_->setDiagnostics(Diags);
}

void DFFIImpl::getCompileError(std::string& Err)
{
  ErrorMsgStream_.flush();
  Err = std::move(ErrorMsg_);
  ErrorMsg_ = std::string{};
  Clang_->getDiagnostics().Clear();
  Clang_->getDiagnostics().Reset();
}

std::unique_ptr<llvm::Module> DFFIImpl::compile_llvm_with_decls(StringRef const Code, StringRef const CUName, FuncAliasesMap& FuncAliases, std::string& Err)
{
  // Two pass compilation!
  // First pass parse the AST of clang and generate wrappers for every
  // defined-only functions. Second pass generates the LLVM IR of the original
  // code with these definitions!
  setNewDiagnostics();
  auto& CI = Clang_->getInvocation();
  CI.getFrontendOpts().Inputs.clear();
  CI.getFrontendOpts().Inputs.push_back(
    FrontendInputFile(CUName, InputKind::C));
  auto Buf = MemoryBuffer::getMemBufferCopy(Code);
  VFS_->addFile(CUName, time(NULL), std::move(Buf));
  auto Action = llvm::make_unique<ASTGenWrappersAction>(FuncAliases);
  if(!Clang_->ExecuteAction(*Action)) {
    getCompileError(Err);
    return nullptr;
  }

  auto BufForceDecl = Code.str() + "\n" + Action->forceDecls();
  SmallString<128> PrivateCU;
  ("/__dffi_private/force_decls/" + CUName).toStringRef(PrivateCU);
  return compile_llvm(BufForceDecl, PrivateCU, Err);
}

std::unique_ptr<llvm::Module> DFFIImpl::compile_llvm(StringRef const Code, StringRef const CUName, std::string& Err)
{
  // DiagnosticsEngine->Reset() does not seem to reset everything, as errors
  // are added up from other compilation units!
  setNewDiagnostics();

  auto Buf = MemoryBuffer::getMemBufferCopy(Code);
  auto& CI = Clang_->getInvocation();
  CI.getFrontendOpts().Inputs.clear();
  CI.getFrontendOpts().Inputs.push_back(
    FrontendInputFile(CUName, InputKind::C));
  VFS_->addFile(CUName, time(NULL), std::move(Buf));

  auto LLVMAction = llvm::make_unique<clang::EmitLLVMOnlyAction>(&Ctx_);
  if(!Clang_->ExecuteAction(*LLVMAction)) {
    getCompileError(Err);
    return nullptr;
  }
  return LLVMAction->takeModule();
}

void getFuncWrapperName(SmallVectorImpl<char>& Ret, StringRef const Name)
{
  (WrapperPrefix + Name).toVector(Ret);
}

StringRef getFuncNameFromWrapper(StringRef const Name)
{
  assert(isWrapperFunction(Name));
  return Name.substr(strlen(WrapperPrefix));
}

bool isWrapperFunction(StringRef const Name)
{
  return Name.startswith(WrapperPrefix);
}

DFFIImpl::~DFFIImpl()
{ }

std::pair<size_t, bool> DFFIImpl::getFuncTypeWrapperId(FunctionType const* FTy)
{
  size_t TyIdx = WrapperIdx_;
  auto Ins = FuncTyWrappers_.try_emplace(FTy, TyIdx);
  if (!Ins.second) {
    return {Ins.first->second, true};
  }
  ++WrapperIdx_;
  return {TyIdx, false};
}

std::pair<size_t, bool> DFFIImpl::getFuncTypeWrapperId(FunctionType const* FTy, ArrayRef<Type const*> VarArgs)
{
  size_t TyIdx = WrapperIdx_;
  auto Ins = VarArgsFuncTyWrappers_.try_emplace(std::make_pair(FTy, VarArgs), TyIdx);
  if (!Ins.second) {
    return {Ins.first->second, true};
  }
  ++WrapperIdx_;
  return {TyIdx, false};
}

void DFFIImpl::genFuncTypeWrapper(TypePrinter& P, size_t WrapperIdx, llvm::raw_string_ostream& ss, FunctionType const* FTy, ArrayRef<Type const*> VarArgs)
{
  ss << "void " << getWrapperName(WrapperIdx) << "(";
  auto RetTy = FTy->getReturnType();
  P.print_def(ss, getPointerType(FTy), TypePrinter::Full, "__FPtr") << ",";
  P.print_def(ss, getPointerType(RetTy), TypePrinter::Full, "__Ret") << ",";
  ss << "void** __Args) {\n  ";
  if (RetTy) {
    ss << "*__Ret = ";
  }
  ss << "(__FPtr)(";
  size_t Idx = 0;
  auto& Params = FTy->getParams();
  for (QualType ATy: Params) {
    ss << "*((";
    P.print_def(ss, getPointerType(ATy), TypePrinter::Full) << ')' << "__Args[" << Idx << ']' << ')';
    if (Idx < Params.size()-1) {
      ss << ',';
    }
    ++Idx;
  }
  if (VarArgs.size() > 0) {
    assert(FTy->hasVarArgs() && "function type must be variadic if VarArgsCount > 0");
    assert(Params.size() >= 1 && "variadic arguments must have at least one defined argument");
    for (Type const* Ty: VarArgs) {
      ss << ", *((";
      P.print_def(ss, getPointerType(Ty), TypePrinter::Full) << ')' << "__Args[" << Idx << ']' << ')';
      ++Idx;
    }
  }
  ss << ");\n}\n";
}

CUImpl* DFFIImpl::compile(StringRef const Code, StringRef CUName, bool IncludeDefs, std::string& Err)
{
  std::string AnonCUName;
  if (CUName.empty()) {
    AnonCUName = "/__dffi_private/anon_cu_" + std::to_string(CUIdx_++) + ".c";
    CUName = AnonCUName;
  }
#ifdef _WIN32
  else {
    // TODO: figure out why!
    Err = "named compilation unit does not work on Windows!";
    return nullptr;
  }
#endif

  std::unique_ptr<llvm::Module> M;
  std::unique_ptr<CUImpl> CU(new CUImpl{*this});

  if (IncludeDefs) {
    M = compile_llvm_with_decls(Code, CUName, CU->FuncAliases_, Err);
  }
  else {
    M = compile_llvm(Code, CUName, Err);
  }
  if (!M) {
    return nullptr;
  }
  auto* pM = M.get();

  // Pre-parse metadata in two passes to find and declare structures
  // First pass will declare every structure as opaque ones. Next pass defines the ones which are.
  DebugInfoFinder DIF;
  DIF.processModule(*pM);
  SmallVector<DICompositeType const*, 16> Composites;
  SmallVector<DIDerivedType const*, 16> Typedefs;
  for (DIType const* Ty: DIF.types()) {
    if (Ty == nullptr) {
      continue;
    }
    if (auto const* DTy = llvm::dyn_cast<DIDerivedType>(Ty)) {
      if (DTy->getTag() == dwarf::DW_TAG_typedef) {
        Typedefs.push_back(DTy);
      }
    }
    else {
      Ty = getCanonicalDIType(Ty);
      if (auto const* CTy = llvm::dyn_cast<DICompositeType>(Ty)) {
        auto Tag = Ty->getTag();
        if (Tag == dwarf::DW_TAG_structure_type || Tag == dwarf::DW_TAG_union_type || Tag == dwarf::DW_TAG_enumeration_type) {
          Composites.push_back(CTy);
          CU->declareDIComposite(CTy);
        }
      }
    }
  }

  // Parse structures and unions
  for (DICompositeType const* Ty: Composites) {
    CU->parseDIComposite(Ty, *M);
  }

  for (auto const* DTy: Typedefs) {
    // TODO: optimize this! We could add the visited typedefs in the alias list
    // as long as we traverse it, and remove them from the list of typedefs to
    // visit.
    CU->setAlias(DTy->getName(), CU->getTypeFromDIType(DTy));
  }

  // Generate function types
  std::string Buf;
  llvm::raw_string_ostream Wrappers(Buf);
  TypePrinter Printer;
  SmallVector<Function*, 16> ToRemove;
  for (Function& F: *M) {
    if (F.isIntrinsic())
      continue;
    if (F.doesNotReturn())
      continue;
    StringRef FName = F.getName();
    if (FName.startswith("__dffi_force_typedef")) {
      ToRemove.push_back(&F);
      continue;
    }
    auto* DFTy = CU->getFunctionType(F);
    if (!DFTy)
      continue;
    if (FName.startswith("__dffi_force_decl_")) {
      FName = FName.substr(strlen("__dffi_force_decl_"));
      ToRemove.push_back(&F);
    }
    else {
      if (FName.size() > 0 && FName[0] == 1) {
        // Clang emits the "\01" prefix in some cases, when ASM function
        // redirects are used!
        F.setName(FName.substr(1));
        FName = F.getName();
      }
      CU->parseFunctionAlias(F);
    }
    CU->FuncTys_[FName] = DFTy;
    auto Id = getFuncTypeWrapperId(DFTy);
    if (!Id.second) {
      // TODO: if varag, do we always generate the wrapper for the version w/o varargs?
      genFuncTypeWrapper(Printer, Id.first, Wrappers, DFTy, {});
    }
  }

  for (Function* F: ToRemove) {
    F->eraseFromParent();
  }

  // Strip debug info (we don't need them anymore)!
  llvm::StripDebugInfo(*pM);

  // Add the module to the EE
  EE_->addModule(std::move(M));
  EE_->generateCodeForModule(pM);

  // Compile wrappers
  compileWrappers(Printer, Wrappers.str());

  // We don't need these anymore
  CU->AnonTys_.clear();

  auto* Ret = CU.get();
  CUs_.emplace_back(std::move(CU));
  return Ret;
}

void DFFIImpl::compileWrappers(TypePrinter& Printer, std::string const& Wrappers)
{
  auto& CI = Clang_->getInvocation();
  auto& CGO = CI.getCodeGenOpts();
  std::string WCode = Printer.getDecls() + "\n" + Wrappers;
  std::stringstream ss;
  ss << "/__dffi_private/wrappers_" << CUIdx_++ << ".c";
  CGO.setDebugInfo(codegenoptions::NoDebugInfo);
  std::string Err;
  auto M = compile_llvm(WCode, ss.str(), Err);
  CGO.setDebugInfo(codegenoptions::FullDebugInfo);
  if (!M) {
    errs() << WCode;
    errs() << Err;
    llvm::report_fatal_error("unable to compile wrappers!");
  }
  auto* pM = M.get();
  EE_->addModule(std::move(M));
  EE_->generateCodeForModule(pM);
}

void* DFFIImpl::getWrapperAddress(FunctionType const* FTy)
{
  // TODO: merge with getWrapperAddress for varargs
  auto Id = getFuncTypeWrapperId(FTy);
  size_t WIdx = Id.first;
  if (!Id.second) {
    std::string Buf;
    llvm::raw_string_ostream ss(Buf);
    TypePrinter P;
    genFuncTypeWrapper(P, WIdx, ss, FTy, None);
    compileWrappers(P, ss.str());
  }
  std::string TName = getWrapperName(WIdx);
  void* Ret = (void*)EE_->getFunctionAddress(TName.c_str());
  assert(Ret && "function wrapper does not exist!");
  return Ret;
}

Function* DFFIImpl::getWrapperLLVMFunc(FunctionType const* FTy, ArrayRef<Type const*> VarArgs)
{
  // TODO: suboptimal. Lookup of the wrapper ID is done twice, and the full
  // compilation of the wrapper is done, whereas it might not be necessary!
  getWrapperAddress(FTy);

  std::pair<size_t, bool> Id;
  if (FTy->hasVarArgs()) {
    Id = getFuncTypeWrapperId(FTy, VarArgs);
  }
  else {
    assert(VarArgs.size() == 0 && "VarArgs specified when function type doesn't support variadic arguments");
    Id = getFuncTypeWrapperId(FTy);
  }
  assert(Id.second && "wrapper should already exist!");
  std::string TName = getWrapperName(Id.first);
  return EE_->FindFunctionNamed(TName);
}

void* DFFIImpl::getWrapperAddress(FunctionType const* FTy, ArrayRef<Type const*> VarArgs)
{
  auto Id = getFuncTypeWrapperId(FTy, VarArgs);
  size_t WIdx = Id.first;
  if (!Id.second) {
    std::string Buf;
    llvm::raw_string_ostream ss(Buf);
    TypePrinter P;
    genFuncTypeWrapper(P, WIdx, ss, FTy, VarArgs);
    compileWrappers(P, ss.str());
  }
  std::string TName = getWrapperName(WIdx);
  void* Ret = (void*)EE_->getFunctionAddress(TName.c_str());
  assert(Ret && "function wrapper does not exist!");
  return Ret;
}

void* DFFIImpl::getFunctionAddress(StringRef Name)
{
  // TODO: chances that this is clearly sub optimal
  // TODO: use getAddressToGlobalIfAvailable?
  Function* F = EE_->FindFunctionNamed(Name);
  if (!F || F->isDeclaration()) {
    return sys::DynamicLibrary::SearchForAddressOfSymbol(Name);
  }
  return (void*)EE_->getFunctionAddress(Name.str());
#if 0
  // TODO: we would like to be able to do this! Unfortunatly, MCJIT API is
  // private...
  auto Sym = EE_->findSymbol(Name.str(), true);
  if (!Sym) {
    return nullptr;
  }
  auto AddrOrErr = Sym.getAddress();
  if (!AddrOrErr) {
    return nullptr;
  }
  return (void*)(*AddrOrErr);
#endif
}

NativeFunc DFFIImpl::getFunction(FunctionType const* FTy, void* FPtr)
{
  auto TFPtr = (NativeFunc::TrampPtrTy)getWrapperAddress(FTy);
  assert(TFPtr && "function type trampoline doesn't exist!");
  return {TFPtr, FPtr, FTy};
}

// TODO: QualType here!
NativeFunc DFFIImpl::getFunction(FunctionType const* FTy, ArrayRef<Type const*> VarArgs, void* FPtr)
{
  if (!FTy->hasVarArgs()) {
    return NativeFunc{};
  }
  auto TFPtr = (NativeFunc::TrampPtrTy)getWrapperAddress(FTy, VarArgs);
  // Generates new function type for this list of variadic arguments
  SmallVector<QualType, 8> Types;
  auto const& Params = FTy->getParams();
  Types.reserve(Params.size() + VarArgs.size());
  for (auto const& P: Params) {
    Types.emplace_back(P.getType());
  }
  for (auto T: VarArgs) {
    Types.emplace_back(T);
  }
  FTy = getContext().getFunctionType(*this, FTy->getReturnType(), Types, FTy->getCC(), false);
  return {TFPtr, FPtr, FTy};
}

BasicType const* DFFIImpl::getBasicType(BasicType::BasicKind K)
{
  return getContext().getBasicType(*this, K);
}

PointerType const* DFFIImpl::getPointerType(QualType Ty)
{
  return getContext().getPtrType(*this, Ty);
}

ArrayType const* DFFIImpl::getArrayType(QualType Ty, uint64_t NElements)
{
  return getContext().getArrayType(*this, Ty, NElements);
}

// Compilation unit
//

CUImpl::CUImpl(DFFIImpl& DFFI):
  DFFI_(DFFI)
{ }

std::tuple<void*, FunctionType const*> CUImpl::getFunctionAddressAndTy(llvm::StringRef Name)
{
  auto ItAlias = FuncAliases_.find(Name);
  if (ItAlias != FuncAliases_.end()) {
    Name = ItAlias->second;
  }
  auto ItFTy = FuncTys_.find(Name);
  if (ItFTy == FuncTys_.end()) {
    return std::tuple<void*, FunctionType const*>{nullptr,nullptr};
  }
  return std::tuple<void*, FunctionType const*>{DFFI_.getFunctionAddress(Name), ItFTy->second};
}

NativeFunc CUImpl::getFunction(llvm::StringRef Name)
{
  void* FPtr;
  FunctionType const* FTy;
  std::tie(FPtr, FTy) = getFunctionAddressAndTy(Name);
  return getFunction(FPtr, FTy);
}

NativeFunc CUImpl::getFunction(llvm::StringRef Name, llvm::ArrayRef<Type const*> VarArgs)
{
  void* FPtr;
  FunctionType const* FTy;
  std::tie(FPtr, FTy) = getFunctionAddressAndTy(Name);
  return getFunction(FPtr, FTy, VarArgs);
}

NativeFunc CUImpl::getFunction(void* FPtr, FunctionType const* FTy, llvm::ArrayRef<Type const*> VarArgs)
{
  if (!FPtr || !FTy) {
    return {};
  }
  return DFFI_.getFunction(FTy, VarArgs, FPtr);
}

NativeFunc CUImpl::getFunction(void* FPtr, FunctionType const* FTy)
{
  if (!FPtr || !FTy) {
    return {};
  }
  return DFFI_.getFunction(FTy, FPtr);
}

template <class T>
static T const* getCompositeType(CompositeTysMap const& Tys, StringRef Name)
{
  auto It = Tys.find(Name);
  if (It == Tys.end()) {
    return nullptr;
  }
  return dffi::dyn_cast<T>(It->second.get());
}

StructType const* CUImpl::getStructType(StringRef Name) const
{
  return getCompositeType<StructType>(CompositeTys_, Name);
}

UnionType const* CUImpl::getUnionType(StringRef Name) const
{
  return getCompositeType<UnionType>(CompositeTys_, Name);
}

EnumType const* CUImpl::getEnumType(StringRef Name) const
{
  return getCompositeType<EnumType>(CompositeTys_, Name);
}

std::vector<std::string> CUImpl::getTypes() const
{
  std::vector<std::string> Ret;
  Ret.reserve(CompositeTys_.size() + AliasTys_.size());
  for (auto const& C: CompositeTys_) {
    Ret.emplace_back(C.getKey().str());
  }
  for (auto const& C: AliasTys_) {
    Ret.emplace_back(C.getKey().str());
  }
  return Ret;
}

std::vector<std::string> CUImpl::getFunctions() const
{
  std::vector<std::string> Ret;
  Ret.reserve(FuncTys_.size() + FuncAliases_.size());
  for (auto const& C: FuncTys_) {
    Ret.emplace_back(C.getKey().str());
  }
  for (auto const& C: FuncAliases_) {
    Ret.emplace_back(C.getKey().str());
  }
  return Ret;
}

void CUImpl::declareDIComposite(DICompositeType const* DCTy)
{
  const auto Tag = DCTy->getTag();
  assert((Tag == dwarf::DW_TAG_structure_type || Tag == dwarf::DW_TAG_union_type ||
    Tag == dwarf::DW_TAG_enumeration_type) && "declareDIComposite called without a valid type!");

  StringRef Name = DCTy->getName();

  auto AddTy = [&](StringRef Name_) {
    CanOpaqueType* Ptr;
    switch (Tag) {
      case dwarf::DW_TAG_structure_type:
        Ptr = new StructType{DFFI_};
        break;
      case dwarf::DW_TAG_union_type:
        Ptr = new UnionType{DFFI_};
        break;
      case dwarf::DW_TAG_enumeration_type:
        Ptr = new EnumType{DFFI_};
        break;
    };
    return CompositeTys_.try_emplace(Name_, std::unique_ptr<CanOpaqueType>{Ptr});
  };

  if (Name.size() > 0) {
    // Sets the struct as an opaque one.
    if (Name.startswith("__dffi")) {
      llvm::report_fatal_error("__dffi is a compiler reserved prefix and can't be used in a structure name!");
    }
    AddTy(Name);
  }
  else {
    // Add to the map of anonymous types, and generate a name
    if (AnonTys_.count(DCTy) == 1) {
      return;
    }
    auto ID = AnonTys_.size() + 1;
    std::stringstream ss;
    ss << "__dffi_anon_struct_" << ID;
    auto It = AddTy(ss.str());
    assert(It.second && "anonymous structure ID already existed!!");
    AnonTys_[DCTy] = It.first->second.get();
  }
}

void CUImpl::parseDIComposite(DICompositeType const* DCTy, llvm::Module& M)
{
  const auto Tag = DCTy->getTag();
  assert((Tag == dwarf::DW_TAG_structure_type || Tag == dwarf::DW_TAG_union_type ||
    Tag == dwarf::DW_TAG_enumeration_type) && "parseDIComposite called without a valid type!");

  StringRef Name = DCTy->getName();
  dffi::CanOpaqueType* CATy;
  if (Name.size() > 0) {
    auto It = CompositeTys_.find(Name);
    assert(It != CompositeTys_.end() && "structure/union/enum hasn't been previously declared!");
    CATy = It->second.get();
  }
  else {
    auto It = AnonTys_.find(DCTy);
    assert(It != AnonTys_.end() && "anonymous structure/union/enum hasn't been previously declared!");
    CATy = dffi::cast<dffi::CanOpaqueType>(It->second);
  }
  // If the structure/union isn't know yet, this sets the composite type as an
  // opaque one. This can be useful if the type is self-referencing
  // itself throught a pointer (classical case in linked list for instance).
  if (DCTy->isForwardDecl()) {
    // Nothing new here!
    return;
  }
  if (!CATy->isOpaque()) {
    return;
  }

  if (auto* CTy = dffi::dyn_cast<CompositeType>(CATy)) {
    // Parse the structure/union and create the associated StructType/UnionType
    std::vector<CompositeField> Fields; 
    unsigned Align = 1;
    for (auto const* Op: DCTy->getElements()) {
      auto const* DOp = llvm::cast<DIDerivedType>(Op);
      assert(DOp->getTag() == llvm::dwarf::DW_TAG_member && "element of a struct/union must be a DW_TAG_member!");

      StringRef FName = DOp->getName();
      unsigned FOffset = DOp->getOffsetInBits()/8;
#ifndef NDEBUG
      if (DCTy->getTag() == dwarf::DW_TAG_union_type) {
        assert(FOffset == 0 && "union field member must have an offset of 0!");
      }
#endif

      DIType const* FDITy = getCanonicalDIType(DOp->getBaseType().resolve());
      dffi::Type const* FTy = getTypeFromDIType(FDITy);

      Fields.emplace_back(CompositeField{FName.str().c_str(), FTy, FOffset});

      Align = std::max(Align, FTy->getAlign());
    }
    auto Size = DCTy->getSizeInBits()/8;
    CTy->setBody(std::move(Fields), Size, Align);
  }
  else {
    auto* ETy = dffi::cast<EnumType>(CATy);
    EnumType::Fields Fields;
    for (auto const* Op: DCTy->getElements()) {
      auto const* EOp = llvm::cast<DIEnumerator>(Op);
      Fields[EOp->getName().str()] = EOp->getValue();
    }
    ETy->setBody(std::move(Fields));
  }
}

dffi::QualType CUImpl::getQualTypeFromDIType(llvm::DIType const* Ty)
{
  // Go throught every typedef and returns the qualified type
  if (auto* DTy = llvm::dyn_cast_or_null<DIDerivedType>(Ty)) {
    switch (DTy->getTag()) {
      case dwarf::DW_TAG_typedef:
      case dwarf::DW_TAG_volatile_type:
      case dwarf::DW_TAG_restrict_type:
        return getQualTypeFromDIType(DTy->getBaseType().resolve());
      case dwarf::DW_TAG_const_type:
        return getQualTypeFromDIType(DTy->getBaseType().resolve()).withConst();
      default:
        break;
    };
  }

  return getTypeFromDIType(Ty);
}

dffi::Type const* CUImpl::getTypeFromDIType(llvm::DIType const* Ty)
{
  Ty = getCanonicalDIType(Ty);
  if (!Ty) {
    return nullptr;
  }

  if (auto* BTy = llvm::dyn_cast<llvm::DIBasicType>(Ty)) {
#define HANDLE_BASICTY(TySize, KTy)\
    if (Size == TySize)\
      return DFFI_.getBasicType(BasicType::getKind<KTy>());

    const auto Size = BTy->getSizeInBits();
    switch (BTy->getEncoding()) {
      case llvm::dwarf::DW_ATE_boolean:
        return DFFI_.getBasicType(BasicType::Bool);
      case llvm::dwarf::DW_ATE_unsigned:
      case llvm::dwarf::DW_ATE_unsigned_char:
      {
        if (BTy->getName() == "char") {
          return DFFI_.getBasicType(BasicType::Char);
        }
        HANDLE_BASICTY(8, uint8_t);
        HANDLE_BASICTY(16, uint16_t);
        HANDLE_BASICTY(32, uint32_t);
        HANDLE_BASICTY(64, uint64_t);
#ifdef DFFI_SUPPORT_I128
        HANDLE_BASICTY(128, __uint128_t);
#endif
        break;
      }
      case llvm::dwarf::DW_ATE_signed:
      case llvm::dwarf::DW_ATE_signed_char:
      {
        if (BTy->getName() == "char") {
          return DFFI_.getBasicType(BasicType::Char);
        }
        HANDLE_BASICTY(8, int8_t);
        HANDLE_BASICTY(16, int16_t);
        HANDLE_BASICTY(32, int32_t);
        HANDLE_BASICTY(64, int64_t);
#ifdef DFFI_SUPPORT_I128
        HANDLE_BASICTY(128, __int128_t);
#endif
        break;
      }
      case llvm::dwarf::DW_ATE_float:
        HANDLE_BASICTY(sizeof(float)*8, c_float);
        HANDLE_BASICTY(sizeof(double)*8, c_double);
        HANDLE_BASICTY(sizeof(long double)*8, c_long_double);
        break;
#ifdef DFFI_SUPPORT_COMPLEX
      case llvm::dwarf::DW_ATE_complex_float:
        HANDLE_BASICTY(sizeof(_Complex float)*8, c_complex_float);
        HANDLE_BASICTY(sizeof(_Complex double)*8, c_complex_double);
        HANDLE_BASICTY(sizeof(_Complex long double)*8, c_complex_long_double);
        break;
#endif
      default:
        break;
    };
#ifdef LLVM_BUILD_DEBUG
    Ty->dump();
#endif
    llvm::report_fatal_error("unsupported type");
  }

  if (auto* PtrTy = llvm::dyn_cast<llvm::DIDerivedType>(Ty)) {
    if (PtrTy->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
      auto Pointee = getQualTypeFromDIType(PtrTy->getBaseType().resolve());
      return getPointerType(Pointee);
    }
#ifdef LLVM_BUILD_DEBUG
    Ty->dump();
#endif
    llvm::report_fatal_error("unsupported type");
  }

  if (auto* DTy = llvm::dyn_cast<DICompositeType>(Ty)) {
    const auto Tag = DTy->getTag();
    switch (Tag) {
      case llvm::dwarf::DW_TAG_structure_type:
      case llvm::dwarf::DW_TAG_union_type:
      case llvm::dwarf::DW_TAG_enumeration_type:
      {
        StringRef Name = DTy->getName();
        if (Name.size() == 0) {
          auto It = AnonTys_.find(DTy);
          if (It == AnonTys_.end()) {
            llvm::report_fatal_error("unknown literal struct/union/enum!");
          }
          return It->second;
        }
        auto It = CompositeTys_.find(Name);
        if (It == CompositeTys_.end()) {
          llvm::report_fatal_error("unknown struct/union/enum!");
        }
        return It->second.get();
      }
      case llvm::dwarf::DW_TAG_array_type:
      {
        auto EltTy = getQualTypeFromDIType(DTy->getBaseType().resolve());
        auto Count = llvm::cast<DISubrange>(*DTy->getElements().begin())->getCount();
        if (auto* CCount = Count.dyn_cast<ConstantInt*>()) {
          return DFFI_.getArrayType(EltTy, CCount->getZExtValue());
        }
        else {
          return getPointerType(EltTy);
        }
      }
    };
  }

  if (auto* FTy = llvm::dyn_cast<DISubroutineType>(Ty)) {
    return getFunctionType(FTy);
  }

#ifdef LLVM_BUILD_DEBUG
  Ty->dump();
#endif
  llvm::report_fatal_error("unsupported type");
}

dffi::FunctionType const* CUImpl::getFunctionType(DISubroutineType const* Ty)
{
  auto ArrayTys = Ty->getTypeArray();
  auto ItTy = ArrayTys.begin();

  auto RetTy = getQualTypeFromDIType((*(ItTy++)).resolve());

  llvm::SmallVector<QualType, 8> ParamsTy;
  ParamsTy.reserve(ArrayTys.size()-1);
  for (auto ItEnd = ArrayTys.end(); ItTy != ItEnd; ++ItTy) {
    auto ATy = getQualTypeFromDIType((*ItTy).resolve());
    ParamsTy.push_back(ATy);
  }
  bool IsVarArgs = false;
  if (ParamsTy.size() > 1 && ParamsTy.back().getType() == nullptr) {
    IsVarArgs = true;
    ParamsTy.pop_back();
  }
  auto CC = dwarfCCToDFFI(Ty->getCC());
  return getContext().getFunctionType(DFFI_, RetTy, ParamsTy, CC, IsVarArgs);
}

void CUImpl::parseFunctionAlias(Function& F)
{
  MDNode* MD = F.getMetadata("dbg");
  if (!MD) {
    return;
  }

  // See tests/asm_redirect.cpp. The name of the DISubprogram object can be
  // different from the LLVM function name! Let's register the debug info name
  // as an alias to the llvm one.
  auto* SP = llvm::cast<DISubprogram>(MD);
  auto AliasName = SP->getName();
  if (AliasName != F.getName()) {
    assert(FuncTys_.count(AliasName) == 0 && "function alias already defined!");
    assert(FuncAliases_.count(AliasName) == 0 && "function alias already in alias list!");
    FuncAliases_[AliasName] = F.getName().str();
  }
}

dffi::FunctionType const* CUImpl::getFunctionType(Function& F)
{
  MDNode* MD = F.getMetadata("dbg");
  if (!MD) {
    return nullptr;
  }
  auto* SP = llvm::cast<DISubprogram>(MD);
  auto* Ty = llvm::cast<DISubroutineType>(SP->getType());
  return getFunctionType(Ty);
}

dffi::Type const* CUImpl::getType(StringRef Name) const
{
  {
    auto It = AliasTys_.find(Name);
    if (It != AliasTys_.end())
      return It->second;
  }
  {
    auto It = CompositeTys_.find(Name);
    if (It != CompositeTys_.end())
      return It->second.get();
  }
  return nullptr;
}


} // details

} // dffi
