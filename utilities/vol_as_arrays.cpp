#include "SPECK1D_FLT.h"

#include <iostream>
#include <cassert>

int main(int argc, char* argv[])
{
  const size_t NX = std::atol(argv[2]);
  const size_t NY = std::atol(argv[3]);
  const size_t NZ = std::atol(argv[4]);
  const auto total_val = NX * NY * NZ;
  const auto bpp = std::atof(argv[5]);

  const auto in = sperr::read_whole_file<float>(argv[1]);
  assert(in.size() == total_val);
  auto out = in;

  auto encoder = sperr::SPECK1D_FLT();
  auto decoder = sperr::SPECK1D_FLT();
  auto stream = sperr::vec8_type();

  // Test each array along X
  for (size_t i = 0; i < NY * NZ; i++) {
    auto* p = out.data() + i * NX;
    encoder.set_dims({NX, 1, 1});
    encoder.copy_data(p, NX);
    encoder.set_bitrate(bpp);
    encoder.compress();
    stream.clear();
    encoder.append_encoded_bitstream(stream);

    decoder.set_dims({NX, 1, 1});
    decoder.use_bitstream(stream.data(), stream.size());
    decoder.decompress();
    const auto& output = decoder.view_decoded_data();
    std::copy(output.begin(), output.end(), p);
  }

  // Write the compressed file to disk.
  sperr::write_n_bytes("wmag.decomp", out.size() * 4, out.data());

  auto [rmse, linfy, psnr, min, max] = sperr::calc_stats(in.data(), out.data(), total_val);
  auto mean_var = sperr::calc_mean_var(in.data(), total_val);
  auto sigma = std::sqrt(mean_var[1]);

  std::printf("Input range = (%.2e, %.2e), L-Infty = %.2e\n", min, max, linfy);
  std::printf("Bitrate = %.2f, PSNR = %.2fdB, Accuracy Gain = %.2f\n", bpp, psnr,
  // std::printf("%f\n",
              std::log2(sigma / rmse) - bpp);
}
