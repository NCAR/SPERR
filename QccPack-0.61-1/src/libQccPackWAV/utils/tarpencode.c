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


#include "tarpencode.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-nl %d:num_levels] [-m %: %s:mask] [-vo %:] [-a %f:alpha] %f:rate %s:imgfile %s:bitstream"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";


int NumLevels = 5;

float Alpha = 0.6;

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
                         &Alpha,
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

  if (QccWAVTarpEncode(&(InputImage.Y),
                       (MaskSpecified ? &(Mask.Y) : NULL),
                       Alpha,
                       NumLevels,
                       TargetBitCnt,
                       &Wavelet,
                       &OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccWAVTarpEncode()",
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
      printf("TARP coding of %s:\n",
             InputImage.filename);
      printf("  Target rate: %f bpp\n", TargetRate);
      printf("  Actual rate: %f bpp\n", ActualRate);
    }

  QccWAVWaveletFree(&Wavelet);
  QccIMGImageFree(&InputImage);
  QccIMGImageFree(&Mask);

  QccExit;
}
