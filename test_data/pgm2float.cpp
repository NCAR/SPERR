#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <memory>

int main()
{
    // These values are specific to the lena image
    const char* infilename   = "lena512.pgm";
    const char* outfilename  = "lena512.float";
    const size_t header_size = 34;
    const size_t total_size  = 262178;
    const size_t body_size   = total_size - header_size;

    std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>( total_size );
    std::ifstream infile( infilename, std::ios::binary );
    infile.read( reinterpret_cast<char*>(buf.get()), total_size );
    infile.close();

    std::vector<float> outbuf( body_size );
    for( int i = 0; i < body_size; i++ )
        outbuf[i] = buf[ i + header_size ];

    std::ofstream outfile( outfilename, std::ios::binary );
    outfile.write( reinterpret_cast<char*>( outbuf.data() ), sizeof(float) * body_size );
    outfile.close();
}
