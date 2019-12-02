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


#include "tarpdecode3d.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-m %: %s:mask] [-r %: %f:rate] %s:bitstream %s:icbfile"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

int TransformType;
int TemporalNumLevels;
int SpatialNumLevels;

QccBitBuffer InputBuffer;

int NumFrames, NumRows, NumCols;

QccIMGImageCube OutputImage;

double ImageMean;
int MaxCoefficientBits;

QccIMGImageCube Mask;
int MaskSpecified = 0;
int MaskNumFrames, MaskNumRows, MaskNumCols;

double Alpha;

float Rate;
int RateSpecified;
int NumPixels;
int TargetBitCnt;


int main(int argc, char *argv[])
{
  int frame, row, col;

  QccInit(argc, argv);
  
  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageCubeInitialize(&OutputImage);
  QccIMGImageCubeInitialize(&Mask);
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
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()", argv[0]);
      QccErrorExit();
    }
  
  InputBuffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()", argv[0]);
      QccErrorExit();
    }

  if (QccWAVTarp3DDecodeHeader(&InputBuffer,
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
      QccErrorAddMessage("%s: Error calling QccWAVTarp3DDecodeHeader()",
                         argv[0]);
      QccErrorExit();
    }

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

      if ((MaskNumFrames != NumFrames) ||
          (MaskNumRows != NumRows) ||
          (MaskNumCols != NumCols))
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

  if (QccWAVTarp3DDecode(&InputBuffer,
                         &OutputImage,
                         (MaskSpecified ? &Mask : NULL),
                         TransformType,
                         TemporalNumLevels,
                         SpatialNumLevels,
                         Alpha,
                         &Wavelet,
                         ImageMean,
                         MaxCoefficientBits,
                         TargetBitCnt))
    {
      QccErrorAddMessage("%s: Error calling QccWAVTarp3DDecode()", argv[0]);
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
  QccIMGImageCubeFree(&Mask);
  QccWAVWaveletFree(&Wavelet);

  QccExit;
}
