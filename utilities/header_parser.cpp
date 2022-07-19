#include "sperr_helper.h"

#include "CLI11.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("Read the header of a SPERR bitstream and print out its content.\n");

  auto input_file = std::string();
  app.add_option("filename", input_file, "SPERR bitstream to parse")
      ->check(CLI::ExistingFile)
      ->required();

  CLI11_PARSE(app, argc, argv);

  // SPECK2D_Compressor has a header of size 10, and SPECK3D_OMP_C has its first 26 bytes
  // storing information of interest, so we read 26 bytes here.
  const size_t read_size = 26;

  auto header = sperr::read_n_bytes(input_file, read_size);
  if (header.size() != read_size) {
    std::cerr << "Read input file error!" << std::endl;
    return 1;
  }

  auto info = sperr::parse_header(header.data());
  std::cout << "Bitstream meta data:\n";
  std::cout << "  Produced by software major version: " << +(info.version_major) << std::endl;

  std::cout << "  ZSTD applied: ";
  if (info.zstd_applied)
    std::cout << "YES\n";
  else
    std::cout << "NO\n";

  if (info.is_3d) {
    std::printf("  It contains a 3D volume of size (%lu x %lu x %lu)\n", info.vol_dims[0],
                info.vol_dims[1], info.vol_dims[2]);
  }
  else {
    std::printf("  It contains a 2D slice of size (%lu x %lu)\n", info.vol_dims[0],
                info.vol_dims[1]);
  }

  if (info.orig_is_float) {
    std::cout << "  The original input data was in single precision float" << std::endl;
  }
  else {
    std::cout << "  The original input data was in double precision float" << std::endl;
  }

  return 0;
}
