#include "SPECK3D_OMP_C.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("");

  std::string input_file;
  app.add_option("filename", input_file, "Input data file to the compressor")
      ->required()
      ->group("Input Specifications")
      ->check(CLI::ExistingFile);

  std::vector<size_t> dims;
  app.add_option("--dims", dims,
                 "Dimensions of the input volume\n"
                 "E.g., `--dims 128 128 128`\n")
      ->expected(3)
      ->required()
      ->group("Input Specifications");

  auto use_double = bool{false};
  app.add_flag("-d", use_double,
               "Specify that input data is in double type.\n"
               "Data is treated as float by default.")
      ->group("Input Specifications");

  std::vector<size_t> chunks{64, 64, 64};
  app.add_option("--chunks", chunks,
                 "Dimensions of the preferred chunk size\n"
                 "If not specified, then 64^3 will be used.\n"
                 "E.g., `--chunks 64 64 64`\n")
      ->expected(3)
      ->group("Compression Parameters");

#ifdef QZ_TERM
  int32_t qz_level = 0;
  app.add_option("-q", qz_level,
                 "Quantization level to reach when encoding\n"
                 "Note 1: smaller n usually yields smaller compression errors.\n"
                 "Note 2: n could be negative integers as well.\n")
      ->group("Compression Parameters")
      ->required();

  auto tolerance = double{0.0};
  app.add_option("-t", tolerance,
                 "Maximum point-wise error tolerance\n"
                 "E.g., `-t 1e-2`\n")
      ->group("Compression Parameters")
      ->required();
#else
  auto bpp = double{0.0};
  app.add_option("--bpp", bpp, "Target bit-per-pixel. E.g., `-bpp 2.3`")
      ->check(CLI::Range(0.0f, 64.0f))
      ->group("Compression Parameters")
      ->required();
#endif

  size_t omp_num_threads = 4;
  app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. Default: 4")
      ->group("Compression Parameters");

  std::string output_file;
  app.add_option("-o", output_file, "Output filename")->required();

  CLI11_PARSE(app, argc, argv);

#ifdef QZ_TERM
  if (tolerance <= 0.0) {
    std::cerr << "Tolerance must be a positive value!\n";
    return 1;
  }
#else
  if (bpp < 0.0 || bpp > 64.0) {
    std::cerr << "Bit-per-pixel must be in the range (0.0, 64.0)!\n";
    return 1;
  }
#endif

  //
  // Let's do the actual work
  //
  const size_t total_vals = dims[0] * dims[1] * dims[2];
  SPECK3D_OMP_C compressor;
  compressor.set_dims({dims[0], dims[1], dims[2]});
  compressor.prefer_chunk_dims({chunks[0], chunks[1], chunks[2]});
  compressor.set_num_threads(omp_num_threads);

#ifdef QZ_TERM
  compressor.set_qz_level(qz_level);
  compressor.set_tolerance(tolerance);
#else
  compressor.set_bpp(bpp);
#endif

  auto orig = sperr::read_whole_file<uint8_t>(input_file.c_str());
  if ((use_double && orig.size() != total_vals * sizeof(double)) ||
      (!use_double && orig.size() != total_vals * sizeof(float))) {
    std::cerr << "Read input file error: " << input_file << std::endl;
    return 1;
  }
  auto rtn = sperr::RTNType::Good;
  if (use_double)
    rtn = compressor.use_volume(reinterpret_cast<const double*>(orig.data()), total_vals);
  else
    rtn = compressor.use_volume(reinterpret_cast<const float*>(orig.data()), total_vals);
  if (rtn != sperr::RTNType::Good) {
    std::cerr << "Copy data failed!" << std::endl;
    return 1;
  }

  // Free memory taken by `orig` since the volume has been put into chunks.
  orig.clear();
  orig.shrink_to_fit();

  if (compressor.compress() != sperr::RTNType::Good) {
    std::cerr << "Compression failed!" << std::endl;
    return 1;
  }

  auto stream = compressor.get_encoded_bitstream();
  if (stream.empty()) {
    std::cerr << "Compression bitstream empty!" << std::endl;
    return 1;
  }

  rtn = sperr::write_n_bytes(output_file.c_str(), stream.size(), stream.data());
  if (rtn != sperr::RTNType::Good) {
    std::cerr << "Write compressed file failed!" << std::endl;
    return 1;
  }

  return 0;
}
