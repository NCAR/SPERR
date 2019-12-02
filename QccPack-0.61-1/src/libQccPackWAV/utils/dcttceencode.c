/*
 * 
 * QccPack: Quantization, compression, and coding libraries
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 * 
 */


/* Written by VIJAY SHAH, at Mississippi State University, 2005 */


#include "dcttceencode.h"

#define USG_STRING "[-nl %d:num_levels] [-ss %: %f:stepsize] [-r %: %f:rate] [-vo %:] [-os %: %d:overlap_sample] [-sf %f:smooth_factor] %s:imgfile %s:bitstream"

QccIMGImage InputImage;
int ImageNumRows, ImageNumCols;

QccBitBuffer OutputBuffer;

int NumLevels = 4;
float TargetRate = -1;
int RateSpecified = 0;
float StepSize = -1;
int StepSizeSpecified = 0;
float ActualRate;
int NumPixels;
int TargetBitCnt;
int ValueOnly = 0;
int OverlapSampleSpecified = 0;
int OverlapSample;
float SmoothFactor = 4.0/3.0;

int BlockSize;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  QccIMGImageInitialize(&InputImage);
  QccBitBufferInitialize(&OutputBuffer);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &NumLevels,
                         &StepSizeSpecified,
                         &StepSize,
                         &RateSpecified,
                         &TargetRate,
                         &ValueOnly,
                         &OverlapSampleSpecified,
                         &OverlapSample,
                         &SmoothFactor,
                         InputImage.filename,
                         OutputBuffer.filename))
    QccErrorExit();
  
  if ((!RateSpecified && !StepSizeSpecified) ||
      (RateSpecified && StepSizeSpecified))
    {
      QccErrorAddMessage("%s: Either rate or stepsize (but not both) must be specified",
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
      QccErrorAddMessage("%s: Error calling QccIMGImageGetResolution()",
                         argv[0]);
      QccErrorExit();
    }

  BlockSize = (1 << NumLevels);
  if ((ImageNumRows % BlockSize) ||
      (ImageNumCols % BlockSize))
    {
      QccErrorAddMessage("%s: Image size (%d x %d) is not an integer multiple of block size %d",
                         argv[0],
                         ImageNumCols,
                         ImageNumRows,
                         BlockSize);
      QccErrorExit();
    }

  NumPixels = ImageNumRows * ImageNumCols;
  
  OutputBuffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (!OverlapSampleSpecified)
    OverlapSample = (1 << (NumLevels - 1));
  else
    if (OverlapSample < 0)
      {
        QccErrorAddMessage("%s: Overlap sample must be positive",
                           argv[0]);
        QccErrorExit();
      }

  if (RateSpecified)
    TargetBitCnt = (int)(ceil((NumPixels * TargetRate)/8.0))*8;
  else
    TargetBitCnt = QCCENT_ANYNUMBITS;
  
  if (QccWAVdcttceEncode(&(InputImage.Y),
                         NumLevels,
                         TargetBitCnt,
                         StepSize,
                         OverlapSample,
                         (double)SmoothFactor,
                         &OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccWAVdcttceEncode()",
                         argv[0]);
      QccErrorExit();
    }
  
  ActualRate = 
    ((float)((double)OutputBuffer.bit_cnt) / NumPixels);
  
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
      printf("DCT-TCE coding of %s:\n",
             InputImage.filename);
      if (RateSpecified)
        printf("  Target rate: %f bpp\n", TargetRate);
      printf("  Actual rate: %f bpp\n", ActualRate);
    }
  
  QccIMGImageFree(&InputImage);
  
  QccExit;
}
