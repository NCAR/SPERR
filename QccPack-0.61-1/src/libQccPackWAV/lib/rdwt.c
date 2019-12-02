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


static int QccWAVWaveletRedundantAnalysis1D(const QccVector input_signal,
                                            QccVector output_signal_low,
                                            QccVector output_signal_high,
                                            int signal_length,
                                            const QccWAVWavelet *wavelet)
{
  int return_value;

  switch (wavelet->implementation)
    {
    case QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK:
      if (QccWAVFilterBankRedundantAnalysis(input_signal,
                                            output_signal_low,
                                            output_signal_high,
                                            signal_length,
                                            &wavelet->filter_bank,
                                            wavelet->boundary))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantAnalysis1D): Error calling QccWAVFilterBankRedundantAnalysis()");
          goto Error;
        }
      break;
    case QCCWAVWAVELET_IMPLEMENTATION_LIFTED:
      if (QccWAVLiftingRedundantAnalysis(input_signal,
                                         output_signal_low,
                                         output_signal_high,
                                         signal_length,
                                         &wavelet->lifting_scheme,
                                         wavelet->boundary))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantAnalysis1D): Error calling QccWAVLiftingRedundantAnalysis()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVWaveletRedundantAnalysis1D): Undefined implementation (%d)",
                         wavelet->implementation);
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVWaveletRedundantSynthesis1D(const QccVector input_signal_low,
                                             const QccVector input_signal_high,
                                             QccVector output_signal,
                                             int signal_length,
                                             const QccWAVWavelet *wavelet)
{
  int return_value;

  switch (wavelet->implementation)
    {
    case QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK:
      if (QccWAVFilterBankRedundantSynthesis(input_signal_low,
                                             input_signal_high,
                                             output_signal,
                                             signal_length,
                                             &wavelet->filter_bank,
                                             wavelet->boundary))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantSynthesis1D): Error calling QccWAVFilterBankRedundantSynthesis()");
          goto Error;
        }
      break;
    case QCCWAVWAVELET_IMPLEMENTATION_LIFTED:
      if (QccWAVLiftingRedundantSynthesis(input_signal_low,
                                          input_signal_high,
                                          output_signal,
                                          signal_length,
                                          &wavelet->lifting_scheme,
                                          wavelet->boundary))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantSynthesis1D): Error calling QccWAVLiftingRedundantSynthesis()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVWaveletRedundantSynthesis1D): Undefined implementation (%d)",
                         wavelet->implementation);
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVWaveletRedundantDWT1D(const QccVector input_signal,
                                QccMatrix output_signals,
                                int signal_length,
                                int num_scales,
                                const QccWAVWavelet *wavelet)
{
  int return_value = 0;
  QccVector temp_signal = NULL;
  QccMatrix output_signals2 = NULL;
  int even_length;
  int odd_length;
  int scale;
  int max_scales;
  
  if (input_signal == NULL)
    return(0);
  if (output_signals == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (num_scales <= 0)
    return(0);
  
  if (signal_length <= 0)
    return(0);
  
  max_scales = (int)floor(QccMathLog2(signal_length));
  
  if (max_scales < num_scales)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): %d transform scales not supported for length %d signal - use %d or fewer scales",
                         num_scales,
                         signal_length,
                         max_scales);
      goto Error;
    }
  
  if (QccWAVWaveletRedundantAnalysis1D(input_signal,
                                       output_signals[0],
                                       output_signals[num_scales],
                                       signal_length,
                                       wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error calling QccWAVWaveletRedundantAnalysis1D()");
      goto Error;
    }
  
  if (num_scales == 1)
    goto Return;
  
  if ((temp_signal = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  even_length = QccWAVWaveletDWTSubbandLength(signal_length,
                                              1, 0, 0, 0);
  odd_length = QccWAVWaveletDWTSubbandLength(signal_length,
                                             1, 1, 0, 0);
  
  if ((output_signals2 =
       (QccMatrix)malloc(sizeof(QccVector) * num_scales)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error allocating memory");
      goto Error;
    }
  
  for (scale = 0; scale < num_scales; scale++)
    output_signals2[scale] = &(output_signals[scale][even_length]);
  
  if (QccWAVWaveletLWT(output_signals[0],
                       temp_signal,
                       signal_length,
                       0, 0))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error calling QccWAVWaveletLWT()");
      goto Error;
    }
  
  if (QccWAVWaveletRedundantDWT1D(temp_signal,
                                  output_signals,
                                  even_length,
                                  num_scales - 1,
                                  wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error calling QccWAVWaveletRedundantDWT1D()");
      goto Error;
    }
  
  if (QccWAVWaveletRedundantDWT1D(&(temp_signal[even_length]),
                                  output_signals2,
                                  odd_length,
                                  num_scales - 1,
                                  wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error calling QccWAVWaveletRedundantDWT1D()");
      goto Error;
    }
  
  for (scale = 0; scale < num_scales; scale++)
    {
      if (QccWAVWaveletInverseLWT(output_signals[scale],
                                  temp_signal,
                                  signal_length,
                                  0, 0))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error calling QccWAVWaveletInverseLWT()");
          goto Error;
        }
      if (QccVectorCopy(output_signals[scale], temp_signal, signal_length))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error calling QccVector()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_signal);
  if (output_signals2 != NULL)
    QccFree(output_signals2);
  return(return_value);
}


void QccWAVWaveletRedundantDWT1DSubsampleGetPattern(int subband,
                                                    int num_scales,
                                                    int pattern,
                                                    int *index_skip,
                                                    int *start_index,
                                                    const QccWAVWavelet
                                                    *wavelet)
{
  int scale = (!subband) ? num_scales : num_scales - (subband - 1);

  *index_skip = 1 << scale;

  *start_index = pattern;

  if (QccWAVWaveletBiorthogonal(wavelet) && subband)
    *start_index += (1 << (scale - 1));

  *start_index = QccMathModulus(*start_index, *index_skip);
}


int QccWAVWaveletRedundantDWT1DSubsample(const QccMatrix input_signals,
                                         QccVector output_signal,
                                         int signal_length,
                                         int num_scales,
                                         int subsample_pattern,
                                         const QccWAVWavelet *wavelet)
{
  int start_index;
  int index_skip;
  int subband;
  int index1;
  int index2;
  int index3;
  int max_scales;
  int baseband_phase_group_size;
  int num_subbands;
  int scale;

  if (input_signals == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  if (!signal_length)
    return(0);
  
  max_scales = (int)floor(QccMathLog2(signal_length));
  
  if (max_scales < num_scales)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1DSubsample): %d transform scales not supported for length %d signal - use %d or fewer scales",
                         num_scales,
                         signal_length,
                         max_scales);
      return(1);
    }
  
  index1 = 0;

  num_subbands = num_scales + 1;

  baseband_phase_group_size = (1 << num_scales);

  if (subsample_pattern > (baseband_phase_group_size - 1))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1DSubsample): subband_pattern can take on only values 0 to %d for a transform of %d scales",
                         baseband_phase_group_size - 1,
                         num_scales);
      return(1);
    }

  index_skip = (1 << num_scales);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      QccWAVWaveletRedundantDWT1DSubsampleGetPattern(subband,
                                                     num_scales,
                                                     subsample_pattern,
                                                     &index_skip,
                                                     &start_index,
                                                     wavelet);

      scale = (!subband) ? num_scales : num_scales - (subband - 1);

      for (index2 = 0, index3 = 0;
           index3 < QccWAVWaveletDWTSubbandLength(signal_length,
                                                  scale,
                                                  subband,
                                                  0, 0);
           index1++, index2 += index_skip, index3++)
        if (wavelet->boundary == QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION)
          output_signal[index1] = 
            input_signals[subband]
            [QccFilterCalcSymmetricExtension(index2 + start_index,
                                             signal_length,
                                             QCCFILTER_SYMMETRICWHOLE)];
        else
          output_signal[index1] = 
            input_signals[subband][QccMathModulus(index2 + start_index,
                                                  signal_length)];
    }
  
  return(0);
}


