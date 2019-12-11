#ifndef SPECK_H
#define SPECK_H

#include <memory>

namespace speck
{

class Wavelet97
{
public:
    template< typename T >
    int assign_data( const T* data, long x, long y, long z  = 1 );

    
    // For debug only ==================//
    const double* get_data() const;     //
    double get_mean() const;            //
    // For debug only ==================//

private:
    void calc_mean();   // Calculate data_mean from data_buf


    std::unique_ptr<double[]> data_buf = nullptr;
    double data_mean = 0.0;                 // Mean of the values in data_buf
    long dim_x = 0, dim_y = 0, dim_z = 0;   // Dimension of the data volume
    int  level_xy = 5,  level_z = 5;        // Transform num. of levels
    

    const double ALPHA   = -1.58615986717275;
    const double BETA    = -0.05297864003258;
    const double GAMMA   =  0.88293362717904;
    const double DELTA   =  0.44350482244527;
    const double EPSILON =  1.14960430535816;
};


};


#endif
