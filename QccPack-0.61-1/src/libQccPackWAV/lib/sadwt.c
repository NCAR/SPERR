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


static int QccWAVWaveletShapeAdaptiveAnalysis1D(const QccVector input_signal,
                                                QccVector output_signal,
                                                QccVector subsequence,
                                                const QccVector input_mask,
                                                QccVector output_mask,
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
  if (QccWAVWaveletLWT(input_mask, output_mask,
                       signal_length, 0, 0))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveAnalysis1D): Error calling QccWAVWaveletLWT()");
      goto Error;
    }
  
  QccVectorZero(output_signal, signal_length);
  QccVectorZero(subsequence, signal_length);
  
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
          
          if (QccVectorCopy(subsequence,
                            &input_signal[subsequence_position],
                            subsequence_length))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveAnalysis1D): Error calling QccVectorCopy()");
              goto Error;
            }

          /* Do transform of current subsequence */
          if (QccWAVWaveletAnalysis1D(subsequence,
                                      subsequence_length,
                                      phase,
                                      wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveAnalysis1D): Error calling QccWAVWaveletAnalysis1D()");
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
          QccVectorCopy(&output_signal[lowband_position],
                        subsequence,
                        lowband_length);
          QccVectorCopy(&output_signal[highband_position],
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


static int QccWAVWaveletShapeAdaptiveSynthesis1D(const QccVector input_signal,
                                                 QccVector output_signal,
                                                 QccVector subsequence,
                                                 const QccVector input_mask,
                                                 QccVector output_mask,
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
  if (QccWAVWaveletInverseLWT(input_mask, output_mask,
                              signal_length, 0, 0))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveSynthesis1D): Error calling QccWAVWaveletInverseLWT()");
      goto Error;
    }
  
  QccVectorZero(output_signal, signal_length);
  QccVectorZero(subsequence, signal_length);
  
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
          QccVectorCopy(subsequence,
                        &input_signal[lowband_position],
                        lowband_length);
          QccVectorCopy(&subsequence[lowband_length],
                        &input_signal[highband_position],
                        highband_length);
          
          /* Inverse transform */
          if (QccWAVWaveletSynthesis1D(subsequence,
                                       subsequence_length,
                                       phase,
                                       wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveSynthesis1D): Error calling QccWAVWaveletSynthesis1D()");
              goto Error;
            }
          
          if (QccVectorCopy(&output_signal[subsequence_position],
                            subsequence,
                            subsequence_length))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveSynthesis1D): Error calling QccVectorCopy()");
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


