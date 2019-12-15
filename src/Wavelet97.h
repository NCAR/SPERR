#ifndef SPECK_H
#define SPECK_H

#include <memory>
#include <cmath>
#include <vector>

namespace speck
{

class Wavelet97
{
public:
    template< typename T >
    int assign_data( const T* data, long x, long y, long z  = 1 );
    int dwt2d();            // 1) calculates the number of levels of dwt,
                            // 2) subtract mean of the data,
                            // 3) perform the actual dwt.

    
    // For debug only ==================//
    const double* get_data() const;     //
    double get_mean() const;            //
    // For debug only ==================//

private:
    //
    // Private methods
    //
    void m_subtract_mean(); // calculate data_mean from data_buf
    void m_dwt2d_one_level( double* plain, long len_x, long len_y ); 
                            // perform one level of 2D dwt on a given plain (dim_x, dim_y),
                            // specifically on its top left (len_x, len_y) subset.
    void m_calc_num_of_levels();    // determine level_xy and level_z
    long m_calc_approx_len( long orig_len, long lev );
                                    // determine the low frequency signal length after
                                    // lev times of transformation (lev > 0).

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
    const long MAX_LEN = 128;               // Max num. of values in one dimension
    double data_mean   = 0.0;               // Mean of the values in data_buf
    long dim_x = 0, dim_y = 0, dim_z = 0;   // Dimension of the data volume
    long level_xy = -1,  level_z = -1;      // Transform num. of levels
    std::unique_ptr<double[]> data_buf = nullptr;
    













    // Note on the coefficients, ALPHA, BETA, etc.
    // The ones from QccPack are slightly different from what's described in the lifting scheme paper:
    // Pg19 of "FACTORING WAVELET TRANSFORMS INTO LIFTING STEPS," DAUBECHIES and SWELDEN.
    // (https://9p.io/who/wim/papers/factor/factor.pdf)
    // JasPer, OpenJPEG, and FFMpeg use coefficients closer to the paper.
    // The filter bank coefficients (h[] array) are from "Biorthogonal Bases of Compactly Supported Wavelets,"
    // by Cohen et al., Page 551. (https://services.math.duke.edu/~ingrid/publications/CPAM_1992_p485.pdf) 

    // Paper coefficients
    static constexpr double h[5]{  .602949018236, .266864118443, -.078223266529,
                                  -.016864118443, .026748757411 };
    static constexpr double r0      = h[0] - 2.0  * h[4] * h[1] / h[3];
    static constexpr double r1      = h[2] - h[4] - h[4] * h[1] / h[3];
    static constexpr double s0      = h[1] - h[3] - h[3] * r0   / r1; 
    static constexpr double t0      = h[0] - 2.0 * (h[2] - h[4]);
    static constexpr double ALPHA   = h[4] / h[3];
    static constexpr double BETA    = h[3] / r1; 
    static constexpr double GAMMA   = r1   / s0; 
    static constexpr double DELTA   = s0   / t0; 
           const     double EPSILON = std::sqrt(2.0) * t0;

    // QccPack coefficients
    //const double ALPHA   = -1.58615986717275;
    //const double BETA    = -0.05297864003258;
    //const double GAMMA   =  0.88293362717904;
    //const double DELTA   =  0.44350482244527;
    //const double EPSILON =  1.14960430535816;
};


};


#endif
