// RUN: %clang "%S/lib.c" -shared -O1 -o "%t.so"
// RUN: "%build_dir/dlopen%exeext" "%t.so"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <array>

#include <dffi/dffi.h>

using namespace dffi;

static bool test(const char* Path) {
  const auto DL = DFFI::dlopen(Path);
  void* BaseAddr = DL.baseAddress();
  // Check first 4 bytes are the same as Path
  std::array<uint8_t, 4> Ref,Buf;
  memcpy(&Ref[0], BaseAddr, sizeof(Ref));

  FILE* F = fopen(Path, "rb");
  if (!F) {
    fprintf(stderr, "Unable to open %s\n", Path);
    return false;
  }
  const auto Read = fread(&Buf, sizeof(Buf), 1, F);
  fclose(F);
  if (Read != 1) {
    fprintf(stderr, "Unable to read header of %s\n", Path);
    return false;
  }
  printf("Base address: %p\n", BaseAddr);
  return Ref == Buf;
}

int main(int argc, char** argv)
{
  if (argc < 1) {
    fprintf(stderr, "Usage: %s lib_path\n", argv[0]);
    return 1;
  }
  return test(argv[1]) ? 0:1;
}
