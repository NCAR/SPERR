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


#include "make_noise_image.h"

#define SQUARE_START 0.25
#define SQUARE_WIDTH 0.5

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-nl %d:levels] [-s %d:subband] [-w %d:image_width] [-h %d:image_height] [-bg %f:background] [-n %f:noise_variance] %s:imgfile"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

int NumLevels = 1;
int Subband = 0;
int ImageWidth = 512;
int ImageHeight = 512;
float Background = 127;
float NoiseVariance = 1.0;

int NumSubbands;

QccWAVSubbandPyramid SubbandPyramid;
QccIMGImage OutputImage;

int main(int argc, char *argv[])
{
  int row, col;
  
  QccInit(argc, argv);
  
  QccWAVWaveletInitialize(&Wavelet);
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);
  QccIMGImageInitialize(&OutputImage);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &NumLevels,
                         &Subband,
                         &ImageWidth,
                         &ImageHeight,
                         &Background,
                         &NoiseVariance,
                         OutputImage.filename))
    QccErrorExit();
  
  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (NumLevels < 0)
    {
      QccErrorAddMessage("%s: Number of levels must be nonnegative",
                         argv[0]);
      QccErrorExit();
    }
  NumSubbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(NumLevels);
  if ((ImageWidth < 1) || (ImageHeight < 1))
    {
      QccErrorAddMessage("%s: Invalid image size (%d x %d)",
                         argv[0], ImageWidth, ImageHeight);
      QccErrorExit();
    }
  if ((ImageWidth != ((int)(ImageWidth >> NumLevels) << NumLevels)) ||
      (ImageHeight != ((int)(ImageHeight >> NumLevels) << NumLevels)))
    {
      QccErrorAddMessage("%s: Image size (%d x %d) is not a multiple of %d (as needed for a decomposition of %d levels)",
                         argv[0],
                         ImageWidth, ImageHeight,
                         (1 << NumLevels), NumLevels);
      QccErrorExit();
    }
  if ((Subband < 0) || (Subband >= NumSubbands))
    {
      QccErrorAddMessage("%s: Requested subband %d is invalid for a %d-level decomposition)",
                         argv[0], Subband, NumLevels);
      QccErrorExit();
    }
  if (NoiseVariance < 0.0)
    {
      QccErrorAddMessage("%s: Invalid noise variance (%f)",
                         argv[0], NoiseVariance);
      QccErrorExit();
    }
  
  SubbandPyramid.num_rows = ImageHeight;
  SubbandPyramid.num_cols = ImageWidth;
  SubbandPyramid.num_levels = NumLevels;
  if (QccWAVSubbandPyramidAlloc(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidAlloc()",
                         argv[0]);
      QccErrorExit();
    }
  for (row = 0; row < ImageHeight; row++)
    for (col = 0; col < ImageWidth; col++)
      SubbandPyramid.matrix[row][col] = 0.0;
  
  if (QccWAVSubbandPyramidAddNoiseSquare(&SubbandPyramid,
                                         Subband,
                                         SQUARE_START,
                                         SQUARE_WIDTH,
                                         (double)NoiseVariance))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidAddNoiseSquare()",
                         argv[0]);
      QccErrorExit();
    }
  if (NumLevels)
    if (QccWAVSubbandPyramidInverseDWT(&SubbandPyramid,
                                       &Wavelet))
      {
        QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidInverseDWT()",
                           argv[0]);
        QccErrorExit();
      }
  
  if (QccWAVSubbandPyramidAddMean(&SubbandPyramid,
                                  (double)Background))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidAddMean()",
                         argv[0]);
      QccErrorExit();
    }

  OutputImage.Y.num_rows = SubbandPyramid.num_rows;
  OutputImage.Y.num_cols = SubbandPyramid.num_cols;
  OutputImage.Y.image = SubbandPyramid.matrix;

  if (QccIMGImageSetMaxMin(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetMaxMin()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccIMGImageWrite(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  QccWAVWaveletFree(&Wavelet);
  QccIMGImageFree(&OutputImage);

  QccExit;
}
