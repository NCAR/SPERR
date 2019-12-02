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


int QccWAVWaveletAnalysis1DInt(QccVectorInt signal,
                               int signal_length,
                               int phase,
                               const QccWAVWavelet *wavelet)
{
  if (signal == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if (wavelet->implementation != QCCWAVWAVELET_IMPLEMENTATION_LIFTED)
    {
      QccErrorAddMessage("(QccWAVWaveletAnalysis1DInt): Wavelet must be integer-valued lifting scheme");
      return(1);
    }
  
  if (QccWAVLiftingAnalysisInt(signal,
                               signal_length,
                               phase,
                               &wavelet->lifting_scheme,
                               wavelet->boundary))
    {
      QccErrorAddMessage("(QccWAVWaveletAnalysis1DInt): Error calling QccWAVLiftingAnalysisInt()");
      return(1);
    }

  return(0);
}


int QccWAVWaveletSynthesis1DInt(QccVectorInt signal,
                                int signal_length,
                                int phase,
                                const QccWAVWavelet *wavelet)
{
  if (signal == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if (wavelet->implementation != QCCWAVWAVELET_IMPLEMENTATION_LIFTED)
    {
      QccErrorAddMessage("(QccWAVWaveletSynthesis1DInt): Wavelet must be integer-valued lifting scheme");
      return(1);
    }
  
  if (QccWAVLiftingSynthesisInt(signal,
                                signal_length,
                                phase,
                                &wavelet->lifting_scheme,
                                wavelet->boundary))
    {
      QccErrorAddMessage("(QccWAVWaveletSynthesis1DInt): Error calling QccWAVLiftingSynthesisInt()");
      return(1);
    }

  return(0);
}


int QccWAVWaveletAnalysis2DInt(QccMatrixInt matrix,
                               int num_rows,
                               int num_cols,
                               int phase_row,
                               int phase_col,
                               const QccWAVWavelet *wavelet)
{
  int return_value;
  int row, col;
  QccVectorInt temp_column = NULL;

  if (matrix == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if ((temp_column = QccVectorIntAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletAnalysis2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }

  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletAnalysis1DInt(matrix[row],
                                   num_cols,
                                   phase_row,
                                   wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletAnalysis2DInt): Error calling QccWAVWaveletAnalysis1DInt()");
        goto Error;
      }

  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        temp_column[row] = matrix[row][col];

      if (QccWAVWaveletAnalysis1DInt(temp_column,
                                     num_rows,
                                     phase_col,
                                     wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletAnalysis2DInt): Error calling QccWAVWaveletAnalysis1DInt()");
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
  QccVectorIntFree(temp_column);
  return(return_value);
}


int QccWAVWaveletSynthesis2DInt(QccMatrixInt matrix,
                                int num_rows,
                                int num_cols,
                                int phase_row,
                                int phase_col,
                                const QccWAVWavelet *wavelet)
{
  int return_value;
  int row, col;
  QccVectorInt temp_column = NULL;

  if (matrix == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if ((temp_column = QccVectorIntAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletSynthesis2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }

  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        temp_column[row] = matrix[row][col];

      if (QccWAVWaveletSynthesis1DInt(temp_column,
                                      num_rows,
                                      phase_col,
                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletSynthesis2DInt): Error calling QccWAVWaveletSynthesis1DInt()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        matrix[row][col] = temp_column[row];
    }

  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletSynthesis1DInt(matrix[row],
                                    num_cols,
                                    phase_row,
                                    wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletSynthesis2DInt): Error calling QccWAVWaveletSynthesis1DInt()");
        goto Error;
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(temp_column);
  return(return_value);
}


int QccWAVWaveletAnalysis3DInt(QccVolumeInt volume,
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
  QccVectorInt temp_z = NULL;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if ((temp_z = QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletAnalysis3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletAnalysis2DInt(volume[frame],
                                   num_rows,
                                   num_cols,
                                   phase_row,
                                   phase_col,
                                   wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletAnalysis3DInt): Error calling QccWAVWaveletAnalysis2DInt()");
        goto Error;
      }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        for (frame = 0; frame < num_frames; frame++)
          temp_z[frame] = volume[frame][row][col];
        
        if (QccWAVWaveletAnalysis1DInt(temp_z,
                                       num_frames,
                                       phase_frame,
                                       wavelet))
          {
            QccErrorAddMessage("(QccWAVWaveletAnalysis3DInt): Error calling QccWAVWaveletAnalysis1DInt()");
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
  QccVectorIntFree(temp_z);
  return(return_value);
}


int QccWAVWaveletSynthesis3DInt(QccVolumeInt volume,
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
  QccVectorInt temp_z = NULL;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if ((temp_z = QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletSynthesis3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        for (frame = 0; frame < num_frames; frame++)
          temp_z[frame] = volume[frame][row][col];
        
        if (QccWAVWaveletSynthesis1DInt(temp_z,
                                        num_frames,
                                        phase_frame,
                                        wavelet))
          {
            QccErrorAddMessage("(QccWAVWaveletSynthesis3DInt): Error calling QccWAVWaveletSynthesis1DInt()");
            goto Error;
          }
        
        for (frame = 0; frame < num_frames; frame++)
          volume[frame][row][col] = temp_z[frame];
      }
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletSynthesis2DInt(volume[frame],
                                    num_rows,
                                    num_cols,
                                    phase_row,
                                    phase_col,
                                    wavelet))
      {
        QccErrorAddMessage("(QccWAVWaveletSynthesis3DInt): Error calling QccWAVWaveletSynthesis2DInt()");
        goto Error;
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(temp_z);
  return(return_value);
}
