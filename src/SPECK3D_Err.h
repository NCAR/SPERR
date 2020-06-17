/*
 * This class implements the error-bound SPECK3D add-on.
 */

#ifndef SPECK3D_ERR_H
#define SPECK3D_ERR_H

#include "speck_helper.h"

namespace speck
{

class Outlier{
public:
    //
    // Member data
    //
    uint32_t x   = 0;
    uint32_t y   = 0;
    uint32_t z   = 0;
    float    err = 0.0;

    // Constructors
    Outlier() = default;
    Outlier( uint32_t x, uint32_t y, uint32_t z, float err);
};

class SPECK3D_Err
{

};

};

#endif
