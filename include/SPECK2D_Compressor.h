//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
//

#ifndef SPECK2D_COMPRESSOR_H
#define SPECK2D_COMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK2D.h"

#ifdef QZ_TERM
#include "SPERR.h"
#endif

using sperr::RTNType;

class SPECK2D_Compressor {
 public:
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  auto copy_data(const T* p, size_t len, sperr::dims_type dims) -> RTNType;

  // Accept incoming data by taking ownership of the memory block
  auto take_data(std::vector<double>&& buf, sperr::dims_type dims) -> RTNType;

  void toggle_conditioning(sperr::Conditioner::settings_type);

#ifdef QZ_TERM
  void set_qz_level(int32_t);
  auto set_tolerance(double) -> RTNType;
  // Return 1) the number of outliers, and 2) the number of bytes to encode them.
  auto get_outlier_stats() const -> std::pair<size_t, size_t>;
#else
  auto set_bpp(double) -> RTNType;
#endif

  auto compress() -> RTNType;

  auto view_encoded_bitstream() const -> const std::vector<uint8_t>&;
  auto release_encoded_bitstream() -> std::vector<uint8_t>&&;

 private:
  sperr::dims_type m_dims = {0, 0, 0};
  sperr::vecd_type m_val_buf;
  const size_t m_meta_size = 2;

#ifdef QZ_TERM
  sperr::vec8_type m_sperr_stream;
  sperr::SPERR m_sperr;
  sperr::vecd_type m_val_buf2;        // A copy of `m_val_buf` for outlier locating.
  sperr::vecd_type m_diffv;           // Store differences to locate outliers.
  int32_t m_qz_lev = 0;
  double m_tol = 0.0;
  size_t m_num_outlier = 0;
#else
  double m_bpp = 0.0;
#endif

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
