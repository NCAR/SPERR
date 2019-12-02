/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "biskdecode.h"

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

  if (QccWAVbiskDecodeHeader(&InputBuffer,
                             &NumLevels,
                             &NumRows, &NumCols,
                             &ImageMean,
                             &MaxCoefficientBits))
    {
      QccErrorAddMessage("%s: Error calling QccWAVbiskDecodeHeader()",
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
  
  if (QccWAVbiskDecode(&InputBuffer,
                       &(OutputImage.Y),
                       (MaskSpecified ? &(Mask.Y) : NULL),
                       NumLevels,
                       &Wavelet,
                       ImageMean,
                       MaxCoefficientBits,
                       TargetBitCnt))
    {
      QccErrorAddMessage("%s: Error calling QccWAVbiskDecode()",
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
