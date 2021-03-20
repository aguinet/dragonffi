void* dffi::DynamicLibrary::baseAddress() const
{
  assert(valid());
  // Value returned by LoadLibrary is already the base address!
  return Data_;
}

// LastError handling
#include <windows.h>

// TODO: verify the initial value
static thread_local DWORD dffi_lasterror = 0;

void dffi::NativeFunc::swapLastError()
{
  const DWORD tmp = dffi_lasterror;
  dffi_lasterror = GetLastError();
  SetLastError(tmp);
}

dffi::NativeFunc::LastErrorTy dffi::NativeFunc::getLastError() {
  return dffi_lasterror;
}

void dffi::NativeFunc::setLastError(LastErrorTy Err) {
  dffi_lasterror = Err;
}