static
int QccWAVWaveletInverseRedundantDWT1DRecursion(QccMatrix input_signals,
                                                QccVector output_signal,
                                                int signal_length,
                                                int num_scales,
                                                const QccWAVWavelet *wavelet)
{
  int return_value;
  int max_scales;
  QccVector temp_signal = NULL;
  QccMatrix input_signals2 = NULL;
  int scale;
  int even_length;
  int odd_length;
  
  if (input_signals == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  if (signal_length <= 0)
    return(0);
  if (num_scales <= 0)
    return(0);
  
  max_scales = (int)floor(QccMathLog2(signal_length));
  
  if (max_scales < num_scales)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): %d transform scales not supported for length %d signal - use %d or fewer scales",
                         num_scales,
                         signal_length,
                         max_scales);
      goto Error;
    }
  
  if (num_scales != 1)
    {
      if ((temp_signal = QccVectorAlloc(signal_length)) == NULL)
        {
          QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): Error calling QccVectorAlloc()");
          goto Error;
        }
      
      for (scale = 0; scale < num_scales; scale++)
        {
          if (QccWAVWaveletLWT(input_signals[scale],
                               temp_signal,
                               signal_length,
                               0, 0))
            {
              QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): Error calling QccWAVWaveletLWT()");
              goto Error;
            }
          if (QccVectorCopy(input_signals[scale], temp_signal, signal_length))
            {
              QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): Error calling QccVector()");
              goto Error;
            }
        }
      
      even_length = QccWAVWaveletDWTSubbandLength(signal_length,
                                                  1, 0, 0, 0);
      odd_length = QccWAVWaveletDWTSubbandLength(signal_length,
                                                 1, 1, 0, 0);
      
      if ((input_signals2 =
           (QccMatrix)malloc(sizeof(QccVector) * num_scales)) == NULL)
        {
          QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): Error allocating memory");
          goto Error;
        }
      
      for (scale = 0; scale < num_scales; scale++)
        input_signals2[scale] = &(input_signals[scale][even_length]);
      
      if (QccWAVWaveletInverseRedundantDWT1DRecursion(input_signals,
                                                      temp_signal,
                                                      even_length,
                                                      num_scales - 1,
                                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): Error calling QccWAVWaveletInverseRedundantDWT1DRecursion()");
          goto Error;
        }
      
      if (QccWAVWaveletInverseRedundantDWT1DRecursion(input_signals2,
                                                      &(temp_signal
                                                        [even_length]),
                                                      odd_length,
                                                      num_scales - 1,
                                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): Error calling QccWAVWaveletInverseRedundantDWT1DRecursion()");
          goto Error;
        }
      
      if (QccWAVWaveletInverseLWT(temp_signal,
                                  input_signals[0],
                                  signal_length,
                                  0, 0))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): Error calling QccWAVWaveletLWT()");
          goto Error;
        }
    }
  
  if (QccWAVWaveletRedundantSynthesis1D(input_signals[0],
                                        input_signals[num_scales],
                                        output_signal,
                                        signal_length,
                                        wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1DRecursion): Error calling QccWAVWaveletRedundantSynthesis1D()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_signal);
  if (input_signals2 != NULL)
    QccFree(input_signals2);
  return(return_value);
}     


