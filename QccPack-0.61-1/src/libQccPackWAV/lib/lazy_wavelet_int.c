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


int QccWAVWaveletLWTInt(const QccVectorInt input_signal,
                        QccVectorInt output_signal,
                        int signal_length,
                        int signal_origin,
                        int subsample_pattern)
{
  int index;
  int midpoint;
  int phase = (signal_origin % 2) ^ (subsample_pattern & 1);

  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);

  if (signal_length < 2)
    {
      QccVectorIntCopy(output_signal, input_signal, signal_length);
      return(0);
    }

  midpoint = 
    QccWAVWaveletDWTSubbandLength(signal_length, 1, 0,
                                  signal_origin, subsample_pattern);

  if (!phase)
    {
      for (index = 0; index < signal_length; index += 2)
        output_signal[index >> 1] = input_signal[index];
      
      for (index = 1; index < signal_length; index += 2)
        output_signal[midpoint + (index >> 1)] = input_signal[index];
    }
  else
    {
      for (index = 1; index < signal_length; index += 2)
        output_signal[index >> 1] = input_signal[index];
      
      for (index = 0; index < signal_length; index += 2)
        output_signal[midpoint + (index >> 1)] = input_signal[index];
    }

  return(0);
}


int QccWAVWaveletInverseLWTInt(const QccVectorInt input_signal,
                               QccVectorInt output_signal,
                               int signal_length,
                               int signal_origin,
                               int subsample_pattern)
{
  int index;
  int midpoint;
  int phase = (signal_origin % 2) ^ (subsample_pattern & 1);

  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);

  if (signal_length < 2)
    {
      QccVectorIntCopy(output_signal, input_signal, signal_length);
      return(0);
    }

  midpoint = 
    QccWAVWaveletDWTSubbandLength(signal_length, 1, 0,
                                  signal_origin, subsample_pattern);

  if (!phase)
    {
      for (index = 0; index < signal_length; index += 2)
        output_signal[index] = input_signal[index >> 1];
      
      for (index = 1; index < signal_length; index += 2)
        output_signal[index] = input_signal[midpoint + (index >> 1)];
    }
  else
    {
      for (index = 1; index < signal_length; index += 2)
        output_signal[index] = input_signal[index >> 1];
      
      for (index = 0; index < signal_length; index += 2)
        output_signal[index] = input_signal[midpoint + (index >> 1)];
    }

  return(0);
}


int QccWAVWaveletLWT2DInt(const QccMatrixInt input_matrix,
                          QccMatrixInt output_matrix,
                          int num_rows,
                          int num_cols,
                          int origin_row,
                          int origin_col,
                          int subsample_pattern_row,
                          int subsample_pattern_col)
{
  int return_value;
  int row, col;
  QccVectorInt input_column = NULL;
  QccVectorInt output_column = NULL;

  if ((input_column = QccVectorIntAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletLWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_column = QccVectorIntAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletLWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }

  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletLWTInt(input_matrix[row],
                            output_matrix[row],
                            num_cols,
                            origin_row,
                            subsample_pattern_row))
      {
        QccErrorAddMessage("(QccWAVWaveletLWT2DInt): Error calling QccWAVWaveletLWTInt()");
        goto Error;
      }

  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        input_column[row] = output_matrix[row][col];

      if (QccWAVWaveletLWTInt(input_column,
                              output_column,
                              num_rows,
                              origin_col,
                              subsample_pattern_col))
        {
          QccErrorAddMessage("(QccWAVWaveletLWT2DInt): Error calling QccWAVWaveletLWTInt()");
          goto Error;
        }

      for (row = 0; row < num_rows; row++)
        output_matrix[row][col] = output_column[row];
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_column);
  QccVectorIntFree(output_column);
  return(return_value);
}


