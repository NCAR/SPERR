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


#include "dcttcedecode.h"

#define USG_STRING "[-r %: %f:rate] %s:bitstream %s:imgfile"

int NumLevels;
QccBitBuffer InputBuffer;

int NumRows, NumCols;
QccIMGImage OutputImage;

double ImageMean;
double StepSize;
int MaxCoefficientBits;
float DecodeRate;
int RateSpecified = 0;
int TargetBitCnt;
int OverlapSample;
double SmoothFactor;


int main(int argc, char *argv[])
{
  
  QccInit(argc, argv);
  
  QccIMGImageInitialize(&OutputImage);
  QccBitBufferInitialize(&InputBuffer);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &RateSpecified,
                         &DecodeRate,
                         InputBuffer.filename,
                         OutputImage.filename))
    QccErrorExit();
  
   
  InputBuffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccWAVdcttceDecodeHeader(&InputBuffer,
                               &NumLevels,
                               &NumRows,
                               &NumCols,
                               &ImageMean,
                               &StepSize,
                               &MaxCoefficientBits,
                               &OverlapSample,
                               &SmoothFactor))
    {
      QccErrorAddMessage("%s: Error calling QccWAVdcttceDecodeHeader()",
                         argv[0]);
      QccErrorExit();
    }
  
  OutputImage.image_type = QCCIMGTYPE_PGM;
  if (QccIMGImageSetSize(&OutputImage,
                         NumRows,
                         NumCols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetResolution()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageAlloc(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (RateSpecified)
    TargetBitCnt = (int)(DecodeRate * NumRows * NumCols);
  else
    TargetBitCnt = QCCENT_ANYNUMBITS;
  
  if (QccWAVdcttceDecode(&InputBuffer,
                         &(OutputImage.Y),
                         NumLevels,
                         ImageMean,
                         StepSize,
                         MaxCoefficientBits,
                         TargetBitCnt,
                         OverlapSample,
                         SmoothFactor))
    {
      QccErrorAddMessage("%s: Error calling QccWAVdcttceDecode()",
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
  
  QccIMGImageFree(&OutputImage);

  QccExit;
}
