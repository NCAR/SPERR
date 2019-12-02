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


#include "speckdecode.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-m %: %s:mask] [-r %: %f:rate] %s:bitstream %s:imgfile"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

int NumLevels;

QccBitBuffer InputBuffer;

int NumRows, NumCols;

QccIMGImage OutputImage;

QccIMGImage Mask;
int MaskSpecified = 0;
int MaskNumRows, MaskNumCols;

double ImageMean;
int MaxCoefficientBits;

float Rate;
int RateSpecified;
int NumPixels;
int TargetBitCnt;


int main(int argc, char *argv[])
{
  int row, col;
  
  QccInit(argc, argv);
  
  QccSPECKHeader();

  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageInitialize(&OutputImage);
  QccIMGImageInitialize(&Mask);
  QccBitBufferInitialize(&InputBuffer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &MaskSpecified,
                         Mask.filename,
                         &RateSpecified,
                         &Rate,
                         InputBuffer.filename,
                         OutputImage.filename))
    QccErrorExit();
  
  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }
  
  InputBuffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccSPECKDecodeHeader(&InputBuffer,
                           &NumLevels,
                           &NumRows, &NumCols,
                           &ImageMean,
                           &MaxCoefficientBits))
    {
      QccErrorAddMessage("%s: Error calling QccSPECKDecodeHeader()",
                         argv[0]);
      QccErrorExit();
    }

  if (MaskSpecified)
    {
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

      if ((MaskNumRows != NumRows) ||
          (MaskNumCols != NumCols))
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
    }
  else
    NumPixels = NumRows * NumCols;

  if (QccIMGImageDetermineType(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageDetermineType()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageColor(&OutputImage))
    {
      QccErrorAddMessage("%s: Output image must be grayscale",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSetSize(&OutputImage,
                         NumRows,
                         NumCols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageAlloc(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (!RateSpecified)
    TargetBitCnt = QCCENT_ANYNUMBITS;
  else
    TargetBitCnt = (int)(ceil((NumPixels * Rate)/8.0))*8;
  
  if (QccSPECKDecode(&InputBuffer,
                     &(OutputImage.Y),
                     (MaskSpecified ? &(Mask.Y) : NULL),
                     NumLevels,
                     &Wavelet,
                     ImageMean,
                     MaxCoefficientBits,
                     TargetBitCnt))
    {
      QccErrorAddMessage("%s: Error calling QccSPECKDecode()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageWrite(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccBitBufferEnd(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()",
                         argv[0]);
      QccErrorExit();
    }

  QccWAVWaveletFree(&Wavelet);
  QccIMGImageFree(&OutputImage);
  QccIMGImageFree(&Mask);

  QccExit;
}
