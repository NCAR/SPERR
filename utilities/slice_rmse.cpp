#include "sperr_helper.h"

#include <cmath>

#define FLOAT float

int main(int argc, char* argv[])
{
  const char* file1 = argv[1];
  const char* file2 = argv[2];
  const size_t NX = std::atol(argv[3]);
  const size_t NY = std::atol(argv[4]);
  const size_t NZ = std::atol(argv[5]);
  const auto total_vals = NX * NY * NZ;
  char D = 'x';
  if (argc == 7)
    D = argv[6][0];

  // Read in both files
  auto in1 = sperr::read_whole_file<FLOAT>(file1);
  auto in2 = sperr::read_whole_file<FLOAT>(file2);
  if (in1.size() != total_vals || in2.size() != total_vals) {
    std::printf("Input file length wrong!\n");
    return (1);
  }

  auto out = std::vector<double>();

  if (D == 'X' || D == 'x') {
    out.assign(NX, 0.0);
    for (size_t i = 0; i < total_vals; i++) {
      auto diff = in1[i] - in2[i];
      out[i % NX] += diff * diff;
    }
    for (auto& v : out)
      v = std::sqrt(v / double(NY * NZ));
  }
  else if (D == 'Y' || D == 'y') {
    out.assign(NY, 0.0);
    for (size_t z = 0; z < NZ; z++)
      for (size_t y = 0; y < NY; y++)
        for (size_t x = 0; x < NX; x++) {
          auto i = z * NX * NY + y * NX + x;
          auto diff = in1[i] - in2[i];
          out[y] += diff * diff;
        }
    for (auto& v : out)
      v = std::sqrt(v / double(NX * NZ));
  }
  else if (D == 'Z' || D == 'z') {
    out.assign(NZ, 0.0);
    for (size_t z = 0; z < NZ; z++)
      for (size_t y = 0; y < NY; y++)
        for (size_t x = 0; x < NX; x++) {
          auto i = z * NX * NY + y * NX + x;
          auto diff = in1[i] - in2[i];
          out[z] += diff * diff;
        }
    for (auto& v : out)
      v = std::sqrt(v / double(NX * NY));
  }

  for (auto v : out)
    std::printf("%g\n", v);
}
