#ifndef SPECK_H
#define SPECK_H

#include <memory>
#include <cmath>

namespace speck
{

#ifndef BUFFER_TYPES
#define BUFFER_TYPES
    using buffer_type_d = std::unique_ptr<double[]>;
    using buffer_type_f = std::unique_ptr<float[]>;
#endif

class CDF97
{
public:
    // Input
    template< typename T >
    void copy_data( const T*, size_t len);
    template< typename T >
    void copy_data( const T&, size_t len);
    void take_data( buffer_type_d );        // Take ownership
    void set_mean( double );
    void set_dims( size_t x, size_t y, size_t z = 1 );
    
    // Output
    const buffer_type_d& get_read_only_data() const;    // Others can read the data
    buffer_type_d        release_data();   // Others take ownership of the data
    double get_mean() const; 
    void   get_dims( std::array<size_t, 2>& ) const;    // 2D case 
    void   get_dims( std::array<size_t, 3>& ) const;    // 3D case 

    // Common methods
    void reset();       // Reset this class to its initial state.
    int  dwt2d();       // 1) calculates the number of levels of dwt,
                        // 2) subtract mean of the data,
                        // 3) perform the actual dwt.
    int idwt2d();       // 1) calculates the number of levels of dwt,
                        // 2) perform the actual idwt
                        // 3) add the mean back to the data
private:
    //
    // Private methods helping DWT.
    //
    // Note: most of these methods operate on a partial array, i.e., not from the 
    //       beginning of an array. Thus, raw pointers are used.
    //
    void m_calc_mean();     // Calculate m_data_mean from m_data_buf
    void m_dwt2d(  double* plane, size_t num_of_xforms );
                            // Multiple levels of 2D DWT on a given plane by repeatedly
                            // invoking m_dwt2d_one_level().
    void m_idwt2d( double* plane, size_t num_of_xforms );
                            // Multiple levels of 2D IDWT on a given plane by repeatedly
                            // invoking m_idwt2d_one_level().
    void m_dwt2d_one_level( double* plane, size_t len_x, size_t len_y ); 
                            // Perform one level of 2D dwt on a given plane (dim_x, dim_y),
                            // specifically on its top left (len_x, len_y) subset.
    void m_idwt2d_one_level( double* plane, size_t len_x, size_t len_y ); 
                            // Perform one level of 2D idwt on a given plane (dim_x, dim_y),
                            // specifically on its top left (len_x, len_y) subset.
    void m_gather_even( double* dest, const double* orig, size_t len ) const;
    void m_gather_odd(  double* dest, const double* orig, size_t len ) const;
                            // Separate even and odd indexed elements to be at the front 
                            // and back of the dest array. Note: sufficient memory space 
                            // should be allocated by the caller.
                            // Note 2: two versions for even and odd length input.
    void m_scatter_even( double* dest, const double* orig, size_t len ) const;
    void m_scatter_odd(  double* dest, const double* orig, size_t len ) const;
                            // Interleave low and high pass elements to be at even and
                            // odd positions of the dest array. Note: sufficient memory 
                            // space should be allocated by the caller.
                            // Note 2: two versions for even and odd length input.
    void m_cut_transpose_XtoZ( double* dest, size_t x, size_t y, size_t z ) const;
                            // Cut a smaller cube (x, y, z) from the main buffer.
                            // It also transposes this cube from X-varying-fastest to
                            // Z-varying fastest.
                            // The destination buffer should already be allocated.
    void m_transpose_put_back_ZtoX( const double* buf, size_t x, size_t y, size_t z ) const;
                            // Transpose a cube of size (x, y, z) by swapping X and Z
                            // indices, and then put back to the main buffer.

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
    buffer_type_d m_data_buf = nullptr;   // Holds the entire input data.
    double m_data_mean  = 0.0;            // Mean of the values in data_buf
    size_t m_dim_x      = 0;              // Dimension of the data volume
    size_t m_dim_y      = 0;
    size_t m_dim_z      = 0;
    size_t m_buf_len    = 0;
    

    /*
     * Note on the coefficients, ALPHA, BETA, etc.
     * The ones from QccPack are slightly different from what's described in the lifting scheme paper:
     * Pg19 of "FACTORING WAVELET TRANSFORMS INTO LIFTING STEPS," DAUBECHIES and SWELDEN.
     * (https://9p.io/who/wim/papers/factor/factor.pdf)
     * JasPer, OpenJPEG, and FFMpeg use coefficients closer to the paper.
     * The filter bank coefficients (h[] array) are from "Biorthogonal Bases of Compactly Supported Wavelets,"
     * by Cohen et al., Page 551. (https://services.math.duke.edu/~ingrid/publications/CPAM_1992_p485.pdf) 
     */

    // Paper coefficients
    const double h[5]{ .602949018236, .266864118443, -.078223266529,
                                     -.016864118443,  .026748757411 };
    const double r0      = h[0] - 2.0  * h[4] * h[1] / h[3];
    const double r1      = h[2] - h[4] - h[4] * h[1] / h[3];
    const double s0      = h[1] - h[3] - h[3] * r0   / r1; 
    const double t0      = h[0] - 2.0 * (h[2] - h[4]);
    const double ALPHA   = h[4] / h[3];
    const double BETA    = h[3] / r1; 
    const double GAMMA   = r1   / s0; 
    const double DELTA   = s0   / t0; 
    const double EPSILON = std::sqrt(2.0) * t0;

    // QccPack coefficients
    /* const double ALPHA   = -1.58615986717275;
    const double BETA    = -0.05297864003258;
    const double GAMMA   =  0.88293362717904;
    const double DELTA   =  0.44350482244527;
    const double EPSILON =  1.14960430535816;
    */
};


};


#endif
