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


#include "sfqdecode.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-pw %: %s:pcpfile] %s:bitstream %s:imgfile"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

QccWAVPerceptualWeights PerceptualWeights;
int UsePerceptualWeights = 0;

int NumLevels;
int NumSubbands;

QccSQScalarQuantizer MeanQuantizer;
QccSQScalarQuantizer BasebandQuantizer;
QccSQScalarQuantizer HighpassQuantizer;

QccBitBuffer InputBuffer;

QccWAVZerotree Zerotree;
QccChannel *Channels;

QccIMGImage OutputImage;

/*  Tree prediction is not working -- do not use!!  */
int NoTreePrediction = 1;
int *HighTreePredictionThresholds = NULL;
int *LowTreePredictionThresholds = NULL;

int main(int argc, char *argv[])
{
  int subband;
  
  QccInit(argc, argv);
  
  QccWAVWaveletInitialize(&Wavelet);
  QccWAVPerceptualWeightsInitialize(&PerceptualWeights);
  QccSQScalarQuantizerInitialize(&MeanQuantizer);
  QccSQScalarQuantizerInitialize(&BasebandQuantizer);
  QccSQScalarQuantizerInitialize(&HighpassQuantizer);
  QccWAVZerotreeInitialize(&Zerotree);
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
  
  MeanQuantizer.type = QCCSQSCALARQUANTIZER_UNIFORM;
  MeanQuantizer.stepsize = QCCWAVSFQ_MEANQUANTIZER_STEPSIZE;
  
  InputBuffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
  
  BasebandQuantizer.type = QCCSQSCALARQUANTIZER_UNIFORM;
  HighpassQuantizer.type = QCCSQSCALARQUANTIZER_UNIFORM;

  if (QccWAVsfqDisassembleBitstreamHeader(&Zerotree,
                                          &MeanQuantizer,
                                          &BasebandQuantizer,
                                          &HighpassQuantizer,
                                          &NoTreePrediction,
                                          &InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsfqDisassembleBitstreamHeader()",
                         argv[0]);
      QccErrorExit();
    }

  NumLevels = Zerotree.num_levels;
  NumSubbands = Zerotree.num_subbands;
  if (QccWAVZerotreeMakeFullTree(&Zerotree))
    {
      QccErrorAddMessage("%s: Error calling QccWAVZerotreeMakeFullTree()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (!NoTreePrediction)
    {
      if ((HighTreePredictionThresholds =
           (int *)malloc(sizeof(int) * NumSubbands)) == NULL)
        {
          QccErrorAddMessage("%s: Error allocating memory",
                             argv[0]);
          QccErrorExit();
        }
      if ((LowTreePredictionThresholds =
           (int *)malloc(sizeof(int) * NumSubbands)) == NULL)
        {
          QccErrorAddMessage("%s: Error allocating memory",
                             argv[0]);
          QccErrorExit();
        }
    }

  if ((Channels = 
       (QccChannel *)malloc(sizeof(QccChannel)*NumSubbands)) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory",
                         argv[0]);
      QccErrorExit();
    }
  
  for (subband = 0; subband < NumSubbands; subband++)
    {
      QccChannelInitialize(&Channels[subband]);
      Channels[subband].channel_length = 0;
      Channels[subband].access_block_size = 
        QCCCHANNEL_ACCESSWHOLEFILE;
      Channels[subband].alphabet_size = (subband) ?
        HighpassQuantizer.num_levels : BasebandQuantizer.num_levels;
    }
  
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
                         Zerotree.image_num_rows,
                         Zerotree.image_num_cols))
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

  if (QccWAVsfqDecode(&InputBuffer,
                      &(OutputImage.Y),
                      &Zerotree,
                      &BasebandQuantizer,
                      &HighpassQuantizer,
                      HighTreePredictionThresholds,
                      LowTreePredictionThresholds,
                      Channels,
                      &Wavelet,
                      ((UsePerceptualWeights) ? &PerceptualWeights : NULL)))
    {
      QccErrorAddMessage("%s: Error calling QccWAVsfqDecode()",
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
  QccSQScalarQuantizerFree(&MeanQuantizer);
  QccSQScalarQuantizerFree(&BasebandQuantizer);
  QccSQScalarQuantizerFree(&HighpassQuantizer);
  QccWAVZerotreeFree(&Zerotree);
  QccIMGImageFree(&OutputImage);

  QccExit;
}
