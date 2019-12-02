/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2007  James E. Fowler
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

/* 
 *
 * Written by
 *
 * Jing Zhang, at Mississippi State University, 2008 
 *
 */


#include "tcedecode3d_lossless.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-r %: %f:rate] %s:bitstream %s:icbfile"

QccWAVWavelet Wavelet;
QccString WaveletFilename = "CohenDaubechiesFeauveau.5-3.int.lft";
QccString Boundary = "symmetric";

int TransformType;
int TemporalNumLevels;
int SpatialNumLevels;

QccBitBuffer InputBuffer;

int NumFrames, NumRows, NumCols;

QccIMGImageCube OutputImage;

int ImageMean;
int MaxCoefficientBits;



double Alpha;

float Rate;
int RateSpecified;
int NumPixels;
int TargetBitCnt;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageCubeInitialize(&OutputImage);
  
  QccBitBufferInitialize(&InputBuffer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                        
                         &RateSpecified,
                         &Rate,
                         InputBuffer.filename,
                         OutputImage.filename))
    QccErrorExit();
  
  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()", argv[0]);
      QccErrorExit();
    }
  
  InputBuffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()", argv[0]);
      QccErrorExit();
    }

  if (QccWAVtce3DLosslessDecodeHeader(&InputBuffer,
                                      &TransformType,
                                      &TemporalNumLevels,
                                      &SpatialNumLevels,
                                      &NumFrames,
                                      &NumRows,
                                      &NumCols,
                                      &ImageMean,
                                      &MaxCoefficientBits,
                                      &Alpha))
    {
      QccErrorAddMessage("%s: Error calling QccWAVtce3DLosslessDecodeHeader()",
                         argv[0]);
      QccErrorExit();
    }
  
  NumPixels = NumFrames * NumRows * NumCols;
  
  OutputImage.num_frames = NumFrames;
  OutputImage.num_rows = NumRows;
  OutputImage.num_cols = NumCols;
  if (QccIMGImageCubeAlloc(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeAlloc()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (!RateSpecified)
    TargetBitCnt = QCCENT_ANYNUMBITS;
  else
    TargetBitCnt = (int)(ceil((NumPixels * Rate)/8.0))*8;


  if (QccWAVtce3DLosslessDecode(&InputBuffer,
                                &OutputImage,
                                TransformType,
                                TemporalNumLevels,
                                SpatialNumLevels,
                                Alpha,
                                &Wavelet,
                                ImageMean,
                                MaxCoefficientBits,
                                TargetBitCnt))
    {
      QccErrorAddMessage("%s: Error calling QccWAVtce3DLosslessDecode()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageCubeWrite(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccBitBufferEnd(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()", argv[0]);
      QccErrorExit();
    }

  QccIMGImageCubeFree(&OutputImage);
  
  QccWAVWaveletFree(&Wavelet);

  QccExit;
}
