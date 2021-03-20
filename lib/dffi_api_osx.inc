#include <dlfcn.h>
#include <mach-o/dyld.h>

// Inspired by https://github.com/JuliaLang/julia/blob/0027ed143e90d0f965694de7ea8c692d75ffa1a5/src/sys.c#L572-L583
void* dffi::DynamicLibrary::baseAddress() const
{
  assert(valid());
  // We start by the end, because chances are we just loaded that library!
  const int32_t Count = _dyld_image_count();
  if (Count <= 0) {
    return nullptr;
  }
  for (int32_t i = Count-1; i >= 0; --i) {
    // dlopen() each image, check handle
    const char *image_name = _dyld_get_image_name(i);
    void *probe_handle = dlopen(image_name, RTLD_LAZY);
    dlclose(probe_handle);

    // If the handle is the same as what was passed in (modulo mode bits), return this image name
    if (((uintptr_t)Data_ & (~uintptr_t(3U))) == ((uintptr_t)probe_handle & (~uintptr_t(3U)))) {
      return (void*)_dyld_get_image_vmaddr_slide(i);
    }
  }
  return nullptr;
}

// LastError handling
#include <algorithm>
#include <sys/errno.h>

// TODO: verify initial value
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
