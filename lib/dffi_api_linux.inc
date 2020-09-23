#include <link.h>
#include <dlfcn.h>

void* dffi::DynamicLibrary::baseAddress() const
{
  assert(valid());
  link_map* lm;
  if (dlinfo(Data_, RTLD_DI_LINKMAP, &lm) != 0) {
    return nullptr;
  }
  return (void*)lm->l_addr;
}