int QccWAVWaveletShapeAdaptiveDWT1D(QccVector signal,
                                    QccVector mask,
                                    int signal_length,
                                    int num_scales,
                                    const QccWAVWavelet *wavelet)
{
  
  int return_value;
  int scale;
  QccVector temp_signal = NULL;
  QccVector temp_mask = NULL;
  QccVector subsequence = NULL;
  int current_length;
  
  if (signal == NULL)
    return(0);
  if (mask == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (num_scales <= 0)
    return(0);
  
  if ((temp_signal = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((temp_mask = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((subsequence = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  if (QccVectorCopy(temp_signal, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1D): Error calling QccVectorCopy()");
      goto Error;
    }
  if (QccVectorCopy(temp_mask, mask, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1D): Error calling QccVectorCopy()");
      goto Error;
    }
  
  current_length = signal_length;
  
  for (scale = 0; scale < num_scales; scale++)
    {
      if (QccWAVWaveletShapeAdaptiveAnalysis1D(temp_signal,
                                               signal,
                                               subsequence,
                                               temp_mask,
                                               mask,
                                               current_length,
                                               wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1D): Error calling QccWAVWaveletShapeAdaptiveAnalysis1D()");
          goto Error;
        }
      
      current_length =
        QccWAVWaveletDWTSubbandLength(signal_length, scale + 1, 0, 0, 0);
      
      if (QccVectorCopy(temp_signal, signal, current_length))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1D): Error calling QccVectorCopy()");
          goto Error;
        }
      if (QccVectorCopy(temp_mask, mask, current_length))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT1D): Error calling QccVectorCopy()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_signal);
  QccVectorFree(temp_mask);
  QccVectorFree(subsequence);
  return(return_value);
}


int QccWAVWaveletInverseShapeAdaptiveDWT1D(QccVector signal,
                                           QccVector mask,
                                           int signal_length,
                                           int num_scales,
                                           const QccWAVWavelet *wavelet)
{
  int return_value;
  int scale;
  QccVector temp_signal = NULL;
  QccVector temp_mask = NULL;
  QccVector subsequence = NULL;
  int current_length;
  
  if (signal == NULL)
    return(0);
  if (mask == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (num_scales <= 0)
    return(0);
  
  if ((temp_signal = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((temp_mask = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((subsequence = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  if (QccVectorCopy(temp_signal, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1D): Error calling QccVectorCopy()");
      goto Error;
    }
  if (QccVectorCopy(temp_mask, mask, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1D): Error calling QccVectorCopy()");
      goto Error;
    }
  
  for (scale = num_scales - 1; scale >= 0 ; scale--)
    {
      current_length = 
        QccWAVWaveletDWTSubbandLength(signal_length, scale, 0, 0, 0);
      
      if (QccWAVWaveletShapeAdaptiveSynthesis1D(temp_signal,
                                                signal,
                                                subsequence,
                                                temp_mask,
                                                mask,
                                                current_length,
                                                wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1D): Error calling QccWAVWaveletShapeAdaptiveSynthesis1D()");
          goto Error;
        }
      
      if (QccVectorCopy(temp_signal, signal, current_length))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1D): Error calling QccVectorCopy()");
          goto Error;
        }
      if (QccVectorCopy(temp_mask, mask, current_length))
        {
          QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveInverseDWT1D): Error calling QccVectorCopy()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_signal);
  QccVectorFree(temp_mask);
  QccVectorFree(subsequence);
  return(return_value);
  
}


int QccWAVWaveletShapeAdaptiveDWT2D(QccMatrix signal, 
                                    QccMatrix mask, 
                                    int num_rows,
                                    int num_cols,
                                    int num_scales,
                                    const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVector input_signal_vector = NULL;
  QccVector output_signal_vector = NULL;
  QccVector subsequence = NULL;
  QccVector input_mask_vector = NULL;
  QccVector output_mask_vector = NULL;
  int scale;
  int row, col;
  int baseband_num_rows;
  int baseband_num_cols;
  
  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDWT2D): Error calling VectorAlloc()");
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
          
          if (QccWAVWaveletShapeAdaptiveAnalysis1D(input_signal_vector,
                                                   output_signal_vector,
                                                   subsequence,
                                                   input_mask_vector,
                                                   output_mask_vector,
                                                   baseband_num_cols,
                                                   wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT2D): Error calling QccWAVWaveletShapeAdaptiveAnalysis1D()");
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
          
          if (QccWAVWaveletShapeAdaptiveAnalysis1D(input_signal_vector,
                                                   output_signal_vector,
                                                   subsequence,
                                                   input_mask_vector,
                                                   output_mask_vector,
                                                   baseband_num_rows,
                                                   wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDWT2D): Error calling QccWAVWaveletShapeAdaptiveAnalysis1D()");
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
  QccVectorFree(input_signal_vector);
  QccVectorFree(output_signal_vector);
  QccVectorFree(subsequence);
  QccVectorFree(input_mask_vector);
  QccVectorFree(output_mask_vector);
  return(return_value);
} 


int QccWAVWaveletInverseShapeAdaptiveDWT2D(QccMatrix signal, 
                                           QccMatrix mask, 
                                           int num_rows,
                                           int num_cols,
                                           int num_scales,
                                           const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVector input_signal_vector = NULL;
  QccVector output_signal_vector = NULL;
  QccVector subsequence = NULL;
  QccVector input_mask_vector = NULL;
  QccVector output_mask_vector = NULL;
  int scale;
  int row, col;
  int baseband_num_rows;
  int baseband_num_cols;
  
  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorAlloc(QccMathMax(num_rows, num_cols))) == NULL)
    
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2D): Error calling QccVectorAlloc()");
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
          
          if (QccWAVWaveletShapeAdaptiveSynthesis1D(input_signal_vector,
                                                    output_signal_vector,
                                                    subsequence,
                                                    input_mask_vector,
                                                    output_mask_vector,
                                                    baseband_num_rows,
                                                    wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2D): Error calling QccWAVWaveletShapeAdaptiveSynthesis1D()");
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
          
          if (QccWAVWaveletShapeAdaptiveSynthesis1D(input_signal_vector,
                                                    output_signal_vector,
                                                    subsequence,
                                                    input_mask_vector,
                                                    output_mask_vector,
                                                    baseband_num_cols,
                                                    wavelet))
            {
              QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDWT2D): Error calling QccWAVWaveletShapeAdaptiveSynthesis1D()");
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
  QccVectorFree(input_signal_vector);
  QccVectorFree(output_signal_vector);
  QccVectorFree(subsequence);
  QccVectorFree(input_mask_vector);
  QccVectorFree(output_mask_vector);
  return(return_value);
}


int QccWAVWaveletShapeAdaptiveDyadicDWT3D(QccVolume signal,
                                          QccVolume mask,
                                          int num_frames,
                                          int num_rows,
                                          int num_cols,
                                          int spatial_num_scales,
                                          const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVector input_signal_vector = NULL;
  QccVector output_signal_vector = NULL;
  QccVector subsequence = NULL;
  QccVector input_mask_vector = NULL;
  QccVector output_mask_vector = NULL;
  int scale;
  int frame, row, col;
  int baseband_num_frames;
  int baseband_num_rows;
  int baseband_num_cols;
  int temp_length;

  temp_length = QccMathMax(num_frames, QccMathMax(num_rows, num_cols));

  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptiveDyadicDWT3D): Error calling VectorAlloc()");
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
            
            if (QccWAVWaveletShapeAdaptiveAnalysis1D(input_signal_vector,
                                                     output_signal_vector,
                                                     subsequence,
                                                     input_mask_vector,
                                                     output_mask_vector,
                                                     baseband_num_cols,
                                                     wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDyadicDWT3D): Error calling QccWAVWaveletShapeAdaptiveAnalysis1D()");
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
            
            if (QccWAVWaveletShapeAdaptiveAnalysis1D(input_signal_vector,
                                                     output_signal_vector,
                                                     subsequence,
                                                     input_mask_vector,
                                                     output_mask_vector,
                                                     baseband_num_rows,
                                                     wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDyadicDWT3D): Error calling QccWAVWaveletShapeAdaptiveAnalysis1D()");
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
            
            if (QccWAVWaveletShapeAdaptiveAnalysis1D(input_signal_vector,
                                                     output_signal_vector,
                                                     subsequence,
                                                     input_mask_vector,
                                                     output_mask_vector,
                                                     baseband_num_frames,
                                                     wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletShapeAdaptiveDyadicDWT3D): Error calling QccWAVWaveletShapeAdaptiveAnalysis1D()");
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
  QccVectorFree(input_signal_vector);
  QccVectorFree(output_signal_vector);
  QccVectorFree(subsequence);
  QccVectorFree(input_mask_vector);
  QccVectorFree(output_mask_vector);
  return(return_value);
}


int QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D(QccVolume signal,
                                                 QccVolume mask,
                                                 int num_frames,
                                                 int num_rows,
                                                 int num_cols,
                                                 int spatial_num_scales,
                                                 const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVector input_signal_vector = NULL;
  QccVector output_signal_vector = NULL;
  QccVector subsequence = NULL;
  QccVector input_mask_vector = NULL;
  QccVector output_mask_vector = NULL;
  int scale;
  int frame, row, col;
  int baseband_num_frames;
  int baseband_num_rows;
  int baseband_num_cols;
  int temp_length;

  temp_length = QccMathMax(num_frames, QccMathMax(num_rows, num_cols));

  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorAlloc(temp_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D): Error calling QccVectorAlloc()");
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
            
            if (QccWAVWaveletShapeAdaptiveSynthesis1D(input_signal_vector,
                                                      output_signal_vector,
                                                      subsequence,
                                                      input_mask_vector,
                                                      output_mask_vector,
                                                      baseband_num_frames,
                                                      wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D): Error calling QccWAVWaveletShapeAdaptiveSynthesis1D()");
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
            
            if (QccWAVWaveletShapeAdaptiveSynthesis1D(input_signal_vector,
                                                      output_signal_vector,
                                                      subsequence,
                                                      input_mask_vector,
                                                      output_mask_vector,
                                                      baseband_num_rows,
                                                      wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D): Error calling QccWAVWaveletShapeAdaptiveSynthesis1D()");
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
            
            if (QccWAVWaveletShapeAdaptiveSynthesis1D(input_signal_vector,
                                                      output_signal_vector,
                                                      subsequence,
                                                      input_mask_vector,
                                                      output_mask_vector,
                                                      baseband_num_cols,
                                                      wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D): Error calling QccWAVWaveletShapeAdaptiveSynthesis1D()");
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
  QccVectorFree(input_signal_vector);
  QccVectorFree(output_signal_vector);
  QccVectorFree(subsequence);
  QccVectorFree(input_mask_vector);
  QccVectorFree(output_mask_vector);
  return(return_value);
}


int QccWAVWaveletShapeAdaptivePacketDWT3D(QccVolume signal,
                                          QccVolume mask,
                                          int num_frames,
                                          int num_rows,
                                          int num_cols,
                                          int temporal_num_scales,
                                          int spatial_num_scales,
                                          const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVector input_signal_vector = NULL;
  QccVector output_signal_vector = NULL;
  QccVector subsequence = NULL;
  QccVector input_mask_vector = NULL;
  QccVector output_mask_vector = NULL;
  int scale;
  int frame, row, col;
  int baseband_num_frames;

  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3D): Error calling VectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWaveletShapeAdaptivePacketDWT3D): Error calling VectorAlloc()");
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
            
            if (QccWAVWaveletShapeAdaptiveAnalysis1D(input_signal_vector,
                                                     output_signal_vector,
                                                     subsequence,
                                                     input_mask_vector,
                                                     output_mask_vector,
                                                     baseband_num_frames,
                                                     wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletShapeAdaptivePacketDWT3D): Error calling QccWAVWaveletShapeAdaptiveAnalysis1D()");
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
    if (QccWAVWaveletShapeAdaptiveDWT2D(signal[frame],
                                        mask[frame],
                                        num_rows,
                                        num_cols,
                                        spatial_num_scales,
                                        wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletShapeAdaptivePacketDWT3D): Error calling QccWAVWaveletShapeAdaptiveDWT2D()");
        goto Error;
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(input_signal_vector);
  QccVectorFree(output_signal_vector);
  QccVectorFree(subsequence);
  QccVectorFree(input_mask_vector);
  QccVectorFree(output_mask_vector);
  return(return_value);
}


int QccWAVWaveletInverseShapeAdaptivePacketDWT3D(QccVolume signal,
                                                 QccVolume mask,
                                                 int num_frames,
                                                 int num_rows,
                                                 int num_cols,
                                                 int temporal_num_scales,
                                                 int spatial_num_scales,
                                                 const QccWAVWavelet *wavelet)
{
  int return_value;
  QccVector input_signal_vector = NULL;
  QccVector output_signal_vector = NULL;
  QccVector subsequence = NULL;
  QccVector input_mask_vector = NULL;
  QccVector output_mask_vector = NULL;
  int scale;
  int frame, row, col;
  int baseband_num_frames;

  /* Allocate temporary working arrays */
  if ((input_signal_vector =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_signal_vector =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((subsequence =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((input_mask_vector =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_mask_vector =
       QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletInverseShapeAdaptiveDWT2D(signal[frame],
                                               mask[frame],
                                               num_rows,
                                               num_cols,
                                               spatial_num_scales,
                                               wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3D): Error calling QccWAVWaveletInverseShapeAdaptiveDWT2D()");
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
            
            if (QccWAVWaveletShapeAdaptiveSynthesis1D(input_signal_vector,
                                                      output_signal_vector,
                                                      subsequence,
                                                      input_mask_vector,
                                                      output_mask_vector,
                                                      baseband_num_frames,
                                                      wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInverseShapeAdaptivePacketDWT3D): Error calling QccWAVWaveletShapeAdaptiveSynthesis1D()");
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
  QccVectorFree(input_signal_vector);
  QccVectorFree(output_signal_vector);
  QccVectorFree(subsequence);
  QccVectorFree(input_mask_vector);
  QccVectorFree(output_mask_vector);
  return(return_value);
}
