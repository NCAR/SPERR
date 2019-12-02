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


#include "imgidwt.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-pw %: %s:pcpfile] [-m %: %s:mask] [-l %d:last_subband] %s:subband_pyramid %s:imgfile"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

QccWAVPerceptualWeights PerceptualWeights;
int UsePerceptualWeights = 0;

QccIMGImage Mask;
int MaskSpecified = 0;
int MaskNumRows, MaskNumCols;
QccWAVSubbandPyramid MaskSubbandPyramid;

QccWAVSubbandPyramid SubbandPyramid;
QccIMGImage OutputImage;

int NumCols;
int NumRows;
int NumSubbands;

int LastSubband = -1;

QccWAVWavelet LazyWaveletTransform;

int main(int argc, char *argv[])
{
  int subband;

  QccInit(argc, argv);

  QccWAVWaveletInitialize(&Wavelet);
  QccWAVWaveletInitialize(&LazyWaveletTransform);
  QccWAVPerceptualWeightsInitialize(&PerceptualWeights);
  QccIMGImageInitialize(&Mask);
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);
  QccWAVSubbandPyramidInitialize(&MaskSubbandPyramid);
  QccIMGImageInitialize(&OutputImage);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &UsePerceptualWeights,
                         PerceptualWeights.filename,
                         &MaskSpecified,
                         Mask.filename,
                         &LastSubband,
                         SubbandPyramid.filename,
                         OutputImage.filename))
    QccErrorExit();
      
  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccWAVSubbandPyramidRead(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidRead()",
                         argv[0]);
      QccErrorExit();
    }

  NumSubbands = 
    QccWAVSubbandPyramidNumLevelsToNumSubbands(SubbandPyramid.num_levels);
  NumRows = SubbandPyramid.num_rows;
  NumCols = SubbandPyramid.num_cols;

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
          QccErrorAddMessage("%s: Number of subbands in %s does not match that of subband pyramid %s",
                             argv[0],
                             PerceptualWeights.filename,
                             SubbandPyramid.filename);
          QccErrorExit();
        }
      if (QccWAVPerceptualWeightsRemove(&SubbandPyramid,
                                        &PerceptualWeights))
        {
          QccErrorAddMessage("%s: Error calling QccWAVPerceptualWeightsRemove()",
                             argv[0]);
          QccErrorExit();
        }
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

      if ((MaskNumRows != NumRows) ||
          (MaskNumCols != NumCols))
        {
          QccErrorAddMessage("%s: Mask must be same size as image",
                             argv[0]);
          QccErrorExit();
        }
    }

  if (LastSubband >= 0)
    for (subband = LastSubband + 1; subband < NumSubbands; subband++)
      if (QccWAVSubbandPyramidZeroSubband(&SubbandPyramid, subband))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidZeroSubband()",
                             argv[0]);
          QccErrorExit();
        }

  if (!MaskSpecified)
    {
      if (QccWAVSubbandPyramidInverseDWT(&SubbandPyramid,
                                         &Wavelet))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidInverseDWT()",
                         argv[0]);
          QccErrorExit();
        }
    }
  else
    {
      MaskSubbandPyramid.num_levels = 0;
      MaskSubbandPyramid.num_rows = NumRows;
      MaskSubbandPyramid.num_cols = NumCols;
      MaskSubbandPyramid.matrix = Mask.Y.image;

      if (QccWAVWaveletCreate(&LazyWaveletTransform, "LWT.lft", "symmetric"))
        {
          QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                             argv[0]);
          QccErrorExit();
        }

      if (QccWAVSubbandPyramidDWT(&MaskSubbandPyramid,
                                  SubbandPyramid.num_levels,
                                  &LazyWaveletTransform))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidDWT()",
                             argv[0]);
          QccErrorExit();
        }

      if (QccWAVSubbandPyramidInverseShapeAdaptiveDWT(&SubbandPyramid,
                                                      &MaskSubbandPyramid,
                                                      &Wavelet))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidInverseShapeAdaptiveDWT()",
                         argv[0]);
          QccErrorExit();
        }
    }

  OutputImage.Y.num_rows = NumRows;
  OutputImage.Y.num_cols = NumCols;
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
  QccWAVWaveletFree(&LazyWaveletTransform);
  QccWAVPerceptualWeightsFree(&PerceptualWeights);
  QccIMGImageFree(&Mask);
  QccIMGImageFree(&OutputImage);

  QccExit;
}
