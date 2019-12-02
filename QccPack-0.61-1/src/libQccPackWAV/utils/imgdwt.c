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


#include "imgdwt.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-pw %: %s:pcpfile] [-m %: %s:mask] [-nl %d:levels] %s:imgfile %s:subband_pyramid"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

QccWAVPerceptualWeights PerceptualWeights;
int UsePerceptualWeights = 0;

int NumLevels = 1;
int NumSubbands;

QccIMGImage Mask;
int MaskSpecified = 0;
int MaskNumRows, MaskNumCols;
QccWAVSubbandPyramid MaskSubbandPyramid;

QccIMGImage InputImage;
QccWAVSubbandPyramid SubbandPyramid;

int ImageNumCols;
int ImageNumRows;


int main(int argc, char *argv[])
{
  
  QccInit(argc, argv);

  QccWAVWaveletInitialize(&Wavelet);
  QccWAVPerceptualWeightsInitialize(&PerceptualWeights);
  QccIMGImageInitialize(&Mask);
  QccIMGImageInitialize(&InputImage);
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);
  QccWAVSubbandPyramidInitialize(&MaskSubbandPyramid);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &UsePerceptualWeights,
                         PerceptualWeights.filename,
                         &MaskSpecified,
                         Mask.filename,
                         &NumLevels,
                         InputImage.filename,
                         SubbandPyramid.filename))
    QccErrorExit();
      
  if (NumLevels < 1)
    {
      QccErrorAddMessage("%s: Number of levels must be 1 or greater",
                         argv[0]);
      QccErrorExit();
    }
  NumSubbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(NumLevels);

  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (UsePerceptualWeights)
    {
      if (QccWAVPerceptualWeightsRead(&PerceptualWeights))
        {
          QccErrorAddMessage("%s: Error calling QccWAVPerceptualWeightsRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (PerceptualWeights.num_subbands != NumSubbands)
        {
          QccErrorAddMessage("%s: Number of subbands in %s does not match specified number of levels (%d)",
                             argv[0],
                             PerceptualWeights.filename,
                             NumLevels);
          QccErrorExit();
        }
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

  ImageNumCols = InputImage.Y.num_cols;
  ImageNumRows = InputImage.Y.num_rows;

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
    }

  SubbandPyramid.num_levels = 0;
  SubbandPyramid.num_rows = ImageNumRows;
  SubbandPyramid.num_cols = ImageNumCols;
  SubbandPyramid.matrix = InputImage.Y.image;
  
  if (!MaskSpecified)
    {
      if (QccWAVSubbandPyramidDWT(&SubbandPyramid,
                                  NumLevels,
                                  &Wavelet))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidDWT()",
                             argv[0]);
          QccErrorExit();
        }
    }
  else
    {
      MaskSubbandPyramid.num_levels = 0;
      MaskSubbandPyramid.num_rows = ImageNumRows;
      MaskSubbandPyramid.num_cols = ImageNumCols;
      MaskSubbandPyramid.matrix = Mask.Y.image;
  
      if (QccWAVSubbandPyramidShapeAdaptiveDWT(&SubbandPyramid,
                                               &MaskSubbandPyramid,
                                               NumLevels,
                                               &Wavelet))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidShapeAdaptiveDWT()",
                             argv[0]);
          QccErrorExit();
        }
    }

  if (UsePerceptualWeights)
    if (QccWAVPerceptualWeightsApply(&SubbandPyramid,
                                     &PerceptualWeights))
      {
        QccErrorAddMessage("%s: Error calling QccWAVPerceptualWeightsApply()",
                           argv[0]);
        QccErrorExit();
      }

  if (QccWAVSubbandPyramidWrite(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccWAVWaveletFree(&Wavelet);
  QccWAVPerceptualWeightsFree(&PerceptualWeights);
  QccIMGImageFree(&Mask);
  QccIMGImageFree(&InputImage);

  QccExit;
}