int QccWAVWaveletInverseRedundantDWT1D(const QccMatrix input_signals,
                                       QccVector output_signal,
                                       int signal_length,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet)
{
  int return_value;
  QccMatrix input_signals2 = NULL;
  int num_subbands;

  num_subbands = num_scales + 1;

  if ((input_signals2 = QccMatrixAlloc(num_subbands,
                                       signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1D): Error calling QccMatrixAlloc()");
      goto Error;
    }

  if (QccMatrixCopy(input_signals2, input_signals,
                    num_subbands, signal_length))
    {
      QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1D): Error calling QccMatrixCopy()");
      goto Error;
    }

  if (QccWAVWaveletInverseRedundantDWT1DRecursion(input_signals2,
                                                  output_signal,
                                                  signal_length,
                                                  num_scales,
                                                  wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1D): Error calling QccWAVWaveletInverseRedundantDWT1DRecursion()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(input_signals2, num_subbands);
  return(return_value);
}


typedef struct
{
  QccMatrix matrix;
  int subband_num_rows;
  int subband_num_cols;
  int subband_row_origin;
  int subband_col_origin;
} QccWAVRDWTSubband;


static void QccWAVRDWTSubbandSetInfo(QccWAVRDWTSubband *subband1,
                                     const QccWAVRDWTSubband *subband2,
                                     int polyphase)
{
  int high_band_row;
  int high_band_col;

  subband1->matrix = subband2->matrix;
  
  high_band_row = ((polyphase == 1) || (polyphase == 3));
  high_band_col = ((polyphase == 2) || (polyphase == 3));

  subband1->subband_num_rows =
    QccWAVWaveletDWTSubbandLength(subband2->subband_num_rows, 1,
                                  high_band_row, 0, 0);
  subband1->subband_num_cols =
    QccWAVWaveletDWTSubbandLength(subband2->subband_num_cols, 1,
                                  high_band_col, 0, 0);
  subband1->subband_row_origin = 
    ((high_band_row) ? 
     QccWAVWaveletDWTSubbandLength(subband2->subband_num_rows,
                                   1, 0, 0, 0) : 0) +
    subband2->subband_row_origin;
  subband1->subband_col_origin =
    ((high_band_col) ?
     QccWAVWaveletDWTSubbandLength(subband2->subband_num_cols,
                                   1, 0, 0, 0) : 0) +
    subband2->subband_col_origin;

  /*
    printf("%d: ", polyphase);
    printf("  %d x %d @ (%d, %d)\n",
    subband1->subband_num_rows,
    subband1->subband_num_cols,
    subband1->subband_row_origin,
    subband1->subband_col_origin);
  */
}


static void QccWAVRDWTSubbandCopy(const QccWAVRDWTSubband *input_subband,
                                  QccWAVRDWTSubband *output_subband)
{
  int row, col;
  
  for (row = 0; row < input_subband->subband_num_rows; row++)
    for (col = 0; col < input_subband->subband_num_cols; col++)
      input_subband->matrix
        [row + input_subband->subband_row_origin]
        [col + input_subband->subband_col_origin] =
        output_subband->matrix
        [row + output_subband->subband_row_origin]
        [col + output_subband->subband_col_origin];
}


QccMatrix *QccWAVWaveletRedundantDWT2DAlloc(int num_rows,
                                            int num_cols,
                                            int num_scales)
{
  int num_subbands;
  int subband;
  QccMatrix *rdwt = NULL;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_scales);

  if ((rdwt = (QccMatrix *)malloc(sizeof(QccMatrix) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DAlloc): Error allocating memory()");
      return(NULL);
    }

  for (subband = 0; subband < num_subbands; subband++)
    if ((rdwt[subband] = QccMatrixAlloc(num_rows, num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DAlloc): Error calling QccMatrixAlloc()");
        return(NULL);
      }

  return(rdwt);
}


void QccWAVWaveletRedundantDWT2DFree(QccMatrix *rdwt,
                                     int num_rows,
                                     int num_scales)
{
  int subband;
  int num_subbands;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_scales);

  if (rdwt != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccMatrixFree(rdwt[subband], num_rows);
      QccFree(rdwt);
    }
}


static int QccWAVRDWTLWT2D(const QccWAVRDWTSubband *input_subband,
                           QccWAVRDWTSubband *output_subband)
{
  int return_value;
  int row, col;
  int num_rows, num_cols;
  QccVector input_column = NULL;
  QccVector output_column = NULL;

  num_rows = input_subband->subband_num_rows;
  num_cols = input_subband->subband_num_cols;

  if ((input_column = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTLWT2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_column = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTLWT2D): Error calling QccVectorAlloc()");
      goto Error;
    }

  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletLWT(&input_subband->matrix
                         [row + input_subband->subband_row_origin]
                         [input_subband->subband_col_origin],
                         &output_subband->matrix
                         [row + output_subband->subband_row_origin]
                         [output_subband->subband_col_origin],
                         num_cols,
                         0, 0))
      {
        QccErrorAddMessage("(QccWAVRDWTLWT2D): Error calling QccWAVWaveletLWT()");
        goto Error;
      }

  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        input_column[row] =
          output_subband->matrix
          [row + output_subband->subband_row_origin]
          [col + output_subband->subband_col_origin];

      if (QccWAVWaveletLWT(input_column,
                           output_column,
                           num_rows,
                           0, 0))
        {
          QccErrorAddMessage("(QccWAVRDWTLWT2D): Error calling QccWAVWaveletLWT()");
          goto Error;
        }

      for (row = 0; row < num_rows; row++)
        output_subband->matrix
          [row + output_subband->subband_row_origin]
          [col + output_subband->subband_col_origin] = output_column[row];
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(input_column);
  QccVectorFree(output_column);
  return(return_value);
}


