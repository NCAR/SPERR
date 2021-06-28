#ifndef CDF97_H
#define CDF97_H

#include <cmath>

#include "speck_helper.h"

namespace speck {

class CDF97 {
public:
    //
    // Input
    //
    // Note that copy_data() and take_data() effectively resets internal states
    // of this class.
    template <typename T>
    auto copy_data(const T* buf,  size_t len,  dims_type dims ) -> RTNType;
    auto take_data( std::vector<double>&& buf, dims_type dims ) -> RTNType;

    //
    // Output
    //
    auto view_data() const -> const std::vector<double>&;
    auto release_data()    -> std::vector<double>;
    auto get_dims() const  -> std::array<size_t, 3>; // In 2D case, the 3rd value equals 1.

    // Action items
    void dwt2d();  // 1) calculates the number of levels of dwt,
                   // 3) perform the actual dwt.
    void idwt2d(); // 1) calculates the number of levels of dwt,
                   // 2) perform the actual idwt
    void dwt1d();
    void idwt1d();
    void dwt3d();
    void idwt3d();

private:
    //
    // Private methods helping DWT.
    //
    // Note: most of these methods operate on a partial array, i.e., not from the
    //       beginning of an array or not ending at the actual end.
    //       Thus, raw pointers are used here.
    //
    void m_dwt1d(double* array,
                 size_t  array_len,
                 size_t  num_of_xforms,
                 double* tmp_buf);
    // Multiple levels of 2D DWT on a given array of length len.
    // A buffer space (tmp_buf) should be passed in for
    // this method to work on with length at least `array_len`.
    void m_idwt1d(double* array,
                  size_t  array_len,
                  size_t  num_of_xforms,
                  double* tmp_buf);
    // Multiple levels of 2D IDWT on a given array of length len.
    // Refer to m_dwt1d() for the requirement of tmp_buf.
    void m_dwt2d(double* plane,
                 size_t  len_x,
                 size_t  len_y,
                 size_t  num_of_xforms,
                 double* tmp_buf);
    // Multiple levels of 2D DWT on a given plane by repeatedly
    // invoking m_dwt2d_one_level().
    // The plane has a dimension (len_x, len_y).
    // A buffer space (tmp_buf) should be passed in for
    // this method to work on with length at least 2*max(len_x, len_y).
    void m_idwt2d(double* plane,
                  size_t  len_x,
                  size_t  len_y,
                  size_t  num_of_xforms,
                  double* tmp_buf);
    // Multiple levels of 2D IDWT on a given plane by repeatedly
    // invoking m_idwt2d_one_level().
    // The plane has a dimension (len_x, len_y).
    // Refer to m_dwt2d() for the requirement of tmp_buf.
    void m_dwt2d_one_level(double* plane,
                           size_t  len_x,
                           size_t  len_y,
                           double* tmp_buf);
    // Perform one level of 2D dwt on a given plane (dim_x, dim_y),
    // specifically on its top left (len_x, len_y) subset.
    // A buffer space (tmp_buf) should be passed in for
    // this method to work on with length at least 2*max(len_x, len_y).
    void m_idwt2d_one_level(double* plane,
                            size_t  len_x,
                            size_t  len_y,
                            double* tmp_buf);
    // Perform one level of 2D idwt on a given plane (dim_x, dim_y),
    // specifically on its top left (len_x, len_y) subset.
    // Refer to m_idwt2d_one_level() for the requirement of tmp_buf.
    void m_gather_even(double* dest, const double* orig, size_t len) const;
    void m_gather_odd(double* dest, const double* orig, size_t len) const;
    // Separate even and odd indexed elements to be at the front
    // and back of the dest array. Note: sufficient memory space
    // should be allocated by the caller.
    // Note 2: two versions for even and odd length input.
    void m_scatter_even(double* dest, const double* orig, size_t len) const;
    void m_scatter_odd(double* dest, const double* orig, size_t len) const;
    // Interleave low and high pass elements to be at even and
    // odd positions of the dest array. Note: sufficient memory
    // space should be allocated by the caller.
    // Note 2: two versions for even and odd length input.

    //
    // Methods from QccPack, keep their original names.
    //
    void QccWAVCDF97AnalysisSymmetricEvenEven( double* signal, size_t signal_length);
    void QccWAVCDF97AnalysisSymmetricOddEven(  double* signal, size_t signal_length);
    void QccWAVCDF97SynthesisSymmetricEvenEven(double* signal, size_t signal_length);
    void QccWAVCDF97SynthesisSymmetricOddEven( double* signal, size_t signal_length);

    //
    // Private data members
    //
    vecd_type     m_data_buf;         // Holds the entire input data.
    dims_type     m_dims = {0, 0, 0}; // Dimension of the data volume

    // Temporary buffers that are big enough for any (1D column * 2) or any 2D slice.
    vecd_type     m_col_buf;
    vecd_type     m_slice_buf;

    /*
     * Note on the coefficients and constants:
     * The ones from QccPack are slightly different from what's described in the
     * lifting scheme paper: Pg19 of "FACTORING WAVELET TRANSFORMS INTO LIFTING
     * STEPS," DAUBECHIES and SWELDEN.
     * (https://9p.io/who/wim/papers/factor/factor.pdf)
     * JasPer, OpenJPEG, and FFMpeg use coefficients closer to the paper.
     * The filter bank coefficients (h[] array) are from "Biorthogonal Bases of
     * Compactly Supported Wavelets," by Cohen et al., Page 551.
     * (https://services.math.duke.edu/~ingrid/publications/CPAM_1992_p485.pdf)
     */

    // Paper coefficients
    const double h[5] { .602949018236,
                        .266864118443,
                       -.078223266529,
                       -.016864118443,
                        .026748757411 };
    const double r0          = h[0] - 2.0  *  h[4] * h[1] / h[3];
    const double r1          = h[2] - h[4] -  h[4] * h[1] / h[3];
    const double s0          = h[1] - h[3] -  h[3] * r0 / r1;
    const double t0          = h[0] - 2.0  * (h[2] - h[4]);
    const double ALPHA       = h[4] / h[3];
    const double BETA        = h[3] / r1;
    const double GAMMA       = r1   / s0;
    const double DELTA       = s0   / t0;
    const double EPSILON     = std::sqrt(2.0) * t0;
    const double INV_EPSILON = 1.0  / (std::sqrt(2.0) * t0);

    // QccPack coefficients
    /* 
     * const double ALPHA   = -1.58615986717275;
     * const double BETA    = -0.05297864003258;
     * const double GAMMA   =  0.88293362717904;
     * const double DELTA   =  0.44350482244527;
     * const double EPSILON =  1.14960430535816;
     */
};

}; // namespace speck

#endif
