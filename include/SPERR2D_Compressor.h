//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
//

#ifndef SPERR2D_COMPRESSOR_H
#define SPERR2D_COMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK2D.h"
#include "SPERR.h"

using sperr::RTNType;

class SPERR2D_Compressor {
 public:
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  auto copy_data(const T* p,             // Input: pointer to the data memory block
                 size_t len,             // Input: number of values in the memory block
                 sperr::dims_type dims)  // Input: dimension of the 2D slice w/ dims[2] == 1
      -> RTNType;

  // Accept incoming data by taking ownership of the memory block
  auto take_data(std::vector<double>&& buf, sperr::dims_type dims) -> RTNType;

  void toggle_conditioning(sperr::Conditioner::settings_type);

  auto set_target_bpp(double) -> RTNType;
  void set_target_psnr(double);
  void set_target_pwe(double);

  // Return 1) the number of outliers, and 2) the number of bytes to encode them.
  auto get_outlier_stats() const -> std::pair<size_t, size_t>;

  auto compress() -> RTNType;

  auto view_encoded_bitstream() const -> const std::vector<uint8_t>&;
  auto release_encoded_bitstream() -> std::vector<uint8_t>&&;

 private:
  sperr::dims_type m_dims = {0, 0, 0};
  sperr::vecd_type m_val_buf;
  const size_t m_meta_size = 10;  // Need to be the same as in SPERR2D_Decompressor.h

  // Data members for fixed-size compression
  size_t m_bit_budget = 0;  // Total bit budget, including headers.
  double m_target_psnr = sperr::max_d;
  double m_target_pwe = 0.0;
  bool m_orig_is_float = true;  // Is the original input float (true) or double (false)?

  // A few data members for outlier correction
  sperr::SPERR m_sperr;
  sperr::vec8_type m_sperr_stream;
  sperr::vecd_type m_val_buf2;        // A copy of `m_val_buf` used for outlier coding
  std::vector<sperr::Outlier> m_LOS;  // List of OutlierS

  sperr::Conditioner m_conditioner;
  sperr::CDF97 m_cdf;
  sperr::SPECK2D m_encoder;

  sperr::Conditioner::settings_type m_conditioning_settings = {true, false, false, false};
  sperr::Conditioner::meta_type m_condi_stream;

  // Store bitstreams from the conditioner and SPECK encoding, and the overall bitstream.
  sperr::vec8_type m_speck_stream;
  sperr::vec8_type m_encoded_stream;

  auto m_assemble_encoded_bitstream() -> RTNType;
};

#endif
