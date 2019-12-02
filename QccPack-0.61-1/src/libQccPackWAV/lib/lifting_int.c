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


int QccWAVLiftingAnalysisInt(QccVectorInt signal,
                             int signal_length,
                             int phase,
                             const QccWAVLiftingScheme *lifting_scheme,
                             int boundary)
{
  int return_value;
  QccVectorInt signal2 = NULL;

  if (signal == NULL)
    return(0);
  if (lifting_scheme == NULL)
    return(0);

  if (!signal_length)
    return(0);

  if (!QccWAVLiftingSchemeInteger(lifting_scheme))
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysisInt): Lifting scheme must be integer-valued");
      goto Error;
    }

  if ((signal2 = QccVectorIntAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysisInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }

  if (QccVectorIntCopy(signal2, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysisInt): Error calling QccVectorIntCopy()");
      goto Error;
    }

  switch (lifting_scheme->scheme)
    {
    case QCCWAVLIFTINGSCHEME_IntLWT:
      break;
    case QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau9_7:
      if (QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7(signal2,
                                                             signal_length,
                                                             phase,
                                                             boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingAnalysisInt): Error calling QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7()");
          goto Error;
        }
      break;
    case QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau5_3:
      if (QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3(signal2,
                                                             signal_length,
                                                             phase,
                                                             boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingAnalysisInt): Error calling QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVLiftingAnalysisInt): Undefined lifting scheme");
      goto Error;
    }

  if (QccWAVWaveletLWTInt(signal2, signal, signal_length, 0, phase))
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysisInt): Error calling QccWAVWaveletLWTInt()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(signal2);
  return(return_value);
}


int QccWAVLiftingSynthesisInt(QccVectorInt signal,
                              int signal_length,
                              int phase,
                              const QccWAVLiftingScheme *lifting_scheme,
                              int boundary)
{
  int return_value;
  QccVectorInt signal2 = NULL;

  if (signal == NULL)
    return(0);
  if (lifting_scheme == NULL)
    return(0);

  if (!signal_length)
    return(0);

  if (!QccWAVLiftingSchemeInteger(lifting_scheme))
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesisInt): Lifting scheme must be integer-valued");
      goto Error;
    }

  if ((signal2 = QccVectorIntAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesisInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }

  if (QccVectorIntCopy(signal2, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesisInt): Error calling QccVectorIntCopy()");
      goto Error;
    }

  if (QccWAVWaveletInverseLWTInt(signal2, signal,
                                 signal_length, 0, phase))
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesisInt): Error calling QccWAVWaveletInverseLWTInt()");
      goto Error;
    }

  switch (lifting_scheme->scheme)
    {
    case QCCWAVLIFTINGSCHEME_IntLWT:
      break;
    case QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau9_7:
      if (QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7(signal,
                                                              signal_length,
                                                              phase,
                                                              boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingSynthesisInt): Error calling QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7()");
          goto Error;
        }
      break;
    case QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau5_3:
      if (QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3(signal,
                                                              signal_length,
                                                              phase,
                                                              boundary))
        {
          QccErrorAddMessage("(QccWAVLiftingSynthesisInt): Error calling QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVLiftingSynthesisInt): Undefined lifting scheme");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(signal2);
  return(return_value);
}
