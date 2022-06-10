#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"

#include "CLI11.hpp"

#include <cctype>  // std::tolower()
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>

//
// This file should only be compiled in non QZ_TERM mode.
//
#ifndef QZ_TERM

template <typename T>
auto test_configuration_omp(const T* in_buf,
                            sperr::dims_type dims,
                            sperr::dims_type chunks,
                            double bpp,
                            sperr::Conditioner::settings_type condi_settings,
                            size_t omp_num_threads,
                            std::vector<T>& output_buf) -> int
{
  // Setup
  const size_t total_vals = dims[0] * dims[1] * dims[2];
  auto compressor = SPERR3D_OMP_C();
  auto rtn = compressor.copy_data(in_buf, total_vals, dims, chunks);
  if (rtn != RTNType::Good)
    return 1;

  compressor.set_bpp(bpp);
  compressor.toggle_conditioning(condi_settings);
  compressor.set_num_threads(omp_num_threads);

  // Perform actual compression work
  auto start_time = std::chrono::steady_clock::now();
  rtn = compressor.compress();
  if (rtn != RTNType::Good)
    return 1;
  auto end_time = std::chrono::steady_clock::now();
  auto diff_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
  std::cout << " -> Compression takes time: " << diff_time << "ms\n";

  auto encoded_stream = compressor.get_encoded_bitstream();
  if (encoded_stream.empty())
    return 1;
  else
    printf("    Total compressed size in bytes = %ld, average bpp = %.2f\n", encoded_stream.size(),
           double(encoded_stream.size() * 8) / double(total_vals));

  // Perform decompression
  SPERR3D_OMP_D decompressor;
  decompressor.set_num_threads(omp_num_threads);
  rtn = decompressor.use_bitstream(encoded_stream.data(), encoded_stream.size());
  if (rtn != RTNType::Good)
    return 1;

  start_time = std::chrono::steady_clock::now();
  rtn = decompressor.decompress(encoded_stream.data());
  if (rtn != RTNType::Good)
    return 1;
  end_time = std::chrono::steady_clock::now();
  diff_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
  std::cout << " -> Decompression takes time: " << diff_time << "ms\n";

  output_buf = decompressor.get_data<T>();
  if (output_buf.size() != total_vals)
    return 1;

  // Collect statistics
  auto ret = sperr::calc_stats(in_buf, output_buf.data(), total_vals, omp_num_threads);
  printf("    Original data range = (%.2e, %.2e)\n", ret[3], ret[4]);
  printf("    Reconstructed data RMSE = %.2e, L-Infty = %.2e, PSNR = %.2fdB\n", ret[0], ret[1],
         ret[2]);

  return 0;
}

