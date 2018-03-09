#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

int main(int argc, char** argv)
{
  DFFI::initialize();
  CCOpts Opts;
  Opts.OptLevel = 2;
  DFFI D(Opts);

  std::string Err;
  auto S = D.shellcode(argv[1], Err);
  if (S.empty()) {
    std::cerr << "Error: " << Err << std::endl;
    return 1;
  }

  std::cout.write((const char*) &S[0], S.size());

  return 0;
}
