#include "sperr_helper.h"

#include <iostream>
#include <cassert>

#define FLOAT double

int main(int argc, char* argv[])
{
  const auto NX = std::atol(argv[2]);
  const auto NY = std::atol(argv[3]);
  const auto NZ = std::atol(argv[4]);

  auto buf = sperr::read_whole_file<FLOAT>(argv[1]);
  assert(buf.size() == NX * NY * NZ);

  std::cout << "Usage: hit 'n' to print the next float, or '3' to take in another 3D location.\n"
            << "Hit any other key to abort." << std::endl;

  size_t idx = 0;
  char c = '3';
  while (c == 'n' || c == '3') {
    if (c == 'n')
      std::printf("  %g\n", buf[++idx]);
    else {
      size_t x, y, z;
      std::cout << "Please provide 3 location indices!" << std::endl;
      std::cin >> x;
      std::cin >> y;
      std::cin >> z;
      idx = z * NX * NY + y * NX + x;
      std::printf("  %g\n", buf[idx]);
    }
    std::cout << "Please indicate the next move!" << std::endl;
    std::cin >> c;
  }
}
