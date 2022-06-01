#include "SPERR2D_Compressor.h"
#include "SPERR2D_Decompressor.h"

#include "CLI11.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("Compress a 2D slice and output a SPERR bitstream\n");

  auto input_file = std::string();
  app.add_option("filename", input_file, "Input file to the compressor")
      ->required()
      ->check(CLI::ExistingFile)
      ->group("Input Specifications");

  auto dims = std::vector<size_t>();
  app.add_option("--dims", dims, "Dimensions of the input 2D slice. E.g., `--dims 128 128`")
      ->expected(2)
      ->required()
      ->group("Input Specifications");

  auto use_double = bool{false};
  app.add_flag("-d", use_double,
               "Specify that input data is in double type.\n"
               "Data is treated as float by default.")
      ->group("Input Specifications");

  auto output_file = std::string();
  app.add_option("-o", output_file, "Output filename")->required()->group("Output Specifications");

#ifdef QZ_TERM
  auto qz_level = int32_t{0};
  app.add_option("-q", qz_level,
                 "Quantization level to reach when encoding\n"
                 "Note 1: smaller quantization levels yield less compression error.\n"
                 "Note 2: quantization levels could be negative integers as well.")
      ->group("Compression Parameters")
      ->required();

  auto tolerance = double{0.0};
  app.add_option("-t", tolerance,
                 "Maximum point-wise error tolerance. E.g., `-t 1e-2`\n"
                 "If not specified or a negative number is given, \n"
                 "then the program will not perform any outlier correction.")
      ->group("Compression Parameters");
#else
  auto bpp = double{0.0};
  app.add_option("--bpp", bpp, "Target bit-per-pixel. E.g., `--bpp 4.5`")
      ->check(CLI::Range(0.0, 64.0))
      ->group("Compression Parameters")
      ->required();
#endif

  auto show_stats = bool{false};
  app.add_flag("--stats", show_stats,
               "Show statistics measuring the compression quality.\n"
               "They are not calculated by default.")
      ->group("Compression Parameters");

  CLI11_PARSE(app, argc, argv);

  // Read in the input file
  const size_t total_vals = dims[0] * dims[1];
  const auto orig = sperr::read_whole_file<uint8_t>(input_file);
  if ((use_double && orig.size() != total_vals * sizeof(double)) ||
      (!use_double && orig.size() != total_vals * sizeof(float))) {
    std::cerr << "Read input file error: " << input_file << std::endl;
    return 1;
  }

  auto rtn = sperr::RTNType::Good;
  auto compressor = SPERR2D_Compressor();

  // Pass data to the compressor
  if (use_double) {
    rtn = compressor.copy_data(reinterpret_cast<const double*>(orig.data()),
                               orig.size() / sizeof(double), {dims[0], dims[1], 1});
  }
  else {
    rtn = compressor.copy_data(reinterpret_cast<const float*>(orig.data()),
                               orig.size() / sizeof(float), {dims[0], dims[1], 1});
  }
  if (rtn != RTNType::Good) {
    std::cerr << "Copy data failed!" << std::endl;
    return 1;
  }

#ifdef QZ_TERM
  compressor.set_qz_level(qz_level);
  compressor.set_tolerance(tolerance);
#else
  rtn = compressor.set_bpp(bpp);
  if (rtn != RTNType::Good) {
    std::cerr << "Set bit-per-pixel failed!" << std::endl;
    return 1;
  }
#endif

  // Perform the actual compression
  rtn = compressor.compress();
  switch (rtn) {
    case sperr::RTNType::QzLevelTooBig:
      std::cerr << "Compression failed because `q` is set too big!" << std::endl;
      return 1;
    case sperr::RTNType::Good:
      break;
    default:
      std::cout << "Compression failed!" << std::endl;
      return 1;
  }

  // Output the encoded bitstream
  const auto& stream = compressor.view_encoded_bitstream();
  if (stream.empty()) {
    std::cerr << "Compression bitstream empty!" << std::endl;
    return 1;
  }

  // Calculate and print statistics
  if (show_stats) {
    std::cout << "Average bpp = " << stream.size() * 8.0 / total_vals;

    // Use a decompressor to decompress and collect error statistics
    SPERR2D_Decompressor decompressor;
    rtn = decompressor.use_bitstream(stream.data(), stream.size());
    if (rtn != RTNType::Good)
      return 1;

    rtn = decompressor.decompress();
    if (rtn != RTNType::Good)
      return 1;

    if (use_double) {
      const auto recover = decompressor.get_data<double>();
      assert(recover.size() * sizeof(double) == orig.size());
      auto stats = sperr::calc_stats(reinterpret_cast<const double*>(orig.data()), recover.data(),
                                     recover.size(), 4);
      std::cout << ", PSNR = " << stats[2] << "dB,  L-Infty = " << stats[1];
      std::printf(", Data range = (%.2e, %.2e).\n", stats[3], stats[4]);
    }
    else {
      const auto recover = decompressor.get_data<float>();
      assert(recover.size() * sizeof(float) == orig.size());
      auto stats = sperr::calc_stats(reinterpret_cast<const float*>(orig.data()), recover.data(),
                                     recover.size(), 4);
      std::cout << ", PSNR = " << stats[2] << "dB,  L-Infty = " << stats[1];
      std::printf(", Data range = (%.2e, %.2e).\n", stats[3], stats[4]);
    }

#ifdef QZ_TERM
    // Also collect outlier statistics
    const auto out_stats = compressor.get_outlier_stats();
    if (out_stats.first == 0) {
      printf("There were no outliers corrected!\n");
    }
    else {
      printf(
          "There were %ld outliers, percentage of total data points = %.2f%%.\n"
          "Correcting them takes bpp = %.2f, percentage of total storage = %.2f%%\n",
          out_stats.first, double(out_stats.first * 100) / double(total_vals),
          double(out_stats.second * 8) / double(out_stats.first),
          double(out_stats.second * 100) / double(stream.size()));
    }
#endif
  }

  rtn = sperr::write_n_bytes(output_file, stream.size(), stream.data());
  if (rtn != sperr::RTNType::Good) {
    std::cerr << "Write compressed file failed: " << output_file << std::endl;
    return 1;
  }

  return 0;
}
