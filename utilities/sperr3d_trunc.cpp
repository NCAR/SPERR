#include "SPERR3D_Stream_Tools.h"

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
  CLI::App app("Truncate a SPERR3D bitstream to a specific bit-per-point (BPP).\n"
                "Optionally, it can also evaluate the compression quality after truncation.\n");

  // Input specification
  auto input_file = std::string();
  app.add_option("filename", input_file,
                 "A SPERR3D bitstream to be truncated.\n")
      ->check(CLI::ExistingFile);

  //
  // Truncation settings
  //
  auto bpp = sperr::max_d;
  app.add_option("--bpp", bpp, "Target bit-per-pixel (BPP) to truncate to.")
      ->required()->group("Truncation settings");

  auto omp_num_threads = size_t{0};  // meaning to use the maximum number of threads.
#ifdef USE_OMP
  app.add_option("--omp", omp_num_threads,
                 "Number of OpenMP threads to use. Default (or 0) to use all.")
      ->group("Truncation settings");
#endif

  //
  // Output settings
  //
  auto o_file = std::string();
  app.add_option("-o", o_file, "Output truncated bitstream.")
      ->group("Output settings");

  CLI11_PARSE(app, argc, argv);

  //
  // A little sanity check
  //
  if (bpp <= 0.0) {
    std::cout << "Target BPP value should be positive!" << std::endl;
    return __LINE__;
  }

  //
  // Really starting the real work!
  //
  auto tool = sperr::SPERR3D_Stream_Tools();
  auto stream_trunc = tool.progressive_read(input_file, bpp);
  if (stream_trunc.empty()) {
    std::cout << "Error while truncating bitstream " << input_file << std::endl;
    return __LINE__;
  }
  auto total_vals = tool.vol_dims[0] * tool.vol_dims[1] * tool.vol_dims[2];
  auto real_bpp = stream_trunc.size() * 8.0 / double(total_vals);
  std::cout << "Truncation resulting BPP = " << real_bpp << std::endl;

  if (!o_file.empty()) {
    auto rtn = sperr::write_n_bytes(o_file, stream_trunc.size(), stream_trunc.data());
    if (rtn != sperr::RTNType::Good)
      return __LINE__;
  }

  return 0;
}
