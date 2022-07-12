#include "SPERR3D_OMP_D.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("Decompress a 3D SPERR bitstream and output a reconstructed volume\n");

  // Input specifications
  auto input_file = std::string();
  app.add_option("input_filename", input_file, "Input bitstream to the decompressor")
      ->check(CLI::ExistingFile)
      ->group("Input Specifications")
      ->required();

  // Output Specifications
  auto output_file = std::string();
  app.add_option("-o", output_file, "Output filename")->group("Output Specifications")->required();

  auto output_double = bool{false};
  app.add_flag("-d", output_double,
               "Specify to output data to be in double type.\n"
               "Data is output as float by default.")
      ->group("Output Specifications");

  auto omp_num_threads = size_t{0}; // meaning to use the maximum number of threads.
#ifdef USE_OMP
  app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. Default: 4")
      ->group("Decompression Options");
#endif

  CLI11_PARSE(app, argc, argv);

  //
  // Let's do the actual work
  //
  auto in_stream = sperr::read_whole_file<uint8_t>(input_file);
  if (in_stream.empty()) {
    std::cerr << "Read input stream error: " << input_file << std::endl;
    return 1;
  }
  SPERR3D_OMP_D decompressor;
  decompressor.set_num_threads(omp_num_threads);
  if (decompressor.use_bitstream(in_stream.data(), in_stream.size()) != sperr::RTNType::Good) {
    std::cerr << "Read compressed file error: " << input_file << std::endl;
    return 1;
  }

  if (decompressor.decompress(in_stream.data()) != sperr::RTNType::Good) {
    std::cerr << "Decompression failed!" << std::endl;
    return 1;
  }

  // Let's free up some memory here
  in_stream.clear();
  in_stream.shrink_to_fit();

  if (output_double) {
    const auto vol = decompressor.view_data();
    if (vol.empty())
      return 1;
    if (sperr::write_n_bytes(output_file, vol.size() * sizeof(double), vol.data()) !=
        sperr::RTNType::Good) {
      std::cerr << "Write to disk failed: " << output_file << std::endl;
      return 1;
    }
  }
  else {
    const auto vol = decompressor.get_data<float>();
    if (vol.empty())
      return 1;
    if (sperr::write_n_bytes(output_file, vol.size() * sizeof(float), vol.data()) !=
        sperr::RTNType::Good) {
      std::cerr << "Write to disk failed: " << output_file << std::endl;
      return 1;
    }
  }

  return 0;
}
