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

#include "sfqencode.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-l %f:lambda] [-bbq %: %f:baseband_quantizer_stepsize] [-hpq %: %f:highpass_quantizer_stepsize] [-pw %: %s:pcpfile] [-nl %d:num_levels] [-vo %:] %s:imgfile %s:bitstream"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

QccWAVPerceptualWeights PerceptualWeights;
int UsePerceptualWeights = 0;

float Lambda = 1.0;
int NumLevels = 3;
int NumSubbands;

int BasebandQuantizerStepSizeSpecified = 0;
int HighpassQuantizerStepSizeSpecified = 0;
float InputBasebandQuantizerStepSize = -1.0;
float InputHighpassQuantizerStepSize = -1.0;
double BasebandQuantizerStepSize;
double HighpassQuantizerStepSize;

QccIMGImage InputImage;
int ImageNumRows, ImageNumCols;

QccSQScalarQuantizer MeanQuantizer;
QccSQScalarQuantizer BasebandQuantizer;
QccSQScalarQuantizer HighpassQuantizer;

QccBitBuffer OutputBuffer;

QccWAVZerotree Zerotree;
QccChannel *Channels;

int ValueOnly = 0;

/*  Tree prediction is not working -- do not use!!  */
int NoTreePrediction = 1;
int *HighTreePredictionThresholds = NULL;
int *LowTreePredictionThresholds = NULL;

QccWAVSubbandPyramid SubbandPyramid;

QccIMGImageComponent ReconstructedBaseband;

