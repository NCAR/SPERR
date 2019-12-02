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


static int QccWAVWaveletDWTSubbandPhaseInt(int level, int signal_origin,
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


int QccWAVWaveletDWT1DInt(QccVectorInt signal,
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
        QccWAVWaveletDWTSubbandPhaseInt(scale + 1, signal_origin,
                                        subsample_pattern);

      if (QccWAVWaveletAnalysis1DInt(signal,
                                     current_length,
                                     phase,
                                     wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletDWT1DInt): Error calling QccWAVWaveletAnalysis1DInt()");
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


int QccWAVWaveletInverseDWT1DInt(QccVectorInt signal,
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
        QccWAVWaveletDWTSubbandPhaseInt(scale + 1, signal_origin,
                                        subsample_pattern);

      if (QccWAVWaveletSynthesis1DInt(signal,
                                      current_length,
                                      phase,
                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseDWT1DInt): Error calling QccWAVWaveletSynthesis1DInt()");
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


int QccWAVWaveletDWT2DInt(QccMatrixInt matrix,
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
      phase_row = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_row,
                                                  subsample_pattern_row);
      phase_col = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_col,
                                                  subsample_pattern_col);

      if (QccWAVWaveletAnalysis2DInt(matrix,
                                     num_rows,
                                     num_cols,
                                     phase_row,
                                     phase_col,
                                     wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletDWT2DInt): Error calling QccWAVWaveletAnalysis2DInt()");
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


int QccWAVWaveletInverseDWT2DInt(QccMatrixInt matrix,
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

      phase_row = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_row,
                                                  subsample_pattern_row);
      phase_col = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_col,
                                                  subsample_pattern_col);

      if (QccWAVWaveletSynthesis2DInt(matrix,
                                      num_rows,
                                      num_cols,
                                      phase_row,
                                      phase_col,
                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseDWT2DInt): Error calling QccWAVWaveletSynthesis2DInt()");
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


int QccWAVWaveletDyadicDWT3DInt(QccVolumeInt volume,
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
      phase_frame = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_frame,
                                                    subsample_pattern_frame);
      phase_row = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_row,
                                                  subsample_pattern_row);
      phase_col = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_col,
                                                  subsample_pattern_col);
      
      if (QccWAVWaveletAnalysis3DInt(volume,
                                     num_frames,
                                     num_rows,
                                     num_cols,
                                     phase_frame,
                                     phase_row,
                                     phase_col,
                                     wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletDyadicDWT3DInt): Error calling QccWAVWaveletAnalysis3DInt()");
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


int QccWAVWaveletInverseDyadicDWT3DInt(QccVolumeInt volume,
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
      
      phase_frame = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_frame,
                                                    subsample_pattern_frame);
      phase_row = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_row,
                                                  subsample_pattern_row);
      phase_col = QccWAVWaveletDWTSubbandPhaseInt(scale + 1, origin_col,
                                                  subsample_pattern_col);

      if (QccWAVWaveletSynthesis3DInt(volume,
                                      num_frames,
                                      num_rows,
                                      num_cols,
                                      phase_frame,
                                      phase_row,
                                      phase_col,
                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInverseDyadicDWT3DInt): Error calling QccWAVWaveletSynthesis3DInt()");
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


int QccWAVWaveletPacketDWT3DInt(QccVolumeInt volume,
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
  QccVectorInt temp_z = NULL;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (temporal_num_scales > 0)
    {
      if ((temp_z = QccVectorIntAlloc(num_frames)) == NULL)
        {
          QccErrorAddMessage("(QccWAVWaveletPacketDWT3DInt): Error calling QccVectorIntAlloc()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          {
            for (frame = 0; frame < num_frames; frame++)
              temp_z[frame] = volume[frame][row][col];
            
            if (QccWAVWaveletDWT1DInt(temp_z,
                                      num_frames,
                                      origin_frame,
                                      subsample_pattern_frame,
                                      temporal_num_scales,
                                      wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletPacketDWT3DInt): Error calling QccWAVWaveletDWT1DInt()");
                goto Error;
              }
            
            for (frame = 0; frame < num_frames; frame++)
              volume[frame][row][col] = temp_z[frame];
          }
    }
  
  if (spatial_num_scales > 0)
    for (frame = 0; frame < num_frames; frame++)
      if (QccWAVWaveletDWT2DInt(volume[frame],
                                num_rows,
                                num_cols,
                                origin_row,
                                origin_col,
                                subsample_pattern_row,
                                subsample_pattern_col,
                                spatial_num_scales,
                                wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletPacketDWT3DInt): Error calling QccWAVWaveletDWT2DInt()");
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


int QccWAVWaveletInversePacketDWT3DInt(QccVolumeInt volume,
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
  QccVectorInt temp_z = NULL;
  
  if (volume == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (spatial_num_scales > 0)
    for (frame = 0; frame < num_frames; frame++)
      if (QccWAVWaveletInverseDWT2DInt(volume[frame],
                                       num_rows,
                                       num_cols,
                                       origin_row,
                                       origin_col,
                                       subsample_pattern_row,
                                       subsample_pattern_col,
                                       spatial_num_scales,
                                       wavelet))
        {
          QccErrorAddMessage("(QccWAVWaveletInversePacketDWT3DInt): Error calling QccWAVWaveletInverseDWT2DInt()");
          goto Error;
        }
  
  if (temporal_num_scales > 0)
    {
      if ((temp_z = QccVectorIntAlloc(num_frames)) == NULL)
        {
          QccErrorAddMessage("(QccWAVWaveletInversePacketDWT3DInt): Error calling QccVectorIntAlloc()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          {
            for (frame = 0; frame < num_frames; frame++)
              temp_z[frame] = volume[frame][row][col];
            
            if (QccWAVWaveletInverseDWT1DInt(temp_z,
                                             num_frames, 
                                             origin_frame,
                                             subsample_pattern_frame,
                                             temporal_num_scales,
                                             wavelet))
              {
                QccErrorAddMessage("(QccWAVWaveletInversePacketDWT3DInt): Error calling QccWAVWaveletInverseDWT1DInt()");
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
  QccVectorIntFree(temp_z);
  return(return_value);
}
