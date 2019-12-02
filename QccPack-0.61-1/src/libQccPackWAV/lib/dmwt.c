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


int QccWAVMultiwaveletDMWT1D(const QccDataset *input_signal,
                             QccDataset *output_signal,
                             int num_scales,
                             const QccWAVMultiwavelet *multiwavelet)
{
  int return_value;
  int scale;
  int signal_length;
  int vector_dimension;
  int minimum_length;
  QccDataset temp_signal;
  int current_length;
  
  QccDatasetInitialize(&temp_signal);
  
  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (multiwavelet == NULL)
    return(0);
  
  if (num_scales < 0)
    return(0);
  if (num_scales == 0)
    {
      QccDatasetCopy(output_signal, input_signal);
      return(0);
    }
  
  signal_length = input_signal->num_vectors;
  vector_dimension = input_signal->vector_dimension;
  
  if ((input_signal->access_block_size != QCCDATASET_ACCESSWHOLEFILE) ||
      (output_signal->access_block_size != QCCDATASET_ACCESSWHOLEFILE))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletDMWT1D): Input and output signals must be whole-file accesses");
      goto Error;
    }
  if ((output_signal->num_vectors != signal_length) ||
      (output_signal->vector_dimension != vector_dimension))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletDMWT1D): Output signal must be same size as input signal");
      goto Error;
    }
  
  minimum_length = 1 << num_scales;
  
  if (signal_length < minimum_length)
    {
      QccErrorAddMessage("(QccWAVMultiwaveletDMWT1D): Input sequence is not long enough for %d multiwavelet decompositions",
                         num_scales);
      goto Error;
    }
  
  if (signal_length % minimum_length)
    {
      QccErrorAddMessage("(QccWAVMultiwaveletDMWT1D): Input sequence does not have a length which is a multiple of %d, i.e., 2^num_scales",
                         minimum_length);
      goto Error;
    }

  temp_signal.num_vectors = signal_length;
  temp_signal.vector_dimension = vector_dimension;
  temp_signal.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  if (QccDatasetAlloc(&temp_signal))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletDMWT1D): Error calling QccDatasetAlloc()");
      goto Error;
    }

  if (QccDatasetCopy(&temp_signal, input_signal))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletDMWT1D): Error calling QccDatasetCopy()");
      goto Error;
    }
    
  current_length = signal_length;

  for (scale = 0; scale < num_scales; scale++)
    {
      if (QccWAVVectorFilterBankAnalysis(&temp_signal,
                                         output_signal,
                                         &multiwavelet->vector_filter_bank))
        {
          QccErrorAddMessage("(QccWAVMultiwaveletDMWT1D): Error calling QccWAVVectorFilterBankAnalysis()");
          goto Error;
        }

      current_length =
        QccWAVWaveletDWTSubbandLength(signal_length, scale + 1, 0, 0, 0);

      temp_signal.num_vectors = output_signal->num_vectors = current_length;

      if (QccDatasetCopy(&temp_signal, output_signal))
        {
          QccErrorAddMessage("(QccWAVMultiwaveletDMWT1D): Error calling QccDatasetCopy()");
          goto Error;
        }
    }

  temp_signal.num_vectors = output_signal->num_vectors = signal_length;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccDatasetFree(&temp_signal);
  return(return_value);
}


int QccWAVMultiwaveletInverseDMWT1D(const QccDataset *input_signal,
                                    QccDataset *output_signal,
                                    int num_scales,
                                    const QccWAVMultiwavelet *multiwavelet)
{
  int return_value;
  int scale;
  int minimum_length;
  int signal_length;
  int vector_dimension;
  QccDataset temp_signal;
  int current_length;

  QccDatasetInitialize(&temp_signal);

  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (multiwavelet == NULL)
    return(0);

  if (num_scales < 0)
    return(0);
  if (num_scales == 0)
    {
      QccDatasetCopy(output_signal, input_signal);
      return(0);
    }

  signal_length = input_signal->num_vectors;
  vector_dimension = input_signal->vector_dimension;
  
  if ((input_signal->access_block_size != QCCDATASET_ACCESSWHOLEFILE) ||
      (output_signal->access_block_size != QCCDATASET_ACCESSWHOLEFILE))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletInverseDMWT1D): Input and output signals must be whole-file accesses");
      goto Error;
    }
  if ((output_signal->num_vectors != signal_length) ||
      (output_signal->vector_dimension != vector_dimension))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletInverseDMWT1D): Output signal must be same size as input signal");
      goto Error;
    }
  
  minimum_length = 1 << num_scales;

  if (signal_length < minimum_length)
    {
      QccErrorAddMessage("(QccWAVMultiwaveletInverseDMWT1D): Input sequence is not long enough for %d multiwavelet reconstructions",
                         num_scales);
      goto Error;
    }

  if (signal_length % minimum_length)
    {
      QccErrorAddMessage("(QccWAVMultiwaveletInverseDMWT1D): Input sequence does not have a length which is a multiple of %d, i.e., 2^num_scales",
                         minimum_length);
      goto Error;
    }

  temp_signal.num_vectors = signal_length;
  temp_signal.vector_dimension = vector_dimension;
  temp_signal.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  if (QccDatasetAlloc(&temp_signal))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletInverseDMWT1D): Error calling QccDatasetAlloc()");
      goto Error;
    }

  if (QccDatasetCopy(&temp_signal, input_signal))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletInverseDMWT1D): Error calling QccDatasetCopy()");
      goto Error;
    }
    
  for (scale = num_scales - 1; scale >= 0; scale--)
    {
      current_length = 
        QccWAVWaveletDWTSubbandLength(signal_length, scale, 0, 0, 0);

      temp_signal.num_vectors = output_signal->num_vectors = current_length;

      if (QccWAVVectorFilterBankSynthesis(&temp_signal,
                                          output_signal,
                                          &multiwavelet->vector_filter_bank))
        {
          QccErrorAddMessage("(QccWAVMultiwaveletInverseDMWT1D): Error calling QccWAVVectorFilterBankSynthesis()");
          goto Error;
        }

      if (QccDatasetCopy(&temp_signal, output_signal))
        {
          QccErrorAddMessage("(QccWAVMultiwaveletInverseDMWT1D): Error calling QccDatasetCopy()");
          goto Error;
        }
    }

  temp_signal.num_vectors = output_signal->num_vectors = signal_length;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccDatasetFree(&temp_signal);
  return(return_value);
}
