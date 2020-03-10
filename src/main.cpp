#include "speck_helper.h"
#include "iostream"

int main()
{
    size_t dims[2] = { 101, 999 };
    double mean    = 3.1416;
    uint16_t max_bit = 128;
    const char* filename = "temp.temp";

    std::vector<bool> a{ true, false, true, false, false, true, false, true, 
                         true, true, false, false, true, true, false, false };

    speck::output_speck2d( dims[0], dims[1], mean, max_bit, a, filename );

    size_t dim_x, dim_y;
    double mean_r;
    uint16_t max_bit_r;
    std::vector<bool> a_r;

    speck::input_speck2d( dim_x, dim_y, mean_r, max_bit_r, a_r, filename );

    std::cout << dim_x << ", " << dim_y << ", " << mean_r << ", " << max_bit_r << std::endl;
    for( size_t i = 0; i < a_r.size(); i++ )
        if( a_r[i] )
            std::cout << "true, ";
        else
            std::cout << "false, ";
    std::cout << std::endl;
}
