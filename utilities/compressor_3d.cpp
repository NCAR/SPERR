#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"

#include "CLI11.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("Compress a 3D data volume\n");

  // Input specifications
  auto input_file = std::string();
  app.add_option("filename", input_file, "Input data file to the compressor")
      ->required()
      ->check(CLI::ExistingFile)
      ->group("Input Specifications");

  auto dims = std::vector<size_t>();
  app.add_option("--dims", dims,
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

  // Output specifications
  auto out_bitstream = std::string();
  app.add_option("--out_bitstream", out_bitstream, "Output compressed bitstream")
      ->group("Output Specifications");
  auto out_decomp_f = std::string();
  app.add_option("--out_decomp_f", out_decomp_f, "Output decompressed volume in single precision")
      ->group("Output Specifications");
  auto out_decomp_d = std::string();
  app.add_option("--out_decomp_d", out_decomp_d, "Output decompressed volume in double precision")
      ->group("Output Specifications");

  auto show_stats = bool{false};
  app.add_flag("--show_stats", show_stats, "Show statistics measuring the compression quality.")
      ->group("Output Specifications");

  // Execution specifications
  auto chunks = std::vector<size_t>{256, 256, 256};
  app.add_option("--chunks", chunks,
                 "Dimensions of the preferred chunk size. Default: 256 256 256\n"
                 "(Volume dims don't need to be divisible of these chunk dims.)")
      ->expected(3)
      ->group("Execution Specifications");

  auto omp_num_threads = size_t{0};  // meaning to use the maximum number of threads.
#ifdef USE_OMP
  app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use.")
      ->group("Execution Specifications");
#endif

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
    bit_budget -= 500;  // Just need to be different from the max size value.
  const auto mode = sperr::compression_mode(bit_budget, psnr, pwe);
  if (mode == sperr::CompMode::Unknown) {
    std::cout << "Compression mode is unclear. Did you give one and only one "
                 "compression specification?"
              << std::endl;
    return __LINE__;
  }

  // Do some sanity check on compression parameters
  if (mode == sperr::CompMode::FixedSize) {
    if (bpp <= 0.0 || bpp >= 64.0) {
      std::cout << "Bit-per-pixel value must be between 0.0 and 64.0!" << std::endl;
      return __LINE__;
    }
  }
  else if (mode == sperr::CompMode::FixedPSNR) {
    if (psnr <= 0.0) {
      std::cout << "Target PSNR must be positive!" << std::endl;
      return __LINE__;
    }
  }
  else if (mode == sperr::CompMode::FixedPWE) {
    if (pwe <= 0.0) {
      std::cout << "Target PWE must be positive!" << std::endl;
      return __LINE__;
    }
  }

  // Read the input file
  const size_t total_vals = dims[0] * dims[1] * dims[2];
  auto orig = sperr::read_whole_file<uint8_t>(input_file);
  if ((use_double && orig.size() != total_vals * sizeof(double)) ||
      (!use_double && orig.size() != total_vals * sizeof(float))) {
    std::cerr << "Read input file error: " << input_file << std::endl;
    return __LINE__;
  }

  // Use a compressor and take the input data
  auto compressor = std::make_unique<SPERR3D_OMP_C>();
  compressor->set_num_threads(omp_num_threads);
  auto rtn = sperr::RTNType::Good;
  if (use_double) {
    rtn = compressor->copy_data(reinterpret_cast<const double*>(orig.data()), total_vals,
                                {dims[0], dims[1], dims[2]}, {chunks[0], chunks[1], chunks[2]});
  }
  else {
    rtn = compressor->copy_data(reinterpret_cast<const float*>(orig.data()), total_vals,
                                {dims[0], dims[1], dims[2]}, {chunks[0], chunks[1], chunks[2]});
  }
  if (rtn != sperr::RTNType::Good) {
    std::cerr << "Copy data failed!" << std::endl;
    return __LINE__;
  }

  // Free up memory if we don't need to compute stats
  if (!show_stats) {
    orig.clear();
    orig.shrink_to_fit();
  }

  // Tell the compressor which compression mode to use.
  switch (mode) {
    case sperr::CompMode::FixedSize:
      rtn = compressor->set_target_bpp(bpp);
      break;
    case sperr::CompMode::FixedPSNR:
      compressor->set_target_psnr(psnr);
      break;
    default:
      compressor->set_target_pwe(pwe);
      break;
  }
  if (rtn != sperr::RTNType::Good) {
    std::cerr << "Set bit-per-pixel failed!" << std::endl;
    return __LINE__;
  }

  // Perform the actual compression
  rtn = compressor->compress();
  switch (rtn) {
    case sperr::RTNType::QzLevelTooBig:
      std::cerr << "Compression failed because `qz` is set too big!" << std::endl;
      return __LINE__;
    case sperr::RTNType::Good:
      break;
    default:
      std::cerr << "Compression failed!" << std::endl;
      return __LINE__;
  }

  // Get a hold of the encoded bitstream.
  auto stream = compressor->get_encoded_bitstream();
  if (stream.empty()) {
    std::cerr << "Compression bitstream empty!" << std::endl;
    return __LINE__;
  }

  // Write out the encoded bitstream.
  if (!out_bitstream.empty()) {
    rtn = sperr::write_n_bytes(out_bitstream, stream.size(), stream.data());
    if (rtn != sperr::RTNType::Good) {
      std::cerr << "Write compressed file failed!" << std::endl;
      return __LINE__;
    }
  }

  // Calculate and print statistics, and/or output the decompressed volume.
  if (show_stats || !out_decomp_f.empty() || !out_decomp_d.empty()) {
    const auto out_stats = compressor->get_outlier_stats();
    compressor.reset(nullptr);  // Free up some memory

    const auto bpp = stream.size() * 8.0 / total_vals;

    // Use a decompressor to decompress and collect error statistics.
    SPERR3D_OMP_D decompressor;
    decompressor.set_num_threads(omp_num_threads);
    rtn = decompressor.use_bitstream(stream.data(), stream.size());
    if (rtn != RTNType::Good)
      return __LINE__;

    rtn = decompressor.decompress(stream.data());
    if (rtn != RTNType::Good)
      return __LINE__;

    if (show_stats) {
      // Make sure that we have a copy of the original data in double precision.
      const double* orig_ptr = nullptr;
      auto orig_tmp = sperr::vecd_type();
      if (use_double)
        orig_ptr = reinterpret_cast<const double*>(orig.data());
      else {
        const float* orig_f = reinterpret_cast<const float*>(orig.data());
        orig_tmp.resize(total_vals);
        std::copy(orig_f, orig_f + total_vals, orig_tmp.begin());
        orig_ptr = orig_tmp.data();
      }

      const auto& recover = decompressor.view_data();
      assert(recover.size() == total_vals);
      auto stats = sperr::calc_stats(orig_ptr, recover.data(), recover.size(), omp_num_threads);
      auto var = sperr::calc_variance(orig_ptr, total_vals, omp_num_threads);
      auto sigma = std::sqrt(var);
      auto gain = std::log2(sigma / stats[0]) - bpp;
      std::cout << "Average BPP = " << bpp << ", PSNR = " << stats[2]
                << "dB, L-Infty = " << stats[1] << ", Accuracy Gain = " << gain << std::endl;
      std::printf("Input data range = %.2e (%.2e, %.2e).\n", (stats[4] - stats[3]), stats[3],
                  stats[4]);

      if (mode == sperr::CompMode::FixedPWE) {
        // Also collect outlier statistics
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
    }  // Finish printing stats

    if (!out_decomp_f.empty()) {
      const auto recover = decompressor.get_data<float>();
      assert(recover.size() == total_vals);
      rtn = sperr::write_n_bytes(out_decomp_f, recover.size() * sizeof(float), recover.data());
      if (rtn != sperr::RTNType::Good) {
        std::cerr << "Write decompressed file failed!" << std::endl;
        return __LINE__;
      }
    }

    if (!out_decomp_d.empty()) {
      const auto& recover = decompressor.view_data();
      assert(recover.size() == total_vals);
      rtn = sperr::write_n_bytes(out_decomp_d, recover.size() * sizeof(double), recover.data());
      if (rtn != sperr::RTNType::Good) {
        std::cerr << "Write decompressed file failed!" << std::endl;
        return __LINE__;
      }
    }
  }  // Finish using the decompressor.

  return 0;
}
