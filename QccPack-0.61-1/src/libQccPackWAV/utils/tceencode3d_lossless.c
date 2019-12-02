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


#include "tceencode3d_lossless.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-nl %: %d:num_levels] [-sl %: %d:spatial_num_levels] [-tl %: %d:temporal_num_levels] [-a %f:alpha]  [-vo %:] %s:icbfile %s:bitstream"

QccWAVWavelet Wavelet;
QccString WaveletFilename = "CohenDaubechiesFeauveau.5-3.int.lft";
QccString Boundary = "symmetric";

int TransformType = QCCWAVSUBBANDPYRAMID3D_PACKET;
int NumLevelsSpecified = 0;
int NumLevels = 5;
int SpatialNumLevelsSpecified = 0;
int SpatialNumLevels = 5;
int TemporalNumLevelsSpecified = 0;
int TemporalNumLevels = 5;

float Alpha = 0.2;

QccIMGImageCube InputImage;
int ImageNumFrames, ImageNumRows, ImageNumCols;

QccBitBuffer OutputBuffer;

int ValueOnly = 0;

float Rate;
int NumPixels;
int TargetBitCnt;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageCubeInitialize(&InputImage);
  
  QccBitBufferInitialize(&OutputBuffer);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &NumLevelsSpecified,
                         &NumLevels,
                         &SpatialNumLevelsSpecified,
                         &SpatialNumLevels,
                         &TemporalNumLevelsSpecified,
                         &TemporalNumLevels,
                         &Alpha,
                         &ValueOnly,
                         InputImage.filename,
                         OutputBuffer.filename))
    QccErrorExit();
  
  if ((NumLevels < 0) || (SpatialNumLevels < 0) || (TemporalNumLevels < 0))
    {
      QccErrorAddMessage("%s: Number of levels of decomposition must be nonnegative",
                         argv[0]);
      QccErrorExit();
    }
  if (NumLevelsSpecified &&
      (SpatialNumLevelsSpecified || TemporalNumLevelsSpecified))
    {
      QccErrorAddMessage("%s: If num_levels is given (dyadic transform), neither spatial_num_levels or temporal_num_levels (packet transform) can be specified",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()", argv[0]);
      QccErrorExit();
    }
  
  if (QccIMGImageCubeRead(&InputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }
  ImageNumFrames = InputImage.num_frames;
  ImageNumRows = InputImage.num_rows;
  ImageNumCols = InputImage.num_cols;
  
  
  NumPixels = ImageNumFrames * ImageNumRows * ImageNumCols;
  
  OutputBuffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccWAVtce3DLosslessEncode(&(InputImage),
                                &OutputBuffer,
                                TransformType,
                                TemporalNumLevels,
                                SpatialNumLevels,
                                Alpha,
                                &Wavelet))
    {
      QccErrorAddMessage("%s: Error calling QccWAVtce3DLosslessEncode()",
                         argv[0]);
      QccErrorExit();
    }
  
  Rate = (double)OutputBuffer.bit_cnt / NumPixels;
  
  if (QccBitBufferEnd(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()", argv[0]);
      QccErrorExit();
    }
  
  if (ValueOnly)
    printf("%f\n", Rate);
  else
    {
      printf("3D-TCE coding of %s:\n", InputImage.filename);
      printf("  Lossless rate: %f bpv\n", Rate);
    }
  
  QccIMGImageCubeFree(&InputImage);
  QccWAVWaveletFree(&Wavelet);
  
  QccExit;
}
