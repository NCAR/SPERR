/* 
 * This program reads a 3D binary file, utilizes SPECK encoder
 * to compress to a target bitrate, and then reports the compression errors.
 */

#define HAVE_SPECK

#include "libQccPack.h"
#include "sam_helper.h"
#include "assert.h"

#ifdef TIME_EXAMPLES
    #include <sys/time.h>
#endif

#include "SpeckConfig.h"


void array_to_image_cube( const float* array, QccIMGImageCube* imagecube )
{
    float min = array[0];
    float max = array[0];
    size_t idx = 0, frame, row, col;
    for( frame = 0; frame < imagecube -> num_frames; frame++ )
        for( row = 0; row < imagecube -> num_rows; row++ )
            for( col = 0; col < imagecube -> num_cols; col++ )
            {
                if( array[idx] < min )         min = array[idx];
                if( array[idx] > max )         max = array[idx];
                imagecube -> volume[frame][row][col] = array[idx];
                idx++;
            }
    imagecube -> min_val = min;
    imagecube -> max_val = max;
}


void image_cube_to_array( const QccIMGImageCube* imagecube, float* array )
{
    size_t idx = 0, frame, row, col;
    for( frame = 0; frame < imagecube -> num_frames; frame++ )
        for( row = 0; row < imagecube -> num_rows; row++ )
            for( col = 0; col < imagecube -> num_cols; col++ )
            {
                array[idx++] = imagecube -> volume[frame][row][col];
            }
}


int calc_num_of_xforms( int len )
{
    assert( len > 0 );
    // I decide 8 is the minimal length to do one level of xform.
    float ratio = (float)len / 8.0f;
    float f     = logf(ratio) / logf(2.0f);
    int num_of_xforms = f < 0.0f ? 0 : (int)f + 1;

    return num_of_xforms;
}


int main( int argc, char** argv )
{
    if( argc != 6 )
    {
        fprintf( stderr, "Usage: ./a.out input_file dim_x dim_y dim_z cratio\n" );
        return 1;
    }
    const char* input_name    = argv[1];
    int         num_of_cols   = atoi( argv[2] );
    int         num_of_rows   = atoi( argv[3] );
    int         num_of_frames = atoi( argv[4] );
    double      cratio        = atof( argv[5] );
    const size_t num_of_vals  = num_of_cols * num_of_rows * num_of_frames;
    const size_t num_of_bytes = sizeof(float) * num_of_vals;

    const int header_size     = 29;
    const int total_bits      = (int)(8.0f * num_of_bytes / cratio) + header_size * 8;

    /* 
     * Stage 1: Encoding
     */

    /* Prepare wavelet */
	QccString       waveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
    QccString       boundary = "symmetric";
    QccWAVWavelet   wavelet;
    if( QccWAVWaveletInitialize(&wavelet) )
    {
        fprintf( stderr, "QccWAVWaveletInitialize failed.\n" );
        return 1;
    }
    if (QccWAVWaveletCreate( &wavelet, waveletFilename, boundary )) 
    {
        fprintf( stderr, "Error calling QccWAVWaveletCreate()\n" );
        return 1;
    }    

    /* Prepare output bit buffer */
    QccBitBuffer    output_buffer;
    if( QccBitBufferInitialize(&output_buffer) )
    {
        fprintf( stderr, "QccBitBufferInitialize() failed\n" );
        return 1;
    }    
    QccStringCopy( output_buffer.filename, "qcc.tmp" );
    output_buffer.type = QCCBITBUFFER_OUTPUT;
    if (QccBitBufferStart(&output_buffer))
    {
        fprintf( stderr, "Error calling QccBitBufferStart()\n");
        return 1;
    }

    /* Prepare image cube */
    float* in_array = (float*)malloc( num_of_bytes );
    if( sam_read_n_bytes( input_name, num_of_bytes, in_array ) )
    {
        fprintf( stderr, "file read error: %s\n", input_name );
        return 1;
    }
    QccIMGImageCube     imagecube;
    if( QccIMGImageCubeInitialize( &imagecube ) )
    {
        fprintf( stderr, "QccIMGImageCubeInitialize() failed\n");
        return 1;
    }
    imagecube.num_cols = num_of_cols;
    imagecube.num_rows = num_of_rows;
    imagecube.num_frames = num_of_frames;
    if( QccIMGImageCubeAlloc( &imagecube ) )
    {
        fprintf( stderr, "QccIMGImageCubeAlloc() failed\n");
        return 1;
    }
    array_to_image_cube( in_array, &imagecube );

    /* Get ready for SPECK! */
    int transform_type   = QCCWAVSUBBANDPYRAMID3D_PACKET;
    int num_of_levels_xy = calc_num_of_xforms( num_of_cols );
    int num_of_levels_z  = calc_num_of_xforms( num_of_frames );
    if( QccSPECK3DEncode( &imagecube, NULL, transform_type,
                          num_of_levels_z, num_of_levels_xy,
                          &wavelet, &output_buffer, total_bits ) )
    {
        fprintf( stderr, "QccSPECK3DEncode() failed.\n" );
        return 1;
    }

    /* Write bit buffer to disk */
    if( QccBitBufferEnd( &output_buffer ) )
    {
        fprintf( stderr, "QccBitBufferEnd() failed.\n" );
        return 1;
    }
}


