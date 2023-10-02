//
// Four member functions and their implementation are from QccPack: link
//   http://qccpack.sourceforge.net/index.shtml
// Thanks to James Fowler for his excellent work!
// These four functions are:
//   void QccWAVCDF97AnalysisSymmetricEvenEven(double* signal, size_t signal_length);
//   void QccWAVCDF97AnalysisSymmetricOddEven(double* signal, size_t signal_length);
//   void QccWAVCDF97SynthesisSymmetricEvenEven(double* signal, size_t signal_length);
//   void QccWAVCDF97SynthesisSymmetricOddEven(double* signal, size_t signal_length);
//

#ifndef CDF97_H
#define CDF97_H

#include "sperr_helper.h"

#include <cmath>

namespace sperr {

class CDF97 {
 public:
  //
  // Input
  //
  // Note that copy_data() and take_data() effectively resets internal states of this class.
  template <typename T>
  auto copy_data(const T* buf, size_t len, dims_type dims) -> RTNType;
  auto take_data(vecd_type&& buf, dims_type dims) -> RTNType;

  //
  // Output
  //
  auto view_data() const -> const vecd_type&;
  auto release_data() -> vecd_type&&;
  auto get_dims() const -> std::array<size_t, 3>;  // In 2D case, the 3rd value equals 1.

  // Action items
  void dwt1d();
  void dwt2d();
  void dwt3d();
  void idwt2d();
  void idwt1d();
  void idwt3d();

  // Peter Lindstrom's scheme
  void dwt1d_pl();
  void idwt1d_pl();

 private:
  using itd_type = vecd_type::iterator;
  using citd_type = vecd_type::const_iterator;

  //
  // Private methods helping DWT.
  //

  // Multiple levels of 1D DWT/IDWT on a given array of length array_len.
  void m_dwt1d(itd_type array, size_t array_len, size_t num_of_xforms);
  void m_idwt1d(itd_type array, size_t array_len, size_t num_of_xforms);

  // Multiple levels of 2D DWT/IDWT on a given plane by repeatedly invoking
  // m_dwt2d_one_level(). The plane has a dimension (len_xy[0], len_xy[1]).
  void m_dwt2d(itd_type plane, std::array<size_t, 2> len_xy, size_t num_of_xforms);
  void m_idwt2d(itd_type plane, std::array<size_t, 2> len_xy, size_t num_of_xforms);

  // Perform one level of interleaved 3D dwt/idwt on a given volume (m_dims),
  // specifically on its top left (len_xyz) subset.
  void m_dwt3d_one_level(itd_type vol, std::array<size_t, 3> len_xyz);
  void m_idwt3d_one_level(itd_type vol, std::array<size_t, 3> len_xyz);

  // Perform one level of 2D dwt/idwt on a given plane (m_dims),
  // specifically on its top left (len_xy) subset.
  void m_dwt2d_one_level(itd_type plane, std::array<size_t, 2> len_xy);
  void m_idwt2d_one_level(itd_type plane, std::array<size_t, 2> len_xy);

  // Perform one level of 1D dwt/idwt on a given array (array_len).
  // A buffer space (tmp_buf) should be passed in for
  // this method to work on with length at least 2*array_len.
  void m_dwt1d_one_level(itd_type array, size_t array_len);
  void m_idwt1d_one_level(itd_type array, size_t array_len);

  // Separate even and odd indexed elements to be at the front and back of the dest array.
  // Note 1: sufficient memory space should be allocated by the caller.
  // Note 2: two versions for even and odd length input.
  void m_gather_even(citd_type begin, citd_type end, itd_type dest) const;
  void m_gather_odd(citd_type begin, citd_type end, itd_type dest) const;

  // Interleave low and high pass elements to be at even and odd positions of the dest array.
  // Note 1: sufficient memory space should be allocated by the caller.
  // Note 2: two versions for even and odd length input.
  void m_scatter_even(citd_type begin, citd_type end, itd_type dest) const;
  void m_scatter_odd(citd_type begin, citd_type end, itd_type dest) const;

