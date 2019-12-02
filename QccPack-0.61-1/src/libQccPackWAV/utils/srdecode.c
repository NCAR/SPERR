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


#include "srdecode.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-pw %: %s:pcpfile] %s:bitstream %s:imgfile"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

QccWAVPerceptualWeights PerceptualWeights;
int UsePerceptualWeights = 0;

int NumLevels;
int NumSubbands;

QccBitBuffer InputBuffer;

QccSQScalarQuantizer Quantizer;
double StepSize;

int NumRows, NumCols;

QccIMGImage OutputImage;

double ImageMean;


int main(int argc, char *argv[])
{
  
  QccInit(argc, argv);
  
  QccWAVWaveletInitialize(&Wavelet);
  QccWAVPerceptualWeightsInitialize(&PerceptualWeights);
  QccSQScalarQuantizerInitialize(&Quantizer);
  QccIMGImageInitialize(&OutputImage);
  QccBitBufferInitialize(&InputBuffer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &UsePerceptualWeights,
                         PerceptualWeights.filename,
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
  
  if (QccWAVsrDecodeHeader(&InputBuffer,
                           &NumLevels,
                           &NumRows, &NumCols,
                           &Quantizer,
                           &ImageMean))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsrDecodeHeader()",
                         argv[0]);
      QccErrorExit();
    }
  NumSubbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(NumLevels);

  if (QccIMGImageDetermineType(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageDetermineType()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageColor(&OutputImage))
    {
      QccErrorAddMessage("%s: Output image must be grayscale",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSetSize(&OutputImage,
                         NumRows,
                         NumCols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageAlloc(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
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
          QccErrorAddMessage("%s: Number of subbands in %s does not match that specified in %s",
                             argv[0],
                             PerceptualWeights.filename,
                             InputBuffer.filename);
        }
    }

  if (QccWAVsrDecode(&InputBuffer,
                     &(OutputImage.Y),
                     &Quantizer,
                     NumLevels,
                     &Wavelet,
                     ((UsePerceptualWeights) ? &PerceptualWeights :
                      NULL),
                     ImageMean))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsrDecode()",
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

  QccWAVWaveletFree(&Wavelet);
  QccWAVPerceptualWeightsFree(&PerceptualWeights);
  QccSQScalarQuantizerFree(&Quantizer);
  QccIMGImageFree(&OutputImage);

  QccExit;
}
