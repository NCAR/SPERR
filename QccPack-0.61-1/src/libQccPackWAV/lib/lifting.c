/*
 * 
 * QccPack: Quantization, compression, and coding libraries
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 * 
 */


#include "libQccPack.h"


int QccWAVLiftingAnalysis(QccVector signal,
                          int signal_length,
                          int phase,
                          const QccWAVLiftingScheme *lifting_scheme,
                          int boundary)
{
  int return_value;
  QccVector signal2 = NULL;

  if (signal == NULL)
    return(0);
  if (lifting_scheme == NULL)
    return(0);

  if (!signal_length)
    return(0);

  if (QccWAVLiftingSchemeInteger(lifting_scheme))
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysis): Lifting scheme cannot be integer-valued");
      goto Error;
    }

  if ((signal2 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysis): Error calling QccVectorAlloc()");
      goto Error;
    }

  if (QccVectorCopy(signal2, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysis): Error calling QccVectorCopy()");
      goto Error;
    }

  switch (lifting_scheme->scheme)
    {
    case QCCWAVLIFTINGSCHEME_LWT:
      break;
    case  QCCWAVLIFTINGSCHEME_Daubechies4:
      if (QccWAVLiftingAnalysisDaubechies4(signal2,
                                           signal_length,
                                           phase,
                                           boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingAnalysis): Error calling QccWAVLiftingAnalysisDaubechies4()");
          goto Error;
        }
      break;
    case QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau9_7:
      if (QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7(signal2,
                                                          signal_length,
                                                          phase,
                                                          boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingAnalysis): Error calling QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7()");
          goto Error;
        }
      break;
    case QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau5_3:
      if (QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3(signal2,
                                                          signal_length,
                                                          phase,
                                                          boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingAnalysis): Error calling QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVLiftingAnalysis): Undefined lifting scheme");
      goto Error;
    }

  if (QccWAVWaveletLWT(signal2, signal, signal_length, 0, phase))
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysis): Error calling QccWAVWaveletLWT()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(signal2);
  return(return_value);
}


int QccWAVLiftingSynthesis(QccVector signal,
                           int signal_length,
                           int phase,
                           const QccWAVLiftingScheme *lifting_scheme,
                           int boundary)
{
  int return_value;
  QccVector signal2 = NULL;

  if (signal == NULL)
    return(0);
  if (lifting_scheme == NULL)
    return(0);

  if (!signal_length)
    return(0);

  if (QccWAVLiftingSchemeInteger(lifting_scheme))
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesis): Lifting scheme cannot be integer-valued");
      goto Error;
    }

  if ((signal2 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesis): Error calling QccVectorAlloc()");
      goto Error;
    }

  if (QccVectorCopy(signal2, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesis): Error calling QccVectorCopy()");
      goto Error;
    }

  if (QccWAVWaveletInverseLWT(signal2, signal,
                              signal_length, 0, phase))
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesis): Error calling QccWAVWaveletInverseLWT()");
      goto Error;
    }

  switch (lifting_scheme->scheme)
    {
    case QCCWAVLIFTINGSCHEME_LWT:
      break;
    case  QCCWAVLIFTINGSCHEME_Daubechies4:
      if (QccWAVLiftingSynthesisDaubechies4(signal,
                                            signal_length,
                                            phase,
                                            boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingSynthesis): Error calling QccWAVLiftingSynthesisDaubechies4()");
          goto Error;
        }
      break;
    case QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau9_7:
      if (QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7(signal,
                                                           signal_length,
                                                           phase,
                                                           boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingSynthesis): Error calling QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7()");
          goto Error;
        }
      break;
    case QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau5_3:
      if (QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3(signal,
                                                           signal_length,
                                                           phase,
                                                           boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingSynthesis): Error calling QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVLiftingSynthesis): Undefined lifting scheme");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(signal2);
  return(return_value);
}


