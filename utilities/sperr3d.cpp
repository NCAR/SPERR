#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("3D SPERR compression and decompression\n");

  // Input specification
  auto input_file = std::string();
  app.add_option("filename", input_file,
                 "A data volume to be compressed, or\n"
                 "a bitstream to be decompressed.")
      ->check(CLI::ExistingFile);

  //
  // Execution settings
  //
  auto cflag = bool{false};
  auto* cptr =
      app.add_flag("-c", cflag, "Perform a compression task.")->group("Execution settings");
  auto dflag = bool{false};
  auto* dptr = app.add_flag("-d", dflag, "Perform a decompression task.")
                   ->excludes(cptr)
                   ->group("Execution settings");

  auto omp_num_threads = size_t{0};  // meaning to use the maximum number of threads.
#ifdef USE_OMP
  app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. 0 is to use all.")
      ->group("Execution settings");
#endif

  //
  // Output settings
  //
  auto out_bitstream = std::string();
  app.add_option("--out_bitstream", out_bitstream, "Output compressed bitstream.")
      ->needs(cptr)
      ->group("Output settings");
  auto out_decomp_f = std::string();
  app.add_option("--out_decomp_f", out_decomp_f, "Output decompressed volume in f32 precision.")
      ->group("Output settings");
  auto out_decomp_d = std::string();
  app.add_option("--out_decomp_d", out_decomp_d, "Output decompressed volume in f64 precision.")
      ->group("Output settings");

  //
  // Compression settings
  //
  auto dims = std::array<size_t, 3>{0, 0, 0};
  app.add_option("--dims", dims,
                 "Dimensions of the input volume. E.g., `--dims 128 128 128`\n"
                 "(The fastest-varying dimension appears first.)")
      ->group("Compression settings");

  auto chunks = std::array<size_t, 3>{256, 256, 256};
  app.add_option("--chunks", chunks,
                 "Dimensions of the preferred chunk size. Default: 256 256 256\n"
                 "(Volume dims don't need to be divisible by these chunk dims.)")
      ->group("Compression settings");

  auto ftype = size_t{0};
  app.add_option("--ftype", ftype, "Specify the input float type in bits: 32 or 64.")
      ->group("Compression settings");

  auto pwe = sperr::max_d;
  auto* pwe_ptr = app.add_option("--pwe", pwe, "Maximum point-wise error (PWE) tolerance.")
                      ->group("Compression settings");

  auto psnr = sperr::max_d;
  auto* psnr_ptr = app.add_option("--psnr", psnr, "Target PSNR to achieve.")
                       ->excludes(pwe_ptr)
                       ->group("Compression settings");

  auto bpp = sperr::max_d;
  auto* bpp_ptr = app.add_option("--bpp", bpp, "Target bit-per-pixel (BPP) to achieve.")
                      ->check(CLI::Range(0.0, 64.0))
                      ->excludes(pwe_ptr)
                      ->excludes(psnr_ptr)
                      ->group("Compression settings");

  auto show_stats = bool{false};
  app.add_flag("--show_stats", show_stats, "Show statistics measuring the compression quality.")
      ->group("Compression settings");

  //
  // Decompression settings
  //
  auto first_bpp = sperr::max_d;
  app.add_option("--first_bpp", first_bpp,
                 "Decompression using partial data, up to a certain BPP.\n"
                 "By default, decompression uses all data in the bitstream.")
      ->needs(dptr)
      ->group("Decompression settings");

  CLI11_PARSE(app, argc, argv);

  //
  // A little extra sanity check.
  //
  if (!cflag && !dflag) {
    std::cout << "Is this compressing (-c) or decompressing (-d) ?" << std::endl;
    return __LINE__;
  }
  if (dims.empty()) {
    std::cout << "What's the dimensions of this 3D volume (--dims) ?" << std::endl;
    return __LINE__;
  }
  if (ftype != 32 && ftype != 64) {
    std::cout << "What's the floating-type precision (--ftype) ?" << std::endl;
    return __LINE__;
  }
  if (pwe == sperr::max_d && psnr == sperr::max_d && bpp == sperr::max_d) {
    std::cout << "What's the compression quality (--psnr, --pwe, --bpp) ?" << std::endl;
    return __LINE__;
  }
  if (pwe <= 0.0 || psnr <= 0.0) {
    std::cout << "Compression quality (--psnr, --pwe) must be positive!" << std::endl;
    return __LINE__;
  }

  //
  // Really starting the real work!
  //
  auto input = sperr::read_whole_file<uint8_t>(input_file);
  if (cflag) {
    const auto total_vals = dims[0] * dims[1] * dims[2];
    if ((ftype == 32 && (total_vals * 4 != input.size())) ||
        (ftype == 64 && (total_vals * 8 != input.size()))) {
      std::cout << "Input file size wrong!" << std::endl;
      return __LINE__;
    }
    auto encoder = std::make_unique<sperr::SPERR3D_OMP_C>();
    encoder->set_dims_and_chunks(dims, chunks);
    encoder->set_num_threads(omp_num_threads);
    if (pwe != sperr::max_d)
      encoder->set_tolerance(pwe);
    else if (psnr != sperr::max_d)
      encoder->set_psnr(psnr);

    auto rtn = sperr::RTNType::Good;
    if (ftype == 32)
      rtn = encoder->compress(reinterpret_cast<const float*>(input.data()), total_vals);
    else
      rtn = encoder->compress(reinterpret_cast<const double*>(input.data()), total_vals);
    if (rtn != sperr::RTNType::Good) {
      std::cout << "Compression failed!" << std::endl;
      return __LINE__;
    }

    // If not calculating stats, we can free up some memory now!
    if (!show_stats) {
      input.clear();
      input.shrink_to_fit();
    }

    auto stream = sperr::vec8_type();
    encoder->append_encoded_bitstream(stream);
    if (!out_bitstream.empty()) {
      rtn = sperr::write_n_bytes(out_bitstream, stream.size(), stream.data());
      if (rtn != sperr::RTNType::Good) {
        std::cout << "Writing compressed bitstream failed: " << out_bitstream << std::endl;
        return __LINE__;
      }
    }
    encoder.reset();  // Free up some more memory.

    // Need to do a decompression in the following cases.
    if (show_stats || !out_decomp_d.empty() || !out_decomp_f.empty()) {
      auto decoder = std::make_unique<sperr::SPERR3D_OMP_D>();
      decoder->set_num_threads(omp_num_threads);
      decoder->setup_decomp(stream.data(), stream.size());
      rtn = decoder->decompress(stream.data());
      if (rtn != sperr::RTNType::Good) {
        std::cout << "Decompression failed!" << std::endl;
        return __LINE__;
      }

      // Output the decompressed volume in double precision.
      auto outputd = decoder->release_decoded_data();
      decoder.reset();  // Free up more memory!
      if (!out_decomp_d.empty()) {
        rtn = sperr::write_n_bytes(out_decomp_d, outputd.size() * sizeof(double), outputd.data());
        if (rtn != sperr::RTNType::Good) {
          std::cout << "Writing decompressed volume failed: " << out_decomp_d << std::endl;
          return __LINE__;
        }
        out_decomp_d.clear();
      }

      // Output the decompressed volume in single precision.
      auto outputf = sperr::vecf_type(total_vals);
      std::copy(outputd.cbegin(), outputd.cend(), outputf.begin());
      if (!out_decomp_f.empty()) {
        rtn = sperr::write_n_bytes(out_decomp_f, outputf.size() * sizeof(float), outputf.data());
        if (rtn != sperr::RTNType::Good) {
          std::cout << "Writing decompressed volume failed: " << out_decomp_f << std::endl;
          return __LINE__;
        }
        out_decomp_f.clear();
      }

      // Calculate statistics.
      if (show_stats) {
        const double bpp = stream.size() * 8.0 / total_vals;
        double rmse, linfy, psnr, min, max, sigma;
        if (ftype == 32) {
          const float* inputf = reinterpret_cast<const float*>(input.data());
          auto stats = sperr::calc_stats(inputf, outputf.data(), total_vals, omp_num_threads);
          auto mean_var = sperr::calc_mean_var(inputf, total_vals, omp_num_threads);
          rmse = stats[0];
          linfy = stats[1];
          psnr = stats[2];
          min = stats[3];
          max = stats[4];
          sigma = std::sqrt(mean_var[1]);
        }
        else {
          const double* inputd = reinterpret_cast<const double*>(input.data());
          auto stats = sperr::calc_stats(inputd, outputd.data(), total_vals, omp_num_threads);
          auto mean_var = sperr::calc_mean_var(inputd, total_vals, omp_num_threads);
          rmse = stats[0];
          linfy = stats[1];
          psnr = stats[2];
          min = stats[3];
          max = stats[4];
          sigma = std::sqrt(mean_var[1]);
        }
        std::printf("Input range = (%.2e, %.2e), L-Infty = %.2e\n", min, max, linfy);
        std::printf("Bitrate = %.2fBPP, PSNR = %.2fdB, Accuracy Gain = %.2f\n", bpp, psnr,
                    std::log2(sigma / rmse) - bpp);
        show_stats = false;
      }
    }
  }
  //
  // Decompression
  //
  else {
    assert(dflag);
  }

  return 0;
}
