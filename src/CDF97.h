#ifndef SPECK_H
#define SPECK_H

#include <memory>
#include <cmath>
#include <vector>

namespace speck
{

class CDF97
{
public:
    template< typename T >
    int assign_data( const T* data, long x, long y, long z  = 1 );
    int dwt2d();            // 1) calculates the number of levels of dwt,
                            // 2) subtract mean of the data,
                            // 3) perform the actual dwt.
    int idwt2d();           // 1) calculates the number of levels of dwt,
                            // 2) perform the actual idwt
                            // 3) add the mean back to the data
    int speck2d();          // 1) make all coefficients positive

    
    // For debug only ==================//
    const double* get_data() const;     //
    double get_mean() const;            //
    // For debug only ==================//

private:
    //
    // Private methods
    //
    void m_calc_mean();     // calculate m_data_mean from m_data_buf
    void m_dwt2d_one_level( double* plane, long len_x, long len_y ); 
                            // Perform one level of 2D dwt on a given plane (dim_x, dim_y),
                            // specifically on its top left (len_x, len_y) subset.
    void m_idwt2d_one_level( double* plane, long len_x, long len_y ); 
                            // Perform one level of 2D idwt on a given plane (dim_x, dim_y),
                            // specifically on its top left (len_x, len_y) subset.
    long m_num_of_levels_xy() const;  // How many levels of DWT on the XY plane?
    long m_num_of_levels_z()  const;  // How many levels of DWT on the Z direction?
    long m_calc_approx_len( long orig_len, long lev ) const;
                            // Determine the low frequency signal length after
                            // lev times of transformation (lev >= 0).
    void m_gather_even( double* dest, const double* orig, long len ) const;
    void m_gather_odd(  double* dest, const double* orig, long len ) const;
                            // Separate even and odd indexed elements to be at the front 
                            // and back of the dest array. Note: sufficient memory space 
                            // should be allocated by the caller.
                            // Note 2: two versions for even and odd length input.
    void m_scatter_even( double* dest, const double* orig, long len ) const;
    void m_scatter_odd(  double* dest, const double* orig, long len ) const;
                            // Interleave low and high pass elements to be at even and
                            // odd positions of the dest array. Note: sufficient memory 
                            // space should be allocated by the caller.
                            // Note 2: two versions for even and odd length input.
    double m_make_positive(); // 1) fill m_sign_array based on m_data_buf signs, and 
                              // 2) make m_data_buf containing all positive values.
                              // Returns the maximum magnitude of all encountered values.

    //
    // Methods from QccPack, keep their original name.
    //
    void QccWAVCDF97AnalysisSymmetricEvenEven( double* signal, long signal_length);
    void QccWAVCDF97AnalysisSymmetricOddEven(  double* signal, long signal_length);
    void QccWAVCDF97SynthesisSymmetricEvenEven(double* signal, long signal_length);
    void QccWAVCDF97SynthesisSymmetricOddEven( double* signal, long signal_length);


    //
    // Private data members
    // Note: use long type to store all integers. int is only used for return values.
    //
    double m_data_mean   = 0.0;                     // Mean of the values in data_buf
    long m_max_coefficient_bits = 0;                // How many bits needed
    long m_dim_x = 0, m_dim_y = 0, m_dim_z = 0;     // Dimension of the data volume
    using buffer_type = std::unique_ptr<double[]>;
    buffer_type m_data_buf = nullptr;               // Holds the entire input data.
    std::vector<bool> m_sign_array;                 // (vector<bool> is a lot faster 
    std::vector<bool> m_significance_map;           // and memory-efficient)
    













    // Note on the coefficients, ALPHA, BETA, etc.
    // The ones from QccPack are slightly different from what's described in the lifting scheme paper:
    // Pg19 of "FACTORING WAVELET TRANSFORMS INTO LIFTING STEPS," DAUBECHIES and SWELDEN.
    // (https://9p.io/who/wim/papers/factor/factor.pdf)
    // JasPer, OpenJPEG, and FFMpeg use coefficients closer to the paper.
    // The filter bank coefficients (h[] array) are from "Biorthogonal Bases of Compactly Supported Wavelets,"
    // by Cohen et al., Page 551. (https://services.math.duke.edu/~ingrid/publications/CPAM_1992_p485.pdf) 

    // Paper coefficients
    /*const double h[5]{ .602949018236, .266864118443, -.078223266529,
                      -.016864118443, .026748757411 };
    const double r0      = h[0] - 2.0  * h[4] * h[1] / h[3];
    const double r1      = h[2] - h[4] - h[4] * h[1] / h[3];
    const double s0      = h[1] - h[3] - h[3] * r0   / r1; 
    const double t0      = h[0] - 2.0 * (h[2] - h[4]);
    const double ALPHA   = h[4] / h[3];
    const double BETA    = h[3] / r1; 
    const double GAMMA   = r1   / s0; 
    const double DELTA   = s0   / t0; 
    const double EPSILON = std::sqrt(2.0) * t0;
    */

    // QccPack coefficients
    const double ALPHA   = -1.58615986717275;
    const double BETA    = -0.05297864003258;
    const double GAMMA   =  0.88293362717904;
    const double DELTA   =  0.44350482244527;
    const double EPSILON =  1.14960430535816;
};


};


#endif