int QccWAVRDWTInverseLWT2D(const QccWAVRDWTSubband *input_subband,
                           QccWAVRDWTSubband *output_subband)
{
  int return_value;
  int row, col;
  int num_rows, num_cols;
  QccVector input_column = NULL;
  QccVector output_column = NULL;
  
  num_rows = input_subband->subband_num_rows;
  num_cols = input_subband->subband_num_cols;

  if ((input_column = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTInverseLWT2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_column = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTInverseLWT2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletInverseLWT(&input_subband->matrix
                                [row + input_subband->subband_row_origin]
                                [input_subband->subband_col_origin],
                                &output_subband->matrix
                                [row + output_subband->subband_row_origin]
                                [output_subband->subband_col_origin],
                                num_cols,
                                0, 0))
      {
        QccErrorAddMessage("(QccWAVRDWTInverseLWT2D): Error calling QccWAVWaveletInverseLWT()");
        goto Error;
      }
  
  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        input_column[row] =
          output_subband->matrix
          [row + output_subband->subband_row_origin]
          [col + output_subband->subband_col_origin];
      
      if (QccWAVWaveletInverseLWT(input_column,
                                  output_column,
                                  num_rows,
                                  0, 0))
        {
          QccErrorAddMessage("(QccWAVRDWTInverseLWT2D): Error calling QccWAVWaveletInverseLWT()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        output_subband->matrix
          [row + output_subband->subband_row_origin]
          [col + output_subband->subband_col_origin] = output_column[row];
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(input_column);
  QccVectorFree(output_column);
  return(return_value);
}


static int QccWAVRDWTAnalysis2D(QccWAVRDWTSubband *input,
                                QccWAVRDWTSubband *baseband,
                                QccWAVRDWTSubband *horizontal,
                                QccWAVRDWTSubband *vertical,
                                QccWAVRDWTSubband *diagonal,
                                const QccWAVWavelet *wavelet)
{
  int return_value;
  int row, col;
  int num_rows, num_cols;
  QccVector input_column = NULL;
  QccVector output_column_low = NULL;
  QccVector output_column_high = NULL;
  
  num_rows = input->subband_num_rows;
  num_cols = input->subband_num_cols;
  
  if ((input_column = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_column_low = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_column_high = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletRedundantAnalysis1D(&input->matrix
                                         [row + input->subband_row_origin]
                                         [input->subband_col_origin],
                                         &baseband->matrix
                                         [row + baseband->subband_row_origin]
                                         [baseband->subband_col_origin],
                                         &vertical->matrix
                                         [row + vertical->subband_row_origin]
                                         [vertical->subband_col_origin],
                                         num_cols,
                                         wavelet))
      {
        QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccWAVWaveletRedundantAnalysis1D()");
        goto Error;
      }
  
  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        input_column[row] = 
          baseband->matrix
          [row + baseband->subband_row_origin]
          [col + baseband->subband_col_origin];
      
      if (QccWAVWaveletRedundantAnalysis1D(input_column,
                                           output_column_low,
                                           output_column_high,
                                           num_rows,
                                           wavelet))
        {
          QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccWAVWaveletRedundantAnalysis1D()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        {
          baseband->matrix
            [row + baseband->subband_row_origin]
            [col + baseband->subband_col_origin] = output_column_low[row];
          horizontal->matrix
            [row + horizontal->subband_row_origin]
            [col + horizontal->subband_col_origin] = output_column_high[row];
          
          input_column[row] =
            vertical->matrix
            [row + vertical->subband_row_origin]
            [col + vertical->subband_col_origin];
        }
      
      if (QccWAVWaveletRedundantAnalysis1D(input_column,
                                           output_column_low,
                                           output_column_high,
                                           num_rows,
                                           wavelet))
        {
          QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccWAVWaveletRedundantAnalysis1D()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        {
          vertical->matrix
            [row + vertical->subband_row_origin]
            [col + vertical->subband_col_origin] = output_column_low[row];
          diagonal->matrix
            [row + diagonal->subband_row_origin]
            [col + diagonal->subband_col_origin] = output_column_high[row];
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(input_column);
  QccVectorFree(output_column_low);
  QccVectorFree(output_column_high);
  return(return_value);
}


static int QccWAVRDWTSynthesis2D(QccWAVRDWTSubband *baseband,
                                 QccWAVRDWTSubband *horizontal,
                                 QccWAVRDWTSubband *vertical,
                                 QccWAVRDWTSubband *diagonal,
                                 QccWAVRDWTSubband *output,
                                 const QccWAVWavelet *wavelet)
{
  int return_value;
  int row, col;
  int num_rows, num_cols;
  QccVector input_column_low = NULL;
  QccVector input_column_high = NULL;
  QccVector output_column = NULL;

  num_rows = baseband->subband_num_rows;
  num_cols = baseband->subband_num_cols;
  
  if ((input_column_low = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((input_column_high = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_column = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTAnalysis2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        {
          input_column_low[row] =
            vertical->matrix
            [row + vertical->subband_row_origin]
            [col + vertical->subband_col_origin];
          input_column_high[row] = 
            diagonal->matrix
            [row + diagonal->subband_row_origin]
            [col + diagonal->subband_col_origin];
        }

      if (QccWAVWaveletRedundantSynthesis1D(input_column_low,
                                            input_column_high,
                                            output_column,
                                            num_rows,
                                            wavelet))
        {
          QccErrorAddMessage("(QccWAVRDWTSynthesis2D): Error calling QccWAVWaveletRedundantSynthesis1D()");
          goto Error;
        }
      
      /****/

      for (row = 0; row < num_rows; row++)
        {
          vertical->matrix
            [row + vertical->subband_row_origin]
            [col + vertical->subband_col_origin] =
            output_column[row];

          input_column_low[row] =
            baseband->matrix
            [row + baseband->subband_row_origin]
            [col + baseband->subband_col_origin];
          input_column_high[row] = 
            horizontal->matrix
            [row + horizontal->subband_row_origin]
            [col + horizontal->subband_col_origin];
          
        }
      
      if (QccWAVWaveletRedundantSynthesis1D(input_column_low,
                                            input_column_high,
                                            output_column,
                                            num_rows,
                                            wavelet))
        {
          QccErrorAddMessage("(QccWAVRDWTSynthesis2D): Error calling QccWAVWaveletRedundantSynthesis1D()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        baseband->matrix
          [row + baseband->subband_row_origin]
          [col + baseband->subband_col_origin] =
          output_column[row];
    }

  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletRedundantSynthesis1D(&baseband->matrix
                                          [row + baseband->subband_row_origin]
                                          [baseband->subband_col_origin],
                                          &vertical->matrix
                                          [row + vertical->subband_row_origin]
                                          [vertical->subband_col_origin],
                                          &output->matrix
                                          [row + output->subband_row_origin]
                                          [output->subband_col_origin],
                                          num_cols,
                                          wavelet))
      {
        QccErrorAddMessage("(QccWAVRDWTSynthesis2D): Error calling QccWAVWaveletRedundantSynthesis1D()");
        goto Error;
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(input_column_low);
  QccVectorFree(input_column_high);
  QccVectorFree(output_column);
  return(return_value);
}


static
int QccWAVWaveletRedundantDWT2DRecursion(QccWAVRDWTSubband
                                         *input_subband,
                                         QccWAVRDWTSubband
                                         *output_subbands,
                                         int num_scales,
                                         const QccWAVWavelet *wavelet)
{
  int return_value = 0;
  QccWAVRDWTSubband baseband[4];
  QccWAVRDWTSubband *new_output_subbands[4] = 
    {NULL, NULL, NULL, NULL};
  int num_subbands;
  int subband;
  int polyphase;
  
  if (QccWAVRDWTAnalysis2D(input_subband,
                           &output_subbands[0],
                           &output_subbands[3*num_scales - 2],
                           &output_subbands[3*num_scales - 1],
                           &output_subbands[3*num_scales],
                           wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DRecursion): Error calling QccWAVRDWTAnalysis2D()");
      goto Error;
    }
  
  if (num_scales == 1)
    goto Return;
  
  if (QccWAVRDWTLWT2D(&output_subbands[0],
                      input_subband))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1D): Error calling QccWAVRDWTLWT2D()");
      goto Error;
    }
  
  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_scales - 1);
  
  for (polyphase = 0; polyphase < 4; polyphase++)
    {
      QccWAVRDWTSubbandSetInfo(&baseband[polyphase],
                               input_subband,
                               polyphase);
      
      if ((new_output_subbands[polyphase] =
           (QccWAVRDWTSubband *)
           malloc(sizeof(QccWAVRDWTSubband) * num_subbands))
          == NULL)
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DRecursion): Error allocating memory");
          goto Error;
        }
      
      for (subband = 0; subband < num_subbands; subband++)
        QccWAVRDWTSubbandSetInfo(&new_output_subbands
                                 [polyphase][subband],
                                 &output_subbands[subband],
                                 polyphase);
    }
  
  for (polyphase = 0; polyphase < 4; polyphase++)
    if (QccWAVWaveletRedundantDWT2DRecursion(&baseband[polyphase],
                                             new_output_subbands[polyphase],
                                             num_scales - 1,
                                             wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DRecursion): Error calling QccWAVWaveletRedundantDWT2DRecursion()");
        goto Error;
      }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVRDWTInverseLWT2D(&output_subbands[subband],
                                 input_subband))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DRecursion): Error calling QccWAVRDWTInverseLWT2D()");
          goto Error;
        }

      QccWAVRDWTSubbandCopy(&output_subbands[subband], input_subband);
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  for (polyphase = 0; polyphase < 4; polyphase++)
    if (new_output_subbands[polyphase] != NULL)
      QccFree(new_output_subbands[polyphase]);
  return(return_value);
}


int QccWAVWaveletRedundantDWT2D(const QccMatrix input_matrix,
                                QccMatrix *output_matrices,
                                int num_rows,
                                int num_cols,
                                int num_scales,
                                const QccWAVWavelet *wavelet)
{
  int return_value = 0;
  int max_scales;
  int subband;
  int num_subbands;
  QccWAVRDWTSubband *subbands = NULL;
  QccWAVRDWTSubband temp_subband;
  QccMatrix temp_matrix = NULL;

  if (input_matrix == NULL)
    return(0);
  if (output_matrices == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  if (num_scales <= 0)
    return(0);
  
  max_scales = (int)QccMathMin(floor(QccMathLog2(num_rows)),
                               floor(QccMathLog2(num_cols)));
  
  if (max_scales < num_scales)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): %d transform scales not supported for size %dx%d matrices - use %d or fewer scales",
                         num_scales,
                         num_cols,
                         num_rows,
                         max_scales);
      goto Error;
    }
  
  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_scales);

  if ((subbands =
       (QccWAVRDWTSubband *)
       malloc(num_subbands * sizeof(QccWAVRDWTSubband))) ==
      NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): Error allocating memory");
      goto Error;
    }

  if ((temp_matrix =
       QccMatrixAlloc(num_rows, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): Error calling QccMatrixAlloc()");
      goto Error;
    }

  temp_subband.matrix = temp_matrix;  
  temp_subband.subband_num_rows = num_rows;
  temp_subband.subband_num_cols = num_cols;
  temp_subband.subband_row_origin = 0;
  temp_subband.subband_col_origin = 0;

  if (QccMatrixCopy(temp_matrix,
                    input_matrix,
                    num_rows,
                    num_cols))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): Error calling QccMatrixCopy()");
      goto Error;
    }

  for (subband = 0; subband < num_subbands; subband++)
    {
      subbands[subband].matrix = output_matrices[subband];
      subbands[subband].subband_num_rows = num_rows;
      subbands[subband].subband_num_cols = num_cols;
      subbands[subband].subband_row_origin = 0;
      subbands[subband].subband_col_origin = 0;
    }

  if (QccWAVWaveletRedundantDWT2DRecursion(&temp_subband,
                                           subbands,
                                           num_scales,
                                           wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): Error calling QccWAVWaveletRedundantDWT2DRecursion()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(temp_matrix, num_rows);
  if (subbands != NULL)
    QccFree(subbands);
  return(return_value);
}


