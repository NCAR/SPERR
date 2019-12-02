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


static int QccWAVWaveletShapeAdaptiveAnalysis1DInt(const QccVectorInt input_signal,
                                                   QccVectorInt output_signal,
                                                   QccVectorInt subsequence,
                                                   const QccVectorInt input_mask,
                                                   QccVectorInt output_mask,
                                                   int signal_length,
                                                   const QccWAVWavelet *wavelet)
{
  int return_value = 0;
  int index1, index2;
  int subsequence_position;
  int subsequence_length;
  int lowband_position;
  int lowband_length;
  int highband_position;
  int highband_length;
  int midpoint;
  int phase;
  
  /*  LWT transform mask */
  if (QccWAVWaveletLWTInt(input_mask, output_mask,
                          signal_length, 0, 0))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveAnalysis1DInt): Error calling QccWAVWaveletLWTInt()");
      goto Error;
    }
  
  QccVectorIntZero(output_signal, signal_length);
  QccVectorIntZero(subsequence, signal_length);
  
  /* Midpoint separates lowpass and highpass subbands in output */
  midpoint = 
    QccWAVWaveletDWTSubbandLength(signal_length, 1, 0, 0, 0);
  
  for (index1 = 0; index1 < signal_length; index1++)
    {
      /* Find start of subsequence of contiguous object */
      if (!QccAlphaTransparent(input_mask[index1]))
        {
          /* Find end of subsequence of contiguous object */
          for (index2 = index1; index2 < signal_length; index2++)
            if (QccAlphaTransparent(input_mask[index2]))
              break;
          
          subsequence_position = index1;
          subsequence_length = index2 - index1;
          phase =
            (subsequence_position % 2) ? QCCWAVWAVELET_PHASE_ODD :
            QCCWAVWAVELET_PHASE_EVEN;
          
          if (QccVectorIntCopy(subsequence,
                               &input_signal[subsequence_position],
                               subsequence_length))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveAnalysis1DInt): Error calling QccVectorIntCopy()");
              goto Error;
            }

          /* Do transform of current subsequence */
          if (QccWAVWaveletAnalysis1DInt(subsequence,
                                         subsequence_length,
                                         phase,
                                         wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveAnalysis1DInt): Error calling QccWAVWaveletAnalysis1DInt()");
              goto Error;
            }
          
          /* Find lowpass subband position */
          lowband_position =
            (phase == QCCWAVWAVELET_PHASE_ODD) ?
            ((subsequence_position + 1) >> 1) :
            (subsequence_position >> 1);
          lowband_length =
            QccWAVWaveletDWTSubbandLength(subsequence_length, 1, 0, 0,
                                          phase);
          
          /* Find highpass subband position */
          highband_position = midpoint +
            ((phase == QCCWAVWAVELET_PHASE_ODD) ?
             (subsequence_position >> 1) :
             (subsequence_position + 1) >> 1);
          highband_length =
            QccWAVWaveletDWTSubbandLength(subsequence_length, 1, 1, 0,
                                          phase);
          
          /* Place subbands into output array */
          QccVectorIntCopy(&output_signal[lowband_position],
                           subsequence,
                           lowband_length);
          QccVectorIntCopy(&output_signal[highband_position],
                           &subsequence[lowband_length],
                           highband_length);
          
          index1 = index2;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVWaveletShapeAdaptiveSynthesis1DInt(const QccVectorInt input_signal,
                                                    QccVectorInt output_signal,
                                                    QccVectorInt subsequence,
                                                    const QccVectorInt input_mask,
                                                    QccVectorInt output_mask,
                                                    int signal_length,
                                                    const QccWAVWavelet *wavelet)
{
  int return_value = 0;
  int index1, index2;
  int subsequence_position;
  int subsequence_length;
  int lowband_position;
  int lowband_length;
  int highband_position;
  int highband_length;
  int midpoint;
  int phase;
  
  /* Inverse LWT mask */
  if (QccWAVWaveletInverseLWTInt(input_mask, output_mask,
                                 signal_length, 0, 0))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveSynthesis1DInt): Error calling QccWAVWaveletInverseLWTInt()");
      goto Error;
    }
  
  QccVectorIntZero(output_signal, signal_length);
  QccVectorIntZero(subsequence, signal_length);
  
  /* Midpoint separates lowpass and highpass subbands in output */
  midpoint = 
    QccWAVWaveletDWTSubbandLength(signal_length, 1, 0, 0, 0);
  
  for (index1 = 0; index1 < signal_length; index1++)
    {
      /* Find start of subsequence of contiguous object */
      if (!QccAlphaTransparent(output_mask[index1]))
        {
          /* Find end of subsequence of contiguous object */
          for (index2 = index1; index2 < signal_length; index2++)
            if (QccAlphaTransparent(output_mask[index2]))
              break;
          
          subsequence_position = index1;
          subsequence_length = index2 - index1;
          
          phase =
            (subsequence_position % 2) ? QCCWAVWAVELET_PHASE_ODD :
            QCCWAVWAVELET_PHASE_EVEN;
          
          /* Find lowpass subband position */
          lowband_position =
            (phase == QCCWAVWAVELET_PHASE_ODD) ?
            ((subsequence_position + 1) >> 1) :
            (subsequence_position >> 1);
          lowband_length =
            QccWAVWaveletDWTSubbandLength(subsequence_length, 1, 0, 0,
                                          phase);
          
          /* Find highpass subband position */
          highband_position = midpoint +
            ((phase == QCCWAVWAVELET_PHASE_ODD) ?
             (subsequence_position >> 1) :
             (subsequence_position + 1) >> 1);
          highband_length =
            QccWAVWaveletDWTSubbandLength(subsequence_length, 1, 1, 0,
                                          phase);
          
          /* Extract subbands */
          QccVectorIntCopy(subsequence,
                           &input_signal[lowband_position],
                           lowband_length);
          QccVectorIntCopy(&subsequence[lowband_length],
                           &input_signal[highband_position],
                           highband_length);
          
          /* Inverse transform */
          if (QccWAVWaveletSynthesis1DInt(subsequence,
                                          subsequence_length,
                                          phase,
                                          wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveSynthesis1DInt): Error calling QccWAVWaveletSynthesis1DInt()");
              goto Error;
            }
          
          if (QccVectorIntCopy(&output_signal[subsequence_position],
                               subsequence,
                               subsequence_length))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveSynthesis1DInt): Error calling QccVectorIntCopy()");
              goto Error;
            }

          index1 = index2;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVWaveletShapeAdaptiveDWT1DInt(QccVectorInt signal,
                                       QccVectorInt mask,
                                       int signal_length,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet)
{
  
  int return_value;
  int scale;
  QccVectorInt temp_signal = NULL;
  QccVectorInt temp_mask = NULL;
  QccVectorInt subsequence = NULL;
  int current_length;
  
  if (signal == NULL)
    return(0);
  if (mask == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (num_scales <= 0)
    return(0);
  
  if ((temp_signal = QccVectorIntAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((temp_mask = QccVectorIntAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((subsequence = QccVectorIntAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  if (QccVectorIntCopy(temp_signal, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1DInt): Error calling QccVectorIntCopy()");
      goto Error;
    }
  if (QccVectorIntCopy(temp_mask, mask, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1DInt): Error calling QccVectorIntCopy()");
      goto Error;
    }
  
  current_length = signal_length;
  
  for (scale = 0; scale < num_scales; scale++)
    {
      if (QccWAVWaveletShapeAdaptiveAnalysis1DInt(temp_signal,
                                                  signal,
                                                  subsequence,
                                                  temp_mask,
                                                  mask,
                                                  current_length,
                                                  wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1DInt): Error calling QccWAVWaveletShapeAdaptiveAnalysis1DInt()");
          goto Error;
        }
      
      current_length =
        QccWAVWaveletDWTSubbandLength(signal_length, scale + 1, 0, 0, 0);
      
      if (QccVectorIntCopy(temp_signal, signal, current_length))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1DInt): Error calling QccVectorIntCopy()");
          goto Error;
        }
      if (QccVectorIntCopy(temp_mask, mask, current_length))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1DInt): Error calling QccVectorIntCopy()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(temp_signal);
  QccVectorIntFree(temp_mask);
  QccVectorIntFree(subsequence);
  return(return_value);
}


int QccWAVWaveletInverseShapeAdaptiveDWT1DInt(QccVectorInt signal,
                                              QccVectorInt mask,
                                              int signal_length,
                                              int num_scales,
                                              const QccWAVWavelet *wavelet)
{
  int return_value;
  int scale;
  QccVectorInt temp_signal = NULL;
  QccVectorInt temp_mask = NULL;
  QccVectorInt subsequence = NULL;
  int current_length;
  
  if (signal == NULL)
    return(0);
  if (mask == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (num_scales <= 0)
    return(0);
  
  if ((temp_signal = QccVectorIntAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((temp_mask = QccVectorIntAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((subsequence = QccVectorIntAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  if (QccVectorIntCopy(temp_signal, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1DInt): Error calling QccVectorIntCopy()");
      goto Error;
    }
  if (QccVectorIntCopy(temp_mask, mask, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1DInt): Error calling QccVectorIntCopy()");
      goto Error;
    }
  
  for (scale = num_scales - 1; scale >= 0 ; scale--)
    {
      current_length = 
        QccWAVWaveletDWTSubbandLength(signal_length, scale, 0, 0, 0);
      
      if (QccWAVWaveletShapeAdaptiveSynthesis1DInt(temp_signal,
                                                   signal,
                                                   subsequence,
                                                   temp_mask,
                                                   mask,
                                                   current_length,
                                                   wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1DInt): Error calling QccWAVWaveletShapeAdaptiveSynthesis1DInt()");
          goto Error;
        }
      
      if (QccVectorIntCopy(temp_signal, signal, current_length))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1DInt): Error calling QccVectorIntCopy()");
          goto Error;
        }
      if (QccVectorIntCopy(temp_mask, mask, current_length))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1DInt): Error calling QccVectorIntCopy()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(temp_signal);
  QccVectorIntFree(temp_mask);
  QccVectorIntFree(subsequence);
  return(return_value);
  
}


int QccWAVWaveletShapeAdaptiveDWT2DInt(QccMatrixInt signal, 
                                       QccMatrixInt mask, 
                                       int num_rows,
                                       int num_cols,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVectorInt input_signal_vector = NULL;
  QccVectorInt output_signal_vector = NULL;
  QccVectorInt subsequence = NULL;
  QccVectorInt input_mask_vector = NULL;
  QccVectorInt output_mask_vector = NULL;
  int scale;
  int row, col;
  int baseband_num_rows;
  int baseband_num_cols;
  
  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2DInt): Error calling VectorAlloc()");
      goto Error;
    }
  
  for (scale = 0; scale < num_scales; scale++)
    {
      baseband_num_rows =
        QccWAVWaveletDWTSubbandLength(num_rows, scale, 0, 0, 0);
      baseband_num_cols =
        QccWAVWaveletDWTSubbandLength(num_cols, scale, 0, 0, 0);
      
      for (row = 0; row < baseband_num_rows; row++)
        {
          for (col = 0; col < baseband_num_cols; col++)
            {
              input_signal_vector[col] = signal[row][col];
              input_mask_vector[col] = mask[row][col];
            }
          
          if (QccWAVWaveletShapeAdaptiveAnalysis1DInt(input_signal_vector,
                                                      output_signal_vector,
                                                      subsequence,
                                                      input_mask_vector,
                                                      output_mask_vector,
                                                      baseband_num_cols,
                                                      wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT2DInt): Error calling QccWAVWaveletShapeAdaptiveAnalysis1DInt()");
              goto Error;
            }
          
          for (col = 0; col < baseband_num_cols; col++)
            {
              signal[row][col] = output_signal_vector[col];
              mask[row][col] = output_mask_vector[col];
            }
        }
      
      for (col = 0; col < baseband_num_cols; col++)
        {
          for (row = 0; row < baseband_num_rows; row++)
            {
              input_signal_vector[row] = signal[row][col];
              input_mask_vector[row] = mask[row][col];
            }
          
          if (QccWAVWaveletShapeAdaptiveAnalysis1DInt(input_signal_vector,
                                                      output_signal_vector,
                                                      subsequence,
                                                      input_mask_vector,
                                                      output_mask_vector,
                                                      baseband_num_rows,
                                                      wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT2DInt): Error calling QccWAVWaveletShapeAdaptiveAnalysis1DInt()");
              goto Error;
            }
          
          for (row = 0; row < baseband_num_rows; row++)
            {
              signal[row][col] = output_signal_vector[row];
              mask[row][col] = output_mask_vector[row];
            }
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_signal_vector);
  QccVectorIntFree(output_signal_vector);
  QccVectorIntFree(subsequence);
  QccVectorIntFree(input_mask_vector);
  QccVectorIntFree(output_mask_vector);
  return(return_value);
} 


int QccWAVWaveletInverseShapeAdaptiveDWT2DInt(QccMatrixInt signal, 
                                              QccMatrixInt mask, 
                                              int num_rows,
                                              int num_cols,
                                              int num_scales,
                                              const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVectorInt input_signal_vector = NULL;
  QccVectorInt output_signal_vector = NULL;
  QccVectorInt subsequence = NULL;
  QccVectorInt input_mask_vector = NULL;
  QccVectorInt output_mask_vector = NULL;
  int scale;
  int row, col;
  int baseband_num_rows;
  int baseband_num_cols;
  
  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorIntAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  for (scale = num_scales - 1; scale >= 0; scale--)
    {
      baseband_num_rows =
        QccWAVWaveletDWTSubbandLength(num_rows, scale, 0, 0, 0);
      baseband_num_cols =
        QccWAVWaveletDWTSubbandLength(num_cols, scale, 0, 0, 0);
      
      for (col = 0; col < baseband_num_cols; col++)
        {
          for (row = 0; row < baseband_num_rows; row++)
            {
              input_signal_vector[row] = signal[row][col];
              input_mask_vector[row] = mask[row][col];
            }
          
          if (QccWAVWaveletShapeAdaptiveSynthesis1DInt(input_signal_vector,
                                                       output_signal_vector,
                                                       subsequence,
                                                       input_mask_vector,
                                                       output_mask_vector,
                                                       baseband_num_rows,
                                                       wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2DInt): Error calling QccWAVWaveletShapeAdaptiveSynthesis1DInt()");
              goto Error;
            }
          
          for (row = 0; row < baseband_num_rows; row++)
            {
              signal[row][col] = output_signal_vector[row];
              mask[row][col] = output_mask_vector[row];
            }
        }
      
      for (row = 0; row < baseband_num_rows; row++)
        {
          for (col = 0; col < baseband_num_cols; col++)
            {
              input_signal_vector[col] = signal[row][col];
              input_mask_vector[col] = mask[row][col];
            }
          
          if (QccWAVWaveletShapeAdaptiveSynthesis1DInt(input_signal_vector,
                                                       output_signal_vector,
                                                       subsequence,
                                                       input_mask_vector,
                                                       output_mask_vector,
                                                       baseband_num_cols,
                                                       wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2DInt): Error calling QccWAVWaveletShapeAdaptiveSynthesis1DInt()");
              goto Error;
            }
          
          for (col = 0; col < baseband_num_cols; col++)
            {
              signal[row][col] = output_signal_vector[col];
              mask[row][col] = output_mask_vector[col];
            }
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_signal_vector);
  QccVectorIntFree(output_signal_vector);
  QccVectorIntFree(subsequence);
  QccVectorIntFree(input_mask_vector);
  QccVectorIntFree(output_mask_vector);
  return(return_value);
}


int QccWAVWaveletShapeAdaptiveDyadicDWT3DInt(QccVolumeInt signal,
                                             QccVolumeInt mask,
                                             int num_frames,
                                             int num_rows,
                                             int num_cols,
                                             int spatial_num_scales,
                                             const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVectorInt input_signal_vector = NULL;
  QccVectorInt output_signal_vector = NULL;
  QccVectorInt subsequence = NULL;
  QccVectorInt input_mask_vector = NULL;
  QccVectorInt output_mask_vector = NULL;
  int scale;
  int frame, row, col;
  int baseband_num_frames;
  int baseband_num_rows;
  int baseband_num_cols;
  int temp_length;

  temp_length = QccMathMax(num_frames, QccMathMax(num_rows, num_cols));

  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  
  for (scale = 0; scale < spatial_num_scales; scale++)
    {
      baseband_num_frames =
        QccWAVWaveletDWTSubbandLength(num_frames, scale, 0, 0, 0);
      baseband_num_rows =
        QccWAVWaveletDWTSubbandLength(num_rows, scale, 0, 0, 0);
      baseband_num_cols =
        QccWAVWaveletDWTSubbandLength(num_cols, scale, 0, 0, 0);
      
      for (frame = 0; frame < baseband_num_frames; frame++)
        for (row = 0; row < baseband_num_rows; row++)
          {
            for (col = 0; col < baseband_num_cols; col++)
              {
                input_signal_vector[col] = signal[frame][row][col];
                input_mask_vector[col] = mask[frame][row][col];
              }
            
            if (QccWAVWaveletShapeAdaptiveAnalysis1DInt(input_signal_vector,
                                                        output_signal_vector,
                                                        subsequence,
                                                        input_mask_vector,
                                                        output_mask_vector,
                                                        baseband_num_cols,
                                                        wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDyadicDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveAnalysis1DInt()");
                goto Error;
              }
            
            for (col = 0; col < baseband_num_cols; col++)
              {
                signal[frame][row][col] = output_signal_vector[col];
                mask[frame][row][col] = output_mask_vector[col];
              }
          }
      
      for (frame = 0; frame < baseband_num_frames; frame++)
        for (col = 0; col < baseband_num_cols; col++)
          {
            for (row = 0; row < baseband_num_rows; row++)
              {
                input_signal_vector[row] = signal[frame][row][col];
                input_mask_vector[row] = mask[frame][row][col];
              }
            
            if (QccWAVWaveletShapeAdaptiveAnalysis1DInt(input_signal_vector,
                                                        output_signal_vector,
                                                        subsequence,
                                                        input_mask_vector,
                                                        output_mask_vector,
                                                        baseband_num_rows,
                                                        wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDyadicDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveAnalysis1DInt()");
                goto Error;
              }
            
            for (row = 0; row < baseband_num_rows; row++)
              {
                signal[frame][row][col] = output_signal_vector[row];
                mask[frame][row][col] = output_mask_vector[row];
              }
          }
      
      for (row = 0; row < baseband_num_rows; row++)
        for (col = 0; col < baseband_num_cols; col++)
          {
            for (frame = 0; frame < baseband_num_frames; frame++)
              {
                input_signal_vector[frame] = signal[frame][row][col];
                input_mask_vector[frame] = mask[frame][row][col];
              }
            
            if (QccWAVWaveletShapeAdaptiveAnalysis1DInt(input_signal_vector,
                                                        output_signal_vector,
                                                        subsequence,
                                                        input_mask_vector,
                                                        output_mask_vector,
                                                        baseband_num_frames,
                                                        wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDyadicDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveAnalysis1DInt()");
                goto Error;
              }
            
            for (frame = 0; frame < baseband_num_frames; frame++)
              {
                signal[frame][row][col] = output_signal_vector[frame];
                mask[frame][row][col] = output_mask_vector[frame];
              }
          }
    }
      
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_signal_vector);
  QccVectorIntFree(output_signal_vector);
  QccVectorIntFree(subsequence);
  QccVectorIntFree(input_mask_vector);
  QccVectorIntFree(output_mask_vector);
  return(return_value);
}


int QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt(QccVolumeInt signal,
                                                    QccVolumeInt mask,
                                                    int num_frames,
                                                    int num_rows,
                                                    int num_cols,
                                                    int spatial_num_scales,
                                                    const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVectorInt input_signal_vector = NULL;
  QccVectorInt output_signal_vector = NULL;
  QccVectorInt subsequence = NULL;
  QccVectorInt input_mask_vector = NULL;
  QccVectorInt output_mask_vector = NULL;
  int scale;
  int frame, row, col;
  int baseband_num_frames;
  int baseband_num_rows;
  int baseband_num_cols;
  int temp_length;

  temp_length = QccMathMax(num_frames, QccMathMax(num_rows, num_cols));

  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorIntAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  for (scale = spatial_num_scales - 1; scale >= 0; scale--)
    {
      baseband_num_frames =
        QccWAVWaveletDWTSubbandLength(num_frames, scale, 0, 0, 0);
      baseband_num_rows =
        QccWAVWaveletDWTSubbandLength(num_rows, scale, 0, 0, 0);
      baseband_num_cols =
        QccWAVWaveletDWTSubbandLength(num_cols, scale, 0, 0, 0);
      
      for (row = 0; row < baseband_num_rows; row++)
        for (col = 0; col < baseband_num_cols; col++)
          {
            for (frame = 0; frame < baseband_num_frames; frame++)
              {
                input_signal_vector[frame] = signal[frame][row][col];
                input_mask_vector[frame] = mask[frame][row][col];
              }
            
            if (QccWAVWaveletShapeAdaptiveSynthesis1DInt(input_signal_vector,
                                                         output_signal_vector,
                                                         subsequence,
                                                         input_mask_vector,
                                                         output_mask_vector,
                                                         baseband_num_frames,
                                                         wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveSynthesis1DInt()");
                goto Error;
              }
            
            for (frame = 0; frame < baseband_num_frames; frame++)
              {
                signal[frame][row][col] = output_signal_vector[frame];
                mask[frame][row][col] = output_mask_vector[frame];
              }
          }
      
      for (frame = 0; frame < baseband_num_frames; frame++)
        for (col = 0; col < baseband_num_cols; col++)
          {
            for (row = 0; row < baseband_num_rows; row++)
              {
                input_signal_vector[row] = signal[frame][row][col];
                input_mask_vector[row] = mask[frame][row][col];
              }
            
            if (QccWAVWaveletShapeAdaptiveSynthesis1DInt(input_signal_vector,
                                                         output_signal_vector,
                                                         subsequence,
                                                         input_mask_vector,
                                                         output_mask_vector,
                                                         baseband_num_rows,
                                                         wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveSynthesis1DInt()");
                goto Error;
              }
            
            for (row = 0; row < baseband_num_rows; row++)
              {
                signal[frame][row][col] = output_signal_vector[row];
                mask[frame][row][col] = output_mask_vector[row];
              }
          }
      
      for (frame = 0; frame < baseband_num_frames; frame++)
        for (row = 0; row < baseband_num_rows; row++)
          {
            for (col = 0; col < baseband_num_cols; col++)
              {
                input_signal_vector[col] = signal[frame][row][col];
                input_mask_vector[col] = mask[frame][row][col];
              }
            
            if (QccWAVWaveletShapeAdaptiveSynthesis1DInt(input_signal_vector,
                                                         output_signal_vector,
                                                         subsequence,
                                                         input_mask_vector,
                                                         output_mask_vector,
                                                         baseband_num_cols,
                                                         wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveSynthesis1DInt()");
                goto Error;
              }
            
            for (col = 0; col < baseband_num_cols; col++)
              {
                signal[frame][row][col] = output_signal_vector[col];
                mask[frame][row][col] = output_mask_vector[col];
              }
          }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_signal_vector);
  QccVectorIntFree(output_signal_vector);
  QccVectorIntFree(subsequence);
  QccVectorIntFree(input_mask_vector);
  QccVectorIntFree(output_mask_vector);
  return(return_value);
}


int QccWAVWaveletShapeAdaptivePacketDWT3DInt(QccVolumeInt signal,
                                             QccVolumeInt mask,
                                             int num_frames,
                                             int num_rows,
                                             int num_cols,
                                             int temporal_num_scales,
                                             int spatial_num_scales,
                                             const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVectorInt input_signal_vector = NULL;
  QccVectorInt output_signal_vector = NULL;
  QccVectorInt subsequence = NULL;
  QccVectorInt input_mask_vector = NULL;
  QccVectorInt output_mask_vector = NULL;
  int scale;
  int frame, row, col;
  int baseband_num_frames;

  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3DInt): Error calling VectorAlloc()");
      goto Error;
    }
  
  for (scale = 0; scale < temporal_num_scales; scale++)
    {
      baseband_num_frames =
        QccWAVWaveletDWTSubbandLength(num_frames, scale, 0, 0, 0);

      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          {
            for (frame = 0; frame < baseband_num_frames; frame++)
              {
                input_signal_vector[frame] = signal[frame][row][col];
                input_mask_vector[frame] = mask[frame][row][col];
              }
            
            if (QccWAVWaveletShapeAdaptiveAnalysis1DInt(input_signal_vector,
                                                        output_signal_vector,
                                                        subsequence,
                                                        input_mask_vector,
                                                        output_mask_vector,
                                                        baseband_num_frames,
                                                        wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletShapeAdaptivePacketDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveAnalysis1DInt()");
                goto Error;
              }
            
            for (frame = 0; frame < baseband_num_frames; frame++)
              {
                signal[frame][row][col] = output_signal_vector[frame];
                mask[frame][row][col] = output_mask_vector[frame];
              }
          }
    }
      
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletShapeAdaptiveDWT2DInt(signal[frame],
                                           mask[frame],
                                           num_rows,
                                           num_cols,
                                           spatial_num_scales,
                                           wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletShapeAdaptivePacketDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveDWT2DInt()");
        goto Error;
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_signal_vector);
  QccVectorIntFree(output_signal_vector);
  QccVectorIntFree(subsequence);
  QccVectorIntFree(input_mask_vector);
  QccVectorIntFree(output_mask_vector);
  return(return_value);
}


int QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt(QccVolumeInt signal,
                                                    QccVolumeInt mask,
                                                    int num_frames,
                                                    int num_rows,
                                                    int num_cols,
                                                    int temporal_num_scales,
                                                    int spatial_num_scales,
                                                    const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVectorInt input_signal_vector = NULL;
  QccVectorInt output_signal_vector = NULL;
  QccVectorInt subsequence = NULL;
  QccVectorInt input_mask_vector = NULL;
  QccVectorInt output_mask_vector = NULL;
  int scale;
  int frame, row, col;
  int baseband_num_frames;

  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletInverseShapeAdaptiveDWT2DInt(signal[frame],
                                                  mask[frame],
                                                  num_rows,
                                                  num_cols,
                                                  spatial_num_scales,
                                                  wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt): Error calling QccWAVWaveletInverseShapeAdaptiveDWT2DInt()");
        goto Error;
      }

  for (scale = temporal_num_scales - 1; scale >= 0; scale--)
    {
      baseband_num_frames =
        QccWAVWaveletDWTSubbandLength(num_frames, scale, 0, 0, 0);
      
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          {
            for (frame = 0; frame < baseband_num_frames; frame++)
              {
                input_signal_vector[frame] = signal[frame][row][col];
                input_mask_vector[frame] = mask[frame][row][col];
              }
            
            if (QccWAVWaveletShapeAdaptiveSynthesis1DInt(input_signal_vector,
                                                         output_signal_vector,
                                                         subsequence,
                                                         input_mask_vector,
                                                         output_mask_vector,
                                                         baseband_num_frames,
                                                         wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt): Error calling QccWAVWaveletShapeAdaptiveSynthesis1DInt()");
                goto Error;
              }
            
            for (frame = 0; frame < baseband_num_frames; frame++)
              {
                signal[frame][row][col] = output_signal_vector[frame];
                mask[frame][row][col] = output_mask_vector[frame];
              }
          }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_signal_vector);
  QccVectorIntFree(output_signal_vector);
  QccVectorIntFree(subsequence);
  QccVectorIntFree(input_mask_vector);
  QccVectorIntFree(output_mask_vector);
  return(return_value);
}
