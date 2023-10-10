#ifndef CDF97_PL_H
#define CDF97_PL_H

#include "sperr_helper.h"

#include <cmath>

namespace sperr {

class CDF97_PL {

 using flt_type = __float128;

 public:
  //
  // Input
  //
  // Note that copy_data() and take_data() effectively resets internal states of this class.
  template <typename T>
  auto copy_data(const T* buf, size_t len, dims_type dims) -> RTNType;

  //
  // Output
  //
  auto view_data() const -> const std::vector<flt_type>&;
  auto get_dims() const -> std::array<size_t, 3>;  // In 2D case, the 3rd value equals 1.

  // Action items
  void dwt1d_pl(flt_type* start = nullptr, ptrdiff_t stride = 1);
  void idwt1d_pl(flt_type* start = nullptr, ptrdiff_t stride = 1);
  void dwt2d_xy_pl(flt_type* start = nullptr);   // `start` points to the origin of an XY plane.
  void idwt2d_xy_pl(flt_type* start = nullptr);  // `start` points to the origin of an XY plane.
  void dwt3d_dyadic_pl();
  void idwt3d_dyadic_pl();

 private:
  //
  // Private data members
  //
  std::vector<flt_type> m_data_buf;          // Holds the entire input data.
  dims_type m_dims = {0, 0, 0};  // Dimension of the data volume

  // Temporary buffers that are big enough for any (1D column * 2).
  std::vector<flt_type> m_col_buf;

  //
  // Peter's method that reduces large coefficients on the boundary.
  //
  const std::array<flt_type, 6> m_lift = {
      -1.586134342059923558,    // 1st w-lift
      -0.05298011857296141462,  // 1st s-lift
      0.8829110755309332959,    // 2nd w-lift
      0.4435068520439711521,    // 2nd s-lift
      1.2301741049140007292,    // w-scale
      0.81289306611596105003    // s-scale = 1 / w-scale
  };

  const std::array<flt_type, 9> m_blift = {
      -1.586134342059923558,    // 1st w-lift
      -0.05298011857296141462,  // 1st s-lift
      0.8829110755309332959,    // 2nd w-lift
      1.0796367753748872,       // 2nd s-lift
      -0.9206964196560029,      // w-scale
      -17.37814947295878,       // s-scale
      -0.13081031898599063,     // special weight for n-3
      10.978432345068303,       // special weight for n-2
      -10.956291035467812       // special w-scale for n-1
  };

  // Perform one level of wavelet transform with improved boundary handling.
  // Return: number of scaling coefficients (for next level)
  size_t m_fwd_pl(const flt_type* src,  // input values
                  ptrdiff_t sstride,  // input stride
                  flt_type* dst,        // output coefficients (can be the same as src)
                  ptrdiff_t dstride,  // output stride
                  size_t n            // input length
  );

  // Perform one level of inverse wavelet transform with improved boundary handling.
  bool m_inv_pl(const flt_type* src,  // input coefficients
                ptrdiff_t sstride,  // input stride
                flt_type* dst,        // output values (can be the same as src)
                ptrdiff_t dstride,  // output stride
                size_t n            // input length (number of function values)
  );

};  // class CDF97_PL

};  // namespace sperr

#endif
