#include <cassert>
#include "SPECK2D_Decompressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("Decompress a 2D SPERR bitstream and output a reconstructed slice\n");

  auto input_file = std::string();
  app.add_option("filename", input_file, "Input bitstream to the decompressor")
      ->check(CLI::ExistingFile)
      ->group("Input Specifications")
      ->required();

  auto output_file = std::string();
  app.add_option("-o", output_file, "Output filename")->group("Output Specifications")->required();

  auto output_double = bool{false};
  app.add_flag("-d", output_double,
               "Specify to output data in double type.\n"
               "Data is output as float by default.")
      ->group("Output Specifications");

#ifndef QZ_TERM
  // Partial bitstream decompression is only applicable to fixed-size mode.
  auto decomp_bpp = double{0.0};
  app.add_option("--bpp", decomp_bpp,
                 "Partially decode the bitstream up to a certain bit rate.\n"
                 "If not specified, the entire bitstream will be decoded.")
      ->check(CLI::Range(0.0, 64.0))
      ->group("Decompression Options");
#endif

  CLI11_PARSE(app, argc, argv);

  // Read in the input stream
  const auto in_stream = sperr::read_whole_file<uint8_t>(input_file);
  if (in_stream.empty()) {
    std::cerr << "Read input stream error: " << input_file << std::endl;
    return 1;
  }

  // Use a decompressor
  SPECK2D_Decompressor decompressor;
  auto rtn = decompressor.use_bitstream(in_stream.data(), in_stream.size());
  if (rtn != RTNType::Good) {
    std::cerr << "Use input stream error!" << std::endl;
    return 1;
  }

#ifndef QZ_TERM
  decompressor.set_bpp(decomp_bpp);
#endif

  rtn = decompressor.decompress();
  if (rtn != RTNType::Good) {
    std::cerr << "Decompression failed!" << std::endl;
    return 1;
  }

  if (output_double) {
    const auto vol = decompressor.view_data();
    if (vol.empty())
      return 1;
    rtn = sperr::write_n_bytes(output_file, vol.size() * sizeof(double), vol.data());
  }
  else {
    const auto vol = decompressor.get_data<float>();
    if (vol.empty())
      return 1;
    rtn = sperr::write_n_bytes(output_file, vol.size() * sizeof(float), vol.data());
  }
  if (rtn != RTNType::Good) {
    std::cerr << "Write to disk failed: " << output_file << std::endl;
    return 1;
  }

  return 0;
}
