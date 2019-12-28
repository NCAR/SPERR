#include "CDF97.h"
#include <iostream>
#include <cstdio>
#include <cmath>


int main()
{
/*
    const long N = 30;
    std::unique_ptr<float[]> buf( new float[N] );
    for( long i = 0; i < N; i++ )
        buf[i] = float(i) * 3.3f - 2.1f;

    speck::CDF97 w;
    w.assign_data( buf.get(), 5, 6, 1 );
    w.subtract_mean();

    double m = w.get_mean();
        std::cout << "mean = " << m << std::endl;
    const double* p = w.get_data();
    for( int i = 0; i < N; i++ )
        std::cout << p[i] << std::endl;
*/

/*
    double h[5]{ .602949018236,
                 .266864118443,
                -.078223266529,
                -.016864118443,
                 .026748757411 };

    double r0    = h[0] - 2.0  * h[4] * h[1] / h[3];
    double r1    = h[2] - h[4] - h[4] * h[1] / h[3];
    double s0    = h[1] - h[3] - h[3] * r0   / r1;
    double t0    = (h[0] - 2.0 * (h[2] - h[4]));
    double alpha = h[4] / h[3];
    double beta  = h[3] / r1;
    double gamma = r1   / s0;
    double delta = s0   / t0;
    double eps   = std::sqrt(2.0) * t0;

    printf( "alpha = %0.15f,  beta = %0.15f,  gamma = %0.15f, delta = %0.15f,  eps = %0.15f\n", 
            alpha, beta, gamma, delta, eps );
*/

}
