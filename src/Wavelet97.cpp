#include "Wavelet97.h"

#include <type_traits>

template< typename T >
int speck::Wavelet97::assign_data( const T* data, long x, long y, long z )
{
    static_assert( std::is_floating_point<T>::value, 
                   "!! Only floating point values are supported !!" );
    dim_x = x;
    dim_y = y;
    dim_z = z;
    long num_of_vals = dim_x * dim_y * dim_z;
    data_buf.reset( new double[ num_of_vals ] );
    for( long i = 0; i < num_of_vals; i++ )
        data_buf[i] = data[i];

    return 0;
}
template int speck::Wavelet97::assign_data( const float*  data, long x, long y, long z );
template int speck::Wavelet97::assign_data( const double* data, long x, long y, long z );

    
//
// Private Methods
//
void speck::Wavelet97::subtract_mean()
{
    //
    // Here we calculate row by row to avoid too big numbers.
    //
    std::unique_ptr<double[]> row_means( new double[ dim_y * dim_z ] );
    const double dim_x1 = 1.0 / double(dim_x);
    long counter1 = 0, counter2 = 0;
    for( long z = 0; z < dim_z; z++ )
        for( long y = 0; y < dim_y; y++ )
        {
            double sum = 0.0;
            for( long x = 0; x < dim_x; x++ )
                sum += data_buf[ counter1++ ];
            row_means[ counter2++ ] = sum * dim_x1;
        }

    std::unique_ptr<double[]> layer_means( new double[ dim_z ] );
    const double dim_y1 = 1.0 / double(dim_y);
    counter1 = 0; counter2 = 0;
    for( long z = 0; z < dim_z; z++ )
    {
        double sum = 0.0;
        for( long y = 0; y < dim_y; y++ )
            sum += row_means[ counter1++ ];
        layer_means[ counter2++ ] = sum * dim_y1;
    }

    double sum = 0.0;
    for( long z = 0; z < dim_z; z++ )
        sum += layer_means[ z ];

    data_mean = sum / double(dim_z);

    for( long i = 0; i < dim_x * dim_y * dim_z; i++ )
        data_buf[i] -= data_mean;
}


//
// Methods from QccPack
//
void speck::Wavelet97::QccWAVCDF97AnalysisSymmetricEvenEven( double* signal, 
                                                             long signal_length)
{
    long index;
    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] += 2.0 * ALPHA * signal[signal_length - 2];
    signal[0] += 2.0 * BETA * signal[1];

    for (index = 2; index < signal_length; index += 2)
        signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] +=  GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] += 2.0 * GAMMA * signal[signal_length - 2];
    signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);

    for (index = 2; index < signal_length; index += 2)
        signal[index] =  EPSILON * (signal[index] + 
                         DELTA * (signal[index + 1] + signal[index - 1]));

    for (index = 1; index < signal_length; index += 2)
        signal[index] /= (-EPSILON);
}


void speck::Wavelet97::QccWAVCDF97SynthesisSymmetricEvenEven( double* signal, 
                                                              long signal_length)
{
    long index;
    for (index = 1; index < signal_length; index += 2)
        signal[index] *= (-EPSILON);

    signal[0] = signal[0]/EPSILON - 2.0 * DELTA * signal[1];

    for (index = 2; index < signal_length; index += 2)
        signal[index] = signal[index]/EPSILON - 
                        DELTA * (signal[index + 1] + signal[index - 1]);

    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] -= 2.0 * GAMMA * signal[signal_length - 2];
    signal[0] -= 2.0 * BETA * signal[1];

    for (index = 2; index < signal_length; index += 2)
        signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] -= 2.0 * ALPHA * signal[signal_length - 2];
}


void speck::Wavelet97::QccWAVCDF97SynthesisSymmetricOddEven( double* signal,
                                                             long signal_length)
{
  long index;
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] *= (-EPSILON);

  signal[0] = signal[0] / EPSILON - 2.0 * DELTA * signal[1];

  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] = signal[index] / EPSILON - 
                    DELTA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] = signal[signal_length - 1] / EPSILON - 
                              2.0 * DELTA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);

  signal[0] -= 2.0 * BETA * signal[1];

  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] -= 2.0 * BETA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);
}


void speck::Wavelet97::QccWAVCDF97AnalysisSymmetricOddEven(double*  signal,
                                                           long signal_length)
{
  long index;

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);
  
  signal[0] += 2.0 * BETA * signal[1];

  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] += 2.0 * BETA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] += GAMMA * (signal[index - 1] + signal[index + 1]);
  
  signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);

  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] = EPSILON * (signal[index] + 
                    DELTA * (signal[index + 1] + signal[index - 1]));

  signal[signal_length - 1] = EPSILON * (signal[signal_length - 1] +
                              2 * DELTA * signal[signal_length - 2]);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] /= (-EPSILON);
}


//
// For debug only 
//
const double* speck::Wavelet97::get_data() const
{
    return data_buf.get();
}
double speck::Wavelet97::get_mean() const
{
    return data_mean;
}