int main(int argc, char* argv[])
{
  //
  // Parse command line options
  //
  CLI::App app("Test and evaluate SPERR compression on 3D volumes\n");

  auto input_file = std::string();
  app.add_option("filename", input_file, "Input data file to probe")
      ->required()
      ->check(CLI::ExistingFile)
      ->group("Input Specifications");

  auto dims_v = std::vector<size_t>();
  app.add_option("--dims", dims_v, "Dimensions of the input volume. E.g., `--dims 128 128 128`")
      ->required()
      ->expected(3)
      ->group("Input Specifications");

  auto use_double = bool{false};
  app.add_flag("-d", use_double,
               "Specify that input data is in double type.\n"
               "Data is treated as float by default.")
      ->group("Input Specifications");

  auto chunks_v = std::vector<size_t>{256, 256, 256};
  app.add_option("--chunks", chunks_v,
                 "Dimensions of the preferred chunk size. Default: 256 256 256\n"
                 "E.g., `--chunks 64 64 64`")
      ->expected(3)
      ->group("Compression Parameters");

  auto bpp = double{0.0};
  app.add_option("--bpp", bpp, "Target bit-per-pixel value. E.g., `--bpp 0.5`.")
      ->check(CLI::Range(0.0, 64.0))
      ->required()
      ->group("Compression Parameters");

  auto div_rms = bool{false};
  app.add_flag("--div-rms", div_rms,
               "Conditioning: calculate rms of each chunk and divide every\n"
               "value by rms of its chunk. Default: not applied.")
      ->group("Compression Parameters");

  auto omp_num_threads = size_t{4};
  app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. Default: 4")
      ->group("Compression Parameters");

  CLI11_PARSE(app, argc, argv);

  const auto dims = std::array<size_t, 3>{dims_v[0], dims_v[1], dims_v[2]};
  const auto condi_settings = sperr::Conditioner::settings_type{true,     // subtract mean
                                                                div_rms,  // divide by rms
                                                                false,    // unused
                                                                false};   // unused

  //
  // Read and keep a copy of input data (will be used for testing different
  // configurations) Also create a buffer to hold decompressed data.
  //
  const auto total_vals = dims[0] * dims[1] * dims[2];
  auto input_buf = sperr::read_whole_file<uint8_t>(input_file);
  if ((use_double && input_buf.size() != total_vals * sizeof(double)) ||
      (!use_double && input_buf.size() != total_vals * sizeof(float))) {
    std::cerr << "  -- reading input file failed!" << std::endl;
    return 1;
  }
  auto output_buf_f = std::vector<float>();
  auto output_buf_d = std::vector<double>();

  //
  // Let's do an initial analysis
  //
  if (use_double) {
    const auto* begin = reinterpret_cast<const double*>(input_buf.data());
    auto minmax = std::minmax_element(begin, begin + total_vals);
    printf("Initial analysis: input data min = %.4e, max = %.4e\n", *minmax.first, *minmax.second);
  }
  else {
    const auto* begin = reinterpret_cast<const float*>(input_buf.data());
    auto minmax = std::minmax_element(begin, begin + total_vals);
    printf("Initial analysis: input data min = %.4e, max = %.4e\n", *minmax.first, *minmax.second);
  }

  printf("Initial analysis: compression at %.2f bit-per-pixel...  \n", bpp);
  int rtn = 0;
  if (use_double) {
    rtn = test_configuration_omp(reinterpret_cast<const double*>(input_buf.data()), dims,
                                 {chunks_v[0], chunks_v[1], chunks_v[2]}, bpp, condi_settings,
                                 omp_num_threads, output_buf_d);
  }
  else {
    rtn = test_configuration_omp(reinterpret_cast<const float*>(input_buf.data()), dims,
                                 {chunks_v[0], chunks_v[1], chunks_v[2]}, bpp, condi_settings,
                                 omp_num_threads, output_buf_f);
  }
  if (rtn != 0)
    return rtn;

  //
  // Now it enters the interactive session
  //
  char answer;
  std::cout << "\nDo you want to explore other bit-per-pixel values             (y),\n"
               "               output the current decompressed file to disk,  (o),\n"
               "               or quit?                                       (q): ";
  std::cin >> answer;
  while (std::tolower(answer) == 'y' || std::tolower(answer) == 'o') {
    switch (std::tolower(answer)) {
      case 'y':
        bpp = -1.0;
        std::cout << "\nPlease input a new bpp value to test [0.0 - 64.0]:  ";
        std::cin >> bpp;
        while (bpp <= 0.0 || bpp >= 64.0) {
          std::cout << "Please input a bpp value inbetween [0.0 - 64.0]:  ";
          std::cin >> bpp;
        }
        printf("\nNow testing bpp = %.2f ...\n", bpp);

        if (use_double) {
          rtn = test_configuration_omp(reinterpret_cast<const double*>(input_buf.data()), dims,
                                       {chunks_v[0], chunks_v[1], chunks_v[2]}, bpp, condi_settings,
                                       omp_num_threads, output_buf_d);
        }
        else {
          rtn = test_configuration_omp(reinterpret_cast<const float*>(input_buf.data()), dims,
                                       {chunks_v[0], chunks_v[1], chunks_v[2]}, bpp, condi_settings,
                                       omp_num_threads, output_buf_f);
        }
        if (rtn != 0)
          return rtn;

        break;

      case 'o':
        std::string fname;
        std::cout << std::endl << "Please input a filename to use: ";
        std::cin >> fname;
        auto rtn2 = RTNType::Good;
        if (use_double) {
          rtn2 = sperr::write_n_bytes(fname, sizeof(double) * output_buf_d.size(),
                                      output_buf_d.data());
        }
        else {
          rtn2 =
              sperr::write_n_bytes(fname, sizeof(float) * output_buf_f.size(), output_buf_f.data());
        }
        if (rtn2 == RTNType::Good)
          std::cout << "written decompressed file: " << fname << std::endl;
        else {
          std::cerr << "writign decompressed file error: " << fname << std::endl;
          return 1;
        }
    }  // end of switch

    std::cout << "\nDo you want to explore other bit-per-pixel values,            (y)\n"
                 "               output the current decompressed file to disk,  (o)\n"
                 "               or quit?                                       (q): ";
    std::cin >> answer;
    answer = std::tolower(answer);
  }  // end of while

  std::cout << "\nHave a good day! \n";

  return 0;
}

#endif