void QccWAVWaveletRedundantDWT2DSubsampleGetPattern(int subband,
                                                    int num_scales,
                                                    int pattern_row,
                                                    int pattern_col,
                                                    int *index_skip,
                                                    int *offset_row,
                                                    int *offset_col,
                                                    const QccWAVWavelet
                                                    *wavelet)
{
  int scale =
    QccWAVSubbandPyramidCalcLevelFromSubband(subband,
                                             num_scales);

  *index_skip = (1 << scale);

  *offset_row = pattern_row;
  *offset_col = pattern_col;
  
  if (QccWAVWaveletBiorthogonal(wavelet) && subband)
    switch ((subband - 1) % 3)
      {
      case 0:
        *offset_row += (1 << (scale - 1));
        break;
      case 1:
        *offset_col += (1 << (scale - 1));
        break;
      case 2:
        *offset_row += (1 << (scale - 1));
        *offset_col += (1 << (scale - 1));
        break;
      }
      
  if (*offset_row >= *index_skip)
    *offset_row = QccMathModulus(*offset_row, *index_skip);
  if (*offset_col >= *index_skip)
    *offset_col = QccMathModulus(*offset_col, *index_skip);
}


int QccWAVWaveletRedundantDWT2DSubsample(const QccMatrix *input_matrices,
                                         QccMatrix output_matrix,
                                         int num_rows,
                                         int num_cols,
                                         int num_scales,
                                         int subsample_pattern_row,
                                         int subsample_pattern_col,
                                         const QccWAVWavelet *wavelet)
{
  int subband;
  int num_subbands;
  int index_skip;
  int index_offset_row;
  int index_offset_col;
  int subband_row_offset;
  int subband_col_offset;
  int subband_num_rows;
  int subband_num_cols;
  int row;
  int col;
  int row_index;
  int col_index;
  int baseband_phase_group_size;

  QccWAVSubbandPyramid subband_pyramid;
  
  if (input_matrices == NULL)
    return(0);
  if (output_matrix == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&subband_pyramid);
  
  subband_pyramid.num_rows = num_rows;
  subband_pyramid.num_cols = num_cols;
  subband_pyramid.num_levels = num_scales;
  
  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(num_scales);
  
  baseband_phase_group_size = (1 << num_scales);

  if (subsample_pattern_row >
      (baseband_phase_group_size - 1))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1DSubsample): subband_pattern_row can take on only values 0 to %d for a transform of %d scales",
                         baseband_phase_group_size - 1,
                         num_scales);
      return(1);
    }
  if (subsample_pattern_col >
      (baseband_phase_group_size - 1))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT1DSubsample): subband_pattern_col can take on only values 0 to %d for a transform of %d scales",
                         baseband_phase_group_size - 1,
                         num_scales);
      return(1);
    }

  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(&subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DSubsample): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      if (QccWAVSubbandPyramidSubbandOffsets(&subband_pyramid,
                                             subband,
                                             &subband_row_offset,
                                             &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DSubsample): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      
      QccWAVWaveletRedundantDWT2DSubsampleGetPattern(subband,
                                                     num_scales,
                                                     subsample_pattern_row,
                                                     subsample_pattern_col,
                                                     &index_skip,
                                                     &index_offset_row,
                                                     &index_offset_col,
                                                     wavelet);

      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          {
            if (wavelet->boundary ==
                QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION)
              {
                row_index = 
                  QccFilterCalcSymmetricExtension(row * index_skip +
                                                  index_offset_row,
                                                  num_rows,
                                                  QCCFILTER_SYMMETRICWHOLE);
                col_index = 
                  QccFilterCalcSymmetricExtension(col * index_skip +
                                                  index_offset_col,
                                                  num_cols,
                                                  QCCFILTER_SYMMETRICWHOLE);
              }
            else
              {
                row_index = 
                  QccMathModulus(row * index_skip +
                                 index_offset_row,
                                 num_rows);
                col_index = 
                  QccMathModulus(col * index_skip +
                                 index_offset_col,
                                 num_cols);
              }

            output_matrix[subband_row_offset + row]
              [subband_col_offset + col] = 
              input_matrices[subband][row_index][col_index];
          }
    }
  
  return(0);
}