  // Two flavors of 3D transforms.
  // They should be invoked by the `dwt3d()` and `idwt3d()` public methods, not users, though.
  void m_dwt3d_wavelet_packet();
  void m_idwt3d_wavelet_packet();
  void m_dwt3d_dyadic();
  void m_idwt3d_dyadic();

  //
  // Methods from QccPack, so keep their original names, interface, and the use of raw pointers.
  //
  void QccWAVCDF97AnalysisSymmetricEvenEven(double* signal, size_t signal_length);
  void QccWAVCDF97AnalysisSymmetricOddEven(double* signal, size_t signal_length);
  void QccWAVCDF97SynthesisSymmetricEvenEven(double* signal, size_t signal_length);
  void QccWAVCDF97SynthesisSymmetricOddEven(double* signal, size_t signal_length);

  //
  // Private data members
  //
  vecd_type m_data_buf;          // Holds the entire input data.
  dims_type m_dims = {0, 0, 0};  // Dimension of the data volume

  // Temporary buffers that are big enough for any (1D column * 2) or any 2D slice.
  // Note: `m_slice_buf` is only used by wavelet-packet transforms.
  vecd_type m_col_buf;
  vecd_type m_slice_buf;

  //
  // Note on the coefficients and constants:
  // The ones from QccPack are slightly different from what's described in the
  // lifting scheme paper: Pg19 of "FACTORING WAVELET TRANSFORMS INTO LIFTING STEPS,"
  // DAUBECHIES and SWELDEN.  (https://9p.io/who/wim/papers/factor/factor.pdf)
  // JasPer, OpenJPEG, and FFMpeg use coefficients closer to the paper.
  // The filter bank coefficients (h[] array) are from "Biorthogonal Bases of
  // Compactly Supported Wavelets," by Cohen et al., Page 551.
  // (https://services.math.duke.edu/~ingrid/publications/CPAM_1992_p485.pdf)
  //

  // Paper coefficients
  const std::array<double, 5> h = {0.602949018236, 0.266864118443, -0.078223266529, -0.016864118443,
                                   0.026748757411};
  const double r0 = h[0] - 2.0 * h[4] * h[1] / h[3];
  const double r1 = h[2] - h[4] - h[4] * h[1] / h[3];
  const double s0 = h[1] - h[3] - h[3] * r0 / r1;
  const double t0 = h[0] - 2.0 * (h[2] - h[4]);
  const double ALPHA = h[4] / h[3];
  const double BETA = h[3] / r1;
  const double GAMMA = r1 / s0;
  const double DELTA = s0 / t0;
  const double EPSILON = std::sqrt(2.0) * t0;
  const double INV_EPSILON = 1.0 / EPSILON;

  // QccPack coefficients
  //
  // const double ALPHA   = -1.58615986717275;
  // const double BETA    = -0.05297864003258;
  // const double GAMMA   =  0.88293362717904;
  // const double DELTA   =  0.44350482244527;
  // const double EPSILON =  1.14960430535816;
  //

  //
  // Peter's method that reduces large coefficients on the boundary.
  //
  const std::array<double, 6> m_lift = {
      -1.586134342059923558,    // 1st w-lift
      -0.05298011857296141462,  // 1st s-lift
      0.8829110755309332959,    // 2nd w-lift
      0.4435068520439711521,    // 2nd s-lift
      1.2301741049140007292,    // w-scale
      0.81289306611596105003    // s-scale = 1 / w-scale
  };

  const std::array<double, 9> m_blift = {
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
  size_t m_forward(const double* src,  // input values
                   ptrdiff_t sstride,  // input stride
                   double* dst,        // output coefficients (can be the same as src)
                   ptrdiff_t dstride,  // output stride
                   size_t n            // input length
  );

  // Perform one level of inverse wavelet transform with improved boundary handling.
  bool m_inverse(const double* src,  // input coefficients
                 ptrdiff_t sstride,  // input stride
                 double* dst,        // output values (can be the same as src)
                 ptrdiff_t dstride,  // output stride
                 size_t n            // output length (number of function values)
  );

};  // class CDF97

};  // namespace sperr

#endif