int main(int argc, char *argv[])
{
  int subband;
  double rate_header, rate_zerotree, rate_channels;
  double rate;

  QccInit(argc, argv);
  
  QccWAVWaveletInitialize(&Wavelet);
  QccWAVPerceptualWeightsInitialize(&PerceptualWeights);
  QccIMGImageInitialize(&InputImage);
  QccSQScalarQuantizerInitialize(&MeanQuantizer);
  QccSQScalarQuantizerInitialize(&BasebandQuantizer);
  QccSQScalarQuantizerInitialize(&HighpassQuantizer);
  QccWAVZerotreeInitialize(&Zerotree);
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);
  QccIMGImageComponentInitialize(&ReconstructedBaseband);
  QccBitBufferInitialize(&OutputBuffer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &Lambda,
                         &BasebandQuantizerStepSizeSpecified,
                         &InputBasebandQuantizerStepSize,
                         &HighpassQuantizerStepSizeSpecified,
                         &InputHighpassQuantizerStepSize,
                         &UsePerceptualWeights,
                         PerceptualWeights.filename,
                         &NumLevels,
                         &ValueOnly,
                         InputImage.filename,
                         OutputBuffer.filename))
    QccErrorExit();
  
  if (Lambda < 0)
    {
      QccErrorAddMessage("%s: Lambda must be nonnegative",
                         argv[0]);
      QccErrorExit();
    }
  
  if (NumLevels < 0)
    {
      QccErrorAddMessage("%s: Number of levels of decomposition must be nonnegative",
                         argv[0]);
      QccErrorExit();
    }
  NumSubbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(NumLevels);
  
  if (BasebandQuantizerStepSizeSpecified)
    if (InputBasebandQuantizerStepSize <= 0)
      {
        QccErrorAddMessage("%s: Baseband-quantizer stepsize must be greater than 0",
                           argv[0]);
        QccErrorExit();
      }
  BasebandQuantizerStepSize = InputBasebandQuantizerStepSize;

  if (HighpassQuantizerStepSizeSpecified)
    if (InputHighpassQuantizerStepSize <= 0)
      {
        QccErrorAddMessage("%s: Highpass-quantizer stepsize must be greater than 0",
                           argv[0]);
        QccErrorExit();
      }
  HighpassQuantizerStepSize = InputHighpassQuantizerStepSize;

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

  MeanQuantizer.type = QCCSQSCALARQUANTIZER_UNIFORM;
  MeanQuantizer.stepsize = QCCWAVSFQ_MEANQUANTIZER_STEPSIZE;
  
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
  
  if ((Channels = 
       (QccChannel *)malloc(sizeof(QccChannel)*NumSubbands)) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory",
                         argv[0]);
      QccErrorExit();
    }
  
  Zerotree.image_num_cols = ImageNumCols;
  Zerotree.image_num_rows = ImageNumRows;
  Zerotree.num_levels = NumLevels;
  Zerotree.num_subbands = NumSubbands;
  
  if (QccWAVZerotreeMakeFullTree(&Zerotree))
    {
      QccErrorAddMessage("%s: Error calling QccWAVZerotreeMakeFullTree()",
                         argv[0]);
      QccErrorExit();
    }
  
  for (subband = 0; subband < NumSubbands; subband++)
    {
      QccChannelInitialize(&Channels[subband]);
      Channels[subband].channel_length = 
        Zerotree.num_rows[subband] * Zerotree.num_cols[subband];
      Channels[subband].access_block_size = 
        QCCCHANNEL_ACCESSWHOLEFILE;
      if (QccChannelAlloc(&Channels[subband]))
        {
          QccErrorAddMessage("%s: Error calling QccChannelAlloc()",
                             argv[0]);
          QccErrorExit();
        }
    }
      
  BasebandQuantizer.type = QCCSQSCALARQUANTIZER_UNIFORM;
  BasebandQuantizer.stepsize = BasebandQuantizerStepSize;

  HighpassQuantizer.type = QCCSQSCALARQUANTIZER_UNIFORM;
  HighpassQuantizer.stepsize = HighpassQuantizerStepSize;


  if (!NoTreePrediction)
    {
      if ((HighTreePredictionThresholds =
           (int *)malloc(sizeof(int) *
                         NumSubbands)) == NULL)
        {
          QccErrorAddMessage("%s: Error allocating memory",
                             argv[0]);
          QccErrorExit();
        }
      if ((LowTreePredictionThresholds =
           (int *)malloc(sizeof(int) *
                         NumSubbands)) == NULL)
        {
          QccErrorAddMessage("%s: Error allocating memory",
                             argv[0]);
          QccErrorExit();
        }
    }

  SubbandPyramid.num_levels = 0;
  SubbandPyramid.num_rows = InputImage.Y.num_rows;
  SubbandPyramid.num_cols = InputImage.Y.num_cols;
  if (QccWAVSubbandPyramidAlloc(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidAlloc()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccWAVsfqWaveletAnalysis(&(InputImage.Y),
                               &SubbandPyramid,
                               &Zerotree,
                               &MeanQuantizer,
                               &Wavelet,
                               ((UsePerceptualWeights) ? &PerceptualWeights :
                                NULL),
                               (double)Lambda))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsfqWaveletAnalysis()",
                         argv[0]);
      QccErrorExit();
    }

  ReconstructedBaseband.num_rows = Zerotree.num_rows[0];
  ReconstructedBaseband.num_cols = Zerotree.num_cols[0];
  if (QccIMGImageComponentAlloc(&ReconstructedBaseband))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccWAVsfqBasebandEncode(&SubbandPyramid,
                              &Zerotree,
                              &BasebandQuantizer,
                              Channels,
                              (double)Lambda,
                              &ReconstructedBaseband))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsfqBasebandEncode()",
                         argv[0]);
      QccErrorExit();
    }
  if (!ValueOnly)
    printf("Baseband quantizer: chosen stepsize = %f\n",
           BasebandQuantizer.stepsize);

  if (QccWAVsfqHighpassEncode(&SubbandPyramid,
                              &Zerotree,
                              &HighpassQuantizer,
                              Channels,
                              (double)Lambda,
                              HighTreePredictionThresholds,
                              LowTreePredictionThresholds,
                              &ReconstructedBaseband))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsfqHighpassEncode()",
                         argv[0]);
      QccErrorExit();
    }

  if (!ValueOnly)
    printf("Highpass quantizer: chosen stepsize = %f\n",
           HighpassQuantizer.stepsize);

  OutputBuffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccWAVsfqAssembleBitstream(&Zerotree,
                                 &MeanQuantizer,
                                 &BasebandQuantizer,
                                 &HighpassQuantizer,
                                 Channels,
                                 HighTreePredictionThresholds,
                                 LowTreePredictionThresholds,
                                 &OutputBuffer,
                                 &rate_header,
                                 &rate_zerotree,
                                 &rate_channels))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsfqAssembleBitstream()",
                         argv[0]);
      QccErrorExit();
    }
      
  if (QccBitBufferEnd(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()",
                         argv[0]);
      QccErrorExit();
    }

  rate = rate_header + rate_zerotree + rate_channels;

  if (ValueOnly)
    printf("%f\n", rate);
  else
    {
      printf("SFQ coding of %s:\n",
             InputImage.filename);
      printf("  Total rate: %f bpp\n", rate);
      printf("      Header: %6.4f bpp (% 6.2f%% of total rate)\n",
             rate_header, QccMathPercent(rate_header, rate));
      printf("    Zerotree: %6.4f bpp (% 6.2f%% of total rate)\n",
             rate_zerotree, QccMathPercent(rate_zerotree, rate));
      printf("    Channels: %6.4f bpp (% 6.2f%% of total rate)\n",
             rate_channels, QccMathPercent(rate_channels, rate));
    }
  
  QccWAVWaveletFree(&Wavelet);
  QccWAVPerceptualWeightsFree(&PerceptualWeights);
  QccIMGImageFree(&InputImage);
  QccSQScalarQuantizerFree(&MeanQuantizer);
  QccSQScalarQuantizerFree(&BasebandQuantizer);
  QccSQScalarQuantizerFree(&HighpassQuantizer);
  QccWAVZerotreeFree(&Zerotree);
  QccWAVSubbandPyramidFree(&SubbandPyramid);
  QccIMGImageComponentFree(&ReconstructedBaseband);

  QccExit;
}