/*
  int QccWAVWaveletInverseRedundantDWT2D(const QccMatrix *input_matrices,
  QccMatrix output_matrix,
  int num_rows,
  int num_cols,
  int num_scales,
  const QccWAVWavelet *wavelet)
  {
  QccWAVSubbandPyramid subband_pyramid;
  
  QccWAVSubbandPyramidInitialize(&subband_pyramid);
  
  subband_pyramid.matrix = output_matrix;
  subband_pyramid.num_levels = num_scales;
  subband_pyramid.num_rows = num_rows;
  subband_pyramid.num_cols = num_cols;
  
  if (QccWAVWaveletRedundantDWT2DSubsample(input_matrices,
  output_matrix,
  num_rows,
  num_cols,
  num_scales,
  wavelet))
  {
  QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT2D): Error calling QccWAVWaveletRedundantDWT2DSubsample()");
  return(1);
  }
  
  if (QccWAVSubbandPyramidInverseDWT(&subband_pyramid,
  wavelet))
  {
  QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT2D): Error calling QccWAVSubbandPyramidInverseDWT()");
  return(1);
  }
  
  return(0);
  }
*/


static int QccWAVWaveletInverseRedundantDWT2DRecursion(QccWAVRDWTSubband
                                                       *input_subbands,
                                                       QccWAVRDWTSubband
                                                       *output_subband,
                                                       int num_scales,
                                                       const QccWAVWavelet
                                                       *wavelet)
{
  int return_value = 0;
  QccWAVRDWTSubband baseband[4];
  QccWAVRDWTSubband *new_input_subbands[4] = 
    {NULL, NULL, NULL, NULL};
  int num_subbands;
  int subband;
  int polyphase;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_scales - 1);
  
  if (num_scales != 1)
    {
      for (subband = 0; subband < num_subbands; subband++)
        {
          if (QccWAVRDWTLWT2D(&input_subbands[subband],
                              output_subband))
            {
              QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT2DRecursion): Error calling QccWAVRDWTLWT2D()");
              goto Error;
            }
          
          QccWAVRDWTSubbandCopy(&input_subbands[subband], output_subband);
        }
      
      for (polyphase = 0; polyphase < 4; polyphase++)
        {
          QccWAVRDWTSubbandSetInfo(&baseband[polyphase],
                                   output_subband,
                                   polyphase);
          
          if ((new_input_subbands[polyphase] =
               (QccWAVRDWTSubband *)
               malloc(sizeof(QccWAVRDWTSubband) * num_subbands))
              == NULL)
            {
              QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT2DRecursion): Error allocating memory");
              goto Error;
            }
          
          for (subband = 0; subband < num_subbands; subband++)
            QccWAVRDWTSubbandSetInfo(&new_input_subbands
                                     [polyphase][subband],
                                     &input_subbands[subband],
                                     polyphase);
        }
      
      for (polyphase = 0; polyphase < 4; polyphase++)
        if (QccWAVWaveletInverseRedundantDWT2DRecursion(new_input_subbands
                                                        [polyphase],
                                                        &baseband[polyphase],
                                                        num_scales - 1,
                                                        wavelet))
          {
            QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT2DRecursion): Error calling QccWAVWaveletInverseRedundantDWT2DRecursion()");
            goto Error;
          }
      
      if (QccWAVRDWTInverseLWT2D(output_subband,
                                 &input_subbands[0]))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT1D): Error calling QccWAVRDWTInverseLWT2D()");
          goto Error;
        }
    }
  
  if (QccWAVRDWTSynthesis2D(&input_subbands[0],
                            &input_subbands[3*num_scales - 2],
                            &input_subbands[3*num_scales - 1],
                            &input_subbands[3*num_scales],
                            output_subband,
                            wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT2DRecursion): Error calling QccWAVRDWTSynthesis2D()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  for (polyphase = 0; polyphase < 4; polyphase++)
    if (new_input_subbands[polyphase] != NULL)
      QccFree(new_input_subbands[polyphase]);
  return(return_value);
}


