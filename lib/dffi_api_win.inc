#include <dffi/dffi.h>

void* dffi::DynamicLibrary::baseAddress() const
{
  assert(valid());
  // Value returned by LoadLibrary is already the base address!
  return Data_;
}