int QccWAVWaveletInverseLWT2DInt(const QccMatrixInt input_matrix,
                                 QccMatrixInt output_matrix,
                                 int num_rows,
                                 int num_cols,
                                 int origin_row,
                                 int origin_col,
                                 int subsample_pattern_row,
                                 int subsample_pattern_col)
{
  int return_value;
  int row, col;
  QccVectorInt input_column = NULL;
  QccVectorInt output_column = NULL;
  
  if ((input_column = QccVectorIntAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseLWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_column = QccVectorIntAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseLWT2DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    if (QccWAVWaveletInverseLWTInt(input_matrix[row],
                                   output_matrix[row],
                                   num_cols,
                                   origin_row,
                                   subsample_pattern_row))
      {
        QccErrorAddMessage("(QccWAVWaveletInverseLWT2DInt): Error calling QccWAVWaveletInverseLWTInt()");
        goto Error;
      }

  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        input_column[row] = output_matrix[row][col];
      
      if (QccWAVWaveletInverseLWTInt(input_column,
                                     output_column,
                                     num_rows,
                                     origin_col,
                                     subsample_pattern_col))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseLWT2DInt): Error calling QccWAVWaveletInverseLWTInt()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        output_matrix[row][col] = output_column[row];
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_column);
  QccVectorIntFree(output_column);
  return(return_value);
}


int QccWAVWaveletLWT3DInt(const QccVolumeInt input_volume,
                          QccVolumeInt output_volume,
                          int num_frames,
                          int num_rows,
                          int num_cols,
                          int origin_frame,
                          int origin_row,
                          int origin_col,
                          int subsample_pattern_frame,
                          int subsample_pattern_row,
                          int subsample_pattern_col)
{
  int return_value;
  int frame, row, col;
  QccVectorInt input_vector = NULL;
  QccVectorInt output_vector = NULL;

  if ((input_vector = QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletLWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_vector = QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletLWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletLWT2DInt(input_volume[frame],
                              output_volume[frame],
                              num_rows,
                              num_cols,
                              origin_row,
                              origin_col,
                              subsample_pattern_row,
                              subsample_pattern_col))
      {
        QccErrorAddMessage("(QccWAVWaveletLWT3DInt): Error calling QccWAVWaveletLWT2DInt()");
        goto Error;
      }

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        for (frame = 0; frame < num_frames; frame++)
          input_vector[frame] = output_volume[frame][row][col];
        
        if (QccWAVWaveletLWTInt(input_vector,
                                output_vector,
                                num_frames,
                                origin_frame,
                                subsample_pattern_frame))
          {
            QccErrorAddMessage("(QccWAVWaveletLWT3DInt): Error calling QccWAVWaveletLWTInt()");
            goto Error;
          }
        
        for (frame = 0; frame < num_frames; frame++)
          output_volume[frame][row][col] = output_vector[frame];
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_vector);
  QccVectorIntFree(output_vector);
  return(return_value);
}


int QccWAVWaveletInverseLWT3DInt(const QccVolumeInt input_volume,
                                 QccVolumeInt output_volume,
                                 int num_frames,
                                 int num_rows,
                                 int num_cols,
                                 int origin_frame,
                                 int origin_row,
                                 int origin_col,
                                 int subsample_pattern_frame,
                                 int subsample_pattern_row,
                                 int subsample_pattern_col)
{
  int return_value;
  int frame, row, col;
  QccVectorInt input_vector = NULL;
  QccVectorInt output_vector = NULL;
  
  if ((input_vector = QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseLWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((output_vector = QccVectorIntAlloc(num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletInverseLWT3DInt): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccWAVWaveletInverseLWT2DInt(input_volume[frame],
                                     output_volume[frame],
                                     num_rows,
                                     num_cols,
                                     origin_row,
                                     origin_col,
                                     subsample_pattern_row,
                                     subsample_pattern_col))
      {
        QccErrorAddMessage("(QccWAVWaveletInverseLWT3DInt): Error calling QccWAVWaveletInverseLWT2DInt()");
        goto Error;
      }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        for (frame = 0; frame < num_frames; frame++)
          input_vector[frame] = output_volume[frame][row][col];
        
        if (QccWAVWaveletInverseLWTInt(input_vector,
                                       output_vector,
                                       num_frames,
                                       origin_frame,
                                       subsample_pattern_frame))
          {
            QccErrorAddMessage("(QccWAVWaveletInverseLWT3DInt): Error calling QccWAVWaveletInverseLWTInt()");
            goto Error;
          }
        
        for (frame = 0; frame < num_frames; frame++)
          output_volume[frame][row][col] = output_vector[frame];
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorIntFree(input_vector);
  QccVectorIntFree(output_vector);
  return(return_value);
}