int QccWAVWaveletInverseRedundantDWT2D(const QccMatrix *input_matrices,
                                       QccMatrix output_matrix,
                                       int num_rows,
                                       int num_cols,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet)
{
  int return_value = 0;
  int max_scales;
  int subband;
  int num_subbands;
  QccWAVRDWTSubband *subbands = NULL;
  QccWAVRDWTSubband temp_subband;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_scales);

  if (input_matrices == NULL)
    return(0);
  if (output_matrix == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  if (num_scales <= 0)
    return(0);
  
  max_scales = (int)QccMathMin(floor(QccMathLog2(num_rows)),
                               floor(QccMathLog2(num_cols)));
  
  if (max_scales < num_scales)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): %d transform scales not supported for size %dx%d matrices - use %d or fewer scales",
                         num_scales,
                         num_cols,
                         num_rows,
                         max_scales);
      goto Error;
    }
  
  if ((subbands =
       (QccWAVRDWTSubband *)
       malloc(num_subbands * sizeof(QccWAVRDWTSubband))) ==
      NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): Error allocating memory");
      goto Error;
    }

  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((subbands[subband].matrix = 
           QccMatrixAlloc(num_rows, num_cols)) == NULL)
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): Error calling QccMatrixAlloc()");
          goto Error;
        }
      if (QccMatrixCopy(subbands[subband].matrix,
                        input_matrices[subband],
                        num_rows, num_cols))
        {
          QccErrorAddMessage("(QccWAVWaveletRedundantDWT2D): Error calling QccMatrixCopy()");
          goto Error;
        }
      subbands[subband].subband_num_rows = num_rows;
      subbands[subband].subband_num_cols = num_cols;
      subbands[subband].subband_row_origin = 0;
      subbands[subband].subband_col_origin = 0;
    }

  temp_subband.matrix = output_matrix;
  temp_subband.subband_num_rows = num_rows;
  temp_subband.subband_num_cols = num_cols;
  temp_subband.subband_row_origin = 0;
  temp_subband.subband_col_origin = 0;

  if (QccWAVWaveletInverseRedundantDWT2DRecursion(subbands,
                                                  &temp_subband,
                                                  num_scales,
                                                  wavelet))
    {
      QccErrorAddMessage("(QccWAVWaveletInverseRedundantDWT2D): Error calling QccWAVWaveletInverseRedundantDWT2DRecursion()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (subbands != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccMatrixFree(subbands[subband].matrix, num_rows);
      QccFree(subbands);
    }
  return(return_value);
}


