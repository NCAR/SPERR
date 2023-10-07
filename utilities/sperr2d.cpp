#include "SPECK2D_FLT.h"

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
  CLI::App app("2D SPERR compression and decompression\n");

  // Input specification
  auto input_file = std::string();
  app.add_option("filename", input_file,
                 "A data slice to be compressed, or\n"
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

  //
  // Input properties
  //
  auto ftype = size_t{0};
  app.add_option("--ftype", ftype, "Specify the input float type in bits. Must be 32 or 64.")
      ->group("Input properties");

  auto dim2d = std::array<uint32_t, 2>{0, 0};
  app.add_option("--dims", dim2d,
                 "Dimensions of the input slice. E.g., `--dims 128 128`\n"
                 "(The fastest-varying dimension appears first.)")
      ->group("Input properties");

  //
  // Output settings
  //
  auto o_bitstream = std::string();
  app.add_option("--o_bitstream", o_bitstream, "Output compressed bitstream.")
      ->group("Output settings");

  auto o_decomp_f32 = std::string();
  app.add_option("--o_decomp_f", o_decomp_f32, "Output decompressed slice in f32 precision.")
      ->group("Output settings");

  auto o_decomp_f64 = std::string();
  app.add_option("--o_decomp_d", o_decomp_f64, "Output decompressed slice in f64 precision.")
      ->group("Output settings");

  auto print_stats = bool{false};
  app.add_flag("--print_stats", print_stats, "Show statistics measuring the compression quality.")
      ->needs(cptr)
      ->group("Output settings");

  //
  // Compression settings
  //
  auto pwe = 0.0;
  auto* pwe_ptr = app.add_option("--pwe", pwe, "Maximum point-wise error (PWE) tolerance.")
                      ->group("Compression settings");

  auto psnr = 0.0;
  auto* psnr_ptr = app.add_option("--psnr", psnr, "Target PSNR to achieve.")
                       ->excludes(pwe_ptr)
                       ->group("Compression settings");

  auto bpp = 0.0;
  auto* bpp_ptr = app.add_option("--bpp", bpp, "Target bit-per-pixel (bpp) to achieve.")
                      ->check(CLI::Range(0.0, 64.0))
                      ->excludes(pwe_ptr)
                      ->excludes(psnr_ptr)
                      ->group("Compression settings");

  CLI11_PARSE(app, argc, argv);

  //
  // A little extra sanity check.
  //
  if (!cflag && !dflag) {
    std::cout << "Is this compressing (-c) or decompressing (-d) ?" << std::endl;
    return __LINE__;
  }
  if (cflag && dim2d == std::array{uint32_t{0}, uint32_t{0}}) {
    std::cout << "What's the dimensions of this 2D slice (--dims) ?" << std::endl;
    return __LINE__;
  }
  if (cflag && ftype != 32 && ftype != 64) {
    std::cout << "What's the floating-type precision (--ftype) ?" << std::endl;
    return __LINE__;
  }
  if (cflag && pwe == 0.0 && psnr == 0.0 && bpp == 0.0) {
    std::cout << "What's the compression quality (--psnr, --pwe, --bpp) ?" << std::endl;
    return __LINE__;
  }
  if (cflag && (pwe < 0.0 || psnr < 0.0)) {
    std::cout << "Compression quality (--psnr, --pwe) must be positive!" << std::endl;
    return __LINE__;
  }
  if (dflag && o_decomp_f32.empty() && o_decomp_f64.empty()) {
    std::cout << "Where to output the decompressed file (--o_decomp_f32, --o_decomp_f64) ?"
              << std::endl;
    return __LINE__;
  }

  //
  // Really starting the real work!
  // Note: the header has the same format as in SPERR3D_OMP_C.
  //
  const auto header_len = 10ul;
  auto input = sperr::read_whole_file<uint8_t>(input_file);
  if (cflag) {
    const auto dims = sperr::dims_type{dim2d[0], dim2d[1], 1ul};
    const auto total_vals = dims[0] * dims[1] * dims[2];
    if ((ftype == 32 && (total_vals * 4 != input.size())) ||
        (ftype == 64 && (total_vals * 8 != input.size()))) {
      std::cout << "Input file size wrong!" << std::endl;
      return __LINE__;
    }
    auto encoder = std::make_unique<sperr::SPECK2D_FLT>();
    encoder->set_dims(dims);
    if (ftype == 32)
      encoder->copy_data(reinterpret_cast<const float*>(input.data()), total_vals);
    else
      encoder->copy_data(reinterpret_cast<const double*>(input.data()), total_vals);

    if (pwe != 0.0)
      encoder->set_tolerance(pwe);
    else if (psnr != 0.0)
      encoder->set_psnr(psnr);
    else {
      assert(bpp != 0.0);
      encoder->set_bitrate(bpp);
    }

    // If not calculating stats, we can free up some memory now!
    if (!print_stats) {
      input.clear();
      input.shrink_to_fit();
    }

    auto rtn = encoder->compress();
    if (rtn != sperr::RTNType::Good) {
      std::cout << "Compression failed!" << std::endl;
      return __LINE__;
    }

    // Assemble the output bitstream.
    auto stream = sperr::vec8_type(header_len);
    stream[0] = static_cast<uint8_t>(SPERR_VERSION_MAJOR);
    const auto b8 = std::array{false,  // not a portion
                               false,  // 2D
                               ftype == 32,
                               false,   // unused
                               false,   // unused
                               false,   // unused
                               false,   // unused
                               false};  // unused
    stream[1] = sperr::pack_8_booleans(b8);
    std::memcpy(stream.data() + 2, dim2d.data(), sizeof(dim2d));
    encoder->append_encoded_bitstream(stream);
    if (!o_bitstream.empty()) {
      rtn = sperr::write_n_bytes(o_bitstream, stream.size(), stream.data());
      if (rtn != sperr::RTNType::Good) {
        std::cout << "Writing compressed bitstream failed: " << o_bitstream << std::endl;
        return __LINE__;
      }
    }
    encoder.reset();  // Free up some more memory.

    //
    // Need to do a decompression in the following cases.
    //
    if (print_stats || !o_decomp_f64.empty() || !o_decomp_f32.empty()) {
      auto decoder = std::make_unique<sperr::SPECK2D_FLT>();
      decoder->set_dims(dims);
      // !! Remember the header thing !!
      decoder->use_bitstream(stream.data() + header_len, stream.size() - header_len);
      rtn = decoder->decompress();
      if (rtn != sperr::RTNType::Good) {
        std::cout << "Decompression failed!" << std::endl;
        return __LINE__;
      }

      // If specified, output the decompressed slice in double precision.
      auto outputd = decoder->release_decoded_data();
      decoder.reset();  // Free up more memory!
      if (!o_decomp_f64.empty()) {
        rtn = sperr::write_n_bytes(o_decomp_f64, outputd.size() * sizeof(double), outputd.data());
        if (rtn != sperr::RTNType::Good) {
          std::cout << "Writing decompressed data failed: " << o_decomp_f64 << std::endl;
          return __LINE__;
        }
        o_decomp_f64.clear();
      }

      // If specified, output the decompressed slice in single precision.
      auto outputf = sperr::vecf_type(total_vals);
      std::copy(outputd.cbegin(), outputd.cend(), outputf.begin());
      if (!o_decomp_f32.empty()) {
        rtn = sperr::write_n_bytes(o_decomp_f32, outputf.size() * sizeof(float), outputf.data());
        if (rtn != sperr::RTNType::Good) {
          std::cout << "Writing decompressed data failed: " << o_decomp_f32 << std::endl;
          return __LINE__;
        }
        o_decomp_f32.clear();
      }

      // Calculate statistics.
      if (print_stats) {
        const double bpp = stream.size() * 8.0 / total_vals;
        double rmse, linfy, psnr, min, max, sigma;
        if (ftype == 32) {
          const float* inputf = reinterpret_cast<const float*>(input.data());
          auto stats = sperr::calc_stats(inputf, outputf.data(), total_vals);
          auto mean_var = sperr::calc_mean_var(inputf, total_vals);
          rmse = stats[0];
          linfy = stats[1];
          psnr = stats[2];
          min = stats[3];
          max = stats[4];
          sigma = std::sqrt(mean_var[1]);
        }
        else {
          const double* inputd = reinterpret_cast<const double*>(input.data());
          auto stats = sperr::calc_stats(inputd, outputd.data(), total_vals);
          auto mean_var = sperr::calc_mean_var(inputd, total_vals);
          rmse = stats[0];
          linfy = stats[1];
          psnr = stats[2];
          min = stats[3];
          max = stats[4];
          sigma = std::sqrt(mean_var[1]);
        }
        std::printf("Input range = (%.2e, %.2e), L-Infty = %.2e\n", min, max, linfy);
        std::printf("Bitrate = %.2f, PSNR = %.2fdB, Accuracy Gain = %.2f\n", bpp, psnr,
                    std::log2(sigma / rmse) - bpp);
        print_stats = false;
      }

      assert(o_decomp_f32.empty() && o_decomp_f64.empty() && !print_stats);
    }
  }
  //
  // Decompression
  //
  else {
    assert(dflag);
    // First, retrieve the slice dimension from the header.
    std::memcpy(dim2d.data(), input.data(), sizeof(dim2d));
    const auto dims = sperr::dims_type{dim2d[0], dim2d[1], 1ul};
    auto decoder = std::make_unique<sperr::SPECK2D_FLT>();
    decoder->set_dims(dims);
    decoder->use_bitstream(input.data() + header_len, input.size() - header_len);
    auto rtn = decoder->decompress();
    if (rtn != sperr::RTNType::Good) {
      std::cout << "Decompression failed!" << std::endl;
      return __LINE__;
    }

    // If specified, output the decompressed slice in double precision.
    auto outputd = decoder->release_decoded_data();
    decoder.reset();  // Free up some memory!
    if (!o_decomp_f64.empty()) {
      rtn = sperr::write_n_bytes(o_decomp_f64, outputd.size() * sizeof(double), outputd.data());
      if (rtn != sperr::RTNType::Good) {
        std::cout << "Writing decompressed data failed: " << o_decomp_f64 << std::endl;
        return __LINE__;
      }
      o_decomp_f64.clear();
    }

    // If specified, output the decompressed slice in single precision.
    if (!o_decomp_f32.empty()) {
      auto outputf = sperr::vecf_type(outputd.size());
      std::copy(outputd.cbegin(), outputd.cend(), outputf.begin());
      rtn = sperr::write_n_bytes(o_decomp_f32, outputf.size() * sizeof(float), outputf.data());
      if (rtn != sperr::RTNType::Good) {
        std::cout << "Writing decompressed data failed: " << o_decomp_f32 << std::endl;
        return __LINE__;
      }
      o_decomp_f32.clear();
    }

    assert(o_decomp_f32.empty() && o_decomp_f64.empty());
  }

  return 0;
}
