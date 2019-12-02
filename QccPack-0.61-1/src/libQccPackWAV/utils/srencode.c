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


#include "srencode.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-pw %: %s:pcpfile] [-nl %d:num_levels] [-q %f:stepsize] [-d %f:deadzone] [-vo %:] %s:imgfile %s:bitstream"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

QccWAVPerceptualWeights PerceptualWeights;
int UsePerceptualWeights = 0;

int NumLevels = 5;
int NumSubbands;

float StepSize = 47.1;
float DeadZone = -1;
QccSQScalarQuantizer Quantizer;

QccIMGImage InputImage;
int ImageNumRows, ImageNumCols;

QccBitBuffer OutputBuffer;

int ValueOnly = 0;

double Rate;

int main(int argc, char *argv[])
{

  QccInit(argc, argv);
  
  QccWAVWaveletInitialize(&Wavelet);
  QccWAVPerceptualWeightsInitialize(&PerceptualWeights);
  QccIMGImageInitialize(&InputImage);
  QccBitBufferInitialize(&OutputBuffer);
  QccSQScalarQuantizerInitialize(&Quantizer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &UsePerceptualWeights,
                         PerceptualWeights.filename,
                         &NumLevels,
                         &StepSize,
                         &DeadZone,
                         &ValueOnly,
                         InputImage.filename,
                         OutputBuffer.filename))
    QccErrorExit();
  
  if (NumLevels < 0)
    {
      QccErrorAddMessage("%s: Number of levels of decomposition must be nonnegative",
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
        }
    }

  if (StepSize <= 0)
    {
      QccErrorAddMessage("%s: stepsize must be > 0",
                         argv[0]);
      QccErrorExit();
    }

  if (DeadZone < 0)
    {
      Quantizer.type = QCCSQSCALARQUANTIZER_UNIFORM;
      Quantizer.stepsize = StepSize;
    }
  else
    {
      Quantizer.type = QCCSQSCALARQUANTIZER_DEADZONE;
      Quantizer.stepsize = StepSize;
      Quantizer.deadzone = DeadZone;
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
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSize()",
                         argv[0]);
      QccErrorExit();
    }
  if (ImageNumCols != ((int)(ImageNumCols >> NumLevels) << NumLevels))
    {
      QccErrorAddMessage("%s: Image size (%d x %d) is not a multiple of %d (as needed for a decomposition of %d levels)",
                         argv[0],
                         ImageNumCols, ImageNumRows,
                         (1 << NumLevels), NumLevels);
      QccErrorExit();
    }
  if (ImageNumRows != ((int)(ImageNumRows >> NumLevels) << NumLevels))
    {
      QccErrorAddMessage("%s: Image size (%d x %d) is not a multiple of %d (as needed for a decomposition of %d levels",
                         argv[0],
                         ImageNumCols, ImageNumRows,
                         (1 << NumLevels), NumLevels);
      QccErrorExit();
    }
  
  OutputBuffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }

   
  if (QccWAVsrEncode(&(InputImage.Y),
                     &OutputBuffer,
                     &Quantizer,
                     NumLevels,
                     &Wavelet,
                     ((UsePerceptualWeights) ? &PerceptualWeights : NULL)))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsrEncode()",
                         argv[0]);
      QccErrorExit();
    }
      
  Rate = 
    (double)OutputBuffer.bit_cnt / ImageNumRows / ImageNumCols;

  if (QccBitBufferEnd(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (ValueOnly)
    printf("%f\n", Rate);
  else
    {
      printf("SR coding of %s:\n",
             InputImage.filename);
      printf("  Total rate: %f bpp\n", Rate);
    }

  QccWAVWaveletFree(&Wavelet);
  QccWAVPerceptualWeightsFree(&PerceptualWeights);
  QccIMGImageFree(&InputImage);
  QccSQScalarQuantizerFree(&Quantizer);

  QccExit;
}
