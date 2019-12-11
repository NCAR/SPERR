#include "Wavelet97.h"
#include <iostream>

int main()
{
    const long N = 30;
    std::unique_ptr<float[]> buf( new float[N] );
    for( long i = 0; i < N; i++ )
        buf[i] = float(i) - 2.1f;

    speck::Wavelet97 w;
    w.assign_data( buf.get(), 5, 2, 3 );

    double m = w.get_mean();
        std::cout << "mean = " << m << std::endl;
    const double* p = w.get_data();
    for( int i = 0; i < N; i++ )
        std::cout << p[i] << std::endl;
}
