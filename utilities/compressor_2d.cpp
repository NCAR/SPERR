#include "SPECK2D_Compressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
  // Parse command line options
  CLI::App app("");

  std::string input_file;
  app.add_option("filename", input_file, "Input file to the compressor")
      ->required()
      ->group("Input Specifications")
      ->check(CLI::ExistingFile);

  std::vector<size_t> dims;
  app.add_option("--dims", dims,
                 "Dimensions of the input 2D plane. \n"
                 "For example, `--dims 128 128`.\n")
      ->expected(2)
      ->required()
      ->group("Input Specifications");

  auto use_double = bool{false};
  app.add_flag("-d", use_double,
               "Specify that input data is in double type.\n"
               "Data is treated as float by default.")
      ->group("Input Specifications");

  auto bpp = double{0.0};
  app.add_option("--bpp", bpp, "Average bit-per-pixel")
      ->check(CLI::Range(0.0, 64.0))
      ->required()
      ->group("Compression Parameters");

  std::string output_file;
  app.add_option("-o", output_file, "Output filename.")->required();

  CLI11_PARSE(app, argc, argv);

  // Read in the input file
  const size_t total_vals = dims[0] * dims[1];
  auto orig = sperr::read_whole_file<uint8_t>(input_file);
  if ((use_double && orig.size() != total_vals * sizeof(double)) ||
      (!use_double && orig.size() != total_vals * sizeof(float))) {
    std::cerr << "Read input file error: " << input_file << std::endl;
    return 1;
  }

  auto rtn = sperr::RTNType{RTNType::Good};
  auto compressor = SPECK2D_Compressor();

  // Pass data to the compressor
  if (use_double)
    rtn = compressor.copy_data(reinterpret_cast<const double*>(orig.data()),
                               orig.size() / sizeof(double), {dims[0], dims[1], 1});
  else
    rtn = compressor.copy_data(reinterpret_cast<const float*>(orig.data()),
                               orig.size() / sizeof(float), {dims[0], dims[1], 1});
  if (rtn != RTNType::Good) {
    std::cerr << "Copy data failed!" << std::endl;
    return 1;
  }

  // Set bpp value
  compressor.set_bpp(bpp);

  // Perform the actual compression
  if ((rtn = compressor.compress()) != sperr::RTNType::Good) {
    std::cerr << "Compression Failed!" << std::endl;
    return 1;
  }

  // Output the encoded bitstream
  auto& stream = compressor.view_encoded_bitstream();
  if (stream.empty()) {
    std::cerr << "Compression bitstream empty!" << std::endl;
    return 1;
  }

  rtn = sperr::write_n_bytes(output_file, stream.size(), stream.data());
  if (rtn != sperr::RTNType::Good) {
    std::cerr << "Write compressed file failed: " << output_file << std::endl;
    return 1;
  }

  return 0;
}