int QccWAVLiftingRedundantAnalysis(const QccVector input_signal,
                                   QccVector output_signal_low,
                                   QccVector output_signal_high,
                                   int signal_length,
                                   const QccWAVLiftingScheme *lifting_scheme,
                                   int boundary)
{
  int return_value;
  QccVector temp1 = NULL;
  QccVector temp2 = NULL;
  QccVector temp3 = NULL;
  int length1, length2;

  if ((temp1 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((temp2 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((temp3 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccVectorAlloc()");
      goto Error;
    }

  if (QccVectorCopy(temp1, input_signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccVectorCopy()");
      goto Error;
    }
  if (QccVectorCopy(temp2, input_signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccVectorCopy()");
      goto Error;
    }

  if (QccWAVLiftingAnalysis(temp1,
                            signal_length,
                            0,
                            lifting_scheme,
                            boundary))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccWAVLiftingAnalysis()");
      goto Error;
    }
  if (QccWAVLiftingAnalysis(temp2,
                            signal_length,
                            1,
                            lifting_scheme,
                            boundary))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccWAVLiftingAnalysis()");
      goto Error;
    }

  if (QccVectorCopy(temp3, temp1, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccVectorCopy()");
      goto Error;
    }

  length1 = QccWAVWaveletDWTSubbandLength(signal_length, 1, 0, 0, 0);
  length2 = QccWAVWaveletDWTSubbandLength(signal_length, 1, 1, 0, 0);

  if (QccVectorCopy(&temp1[length1], temp2, length2))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccVectorCopy()");
      goto Error;
    }
  if (QccVectorCopy(temp2, &temp3[length1], length2))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccVectorCopy()");
      goto Error;
    }

  if (QccWAVWaveletInverseLWT(temp1, output_signal_low, signal_length, 0, 0))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccWAVWaveletInverseLWT()");
      goto Error;
    }
  if (QccWAVWaveletInverseLWT(temp2, output_signal_high, signal_length, 0, 1))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantAnalysis): Error calling QccWAVWaveletInverseLWT()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp1);
  QccVectorFree(temp2);
  QccVectorFree(temp3);
  return(return_value);
}


int QccWAVLiftingRedundantSynthesis(const QccVector input_signal_low,
                                    const QccVector input_signal_high,
                                    QccVector output_signal,
                                    int signal_length,
                                    const QccWAVLiftingScheme *lifting_scheme,
                                    int boundary)
{
  int return_value;
  QccVector temp1 = NULL;
  QccVector temp2 = NULL;
  QccVector temp3 = NULL;
  int length1, length2;

  if ((temp1 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((temp2 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((temp3 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorAlloc()");
      goto Error;
    }

  if (QccWAVWaveletLWT(input_signal_low, temp1, signal_length, 0, 0))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccWAVWaveletInverseLWT()");
      goto Error;
    }
  if (QccWAVWaveletLWT(input_signal_high, temp2, signal_length, 0, 1))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccWAVWaveletInverseLWT()");
      goto Error;
    }

  length1 = QccWAVWaveletDWTSubbandLength(signal_length, 1, 0, 0, 0);
  length2 = QccWAVWaveletDWTSubbandLength(signal_length, 1, 1, 0, 0);

  if (QccVectorCopy(temp3, temp1, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorCopy()");
      goto Error;
    }

  if (QccVectorCopy(&temp1[length1], temp2, length2))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorCopy()");
      goto Error;
    }
  if (QccVectorCopy(temp2, &temp3[length1], length2))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorCopy()");
      goto Error;
    }

  if (QccVectorCopy(output_signal, temp1, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorCopy()");
      goto Error;
    }

  if (QccWAVLiftingSynthesis(output_signal,
                             signal_length,
                             0,
                             lifting_scheme,
                             boundary))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccWAVLiftingSynthesis()");
      goto Error;
    }
  if (QccWAVLiftingSynthesis(temp2,
                             signal_length,
                             1,
                             lifting_scheme,
                             boundary))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccWAVLiftingSynthesis()");
      goto Error;
    }

  if (QccVectorAdd(output_signal,
                   temp2,
                   signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorAdd()");
      goto Error;
    }

  if (QccVectorScalarMult(output_signal,
                          0.5,
                          signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingRedundantSynthesis): Error calling QccVectorScalarMult()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp1);
  QccVectorFree(temp2);
  QccVectorFree(temp3);
  return(return_value);
}

