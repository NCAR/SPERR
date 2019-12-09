/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 */


/*
 * ----------------------------------------------------------------------------
 * 
 * Public License for the SPECK Algorithm
 * Version 1.1, March 8, 2004
 * 
 * ----------------------------------------------------------------------------
 * 
 * The Set-Partitioning Embedded Block (SPECK) algorithm is protected by US
 * Patent #6,671,413 (issued December 30, 2003) and by patents pending.
 * An implementation of the SPECK algorithm is included herein with the
 * gracious permission of Dr. William A. Pearlman, exclusive holder of patent
 * rights. Dr. Pearlman has granted the following license governing the terms
 * and conditions for use, copying, distribution, and modification of the
 * SPECK algorithm implementation contained herein (hereafter referred to as
 * "the SPECK source code").
 * 
 * 0. Use of the SPECK source code, including any executable-program or
 *    linkable-library form resulting from its compilation, is restricted to
 *    solely academic or non-commercial research activities.
 * 
 * 1. Any other use, including, but not limited to, use in the development
 *    of a commercial product, use in a commercial application, or commercial
 *    distribution, is prohibited by this license. Such acts require a separate
 *    license directly from Dr. Pearlman.
 * 
 * 2. For academic and non-commercial purposes, this license does not restrict
 *    use; copying, distribution, and modification are permitted under the
 *    terms of the GNU General Public License as published by the Free Software
 *    Foundation, with the further restriction that the terms of the present
 *    license shall also apply to all subsequent copies, distributions,
 *    or modifications of the SPECK source code.
 * 
 * NO WARRANTY
 * 
 * 3. Dr. Pearlman dislaims all warranties, expressed or implied, including
 *    without limitation any warranty whatsoever as to the fitness for a
 *    particular use or the merchantability of the SPECK source code.
 * 
 * 4. In no event shall Dr. Pearlman be liable for any loss of profits, loss
 *    of business, loss of use or loss of data, nor for indirect, special,
 *    incidental or consequential damages of any kind related to use of the
 *    SPECK source code.
 * 
 * 
 * END OF TERMS AND CONDITIONS
 * 
 * 
 * Persons desiring to license the SPECK algorithm for commercial purposes or
 * for uses otherwise prohibited by this license may wish to contact
 * Dr. Pearlman regarding the possibility of negotiating such licenses:
 * 
 *   Dr. William A. Pearlman
 *   Dept. of Electrical, Computer, and Systems Engineering
 *   Rensselaer Polytechnic Institute
 *   Troy, NY 12180-3590
 *   U.S.A.
 *   email: pearlman@ecse.rpi.edu
 *   tel.: (518) 276-6082
 *   fax: (518) 276-6261
 *  
 * ----------------------------------------------------------------------------
 */
#include "speckencode.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-nl %d:num_levels] [-m %: %s:mask] [-vo %:] %f:rate %s:imgfile %s:bitstream"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";


int NumLevels = 5;

QccIMGImage Mask;
int MaskSpecified = 0;
int MaskNumRows, MaskNumCols;

QccIMGImage InputImage;
int ImageNumRows, ImageNumCols;

QccBitBuffer OutputBuffer;


int ValueOnly = 0;

float TargetRate;
float ActualRate;
int NumPixels;
int TargetBitCnt;


int main(int argc, char *argv[])
{
  int row, col;

  QccInit(argc, argv);

  QccSPECKHeader();

  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageInitialize(&InputImage);
  QccIMGImageInitialize(&Mask);
  QccBitBufferInitialize(&OutputBuffer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &NumLevels,
                         &MaskSpecified,
                         Mask.filename,
                         &ValueOnly,
                         &TargetRate,
                         InputImage.filename,
                         OutputBuffer.filename))
    QccErrorExit();

  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageRead(&InputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageColor(&InputImage))
    {
      QccErrorAddMessage("%s: Color images not supported",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageGetSize(&InputImage,
                         &ImageNumRows, &ImageNumCols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSize()",
                         argv[0]);
      QccErrorExit();
    }

  if (MaskSpecified)
  {/*
      if (QccIMGImageRead(&Mask))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageColor(&Mask))
        {
          QccErrorAddMessage("%s: Mask must be grayscale",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageGetSize(&Mask,
                             &MaskNumRows, &MaskNumCols))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageGetSize()",
                             argv[0]);
          QccErrorExit();
        }

      if ((MaskNumRows != ImageNumRows) ||
          (MaskNumCols != ImageNumCols))
        {
          QccErrorAddMessage("%s: Mask must be same size as image",
                             argv[0]);
          QccErrorExit();
        }

      NumPixels = 0;

      for (row = 0; row < MaskNumRows; row++)
        for (col = 0; col < MaskNumCols; col++)
          if (!QccAlphaTransparent(Mask.Y.image[row][col]))
            NumPixels++;
    */
  }
  else
    NumPixels = ImageNumRows * ImageNumCols;

  OutputBuffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
   
  TargetBitCnt = (int)(ceil((NumPixels * TargetRate)/8.0))*8;

  if (QccSPECKEncode(&(InputImage.Y),
                     (MaskSpecified ? &(Mask.Y) : NULL),
                     NumLevels,
                     TargetBitCnt,
                     &Wavelet,
                     &OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccSPECKEncode()",
                         argv[0]);
      QccErrorExit();
    }
  
  ActualRate = 
    (double)OutputBuffer.bit_cnt / NumPixels;

  if (QccBitBufferEnd(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (ValueOnly)
    printf("%f\n", ActualRate);
  else
    {
      printf("SPECK coding of %s:\n",
             InputImage.filename);
      printf("  Target rate: %f bpp\n", TargetRate);
      printf("  Actual rate: %f bpp\n", ActualRate);
    }

  QccWAVWaveletFree(&Wavelet);
  QccIMGImageFree(&InputImage);
  QccIMGImageFree(&Mask);

  QccExit;
}
