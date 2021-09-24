#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "SPECK3D_OMP_C.h"
#include "SPECK3D_OMP_D.h"

#include "CLI11.hpp"

#include <cassert>
#include <cctype>  // std::tolower()
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>

#ifdef QZ_TERM
//
// This file should only be compiled in QZ_TERM mode.
//
template <typename T>
auto test_configuration_omp(const T* in_buf,
                            speck::dims_type dims,
                            speck::dims_type chunks,
                            int32_t qz_level,
                            double tolerance,
                            speck::Conditioner::settings_type condi_settings,
                            size_t omp_num_threads,
                            std::vector<T>& output_buf) -> int
{
  // Setup
  const size_t total_vals = dims[0] * dims[1] * dims[2];
  SPECK3D_OMP_C compressor;
  compressor.set_dims(dims);
  compressor.prefer_chunk_dims(chunks);
  compressor.toggle_conditioning(condi_settings);
  compressor.set_num_threads(omp_num_threads);
  auto rtn = compressor.use_volume(in_buf, total_vals);
  if (rtn != RTNType::Good)
    return 1;

  compressor.set_qz_level(qz_level);
  compressor.set_tolerance(tolerance);

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
           T(encoded_stream.size() * 8) / T(total_vals));

  auto outlier = compressor.get_outlier_stats();
  if (outlier.first == 0) {
    printf("    There are no outliers at this quantization level!\n");
  }
  else {
    printf(
        "    Outliers: num = %ld, pct = %.2f%%, bpp ~ %.2f, "
        "using total storage ~ %.2f%%\n",
        outlier.first, float(outlier.first * 100) / float(total_vals),
        float(outlier.second * 8) / float(outlier.first),
        float(outlier.second * 100) / float(encoded_stream.size()));
  }

  // Perform decompression
  SPECK3D_OMP_D decompressor;
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
  T rmse, lmax, psnr, arr1min, arr1max;
  speck::calc_stats(in_buf, output_buf.data(), total_vals, omp_num_threads, 
                    rmse, lmax, psnr, arr1min, arr1max);
  printf("    Original data range = (%.2e, %.2e)\n", arr1min, arr1max);
  printf("    Reconstructed data RMSE = %.2e, L-Infty = %.2e, PSNR = %.2fdB\n", rmse, lmax, psnr);

  return 0;
}

