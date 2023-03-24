#include "SPERR3D.h"

#include "CLI11.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("Experiment with mid-tread integer 3D SPECK.\n");

  // Input specifications
  auto input_file = std::string();
  app.add_option("filename", input_file, "Input data file to the compressor")
      ->required()
      ->check(CLI::ExistingFile)
      ->group("Input Specifications");

  auto dimsv = std::vector<size_t>();
  app.add_option("--dims", dimsv,
                 "Dimensions of the input volume. E.g., `--dims 128 128 128`\n"
                 "(The fastest-varying dimension appears first.)")
      ->expected(3)
      ->required()
      ->group("Input Specifications");

  auto use_double = bool{false};
  app.add_flag("-d", use_double,
               "Specify that input data is in double type.\n"
               "Data is treated as float by default.")
      ->group("Input Specifications");

  auto omp_num_threads = size_t{0};  // meaning to use the maximum number of threads.
#ifdef USE_OMP
  app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use.")
      ->group("Execution Specifications");
#endif

  // Compression specifications
  auto pwe = double{0.0};
  app.add_option("--pwe", pwe, "Maximum point-wise error tolerance.")
      ->group("Compression Specifications (must choose one and only one)");

  // Output specifications
  // auto zipfile = std::string();
  // app.add_option("-z", zipfile, "Compressed bitstream.")->group("Output specifications");
  // auto reconfile = std::string();
  // app.add_option("-r", reconfile, "Reconstructed file.")->group("Output specifications");

  CLI11_PARSE(app, argc, argv);

  // Read the input file
  const auto dims = sperr::dims_type{dimsv[0], dimsv[1], dimsv[2]};
  const size_t total_vals = dims[0] * dims[1] * dims[2];
  auto orig = sperr::read_whole_file<uint8_t>(input_file);
  if ((use_double && orig.size() != total_vals * sizeof(double)) ||
      (!use_double && orig.size() != total_vals * sizeof(float))) {
    std::cerr << "Read input file error: " << input_file << std::endl;
    return 1;
  }

  // Use a compressor
  auto encoder = sperr::SPERR3D();
  encoder.set_dims(dims);
  encoder.set_q(1.5 * pwe);
  if (use_double)
    encoder.copy_data(reinterpret_cast<const double*>(orig.data()), total_vals);
  else
    encoder.copy_data(reinterpret_cast<const float*>(orig.data()), total_vals);
  auto rtn = encoder.compress();
  if (rtn == sperr::RTNType::FE_Invalid) {
    std::cerr << "FE_Invalid detected!" << std::endl;
    return 1;
  }
  // else
  //   std::cerr << "No FE_Invalid detected!" << std::endl;
  auto bitstream = encoder.get_encoded_bitstream();
  const auto bpp = double(bitstream.size() * 8) / double(total_vals);
  std::printf("%.4f,  ", bpp);

  // Write out the encoded bitstream
  // sperr::write_n_bytes(zipfile, bitstream.size(), bitstream.data());

  // Use a decompressor
  auto decoder = sperr::SPERR3D();
  decoder.set_dims(dims);
  decoder.set_q(1.5 * pwe);
  decoder.use_bitstream(bitstream.data(), bitstream.size());
  decoder.decompress();
  auto output = decoder.release_decoded_data();
  // sperr::write_n_bytes(reconfile, 8 * output.size(), output.data());

  // Compute statistics
  const double* input_p = nullptr;
  std::vector<double> input_v;
  if (use_double)
    input_p = reinterpret_cast<const double*>(orig.data());
  else {
    const float* tmp = reinterpret_cast<const float*>(orig.data());
    input_v.resize(total_vals);
    std::copy(tmp, tmp + total_vals, input_v.begin());
    input_p = input_v.data();
  }
  auto [rmse, infy, psnr, min, max] = sperr::calc_stats(input_p, output.data(), total_vals, 4);
  auto [mean, var] = sperr::calc_mean_var(input_p, total_vals, 4);
  auto sigma = std::sqrt(var);
  auto gain = std::log2(sigma / rmse) - bpp;
  std::printf("%.4f,  %.2e\n", gain, rmse);

  return 0;
}
