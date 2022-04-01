#include "SPECK3D_OMP_C.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("Compress a 3D volume and output a SPERR bitstream");

  auto input_file = std::string();
  app.add_option("filename", input_file, "Input data file to the compressor")
      ->required()
      ->check(CLI::ExistingFile)
      ->group("Input Specifications");

  auto dims = std::vector<size_t>();
  app.add_option("--dims", dims, "Dimensions of the input volume. E.g., `--dims 128 128 128`")
      ->expected(3)
      ->required()
      ->group("Input Specifications");

  auto use_double = bool{false};
  app.add_flag("-d", use_double,
               "Specify that input data is in double type.\n"
               "Data is treated as float by default.")
      ->group("Input Specifications");

  auto output_file = std::string();
  app.add_option("-o", output_file, "Output filename")->required()->group("Output Specifications");

  auto chunks = std::vector<size_t>{128, 128, 128};
  app.add_option("--chunks", chunks,
                 "Dimensions of the preferred chunk size. Default: 128 128 128\n"
                 "E.g., `--chunks 64 64 64`")
      ->expected(3)
      ->group("Compression Parameters");

#ifdef QZ_TERM
  auto qz_level = int32_t{0};
  app.add_option("-q", qz_level,
                 "Quantization level to reach when encoding\n"
                 "Note 1: smaller n usually yields smaller compression errors.\n"
                 "Note 2: n could be negative integers as well.")
      ->group("Compression Parameters")
      ->required();

  auto tolerance = double{0.0};
  app.add_option("-t", tolerance, "Maximum point-wise error tolerance. E.g., `-t 1e-2`")
      ->check(CLI::PositiveNumber)
      ->required()
      ->group("Compression Parameters");
#else
  auto bpp = double{0.0};
  app.add_option("--bpp", bpp, "Target bit-per-pixel. E.g., `-bpp 2.3`")
      ->check(CLI::Range(0.0, 64.0))
      ->group("Compression Parameters")
      ->required();
#endif

  auto omp_num_threads = size_t{4};
  app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. Default: 4")
      ->group("Compression Parameters");

  CLI11_PARSE(app, argc, argv);

  //
  // Let's do the actual work
  //
  const size_t total_vals = dims[0] * dims[1] * dims[2];
  SPECK3D_OMP_C compressor;
  compressor.set_num_threads(omp_num_threads);

#ifdef QZ_TERM
  compressor.set_qz_level(qz_level);
  compressor.set_tolerance(tolerance);
#else
  compressor.set_bpp(bpp);
#endif

  auto orig = sperr::read_whole_file<uint8_t>(input_file);
  if ((use_double && orig.size() != total_vals * sizeof(double)) ||
      (!use_double && orig.size() != total_vals * sizeof(float))) {
    std::cerr << "Read input file error: " << input_file << std::endl;
    return 1;
  }
  auto rtn = sperr::RTNType::Good;
  if (use_double) {
    rtn = compressor.copy_data(reinterpret_cast<const double*>(orig.data()), total_vals,
                               {dims[0], dims[1], dims[2]}, {chunks[0], chunks[1], chunks[2]});
  }
  else {
    rtn = compressor.copy_data(reinterpret_cast<const float*>(orig.data()), total_vals,
                               {dims[0], dims[1], dims[2]}, {chunks[0], chunks[1], chunks[2]});
  }
  if (rtn != sperr::RTNType::Good) {
    std::cerr << "Copy data failed!" << std::endl;
    return 1;
  }

  // Free memory taken by `orig` since the volume has been put into chunks.
  orig.clear();
  orig.shrink_to_fit();

  rtn = compressor.compress();
  switch (rtn) {
    case sperr::RTNType::QzLevelTooBig :
      std::cerr << "Compression failed because `q` is too big!" << std::endl;
      return 1;
    case sperr::RTNType::Good :
      break;
    default:
      std::cerr << "Compression failed!" << std::endl;
      return 1;
  }

  auto stream = compressor.get_encoded_bitstream();
  if (stream.empty()) {
    std::cerr << "Compression bitstream empty!" << std::endl;
    return 1;
  }

  rtn = sperr::write_n_bytes(output_file, stream.size(), stream.data());
  if (rtn != sperr::RTNType::Good) {
    std::cerr << "Write compressed file failed!" << std::endl;
    return 1;
  }

  return 0;
}
