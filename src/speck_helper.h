#ifndef SPECK_HELPER_H
#define SPECK_HELPER_H

/* We put common functions that are used across the speck encoder here. */

namespace speck
{
    // Given a certain length, how many levels of transforms to be performed?
    long calc_num_of_xform_levels( long len );

    // Determine the approximation and detail signal length at a certain 
    // transformation level lev: 0 <= lev < num_of_levels. 
    // It also returns the approximate length for ease of use in some cases.
    long calc_approx_detail_len( long  orig_len,   long  lev,          // input
                                 long& approx_len, long& detail_len ); // output
};

#endif