#if 0
void myspeckdecode3d_64bit( const char*  inputFilename,
                            double* dstBuf,
                            int     outSize )
{
    QccBitBuffer InputBuffer;
    QccBitBufferInitialize( &InputBuffer );
    QccStringCopy( InputBuffer.filename, inputFilename );
    InputBuffer.type = QCCBITBUFFER_INPUT;

    QccWAVWavelet Wavelet;
    QccWAVWaveletInitialize( &Wavelet );
    QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
    QccString Boundary = "symmetric";
    if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("Error calling QccWAVWaveletCreate()");
      QccErrorExit();
    }    
    if (QccBitBufferStart(&InputBuffer)) 
    {
        QccErrorAddMessage("Error calling QccBitBufferStart()" );
        QccErrorExit();
    }

    int TransformType;
    int TemporalNumLevels;
    int SpatialNumLevels;
    int NumFrames, NumRows, NumCols;
    double ImageMean;
    int MaxCoefficientBits;
    if (QccSPECK3DDecodeHeader( &InputBuffer, 
                                &TransformType, 
                                &TemporalNumLevels, 
                                &SpatialNumLevels,
                                &NumFrames, 
                                &NumRows, 
                                &NumCols,
                                &ImageMean, 
                                &MaxCoefficientBits ))
    {
      QccErrorAddMessage("Error calling QccSPECK3DDecodeHeader()");
      QccErrorExit();
    }

    int NumPixels = NumFrames * NumRows * NumCols;
    long long int pxlcount = (long long int)NumFrames * NumRows * NumCols;
    if( pxlcount > INT_MAX ) 
    {
        QccErrorAddMessage("NumPixels overflow. Please try smaller data sets.");
        QccErrorExit();
    }
    if( outSize != NumPixels ) 
    {
        QccErrorAddMessage("Decode output buffer size doesn't match signal length.");
        QccErrorExit();
    }    

    QccIMGImageCube imagecube;
    QccIMGImageCubeInitialize( &imagecube );
    imagecube.num_frames = NumFrames;
    imagecube.num_rows = NumRows;
    imagecube.num_cols = NumCols;
    if (QccIMGImageCubeAlloc(&imagecube) ) 
    {
      QccErrorAddMessage("Error calling QccIMGImageCubeAlloc()" );
      QccErrorExit();
    }
    int TargetBitCnt = QCCENT_ANYNUMBITS;

    if (QccSPECK3DDecode( &InputBuffer, 
                          &imagecube, 
                          NULL, 
                          TransformType, 
                          TemporalNumLevels, 
                          SpatialNumLevels, 
                          &Wavelet, 
                          ImageMean,
                          MaxCoefficientBits, 
                          TargetBitCnt ))
    {
        QccErrorAddMessage("Error calling QccSPECK3DDecode()" );
        QccErrorExit();
    }
    if (QccBitBufferEnd(&InputBuffer)) 
    {
      QccErrorAddMessage("Error calling QccBitBufferEnd()" );
      QccErrorExit();
    }

    int idx = 0;
    int frame, row, col;
    for( frame = 0; frame < imagecube.num_frames; frame++ )
        for( row = 0; row < imagecube.num_rows; row++ )
            for( col = 0; col < imagecube.num_cols; col++ )
                dstBuf[ idx++ ] = imagecube.volume[frame][row][col];    

    QccIMGImageCubeFree( &imagecube );
    QccWAVWaveletFree( &Wavelet );

}
#endif

















