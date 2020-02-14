#ifndef SPECK_HELPER_H
#define SPECK_HELPER_H

/* We put common functions that are used across the speck encoder here. */

#include <vector>

namespace speck
{
    //
    // Helper classes
    //
    enum class SPECKSetType : unsigned char
    {
        TypeI,
        TypeS
    };

    enum class Significance : unsigned char
    {
        Insignificant,
        Significant,
        Newly_Significant
    };


    //
    // Helper functions
    //
    // Given a certain length, how many levels of transforms to be performed?
    long calc_num_of_xform_levels( long len );


    // Determine the approximation and detail signal length at a certain 
    // transformation level lev: 0 <= lev < num_of_levels. 
    // It also returns the approximate length for ease of use in some cases.
    long calc_approx_detail_len( long  orig_len,   long  lev,          // input
                                 long& approx_len, long& detail_len ); // output


    // 1) fill sign_array based on data_buf signs, and 
    // 2) make data_buf containing all positive values.
    // Returns the maximum magnitude of all encountered values.
    // Note: sign_array will be resized to have length len.
    double make_positive( double* data_buf, const long len, std::vector<bool>& sign_array ); 


    // Update the significance map according to threshold.
    // Note: significance_map will be resized to have length len.
    void update_significance_map( const double* data_buf,  const long len, 
                                  const double threshold,  std::vector<bool>& significance_map  );
};

#endif
