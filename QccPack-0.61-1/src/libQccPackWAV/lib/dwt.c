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


static int QccWAVWaveletDWTSubbandPhase(int level, int signal_origin,
                                        int subsample_pattern)
{
  int phase = -(signal_origin % (1 << level));
  
  phase &= (1 << (level - 1));

  phase ^= (subsample_pattern & (1 << (level - 1))) << 1;

  if (phase)
    return(QCCWAVWAVELET_PHASE_ODD);
  else
    return(QCCWAVWAVELET_PHASE_EVEN);
}


int QccWAVWaveletDWTSubbandLength(int original_length, int level,
                                  int highband, int signal_origin,
                                  int subsample_pattern)
{
  int scale;
  int length;
  int phase;

  length = original_length;

  if (!level)
    {
      length = (highband) ? 0 : original_length;
      return(length);
    }

  for (scale = 0; scale < level - 1; scale++)
    {
      phase =
        QccWAVWaveletDWTSubbandPhase(scale + 1, signal_origin,
                                     subsample_pattern);
      if (length % 2)
        length =
          (phase == QCCWAVWAVELET_PHASE_ODD) ?
          (length >> 1) : (length >> 1) + 1;
      else
        length >>= 1;
      phase >>= 1;
    }

  phase =
    QccWAVWaveletDWTSubbandPhase(level, signal_origin, subsample_pattern);

  if (length % 2)
    length =
      (((phase == QCCWAVWAVELET_PHASE_EVEN) && highband) ||
       ((phase == QCCWAVWAVELET_PHASE_ODD) && !highband)) ?
      (length >> 1) : (length >> 1) + 1;
  else
    length >>= 1;

  return(length);
}


