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

  // Input specifications
  auto input_file = std::string();
  app.add_option("filename", input_file, "Input file to the compressor")
      ->required()
      ->check(CLI::ExistingFile)
      ->group("Input Specifications");

  auto dims = std::vector<size_t>();
  app.add_option("--dims", dims,
                 "Dimensions of the input 2D slice. E.g., `--dims 128 128`\n"
                 "(The fastest-varying dimension appears first.)")
      ->expected(2)
      ->required()
      ->group("Input Specifications");

  auto use_double = bool{false};
  app.add_flag("-d", use_double,
               "Specify that input data is in double type.\n"
               "Data is treated as float by default.")
      ->group("Input Specifications");

  // Output specification
  auto output_file = std::string();
  app.add_option("-o", output_file, "Output filename")->group("Output Specifications");

  auto show_stats = bool{false};
  app.add_flag("--show_stats", show_stats, "Show statistics measuring the compression quality.")
      ->group("Output Specifications");

  // Compression specifications
  auto bpp = sperr::max_d;
  app.add_option("--bpp", bpp, "Target bit-per-pixel to achieve.")
      ->group("Compression Specifications (must choose one and only one)");

  auto pwe = double{0.0};
  app.add_option("--pwe", pwe, "Maximum point-wise error tolerance.")
      ->group("Compression Specifications (must choose one and only one)");

  auto psnr = sperr::max_d;
  app.add_option("--psnr", psnr, "Target PSNR to achieve.")
      ->group("Compression Specifications (must choose one and only one)");

  CLI11_PARSE(app, argc, argv);

  // Make sure that we have a valid compression mode
  auto bit_budget = sperr::max_size;
  if (bpp != sperr::max_d)
    bit_budget -= 100;  // Just need to be different from the max size value.
  const auto mode = sperr::compression_mode(bit_budget, psnr, pwe);
  if (mode == sperr::CompMode::Unknown) {
    std::cout << "Compression mode is unclear. Did you give one and only one "
                 "compression specification?"
              << std::endl;
    return 1;
  }

  // Do some sanity check on compression parameters
  if (mode == sperr::CompMode::FixedSize) {
    if (bpp <= 0.0 || bpp >= 64.0) {
      std::cout << "Bit-per-pixel value must be between 0.0 and 64.0!" << std::endl;
      return 1;
    }
  }
  else if (mode == sperr::CompMode::FixedPSNR) {
    if (psnr <= 0.0) {
      std::cout << "Target PSNR must be positive!" << std::endl;
      return 1;
    }
  }
  else if (mode == sperr::CompMode::FixedPWE) {
    if (pwe <= 0.0) {
      std::cout << "Target PWE must be positive!" << std::endl;
      return 1;
    }
  }

  // Read in the input file
  const size_t total_vals = dims[0] * dims[1];
  auto orig = sperr::read_whole_file<uint8_t>(input_file);
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

  // Free up memory if we don't need to compute stats
  if (!show_stats) {
    orig.clear();
    orig.shrink_to_fit();
  }

  // Tell the compressor which compression mode to use.
  switch (mode) {
    case sperr::CompMode::FixedSize:
      rtn = compressor.set_target_bpp(bpp);
      break;
    case sperr::CompMode::FixedPSNR:
      compressor.set_target_psnr(psnr);
      break;
    default:
      compressor.set_target_pwe(pwe);
      break;
  }
  if (rtn != RTNType::Good) {
    std::cerr << "Set bit-per-pixel failed!" << std::endl;
    return 1;
  }

  // Perform the actual compression
  rtn = compressor.compress();
  switch (rtn) {
    case sperr::RTNType::QzLevelTooBig:
      std::cerr << "Compression failed because `qz` is set too big!" << std::endl;
      return 1;
    case sperr::RTNType::Good:
      break;
    default:
      std::cout << "Compression failed!" << std::endl;
      return 1;
  }

  // Get a hold of the encoded bitstream.
  const auto& stream = compressor.view_encoded_bitstream();
  if (stream.empty()) {
    std::cerr << "Compression bitstream empty!" << std::endl;
    return 1;
  }

  // Calculate and print statistics
  if (show_stats) {
    const auto bpp = stream.size() * 8.0 / total_vals;

    // Use a decompressor to decompress and collect error statistics
    SPERR2D_Decompressor decompressor;
    rtn = decompressor.use_bitstream(stream.data(), stream.size());
    if (rtn != RTNType::Good)
      return 1;

    rtn = decompressor.decompress();
    if (rtn != RTNType::Good)
      return 1;

    if (use_double) {
      const auto& recover = decompressor.view_data();
      assert(recover.size() * sizeof(double) == orig.size());
      auto stats = sperr::calc_stats(reinterpret_cast<const double*>(orig.data()), recover.data(),
                                     recover.size());
      auto var = sperr::calc_variance(reinterpret_cast<const double*>(orig.data()), total_vals);
      auto sigma = std::sqrt(var);
      auto gain = std::log2(sigma / stats[0]) - bpp;
      std::cout << "Average BPP = " << bpp << ", PSNR = " << stats[2]
                << "dB, L-Infty = " << stats[1] << ", Accuracy Gain = " << gain << std::endl;
      std::printf("Input data range = %.2e (%.2e, %.2e).\n", (stats[4] - stats[3]), stats[3],
                  stats[4]);
    }
    else {
      const auto recover = decompressor.get_data<float>();
      assert(recover.size() * sizeof(float) == orig.size());
      auto stats = sperr::calc_stats(reinterpret_cast<const float*>(orig.data()), recover.data(),
                                     recover.size());
      auto var = sperr::calc_variance(reinterpret_cast<const float*>(orig.data()), total_vals);
      auto sigma = std::sqrt(var);
      auto gain = std::log2(sigma / stats[0]) - float(bpp);
      std::cout << "Average BPP = " << bpp << ", PSNR = " << stats[2]
                << "dB, L-Infty = " << stats[1] << ", Accuracy Gain = " << gain << std::endl;
      std::printf("Input data range = %.2e (%.2e, %.2e).\n", (stats[4] - stats[3]), stats[3],
                  stats[4]);
    }

    if (mode == sperr::CompMode::FixedPWE) {
      // Also collect outlier statistics
      const auto out_stats = compressor.get_outlier_stats();
      if (out_stats.first == 0) {
        std::cout << "There were no outliers corrected!\n";
      }
      else {
        std::printf(
            "There were %ld outliers, percentage of total data points = %.2f%%.\n"
            "Correcting them takes bpp = %.2f, percentage of total storage = %.2f%%.\n",
            out_stats.first, double(out_stats.first * 100) / double(total_vals),
            double(out_stats.second * 8) / double(out_stats.first),
            double(out_stats.second * 100) / double(stream.size()));
      }
    }
  }

  // Write out the encoded bitstream
  if (!output_file.empty()) {
    rtn = sperr::write_n_bytes(output_file, stream.size(), stream.data());
    if (rtn != sperr::RTNType::Good) {
      std::cerr << "Write compressed file failed: " << output_file << std::endl;
      return 1;
    }
  }

  return 0;
}
