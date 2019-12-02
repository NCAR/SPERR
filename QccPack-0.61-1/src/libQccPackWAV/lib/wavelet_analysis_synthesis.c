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


int QccWAVWaveletAnalysis1D(QccVector signal,
                            int signal_length,
                            int phase,
                            const QccWAVWavelet *wavelet)
{
  int return_value;

  if (signal == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  switch (wavelet->implementation)
    {
    case QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK:
      if (QccWAVFilterBankAnalysis(signal,
                                   signal_length,
                                   phase,
                                   &wavelet->filter_bank,
                                   wavelet->boundary))
        {
          QccErrorAddMessage("(QccWAVWaveletAnalysis1D): Error calling QccWAVFilterBankAnalysis()");
          goto Error;
        }
      break;
    case QCCWAVWAVELET_IMPLEMENTATION_LIFTED:
      if (QccWAVLiftingAnalysis(signal,
                                signal_length,
                                phase,
                                &wavelet->lifting_scheme,
                                wavelet->boundary))
        {
          QccErrorAddMessage("(QccWAVWaveletAnalysis1D): Error calling QccWAVLiftingAnalysis()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVWaveletAnalysis1D): Undefined implementation (%d)",
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


int QccWAVWaveletSynthesis1D(QccVector signal,
                             int signal_length,
                             int phase,
                             const QccWAVWavelet *wavelet)
{
  int return_value;

  if (signal == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  switch (wavelet->implementation)
    {
    case QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK:
      if (QccWAVFilterBankSynthesis(signal,
                                    signal_length,
                                    phase,
                                    &wavelet->filter_bank,
                                    wavelet->boundary))
        {
          QccErrorAddMessage("(QccWAVWaveletSynthesis1D): Error calling QccWAVFilterBankSynthesis()");
          goto Error;
        }
      break;
    case QCCWAVWAVELET_IMPLEMENTATION_LIFTED:
      if (QccWAVLiftingSynthesis(signal,
                                 signal_length,
                                 phase,
                                 &wavelet->lifting_scheme,
                                 wavelet->boundary))
        {
          QccErrorAddMessage("(QccWAVWaveletSynthesis1D): Error calling QccWAVLiftingSynthesis()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVWaveletSynthesis1D): Undefined implementation (%d)",
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


int QccWAVWaveletAnalysis2D(QccMatrix matrix,
                            int num_rows,
                            int num_cols,
                            int phase_row,
                            int phase_col,
                            const QccWAVWavelet *wavelet)
{
  int return_value;
  int row, col;
  QccVector temp_column = NULL;

  if (matrix == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if ((temp_column = QccVectorAlloc(num_rows)) == NULL)
    {
        QccErrorAddMessage("(QccWAVWaveletAnalysis2D): Error calling QccVectorAlloc()");
        goto Error;
    }

  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletAnalysis1D(matrix[row],
                                num_cols,
                                phase_row,
                                wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletAnalysis2D): Error calling QccWAVWaveletAnalysis1D()");
        goto Error;
      }

  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        temp_column[row] = matrix[row][col];

      if (QccWAVWaveletAnalysis1D(temp_column,
                                  num_rows,
                                  phase_col,
                                  wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletAnalysis2D): Error calling QccWAVWaveletAnalysis1D()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        matrix[row][col] = temp_column[row];
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_column);
  return(return_value);
}


int QccWAVWaveletSynthesis2D(QccMatrix matrix,
                             int num_rows,
                             int num_cols,
                             int phase_row,
                             int phase_col,
                             const QccWAVWavelet *wavelet)
{
  int return_value;
  int row, col;
  QccVector temp_column = NULL;

  if (matrix == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if ((temp_column = QccVectorAlloc(num_rows)) == NULL)
    {
        QccErrorAddMessage("(QccWAVWaveletSynthesis2D): Error calling QccVectorAlloc()");
        goto Error;
    }

  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        temp_column[row] = matrix[row][col];

      if (QccWAVWaveletSynthesis1D(temp_column,
                                   num_rows,
                                   phase_col,
                                   wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletSynthesis2D): Error calling QccWAVWaveletSynthesis1D()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        matrix[row][col] = temp_column[row];
    }

  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletSynthesis1D(matrix[row],
                                 num_cols,
                                 phase_row,
                                 wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletSynthesis2D): Error calling QccWAVWaveletSynthesis1D()");
        goto Error;
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_column);
  return(return_value);
}


int QccWAVWaveletAnalysis3D(QccVolume volume,
                            int num_frames,
                            int num_rows,
                            int num_cols,
                            int phase_frame,
                            int phase_row,
                            int phase_col,
                            const QccWAVWavelet *wavelet)
{
  int return_value;
  int frame, row, col;
  QccVector temp_z = NULL;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if ((temp_z = QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletAnalysis3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletAnalysis2D(volume[frame],
                                num_rows,
                                num_cols,
                                phase_row,
                                phase_col,
                                wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletAnalysis3D): Error calling QccWAVWaveletAnalysis2D()");
        goto Error;
      }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        for (frame = 0; frame < num_frames; frame++)
          temp_z[frame] = volume[frame][row][col];
        
        if (QccWAVWaveletAnalysis1D(temp_z,
                                    num_frames,
                                    phase_frame,
                                    wavelet))
          {
            QccErrorAddMessage("(QccWAVWaveletAnalysis3D): Error calling QccWAVWaveletAnalysis1D()");
            goto Error;
          }
        
        for (frame = 0; frame < num_frames; frame++)
          volume[frame][row][col] = temp_z[frame];
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_z);
  return(return_value);
}


int QccWAVWaveletSynthesis3D(QccVolume volume,
                             int num_frames,
                             int num_rows,
                             int num_cols,
                             int phase_frame,
                             int phase_row,
                             int phase_col,
                             const QccWAVWavelet *wavelet)
{
  int return_value;
  int frame, row, col;
  QccVector temp_z = NULL;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if ((temp_z = QccVectorAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletSynthesis3D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        for (frame = 0; frame < num_frames; frame++)
          temp_z[frame] = volume[frame][row][col];
        
        if (QccWAVWaveletSynthesis1D(temp_z,
                                     num_frames,
                                     phase_frame,
                                     wavelet))
          {
            QccErrorAddMessage("(QccWAVWaveletSynthesis3D): Error calling QccWAVWaveletSynthesis1D()");
            goto Error;
          }
        
        for (frame = 0; frame < num_frames; frame++)
          volume[frame][row][col] = temp_z[frame];
      }
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletSynthesis2D(volume[frame],
                                 num_rows,
                                 num_cols,
                                 phase_row,
                                 phase_col,
                                 wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletSynthesis3D): Error calling QccWAVWaveletSynthesis2D()");
        goto Error;
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_z);
  return(return_value);
}