int QccWAVWaveletRedundantDWT2DCorrelationMask(const QccMatrix *input_matrices,
                                               QccMatrix output_matrix,
                                               int num_rows,
                                               int num_cols,
                                               int num_scales,
                                               int start_scale,
                                               int stop_scale)
{
  int row, col;
  int scale;
  int subband_orientation;
  int current_subband;
  double correlation;
  double max;

  if (input_matrices == NULL)
    return(0);
  if (output_matrix == NULL)
    return(0);

  if ((start_scale < 0) || (start_scale > num_scales - 1) ||
      (stop_scale < 0) || (stop_scale > num_scales - 1))
    {
      QccErrorAddMessage("(QccWAVWaveletRedundantDWT2DCorrelationMask): Invalid stop and/or start scale specified");
      return(1);
    }

  for (scale = start_scale; scale <= stop_scale; scale++)
    if (input_matrices[scale] == NULL)
      return(0);

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        output_matrix[row][col] = 0;

        for (subband_orientation = 1; subband_orientation <= 3;
             subband_orientation++)
          {
            correlation = 1.0;

            for (scale = start_scale; scale <= stop_scale; scale++)
              {
                current_subband = subband_orientation + 3 * scale;
                correlation *= fabs(input_matrices[current_subband][row][col]);
              }
            output_matrix[row][col] += correlation;
          }
      }

  max = QccMatrixMaxValue(output_matrix, num_rows, num_cols);

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      output_matrix[row][col] /= max;

  return(0);
}
