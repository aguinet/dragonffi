#include <link.h>
#include <dlfcn.h>
#include <errno.h>

void* dffi::DynamicLibrary::baseAddress() const
{
  assert(valid());
  link_map* lm;
  if (dlinfo(Data_, RTLD_DI_LINKMAP, &lm) != 0) {
    return nullptr;
  }
  return (void*)lm->l_addr;
}

// LastError handling
#include <algorithm>

// The initial value of errno at program startup is zero.
// (https://www.gnu.org/software/libc/manual/html_node/Checking-for-Errors.html).
static thread_local int dffi_errno = 0;

void dffi::NativeFunc::swapLastError()
{
  std::swap(dffi_errno, errno);
}

dffi::NativeFunc::LastErrorTy dffi::NativeFunc::getLastError() {
  return dffi_errno;
}

void dffi::NativeFunc::setLastError(LastErrorTy Err) {
  dffi_errno = Err;
}
