/* 
 * This program reads a 2D binary file, utilizes SPECK encoder
 * to compress to a target bitrate, and then reports the compression errors.
 */

#define HAVE_SPECK

#include "libQccPack.h"


int sam_read_data( QccIMGImageComponent* image )
{
    const char* filename     = image->filename;
    const long  num_of_vals  = image->num_rows * image->num_cols;
    const long  num_of_bytes = sizeof(float) * num_of_vals;

    /* Test if the input file is healthy. */
    FILE* f = fopen( filename, "r" );
    if( f == NULL )
    {
        fprintf( stderr, "Error! Cannot open input file: %s\n", filename );
        return 1;
    }
    fseek( f, 0, SEEK_END );
    if( ftell(f) != num_of_bytes )
    {
        fprintf( stderr, "Error! Input file size error: %s\n", filename );
        fprintf( stderr, "  Expecting %ld bytes, got %ld bytes.\n", num_of_bytes, ftell(f) );
        fclose( f );
        return 1;
    }
    fseek( f, 0, SEEK_SET );

    /* Allocate space for the input data */
    float* data_buf = (float*)malloc( num_of_bytes );
    if( fread( data_buf, sizeof(float), num_of_vals, f ) != num_of_vals )
    {
        fprintf( stderr, "Error! Input file read error: %s\n", filename );
        free( data_buf );
        fclose( f );
        return 1;
    }
    fclose( f );

    /* Fill the image */
    float min = data_buf[0], max = data_buf[0];
    long counter = 0, row, col;
    for( row = 0; row < image->num_rows; row++ )
        for( col = 0; col < image->num_cols; col++ )
        {
            float val = data_buf[ counter++ ];
            image->image[row][col] = val;
            if( val < min )     min = val;
            if( val > max )     max = val;
        }

    image->min_val = min;
    image->max_val = max;

    free( data_buf );
    return 0;
}


int main( int argc, char** argv )
{
    if( argc != 4 )
    {
        fprintf( stderr, "Usage: ./a.out input_raw_file num_of_cols(DimX) num_of_rows(DimY)\n" );
        return 1;
    }
    const char* input_name  = argv[1];
    const char* output_name = "stream.tmp";
    const int   num_of_cols = atoi( argv[2] );
    const int   num_of_rows = atoi( argv[3] );
    const int   bpp         = 1;    /* bit per pixel */
    const int   total_bits  = bpp * num_of_cols * num_of_rows;
    const int   num_of_levels = 3;

    /* Prepare necessary Wavelets. */
    QccString             WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
    QccString             Boundary = "symmetric";
    QccWAVWavelet         Wavelet;
    if( QccWAVWaveletInitialize( &Wavelet ) ) 
    {
        fprintf( stderr, "QccWAVWaveletInitialize failed.\n" );
        return 1;
    }
    if( QccWAVWaveletCreate( &Wavelet, WaveletFilename, Boundary ) )
    {
        fprintf( stderr, "QccWAVWaveletCreate failed.\n" );
        return 1;
    }

    /* Prepare necessary output buffer. */
    QccBitBuffer          OutputBuffer;
    if( QccBitBufferInitialize( &OutputBuffer ) )
    {
        fprintf( stderr, "QccBitBufferInitialize failed.\n" );
        return 1;
    }
    QccStringCopy( OutputBuffer.filename, output_name );
    OutputBuffer.type     = QCCBITBUFFER_OUTPUT;
    if (QccBitBufferStart(&OutputBuffer))
    {
        fprintf( stderr, "QccBitBufferStart failed.\n" );
        return 1;
    }

    /* Prepare necessary image component, and read data. */
    QccIMGImageComponent    Image;
    if( QccIMGImageComponentInitialize( &Image ) )
    {
        fprintf( stderr, "QccIMGImageComponentInitialize failed.\n" );
        return 1;
    }
    QccStringCopy( Image.filename, input_name );
    Image.num_cols        = num_of_cols;
    Image.num_rows        = num_of_rows;
    if( QccIMGImageComponentAlloc( &Image ) )
    {
        fprintf( stderr, "QccIMGImageComponentAlloc failed.\n" );
        return 1;
    }
    if( sam_read_data( &Image ) )
    {
        fprintf( stderr, "sam_read_data failed.\n" );
        return 1;
    }

    /* Encode to a bitstream, and write to the bit stream. */
    if( QccSPECKEncode( &Image, NULL, num_of_levels, 
                        total_bits, &Wavelet, &OutputBuffer ) )
    {
        fprintf( stderr, "QccSPECKEncode failed.\n" );
        return 1;
    }
    if( QccBitBufferEnd( &OutputBuffer ) )
    {
        fprintf( stderr, "QccBitBufferEnd failed.\n" );
        return 1;
    }


    /* Cleanup */
    QccWAVWaveletFree(&Wavelet);
    QccIMGImageComponentFree( &Image );

    return 0;
}
