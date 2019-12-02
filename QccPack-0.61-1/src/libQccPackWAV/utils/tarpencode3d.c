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


#include "tarpencode3d.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-nl %: %d:num_levels] [-sl %: %d:spatial_num_levels] [-tl %: %d:temporal_num_levels] [-a %f:alpha] [-m %: %s:mask] [-vo %:] %f:rate %s:icbfile %s:bitstream"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

int TransformType = QCCWAVSUBBANDPYRAMID3D_DYADIC;
int NumLevelsSpecified = 0;
int NumLevels = 5;
int SpatialNumLevelsSpecified = 0;
int SpatialNumLevels = 0;
int TemporalNumLevelsSpecified = 0;
int TemporalNumLevels = 0;

float Alpha = 0.3;

QccIMGImageCube InputImage;
int ImageNumFrames, ImageNumRows, ImageNumCols;

QccIMGImageCube Mask;
int MaskSpecified = 0;
int MaskNumFrames, MaskNumRows, MaskNumCols;

QccBitBuffer OutputBuffer;

int ValueOnly = 0;

float TargetRate;
float ActualRate;
int NumPixels;
int TargetBitCnt;


int main(int argc, char *argv[])
{
  int frame, row, col;
  
  QccInit(argc, argv);

  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageCubeInitialize(&InputImage);
  QccIMGImageCubeInitialize(&Mask);
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
                         &MaskSpecified,
                         Mask.filename,
                         &ValueOnly,
                         &TargetRate,
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
  if (NumLevelsSpecified ||
      ((!SpatialNumLevelsSpecified) && (!TemporalNumLevelsSpecified)))
    {
      TransformType = QCCWAVSUBBANDPYRAMID3D_DYADIC;
      SpatialNumLevels = NumLevels;
      TemporalNumLevels = NumLevels;
    }
  else
    {
      TransformType = QCCWAVSUBBANDPYRAMID3D_PACKET;
      if (SpatialNumLevelsSpecified && (!TemporalNumLevelsSpecified))
        TemporalNumLevels = SpatialNumLevels;
      else
        if ((!SpatialNumLevelsSpecified) && TemporalNumLevelsSpecified)
          SpatialNumLevels = TemporalNumLevels;
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

  if (MaskSpecified)
    {
      if (QccIMGImageCubeRead(&Mask))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                             argv[0]);
          QccErrorExit();
        }

      MaskNumFrames = Mask.num_frames;
      MaskNumRows = Mask.num_rows;
      MaskNumCols = Mask.num_cols;

      if ((MaskNumFrames != ImageNumFrames) ||
          (MaskNumRows != ImageNumRows) ||
          (MaskNumCols != ImageNumCols))
        {
          QccErrorAddMessage("%s: Mask must be same size as image cube",
                             argv[0]);
          QccErrorExit();
        }

      NumPixels = 0;

      for (frame = 0; frame < MaskNumFrames; frame++)
        for (row = 0; row < MaskNumRows; row++)
          for (col = 0; col < MaskNumCols; col++)
            if (!QccAlphaTransparent(Mask.volume[frame][row][col]))
              NumPixels++;
    }
  else
    NumPixels = ImageNumFrames * ImageNumRows * ImageNumCols;

  OutputBuffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
   
  TargetBitCnt = (int)(ceil((NumPixels * TargetRate)/8.0))*8;

  if (QccWAVTarp3DEncode(&(InputImage),
                         (MaskSpecified ? &(Mask) : NULL),
                         &OutputBuffer,
                         TransformType,
                         TemporalNumLevels,
                         SpatialNumLevels,
                         Alpha,
                         &Wavelet,
                         TargetBitCnt))
    {
      QccErrorAddMessage("%s: Error calling QccWAVTarp3DEncode()",
                         argv[0]);
      QccErrorExit();
    }

  ActualRate = (double)OutputBuffer.bit_cnt / NumPixels;

  if (QccBitBufferEnd(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()", argv[0]);
      QccErrorExit();
    }
  
  if (ValueOnly)
    printf("%f\n", ActualRate);
  else
    {
      printf("Tarp3D coding of %s:\n", InputImage.filename);
      printf("  Target rate: %f bpv\n", TargetRate);
      printf("  Actual rate: %f bpv\n", ActualRate);
    }

  QccIMGImageCubeFree(&InputImage);
  QccIMGImageCubeFree(&Mask);
  QccWAVWaveletFree(&Wavelet);

  QccExit;
}