int QccWAVWaveletDWT1D(QccVector signal,
                       int signal_length,
                       int signal_origin,
                       int subsample_pattern,
                       int num_scales,
                       const QccWAVWavelet *wavelet)
{
  int return_value;
  int scale;
  int current_length;
  int phase;

  if (signal == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if (num_scales <= 0)
    return(0);

  current_length = signal_length;

  for (scale = 0; scale < num_scales; scale++)
    {
      phase =
        QccWAVWaveletDWTSubbandPhase(scale + 1, signal_origin,
                                     subsample_pattern);

      if (QccWAVWaveletAnalysis1D(signal,
                                  current_length,
                                  phase,
                                  wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletDWT1D): Error calling QccWAVWaveletAnalysis1D()");
          goto Error;
        }

      current_length =
        QccWAVWaveletDWTSubbandLength(signal_length, scale + 1, 0,
                                      signal_origin, subsample_pattern);
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVWaveletInverseDWT1D(QccVector signal,
                              int signal_length,
                              int signal_origin,
                              int subsample_pattern,
                              int num_scales,
                              const QccWAVWavelet *wavelet)
{
  int return_value;
  int scale;
  int current_length;
  int phase;

  if (signal == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if (num_scales <= 0)
    return(0);
    
  for (scale = num_scales - 1; scale >= 0; scale--)
    {
      current_length = 
        QccWAVWaveletDWTSubbandLength(signal_length, scale,
                                      0, signal_origin,
                                      subsample_pattern);

      phase =
        QccWAVWaveletDWTSubbandPhase(scale + 1, signal_origin,
                                     subsample_pattern);

      if (QccWAVWaveletSynthesis1D(signal,
                                   current_length,
                                   phase,
                                   wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseDWT1D): Error calling QccWAVWaveletSynthesis1D()");
          goto Error;
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVWaveletDWT2D(QccMatrix matrix,
                       int num_rows,
                       int num_cols,
                       int origin_row,
                       int origin_col,
                       int subsample_pattern_row,
                       int subsample_pattern_col,
                       int num_scales,
                       const QccWAVWavelet *wavelet)
{
  int return_value;
  int scale;
  int original_num_rows = 0;
  int original_num_cols = 0;
  int phase_row;
  int phase_col;

  if (matrix == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if (num_scales <= 0)
    return(0);

  original_num_rows = num_rows;
  original_num_cols = num_cols;

  for (scale = 0; scale < num_scales; scale++)
    {
      phase_row = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_row,
                                               subsample_pattern_row);
      phase_col = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_col,
                                               subsample_pattern_col);

      if (QccWAVWaveletAnalysis2D(matrix,
                                  num_rows,
                                  num_cols,
                                  phase_row,
                                  phase_col,
                                  wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletDWT2D): Error calling QccWAVWaveletAnalysis2D()");
          goto Error;
        }

      num_rows =
        QccWAVWaveletDWTSubbandLength(original_num_rows, scale + 1,
                                      0, origin_row, subsample_pattern_row);
      num_cols =
        QccWAVWaveletDWTSubbandLength(original_num_cols, scale + 1,
                                      0, origin_col, subsample_pattern_col);
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVWaveletInverseDWT2D(QccMatrix matrix,
                              int num_rows,
                              int num_cols,
                              int origin_row,
                              int origin_col,
                              int subsample_pattern_row,
                              int subsample_pattern_col,
                              int num_scales,
                              const QccWAVWavelet *wavelet)
{
  int return_value;
  int scale;
  int original_num_rows = 0;
  int original_num_cols = 0;
  int phase_row;
  int phase_col;

  if (matrix == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if (num_scales <= 0)
    return(0);

  original_num_rows = num_rows;
  original_num_cols = num_cols;

  for (scale = num_scales - 1; scale >= 0; scale--)
    {
      num_rows =
        QccWAVWaveletDWTSubbandLength(original_num_rows, scale,
                                      0, origin_row, subsample_pattern_row);
      num_cols =
        QccWAVWaveletDWTSubbandLength(original_num_cols, scale,
                                      0, origin_col, subsample_pattern_col);

      phase_row = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_row,
                                               subsample_pattern_row);
      phase_col = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_col,
                                               subsample_pattern_col);

      if (QccWAVWaveletSynthesis2D(matrix,
                                   num_rows,
                                   num_cols,
                                   phase_row,
                                   phase_col,
                                   wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseDWT2D): Error calling QccWAVWaveletSynthesis2D()");
          goto Error;
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVWaveletDyadicDWT3D(QccVolume volume,
                             int num_frames,
                             int num_rows,
                             int num_cols,
                             int origin_frame,
                             int origin_row,
                             int origin_col,
                             int subsample_pattern_frame,
                             int subsample_pattern_row,
                             int subsample_pattern_col,
                             int num_scales,
                             const QccWAVWavelet *wavelet)
{
  int return_value;
  int scale;
  int original_num_frames = 0;
  int original_num_rows = 0;
  int original_num_cols = 0;
  int phase_frame;
  int phase_row;
  int phase_col;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (num_scales <= 0)
    return(0);
  
  original_num_frames = num_frames;
  original_num_rows = num_rows;
  original_num_cols = num_cols;
  
  for (scale = 0; scale < num_scales; scale++)
    {
      phase_frame = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_frame,
                                                 subsample_pattern_frame);
      phase_row = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_row,
                                               subsample_pattern_row);
      phase_col = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_col,
                                               subsample_pattern_col);
      
      if (QccWAVWaveletAnalysis3D(volume,
                                  num_frames,
                                  num_rows,
                                  num_cols,
                                  phase_frame,
                                  phase_row,
                                  phase_col,
                                  wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletDyadicDWT3D): Error calling QccWAVWaveletAnalysis3D()");
          goto Error;
        }
      
      num_frames =
        QccWAVWaveletDWTSubbandLength(original_num_frames, scale + 1,
                                      0, origin_frame,
                                      subsample_pattern_frame);
      num_rows =
        QccWAVWaveletDWTSubbandLength(original_num_rows, scale + 1,
                                      0, origin_row,
                                      subsample_pattern_row);
      num_cols =
        QccWAVWaveletDWTSubbandLength(original_num_cols, scale + 1,
                                      0, origin_col,
                                      subsample_pattern_col);
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVWaveletInverseDyadicDWT3D(QccVolume volume,
                                    int num_frames,
                                    int num_rows,
                                    int num_cols,
                                    int origin_frame,
                                    int origin_row,
                                    int origin_col,
                                    int subsample_pattern_frame,
                                    int subsample_pattern_row,
                                    int subsample_pattern_col,
                                    int num_scales,
                                    const QccWAVWavelet *wavelet)
{
  int return_value;
  int scale;
  int original_num_frames = 0;
  int original_num_rows = 0;
  int original_num_cols = 0;
  int phase_frame;
  int phase_row;
  int phase_col;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (num_scales <= 0)
    return(0);
  
  original_num_frames = num_frames;
  original_num_rows = num_rows;
  original_num_cols = num_cols;
  
  for (scale = num_scales - 1; scale >= 0; scale--)
    {
      num_frames =
        QccWAVWaveletDWTSubbandLength(original_num_frames, scale,
                                      0, origin_frame,
                                      subsample_pattern_frame);
      num_rows =
        QccWAVWaveletDWTSubbandLength(original_num_rows, scale,
                                      0, origin_row,
                                      subsample_pattern_row);
      num_cols =
        QccWAVWaveletDWTSubbandLength(original_num_cols, scale,
                                      0, origin_col,
                                      subsample_pattern_col);
      
      phase_frame = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_frame,
                                                 subsample_pattern_frame);
      phase_row = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_row,
                                               subsample_pattern_row);
      phase_col = QccWAVWaveletDWTSubbandPhase(scale + 1, origin_col,
                                               subsample_pattern_col);

      if (QccWAVWaveletSynthesis3D(volume,
                                   num_frames,
                                   num_rows,
                                   num_cols,
                                   phase_frame,
                                   phase_row,
                                   phase_col,
                                   wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseDyadicDWT3D): Error calling QccWAVWaveletSynthesis3D()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVWaveletPacketDWT3D(QccVolume volume,
                             int num_frames,
                             int num_rows,
                             int num_cols,
                             int origin_frame,
                             int origin_row,
                             int origin_col,
                             int subsample_pattern_frame,
                             int subsample_pattern_row,
                             int subsample_pattern_col,
                             int temporal_num_scales,
                             int spatial_num_scales,
                             const QccWAVWavelet *wavelet)
{
  int return_value;
  int frame, row, col;
  QccVector temp_z = NULL;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (temporal_num_scales > 0)
    {
      if ((temp_z = QccVectorAlloc(num_frames)) == NULL)
        {
          QccErrorAddMessage("(QccWAVWaveletPacketDWT3D): Error calling QccVectorAlloc()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          {
            for (frame = 0; frame < num_frames; frame++)
              temp_z[frame] = volume[frame][row][col];
            
            if (QccWAVWaveletDWT1D(temp_z,
                                   num_frames,
                                   origin_frame,
                                   subsample_pattern_frame,
                                   temporal_num_scales,
                                   wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletPacketDWT3D): Error calling QccWAVWaveletDWT1D()");
                goto Error;
              }
            
            for (frame = 0; frame < num_frames; frame++)
              volume[frame][row][col] = temp_z[frame];
          }
    }
  
  if (spatial_num_scales > 0)
    for (frame = 0; frame < num_frames; frame++)
      if (QccWAVWaveletDWT2D(volume[frame],
                             num_rows,
                             num_cols,
                             origin_row,
                             origin_col,
                             subsample_pattern_row,
                             subsample_pattern_col,
                             spatial_num_scales,
                             wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletPacketDWT3D): Error calling QccWAVWaveletDWT2D()");
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


int QccWAVWaveletInversePacketDWT3D(QccVolume volume,
                                    int num_frames,
                                    int num_rows,
                                    int num_cols,
                                    int origin_frame,
                                    int origin_row,
                                    int origin_col,
                                    int subsample_pattern_frame,
                                    int subsample_pattern_row,
                                    int subsample_pattern_col,
                                    int temporal_num_scales,
                                    int spatial_num_scales,
                                    const QccWAVWavelet *wavelet)
{
  int return_value;
  int frame, row, col;
  QccVector temp_z = NULL;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (spatial_num_scales > 0)
    for (frame = 0; frame < num_frames; frame++)
      if (QccWAVWaveletInverseDWT2D(volume[frame],
                                    num_rows,
                                    num_cols,
                                    origin_row,
                                    origin_col,
                                    subsample_pattern_row,
                                    subsample_pattern_col,
                                    spatial_num_scales,
                                    wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInversePacketDWT3D): Error calling QccWAVWaveletInverseDWT2D()");
          goto Error;
        }
  
  if (temporal_num_scales > 0)
    {
      if ((temp_z = QccVectorAlloc(num_frames)) == NULL)
        {
          QccErrorAddMessage("(QccWAVWaveletInversePacketDWT3D): Error calling QccVectorAlloc()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          {
            for (frame = 0; frame < num_frames; frame++)
              temp_z[frame] = volume[frame][row][col];
            
            if (QccWAVWaveletInverseDWT1D(temp_z,
                                          num_frames, 
                                          origin_frame,
                                          subsample_pattern_frame,
                                          temporal_num_scales,
                                          wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInversePacketDWT3D): Error calling QccWAVWaveletInverseDWT1D()");
                goto Error;
              }
            
            for (frame = 0; frame < num_frames; frame++)
              volume[frame][row][col] = temp_z[frame];
          }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_z);
  return(return_value);
}
