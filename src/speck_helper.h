#ifndef SPECK_HELPER_H
#define SPECK_HELPER_H

/* We put common functions that are used across the speck encoder here. */

#include <vector>
#include <array>

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
        Insig,
        Sig,
        NewlySig,
        Empty,
    };


    //
    // Helper functions
    //
    // Given a certain length, how many transforms to be performed?
    size_t calc_num_of_xforms( size_t len );


    // Determine the approximation and detail signal length at a certain 
    // transformation level lev: 0 <= lev < num_of_xforms. 
    // It puts the approximation and detail length as the 1st and 2nd 
    // element of the output array.
    void calc_approx_detail_len( size_t orig_len, size_t lev,   // input
                                 std::array<size_t, 2>&    );   // output           


    // 1) fill sign_array based on data_buf signs, and 
    // 2) make data_buf containing all positive values.
    // Returns the maximum magnitude of all encountered values.
    // Note: sign_array will be resized to have length len.
    double make_positive( double* data_buf, size_t len, std::vector<bool>& sign_array ); 


    // Update the significance map according to threshold.
    // Note: significance_map will be resized to have length len.
    void update_significance_map( const double* data_buf,  size_t len, double threshold,  
                                  std::vector<bool>& significance_map  );
};

#endif
