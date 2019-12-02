/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2004  James E. Fowler
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
 *  This code was written by Justin Rucker, based on
 *  the 2D WDR implementation by Yufei Yuan.
 */


#include "wdrdecode3d.h"

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

float Rate;
int RateSpecified;
int NumPixels;
int TargetBitCnt;


int main(int argc, char *argv[])
{ 
  int row, frame, col;
  
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
  
  if (QccWAVwdr3DDecodeHeader(&InputBuffer,
                             &TransformType,
                             &TemporalNumLevels,
                             &SpatialNumLevels,
                             &NumFrames,
                             &NumRows,
                             &NumCols,
                             &ImageMean,
                             &MaxCoefficientBits))
    {
      QccErrorAddMessage("%s: Error calling QccWAVwdr3DDecodeHeader()",
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
  
  if (QccWAVwdr3DDecode(&InputBuffer,
                        &OutputImage,
                        ((MaskSpecified) ? &Mask : NULL),
                        TransformType,
                        TemporalNumLevels,
                        SpatialNumLevels,
                        &Wavelet,
                        ImageMean,
                        MaxCoefficientBits,
                        TargetBitCnt))
    {
      QccErrorAddMessage("%s: Error calling QccWAVwdrDecode()",
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
  QccIMGImageCubeFree(&Mask);
  QccWAVWaveletFree(&Wavelet);
  
  QccExit;
}