int main(int argc, char* argv[])
{
  //
  // Parse command line options
  //
  CLI::App app("CLI options to probe_3d");

  std::string input_file;
  app.add_option("filename", input_file, "Input file to the probe")
      ->required()
      ->check(CLI::ExistingFile);

  std::vector<size_t> dims_v;
  app.add_option("--dims", dims_v,
                 "Dimensions of the input volume. "
                 "E.g., `--dims 128 128 128`.\n")
      ->required()
      ->expected(3);

  std::vector<size_t> chunks_v{64, 64, 64};
  app.add_option("--chunks", chunks_v,
                 "Dimensions of the preferred chunk size. "
                 "E.g., `--chunks 64 64 64`.\n"
                 "If not specified, then 64^3 will be used\n")
      ->expected(3);

  double tolerance = 0.0;
  app.add_option("-t", tolerance,
                 "Maximum point-wise error tolerance. E.g., `-t 0.001`.\n"
                 "It takes a positive value as the absolute error tolerance.\n"
                 "Note: if flag --div-rms is enabled, then this tolerance applies\n"
                 "to conditioned data.\n")
      ->required()
      ->check(CLI::PositiveNumber);

  int32_t qz_level;
  auto* qz_level_ptr = app.add_option("-q,--qz_level", qz_level,
                                      "Integer quantization level to test. E.g., `-q -10`. \n"
                                      "If not specified, the probe will pick one for you.\n");

  bool div_rms = false;
  app.add_flag("--div-rms", div_rms,
               "Conditioning: calculate rms of each chunk and divide every\n"
               "value by rms of its chunk. Default: not applied.\n");

  size_t omp_num_threads = 4;
  app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. Default: 4\n");

  bool use_double = false;
  app.add_flag("-d", use_double,
               "Specify that input data is in double type.\n"
               "Data is treated as float by default.\n");

  CLI11_PARSE(app, argc, argv);

  if (tolerance <= 0.0) {
    std::cerr << "Absolute error tolerance must be a positive value!\n";
    return 1;
  }

  const auto dims = std::array<size_t, 3>{dims_v[0], dims_v[1], dims_v[2]};
  const auto condi_settings = speck::Conditioner::settings_type{true,     // subtract mean
                                                                div_rms,  // divide by rms
                                                                false,    // unused
                                                                false};   // unused

  //
  // Read and keep a copy of input data (will be re-used for different qz
  // levels). Also create an output buffer that keeps the decompressed volume.
  //
  const size_t total_vals = dims[0] * dims[1] * dims[2];
  auto input_buf = speck::read_whole_file<uint8_t>(input_file.c_str());
  if ((use_double && input_buf.size() != total_vals * sizeof(double)) ||
      (!use_double && input_buf.size() != total_vals * sizeof(float))) {
    std::cerr << "  -- reading input file failed!" << std::endl;
    return 1;
  }
  auto output_buf_f = std::vector<float>();
  auto output_buf_d = std::vector<double>();

  //
  // Let's do an initial analysis to pick an initial QZ level.
  // The strategy is to use a QZ level that about 1/1000 of the input data
  // range.
  //
  double min_orig = 0.0, max_orig = 0.0;
  if (use_double) {
    const auto* begin = reinterpret_cast<const double*>(input_buf.data());
    auto minmax = std::minmax_element(begin, begin + total_vals);
    min_orig = *minmax.first;
    max_orig = *minmax.second;
  }
  else {
    const auto* begin = reinterpret_cast<const float*>(input_buf.data());
    auto minmax = std::minmax_element(begin, begin + total_vals);
    min_orig = *minmax.first;
    max_orig = *minmax.second;
  }

  if (!(*qz_level_ptr)) {
    double range = max_orig - min_orig;
    assert(range > 0.0);
    qz_level = int32_t(std::floor(std::log2(range / 1000.0)));
  }

  printf(
      "Initial analysis: input data min = %.4e, max = %.4e,\n"
      "absolute error tolerance = %.2e, quantization level = %d ...  \n\n",
      min_orig, max_orig, tolerance, qz_level);

  int rtn = 0;
  if (use_double) {
    rtn = test_configuration_omp(reinterpret_cast<const double*>(input_buf.data()), dims,
                                 {chunks_v[0], chunks_v[1], chunks_v[2]}, qz_level, tolerance,
                                 condi_settings, omp_num_threads, output_buf_d);
  }
  else {
    rtn = test_configuration_omp(reinterpret_cast<const float*>(input_buf.data()), dims,
                                 {chunks_v[0], chunks_v[1], chunks_v[2]}, qz_level, tolerance,
                                 condi_settings, omp_num_threads, output_buf_f);
  }
  if (rtn != 0)
    return rtn;

  //
  // Now it enters the interactive session
  //
  char answer;
  std::cout << "\nDo you want to explore another quantization level,            (y)\n"
               "               output the current decompressed file to disk,  (o)\n"
               "               or quit ?                                      (q): ";
  std::cin >> answer;

  while (std::tolower(answer) == 'y' || std::tolower(answer) == 'o') {
    switch (std::tolower(answer)) {
      case 'y':
        int32_t tmp;
        std::cout << std::endl << "Please input a new qz level to test: ";
        std::cin >> tmp;
        while (tmp < qz_level - 15 || tmp > qz_level + 15) {
          printf("Please input a qz_level within (-15, 15) of %d:  ", qz_level);
          std::cin >> tmp;
        }
        qz_level = tmp;
        printf("\nNow testing qz_level = %d ...\n", qz_level);

        if (use_double) {
          rtn = test_configuration_omp(reinterpret_cast<const double*>(input_buf.data()), dims,
                                       {chunks_v[0], chunks_v[1], chunks_v[2]}, qz_level, tolerance,
                                       condi_settings, omp_num_threads, output_buf_d);
        }
        else {
          rtn = test_configuration_omp(reinterpret_cast<const float*>(input_buf.data()), dims,
                                       {chunks_v[0], chunks_v[1], chunks_v[2]}, qz_level, tolerance,
                                       condi_settings, omp_num_threads, output_buf_f);
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
          rtn2 = speck::write_n_bytes(fname.c_str(), sizeof(double) * output_buf_d.size(),
                                      output_buf_d.data());
        }
        else {
          rtn2 = speck::write_n_bytes(fname.c_str(), sizeof(float) * output_buf_f.size(),
                                      output_buf_f.data());
        }
        if (rtn2 == RTNType::Good)
          std::cout << "written decompressed file: " << fname << std::endl;
        else {
          std::cerr << "write decompressed file error: " << fname << std::endl;
          return 1;
        }
    }  // end of switch

    std::cout << "\nDo you want to explore another quantization level,            (y)\n"
                 "               output the current decompressed file to disk,  (o)\n"
                 "               or quit ?                                      (q): ";
    std::cin >> answer;
    answer = std::tolower(answer);
  }  // end of while

  std::cout << "\nHave a good day! \n";

  return 0;
}

#endif
