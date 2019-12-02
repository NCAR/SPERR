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

/*
 *  This code was written by Joe Boettcher <jbb15@msstate.edu> based on
 *  the originally developed algorithm and code by Suxia Cui.
 */

#include "libQccPack.h"


#ifdef LIBQCCPACKSPIHT_H

#define QCCVIDRWMH_BLOCKSIZE 16

#define QCCVIDRWMH_WINDOWSIZE 15


typedef struct
{
  int intraframe;
  int motion_vector_bits;
  int intraframe_bits;
} QccVIDRWMHStatistics;


typedef struct
{
  int blocksize;
  int num_levels;
  QccMatrix *block;
} QccVIDRWMHBlock;


static void QccVIDRWMHBlockInitialize(QccVIDRWMHBlock *block)
{
  block->blocksize = 0;
  block->num_levels = 0;
  block->block = NULL;
}


static int QccVIDRWMHBlockAlloc(QccVIDRWMHBlock *block)
{
  if ((block->block =
       QccWAVWaveletRedundantDWT2DAlloc(block->blocksize,
                                        block->blocksize,
                                        block->num_levels)) ==
      NULL)
    {
      QccErrorAddMessage("(QccVIDRWMHBlockAlloc): Error calling QccWAVWaveletRedundantDWT2DAlloc()");
      return(1);
    }

  return(0);
}


static void QccVIDRWMHBlockFree(QccVIDRWMHBlock *block)
{
  QccWAVWaveletRedundantDWT2DFree(block->block,
                                  block->blocksize,
                                  block->num_levels);
}


static int QccVIDRWMHBlockExtract(QccVIDRWMHBlock *block,
                                  QccMatrix *frame_rdwt,
                                  int num_rows,
                                  int num_cols,
                                  int num_levels,
                                  double row,
                                  double col,
                                  int subpixel_accuracy)
{
  int num_subbands;
  int subband;
  int step;
  int row2, col2;
  int block_row, block_col;
  int extraction_row, extraction_col;

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      step = 1;
      row2 = (int)row;
      col2 = (int)col;
      break;
    case QCCVID_ME_HALFPIXEL:
      step = 2;
      row2 = (int)(row * 2);
      col2 = (int)(col * 2);
      break;
    case QCCVID_ME_QUARTERPIXEL:
      step = 4;
      row2 = (int)(row * 4);
      col2 = (int)(col * 4);
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      step = 8;
      row2 = (int)(row * 8);
      col2 = (int)(col * 8);
      break;
    default:
      QccErrorAddMessage("(QccVIDRWMHBlockExtract): Unrecognized subpixel accuracy");
      return(1);
    }

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);

  for (subband = 0; subband < num_subbands; subband++)
    for (block_row = 0; block_row < block->blocksize; block_row++)
      for (block_col = 0; block_col < block->blocksize; block_col++)
        {
          extraction_row = row2 + block_row * step;
          extraction_col = col2 + block_col * step;
          
          if (extraction_row < 0)
            extraction_row = 0;
          else
            if (extraction_row >= num_rows)
              extraction_row = num_rows - 1;
          
          if (extraction_col < 0)
            extraction_col = 0;
          else
            if (extraction_col >= num_cols)
              extraction_col = num_cols - 1;

          block->block[subband][block_row][block_col] =
            frame_rdwt[subband][extraction_row][extraction_col];
        }

  return(0);
}


static int QccVIDRWMHBlockInsert(QccVIDRWMHBlock *block,
                                 QccMatrix *frame_rdwt,
                                 int num_rows,
                                 int num_cols,
                                 int num_levels,
                                 int row,
                                 int col)
{
  int num_subbands;
  int subband;
  int block_row, block_col;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);

  for (subband = 0; subband < num_subbands; subband++)
    for (block_row = 0; block_row < block->blocksize; block_row++)
      for (block_col = 0; block_col < block->blocksize; block_col++)
        frame_rdwt[subband][block_row + row][block_col + col] =
          block->block[subband][block_row][block_col];

  return(0);
}


static double QccVIDRWMHBlockMAE(QccVIDRWMHBlock *block1,
                                 QccVIDRWMHBlock *block2,
                                 int num_levels)
{
  int num_subbands;
  int subband;
  int block_row, block_col;
  double mae = 0;
  double mae1;
  int level;
  int scale_factor;
  int end_subband;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);

  end_subband = 0;

  for (subband = num_subbands - 1; subband >= end_subband; subband--)
    {
      level =
        QccWAVSubbandPyramidCalcLevelFromSubband(subband,
                                                 num_levels);
      scale_factor = (1 << (level * 2 + 1));
      
      mae1 = 0.0;

      for (block_row = 0; block_row < block1->blocksize; block_row++)
        for (block_col = 0; block_col < block1->blocksize; block_col++)
          mae1 +=
            fabs(block1->block[subband][block_row][block_col] -
                 block2->block[subband][block_row][block_col]);

      mae += mae1 / scale_factor;
    }
  
  mae /=
    (double)(block1->blocksize * block1->blocksize * num_subbands);
  
  return(mae);
}


static void QccVIDRWMHPrintStatistics(const QccVIDRWMHStatistics
                                      *statistics)
{
  int total_bits;
  int field_width;
  
  total_bits =
    statistics->motion_vector_bits + statistics->intraframe_bits;
  
  field_width = (int)ceil(log10((double)total_bits));
  
  printf("            Frame type: ");
  if (statistics->intraframe)
    printf("I\n");
  else
    printf("P\n");
  printf("    Motion-vector bits: %*d (%5.1f%%)\n",
         field_width,
         statistics->motion_vector_bits,
         QccMathPercent(statistics->motion_vector_bits, total_bits));
  printf("       Intraframe bits: %*d (%5.1f%%)\n",
         field_width,
         statistics->intraframe_bits,
         QccMathPercent(statistics->intraframe_bits, total_bits));
  
  printf("\n");
}


static int QccVIDRWMHCreateReferenceFrame(QccMatrix reconstructed_frame,
                                          QccMatrix *reconstructed_frame_rdwt,
                                          QccMatrix *reference_frame_rdwt,
                                          int num_rows,
                                          int num_cols,
                                          int reference_num_rows,
                                          int reference_num_cols,
                                          int num_levels,
                                          int subpixel_accuracy,
                                          const QccFilter *filter1,
                                          const QccFilter *filter2,
                                          const QccFilter *filter3,
                                          const QccWAVWavelet *wavelet)
{
  int subband;
  int num_subbands;
  QccIMGImageComponent reconstructed_subband;
  QccIMGImageComponent reference_subband;

  QccIMGImageComponentInitialize(&reconstructed_subband);
  QccIMGImageComponentInitialize(&reference_subband);

  if (QccWAVWaveletRedundantDWT2D(reconstructed_frame,
                                  reconstructed_frame_rdwt,
                                  num_rows,
                                  num_cols,
                                  num_levels,
                                  wavelet))
    {
      QccErrorAddMessage("(QccVIDRWMHCreateReferenceFrame): Error calling QccWAVWaveletRedundantDWT2D()");
      return(1);
    }
  
  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);

  reconstructed_subband.num_rows = num_rows;
  reconstructed_subband.num_cols = num_cols;
  reference_subband.num_rows = reference_num_rows;
  reference_subband.num_cols = reference_num_cols;

  for (subband = 0; subband < num_subbands; subband++)
    {
      reconstructed_subband.image =
        (QccIMGImageArray)reconstructed_frame_rdwt[subband];
      reference_subband.image =
        (QccIMGImageArray)reference_frame_rdwt[subband];

      if (QccVIDMotionEstimationCreateReferenceFrame(&reconstructed_subband,
                                                     &reference_subband,
                                                     subpixel_accuracy,
                                                     filter1,
                                                     filter2,
                                                     filter3))
        {
          QccErrorAddMessage("(QccVIDRWMHCreateReferenceFrame): Error calling QccVIDMotionEstimationCreateReferenceFrame()");
          return(1);
        }
    }

  return(0);
}


static int QccVIDRWMHMotionEstimationSearch(QccMatrix *reference_frame_rdwt,
                                            int reference_num_rows,
                                            int reference_num_cols,
                                            int num_levels,
                                            QccVIDRWMHBlock *current_block,
                                            QccVIDRWMHBlock *reference_block,
                                            double search_row,
                                            double search_col,
                                            double window_size,
                                            double search_step,
                                            int subpixel_accuracy,
                                            double *mv_horizontal,
                                            double *mv_vertical)
{
  double u, v;
  double reference_frame_row, reference_frame_col;
  double current_mae;
  double min_mae = MAXDOUBLE;

  for (v = -window_size; v <= window_size; v += search_step)
    for (u = -window_size; u <= window_size; u += search_step)
      {
        reference_frame_row = search_row + v;
        reference_frame_col = search_col + u;

        if (QccVIDRWMHBlockExtract(reference_block,
                                   reference_frame_rdwt,
                                   reference_num_rows,
                                   reference_num_cols,
                                   num_levels,
                                   reference_frame_row,
                                   reference_frame_col,
                                   subpixel_accuracy))
          {
            QccErrorAddMessage("(QccVIDRWMHMotionEstimationSearch): Error calling QccVIDRWMHBlockExtract()");
            return(1);
          }
        
        current_mae =
          QccVIDRWMHBlockMAE(current_block,
                             reference_block,
                             num_levels);
        
        if (current_mae < min_mae)
          {
            min_mae = current_mae;
            *mv_horizontal = u;
            *mv_vertical = v;
          }
      }

  return(0);
}


static int QccVIDRWMHMotionEstimation(QccMatrix *current_frame_rdwt,
                                      QccMatrix *reference_frame_rdwt,
                                      int num_rows,
                                      int num_cols,
                                      int reference_num_rows,
                                      int reference_num_cols,
                                      int blocksize,
                                      int num_levels,
                                      int subpixel_accuracy,
                                      QccIMGImageComponent *horizontal_motion,
                                      QccIMGImageComponent *vertical_motion)
{
  int return_value;
  int row, col;
  double u = 0;
  double v = 0;
  QccVIDRWMHBlock current_block;
  QccVIDRWMHBlock reference_block;
  double search_row, search_col;
  double current_window_size;
  double final_search_step;
  double current_search_step;
  int mv_row, mv_col;

  QccVIDRWMHBlockInitialize(&current_block);
  QccVIDRWMHBlockInitialize(&reference_block);

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      final_search_step = 1.0;
      break;
    case QCCVID_ME_HALFPIXEL:
      final_search_step = 0.5;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      final_search_step = 0.25;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      final_search_step = 0.125;
      break;
    default:
      QccErrorAddMessage("(QccVIDRWMHMotionEstimation): Unrecognized subpixel accuracy");
      goto Error;
    }

  current_block.blocksize = blocksize;
  current_block.num_levels = num_levels;
  if (QccVIDRWMHBlockAlloc(&current_block))
    {
      QccErrorAddMessage("(QccVIDRWMHMotionEstimation): Error calling QccVIDRWMHBlockAlloc()");
      goto Error;
    }
  reference_block.blocksize = blocksize;
  reference_block.num_levels = num_levels;
  if (QccVIDRWMHBlockAlloc(&reference_block))
    {
      QccErrorAddMessage("(QccVIDRWMHMotionEstimation): Error calling QccVIDRWMHBlockAlloc()");
      goto Error;
    }

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      final_search_step = 1.0;
      break;
    case QCCVID_ME_HALFPIXEL:
      final_search_step = 0.5;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      final_search_step = 0.25;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      final_search_step = 0.125;
      break;
    default:
      QccErrorAddMessage("(QccVIDRWMHMotionEstimation): Unrecognized subpixel accuracy");
      goto Error;
    }

  for (row = 0; row < num_rows; row += blocksize)
    for (col = 0; col < num_cols; col += blocksize)
      {
        if (QccVIDRWMHBlockExtract(&current_block,
                                   current_frame_rdwt,
                                   num_rows,
                                   num_cols,
                                   num_levels,
                                   (double)row,
                                   (double)col,
                                   QCCVID_ME_FULLPIXEL))
          {
            QccErrorAddMessage("(QccVIDRWMHMotionEstimation): Error calling QccVIDRWMHBlockExtract()");
            goto Error;
          }

        mv_row = row / blocksize;
        mv_col = col / blocksize;

        horizontal_motion->image[mv_row][mv_col] = 0.0;
        vertical_motion->image[mv_row][mv_col] = 0.0;

        for (current_search_step = 1.0;
             current_search_step >= final_search_step;
             current_search_step /= 2)
          {
            search_row = row + 
              vertical_motion->image[mv_row][mv_col];
            search_col = col + 
              horizontal_motion->image[mv_row][mv_col];

            current_window_size =
              (current_search_step == 1.0) ?
              (double)QCCVIDRWMH_WINDOWSIZE : current_search_step;
            
            if (QccVIDRWMHMotionEstimationSearch(reference_frame_rdwt,
                                                 reference_num_rows,
                                                 reference_num_cols,
                                                 num_levels,
                                                 &current_block,
                                                 &reference_block,
                                                 search_row,
                                                 search_col,
                                                 current_window_size,
                                                 current_search_step,
                                                 subpixel_accuracy,
                                                 &u,
                                                 &v))
              {
                QccErrorAddMessage("(QccVIDRWMHMotionEstimation): Error calling QccVIRWMHMotionEstimationSearch()");
                goto Error;
              }
            horizontal_motion->image[mv_row][mv_col] += u;
            vertical_motion->image[mv_row][mv_col] += v;
          }
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVIDRWMHBlockFree(&current_block);
  QccVIDRWMHBlockFree(&reference_block);
  return(return_value);
}


static
int QccVIDRWMHMotionCompensation(QccMatrix *reference_frame_rdwt,
                                 QccMatrix *compensated_frame_rdwt,
                                 int num_rows,
                                 int num_cols,
                                 int reference_num_rows,
                                 int reference_num_cols,
                                 int num_levels,
                                 int blocksize,
                                 int subpixel_accuracy,
                                 QccIMGImageComponent *horizontal_motion,
                                 QccIMGImageComponent *vertical_motion)
{
  int return_value;
  int row, col;
  int mv_row, mv_col;
  double reference_row, reference_col;
  QccVIDRWMHBlock reference_block;

  QccVIDRWMHBlockInitialize(&reference_block);

  reference_block.blocksize = blocksize;
  reference_block.num_levels = num_levels;
  if (QccVIDRWMHBlockAlloc(&reference_block))
    {
      QccErrorAddMessage("(QccVIDRWMHMotionCompensation): Error calling QccVIDRWMHBlockAlloc()");
      goto Error;
    }

  for (row = 0; row < num_rows; row += blocksize)
    for (col = 0; col < num_cols; col += blocksize)
      {
        mv_row = row / blocksize;
        mv_col = col / blocksize;

        reference_row = row + vertical_motion->image[mv_row][mv_col];
        reference_col = col + horizontal_motion->image[mv_row][mv_col];

        if (QccVIDRWMHBlockExtract(&reference_block,
                                   reference_frame_rdwt,
                                   reference_num_rows,
                                   reference_num_cols,
                                   num_levels,
                                   reference_row,
                                   reference_col,
                                   subpixel_accuracy))
          {
            QccErrorAddMessage("(QccVIDRWMHMotionCompensation): Error calling QccVIDRWMHBlockExtract()");
            goto Error;
          }

        if (QccVIDRWMHBlockInsert(&reference_block,
                                  compensated_frame_rdwt,
                                  num_rows,
                                  num_cols,
                                  num_levels,
                                  row,
                                  col))
          {
            QccErrorAddMessage("(QccVIDRWMHMotionCompensation): Error calling QccVIDRWMHBlockInsert()");
            goto Error;
          }
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVIDRWMHBlockFree(&reference_block);
  return(return_value);
}


static int QccVIDRWMHEncodeHeader(QccBitBuffer *output_buffer,
                                  int num_rows,
                                  int num_cols,
                                  int start_frame_num,
                                  int end_frame_num,
                                  int num_levels,
                                  int blocksize,
                                  int target_bit_cnt)
{
  if (QccBitBufferPutInt(output_buffer, num_rows))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, start_frame_num))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, end_frame_num))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, num_levels))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, blocksize))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, target_bit_cnt))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferFlush(output_buffer))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeHeader): Error calling QccBitBufferFlush()");
      return(1);
    }
  
  return(0);
}


#if 0
static int QccWAVRWMHCorrelation(QccMatrix *current_frame_rdwt,
                                 QccMatrix *compensated_frame_rdwt,
                                 int num_rows,
                                 int num_cols,
                                 int num_levels)
{
  int num_subbands;
  int subband1;
  int subband2;
  int row, col;
  double difference1;
  double difference2;
  double correlation;
  double variance1;
  double variance2;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);

  for (subband1 = 0; subband1 < num_subbands; subband1++)
    {
      for (subband2 = 0; subband2 < num_subbands; subband2++)
        {
          correlation = 0;
          variance1 = 0;
          variance2 = 0;

          for (row = 0; row < num_rows; row++)
            for (col = 0; col < num_cols; col++)
              {
                difference1 =
                  current_frame_rdwt[subband1][row][col] -
                  compensated_frame_rdwt[subband1][row][col];
                difference2 =
                  current_frame_rdwt[subband2][row][col] -
                  compensated_frame_rdwt[subband2][row][col];
                correlation +=
                  difference1 * difference2;
                variance1 +=
                  difference1 * difference1;
                variance2 +=
                  difference2 * difference2;
              }
          
          variance1 /= num_rows * num_cols;
          variance2 /= num_rows * num_cols;
          correlation /= num_rows * num_cols;
          
          correlation /= sqrt(variance1 * variance2);
          
          printf("% 0.2f ",
                 correlation);
        }
      printf("\n");
    }

  return(0);
}
#endif


static int QccVIDRWMHDecodeFrame();

static
int QccVIDRWMHEncodeFrame(QccIMGImageComponent *current_frame,
                          QccIMGImageComponent *reconstructed_frame,
                          QccMatrix *current_frame_rdwt,
                          QccMatrix *reference_frame_rdwt,
                          int num_levels,
                          int blocksize,
                          int subpixel_accuracy,
                          const QccFilter *filter1,
                          const QccFilter *filter2,
                          const QccFilter *filter3,
                          const QccWAVWavelet *wavelet,
                          QccBitBuffer *output_buffer,
                          QccBitBuffer *input_buffer,
                          QccIMGImageComponent *motion_vector_horizontal,
                          QccIMGImageComponent *motion_vector_vertical,
                          int read_motion_vectors,
                          int target_bit_cnt,
                          QccVIDRWMHStatistics *statistics)
{
  int return_value;
  int row, col;
  int num_rows, num_cols;
  int reference_num_rows, reference_num_cols;
  int intraframe;
  int start_position;
  QccMatrix compensated_frame = NULL;
  QccMatrix *compensated_frame_rdwt = NULL;

  start_position = output_buffer->bit_cnt;
  
  num_rows = current_frame->num_rows;
  num_cols = current_frame->num_cols;

  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }
  
  intraframe =
    (motion_vector_horizontal == NULL) || (motion_vector_vertical == NULL);
  
  if (statistics != NULL)
    statistics->intraframe = intraframe;
  
  if ((compensated_frame = QccMatrixAlloc(num_rows, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error allocating memory");
      goto Error;
    }
  
  if ((compensated_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(num_rows, num_cols, num_levels)) ==
      NULL)
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccWAVWaveletRedundantDWT2DAlloc()");
      goto Error;
    }

  if (!intraframe)
    {
      if (!read_motion_vectors)
        {
          if (QccWAVWaveletRedundantDWT2D(current_frame->image,
                                          current_frame_rdwt,
                                          num_rows,
                                          num_cols,
                                          num_levels,
                                          wavelet))
            {
              QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccWAVWaveletRedundantDWT2D()");
              goto Error;
            }
          
          if (QccVIDRWMHMotionEstimation(current_frame_rdwt,
                                         reference_frame_rdwt,
                                         num_rows,
                                         num_cols,
                                         reference_num_rows,
                                         reference_num_cols,
                                         blocksize,
                                         num_levels,
                                         subpixel_accuracy,
                                         motion_vector_horizontal,
                                         motion_vector_vertical))
            {
              QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccVIDRWMHMotionEstimation()");
              goto Error;
            }
        }
      
      if (QccVIDRWMHMotionCompensation(reference_frame_rdwt,
                                       compensated_frame_rdwt,
                                       num_rows,
                                       num_cols,
                                       reference_num_rows,
                                       reference_num_cols,
                                       num_levels,
                                       blocksize,
                                       subpixel_accuracy,
                                       motion_vector_horizontal,
                                       motion_vector_vertical))
        {
          QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccVIDRWMHMotionCompensation()");
          goto Error;
        }
      
      if (QccWAVWaveletInverseRedundantDWT2D(compensated_frame_rdwt,
                                             compensated_frame,
                                             num_rows,
                                             num_cols,
                                             num_levels,
                                             wavelet))
        {
          QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccWAVWaveletInverseRedundantDWT2D()");
          goto Error;
        }     
      
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          current_frame->image[row][col] -= compensated_frame[row][col];
      
      if (!read_motion_vectors)
        if (QccVIDMotionVectorsEncode(motion_vector_horizontal,
                                      motion_vector_vertical,
                                      NULL,
                                      subpixel_accuracy,
                                      output_buffer))
          {
            QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccVIDMotionVectorsEncode()");
            goto Error;
          }
      
      if (statistics != NULL)
        statistics->motion_vector_bits =
          output_buffer->bit_cnt - start_position;
      
      if (output_buffer->bit_cnt > start_position + target_bit_cnt)
        {
          QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Rate is not sufficient to store all motion vectors for frame");
          goto Error;
        }
      
    }
  else
    if (statistics != NULL)
      statistics->motion_vector_bits = 0;
  
  if (QccSPIHTEncode(current_frame,
                     NULL,
                     output_buffer,
                     num_levels,
                     wavelet,
                     NULL,
                     start_position + target_bit_cnt,
                     1,
                     NULL))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccSPIHTEncode()");
      goto Error;
    }
  
  if (statistics != NULL)
    statistics->intraframe_bits =
      output_buffer->bit_cnt - start_position - statistics->motion_vector_bits;
  
  if (QccBitBufferFlush(output_buffer))
    {
      QccErrorAddMessage("(QccVIDRWMHkEncodeFrame): Error calling QccBitBufferFlush()");
      goto Error;
    }
  
  if (QccVIDRWMHDecodeFrame(reconstructed_frame,
                            compensated_frame_rdwt,
                            reference_frame_rdwt,
                            num_levels,
                            blocksize,
                            subpixel_accuracy,
                            filter1,
                            filter2,
                            filter3,
                            target_bit_cnt,
                            wavelet,
                            input_buffer,
                            read_motion_vectors,
                            motion_vector_horizontal,
                            motion_vector_vertical))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccVIDRWMHDecodeFrame()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(compensated_frame, num_rows);
  QccWAVWaveletRedundantDWT2DFree(compensated_frame_rdwt,
                                  num_rows, num_levels);
  return(return_value);
}


int QccVIDRWMHDecodeHeader(QccBitBuffer *input_buffer,
                           int *num_rows,
                           int *num_cols,
                           int *start_frame_num,
                           int *end_frame_num,
                           int *num_levels,
                           int *blocksize,
                           int *target_bit_cnt)
{
  int val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_rows != NULL)
    *num_rows = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRWMHHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_cols != NULL)
    *num_cols = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (start_frame_num != NULL)
    *start_frame_num = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (end_frame_num != NULL)
    *end_frame_num = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_levels != NULL)
    *num_levels = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (blocksize != NULL)
    *blocksize = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (target_bit_cnt != NULL)
    *target_bit_cnt = val;
  
  if (QccBitBufferFlush(input_buffer))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeHeader): Error calling QccBitBufferFlush()");
      return(1);
    }
  
  return(0);
}


static
int QccVIDRWMHDecodeFrame(QccIMGImageComponent *reconstructed_frame,
                          QccMatrix *compensated_frame_rdwt,
                          QccMatrix *reference_frame_rdwt,
                          int num_levels,
                          int blocksize,
                          int subpixel_accuracy,
                          const QccFilter *filter1,
                          const QccFilter *filter2,
                          const QccFilter *filter3,
                          int target_bit_cnt,
                          const QccWAVWavelet *wavelet,
                          QccBitBuffer *input_buffer,
                          int read_motion_vectors,
                          QccIMGImageComponent *motion_vector_horizontal,
                          QccIMGImageComponent *motion_vector_vertical)
{
  int return_value;
  int row, col;
  int num_rows, num_cols;
  int reference_num_rows, reference_num_cols;
  double image_mean;
  int max_coefficient_bits;
  int arithmetic_coded;
  int intraframe;
  int start_position;
  QccMatrix compensated_frame = NULL;
  
  start_position = input_buffer->bit_cnt;
  
  num_rows = reconstructed_frame->num_rows;
  num_cols = reconstructed_frame->num_cols;
  
  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDRWMHDDecodeFrame): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }
  
  if ((compensated_frame = QccMatrixAlloc(num_rows, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeFrame): Error allocating memory");
      goto Error;
    }
  
  intraframe =
    (motion_vector_horizontal == NULL) || (motion_vector_vertical == NULL);
  
  if (!intraframe)
    {
      if (!read_motion_vectors)
        if (QccVIDMotionVectorsDecode(motion_vector_horizontal,
                                      motion_vector_vertical,
                                      NULL,
                                      subpixel_accuracy,
                                      input_buffer))
          {
            QccErrorAddMessage("(QccVIDRWMHDecodeFrame): Error calling QccVIDMotionVectorsDecode()");
            goto Error;
          }
      
      if (QccVIDRWMHMotionCompensation(reference_frame_rdwt,
                                       compensated_frame_rdwt,
                                       num_rows,
                                       num_cols,
                                       reference_num_rows,
                                       reference_num_cols,
                                       num_levels,
                                       blocksize,
                                       subpixel_accuracy,
                                       motion_vector_horizontal,
                                       motion_vector_vertical))
        {
          QccErrorAddMessage("(QccVIDRWMHDecodeFrame): Error calling QccVIDRWMHMotionCompensation()");
          goto Error;
        }
      
      if (QccWAVWaveletInverseRedundantDWT2D(compensated_frame_rdwt,
                                             compensated_frame,
                                             num_rows,
                                             num_cols,
                                             num_levels,
                                             wavelet))
        {
          QccErrorAddMessage("(QccVIDRWMHDecodeFrame): Error calling QccWAVWaveletRedundantDWT2D()");
          goto Error;
        }
    }
  
  if (QccMatrixZero(reconstructed_frame->image,
                    num_rows, num_cols))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeFrame): Error calling QccMatrixZero()");
      goto Error;
    }
  
  if (QccSPIHTDecodeHeader(input_buffer,
                           &num_levels,
                           &num_rows,
                           &num_cols,
                           &image_mean,
                           &max_coefficient_bits,
                           &arithmetic_coded))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeFrame): Error calling QccSPIHTDecodeHeader()");
      goto Error;
    }
  
  if (QccSPIHTDecode(input_buffer,
                     reconstructed_frame,
                     NULL,
                     num_levels,
                     wavelet,
                     NULL,
                     image_mean,
                     max_coefficient_bits,
                     start_position + target_bit_cnt,
                     arithmetic_coded))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeFrame):Error calling QccSPIHTDecode()");
      goto Error;
    }
  
  if (QccBitBufferFlush(input_buffer))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeFrame):Error calling QccBitBufferFlush()");
      goto Error;
    }
  
  if (!intraframe)
    for (row = 0; row < num_rows; row++)
      for (col = 0; col < num_cols; col++)
        reconstructed_frame->image[row][col] += compensated_frame[row][col];
  
  if (QccVIDRWMHCreateReferenceFrame(reconstructed_frame->image,
                                     compensated_frame_rdwt,
                                     reference_frame_rdwt,
                                     num_rows,
                                     num_cols,
                                     reference_num_rows,
                                     reference_num_cols,
                                     num_levels,
                                     subpixel_accuracy,
                                     filter1,
                                     filter2,
                                     filter3,
                                     wavelet))
    {
      QccErrorAddMessage("(QccVIDRWMHDecodeFrame): Error calling QccVIDRWMHCreateReferenceFrame()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(compensated_frame, num_rows);
  return(return_value);
}


int QccVIDRWMHEncode(QccIMGImageSequence *image_sequence,
                     const QccFilter *filter1,
                     const QccFilter *filter2,
                     const QccFilter *filter3,
                     int subpixel_accuracy,
                     int blocksize,
                     QccBitBuffer *output_buffer,
                     int num_levels,
                     int target_bit_cnt,
                     const QccWAVWavelet *wavelet,
                     const QccString mv_filename,
                     int read_motion_vectors,
                     int quiet)
{
  int return_value;
  QccIMGImageComponent *current_frame;
  QccIMGImageComponent reconstructed_frame;
  QccMatrix *current_frame_rdwt = NULL;
  QccMatrix *reference_frame_rdwt = NULL;
  QccIMGImageComponent motion_vector_horizontal;
  QccIMGImageComponent motion_vector_vertical;
  int frame = 0;
  int num_rows, num_cols;
  int reference_num_rows, reference_num_cols;
  QccBitBuffer input_buffer;
  QccVIDRWMHStatistics statistics;

  QccBitBufferInitialize(&input_buffer);
  QccIMGImageComponentInitialize(&motion_vector_horizontal);
  QccIMGImageComponentInitialize(&motion_vector_vertical);
  QccIMGImageComponentInitialize(&reconstructed_frame);
  
  if (QccIMGImageSequenceStartRead(image_sequence))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccIMGImageSequenceStartRead()");
      goto Error;
    }
  
  if (QccIMGImageGetSize(&image_sequence->current_frame,
                         &num_rows, &num_cols))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccIMGImageGetSize()");
      goto Error;
    }
  
  if ((num_rows % blocksize) || (num_cols % blocksize))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Image size is not an integer multiple of block size");
      goto Error;
    }

  current_frame = &image_sequence->current_frame.Y;
  
  motion_vector_horizontal.num_rows =
    num_rows / blocksize;
  motion_vector_horizontal.num_cols =
    num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_horizontal))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  motion_vector_vertical.num_rows =
    num_rows / blocksize;
  motion_vector_vertical.num_cols =
    num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_vertical))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  
  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }

  reconstructed_frame.num_rows = num_rows;
  reconstructed_frame.num_cols = num_cols;
  if (QccIMGImageComponentAlloc(&reconstructed_frame))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  if ((current_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(num_rows,
                                        num_cols,
                                        num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccWAVWaveletRedundantDWT2DAlloc");
      goto Error;
    }
  if ((reference_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(reference_num_rows,
                                        reference_num_cols,
                                        num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccWAVWaveletRedundantDWT2DAlloc");
      goto Error;
    }
  
  QccStringCopy(input_buffer.filename, output_buffer->filename);
  input_buffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&input_buffer))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccBitBufferStart()");
      goto Error;
    }
  
  if (QccVIDRWMHEncodeHeader(output_buffer,
                             num_rows,
                             num_cols,
                             image_sequence->start_frame_num,
                             image_sequence->end_frame_num,
                             num_levels,
                             blocksize,
                             target_bit_cnt))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccVIDRWMHEncodeHeader()");
      goto Error;
    }
  
  if (QccVIDRWMHDecodeHeader(&input_buffer,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL))
    {
      QccErrorAddMessage("(QccVIDRWMHEncodeFrame): Error calling QccVIDRWMHDecodeHeader()");
      goto Error;
    }
  
  if (!quiet)
    {
      printf("===========================================================\n");
      printf("\n");
      printf("  RWMH -- encoding sequence:\n      %s\n         to\n      %s\n\n",
             image_sequence->filename,
             output_buffer->filename);
      printf("         Frame size: %d x %d\n",
             num_cols, num_rows);
      printf("        Start frame: %d\n",
             image_sequence->start_frame_num);
      printf("          End frame: %d\n\n",
             image_sequence->end_frame_num);
      printf("         Block size: %d\n\n",
             blocksize);
      printf("  Subpixel accuracy: ");
      switch (subpixel_accuracy)
        {
        case QCCVID_ME_FULLPIXEL:
          printf("full-pixel\n");
          break;
        case QCCVID_ME_HALFPIXEL:
          printf("half-pixel\n");
          break;
        case QCCVID_ME_QUARTERPIXEL:
          printf("quarter-pixel\n");
          break;
        case QCCVID_ME_EIGHTHPIXEL:
          printf("eighth-pixel\n");
          break;
        }
      printf("\n");
      printf("===========================================================\n");
      printf("\n");
    }
  
  if (!quiet)
    printf("  Frame: %d\n",
           image_sequence->start_frame_num);
  
  if (QccVIDRWMHEncodeFrame(current_frame,
                            &reconstructed_frame,
                            current_frame_rdwt,
                            reference_frame_rdwt,
                            num_levels,
                            blocksize,
                            subpixel_accuracy,
                            filter1,
                            filter2,
                            filter3,
                            wavelet,
                            output_buffer,
                            &input_buffer,
                            NULL,
                            NULL,
                            0,
                            target_bit_cnt,
                            &statistics))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccVIDRWMHEncodeFrame()");
      goto Error;
    }
  
  if (!quiet)
    QccVIDRWMHPrintStatistics(&statistics);
  
  for (frame = image_sequence->start_frame_num + 1;
       frame <= image_sequence->end_frame_num;
       frame++)
    {
      if (!quiet)
        printf("  Frame: %d\n", frame);
      
      if (QccIMGImageSequenceIncrementFrameNum(image_sequence))
        {
          QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccIMGImageSequenceIncrementFrameNum()");
          goto Error;
        }
      
      if (QccIMGImageSequenceReadFrame(image_sequence))
        {
          QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccIMGImageSequenceReadFrame()");
          goto Error;
        }
      
      if ((mv_filename != NULL) && read_motion_vectors)
        if (QccVIDMotionVectorsReadFile(&motion_vector_horizontal,
                                        &motion_vector_vertical,
                                        mv_filename,
                                        frame))
          {
            QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccVIDMotionVectorsReadFile()");
            goto Error;
          }

      if (QccVIDRWMHEncodeFrame(current_frame,
                                &reconstructed_frame,
                                current_frame_rdwt,
                                reference_frame_rdwt,
                                num_levels,
                                blocksize,
                                subpixel_accuracy,
                                filter1,
                                filter2,
                                filter3,
                                wavelet,
                                output_buffer,
                                &input_buffer,
                                &motion_vector_horizontal,
                                &motion_vector_vertical,
                                read_motion_vectors,
                                target_bit_cnt,
                                &statistics))
        {
          QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccVIDRWMHEncodeFrame()");
          goto Error;
        }
      
      if ((mv_filename != NULL) && (!read_motion_vectors))
        if (QccVIDMotionVectorsWriteFile(&motion_vector_horizontal,
                                         &motion_vector_vertical,
                                         mv_filename,
                                         frame))
          {
            QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccVIDMotionVectorsWriteFile()");
            goto Error;
          }

      if (!quiet)
        QccVIDRWMHPrintStatistics(&statistics);
    }
  
  if (QccBitBufferEnd(&input_buffer))
    {
      QccErrorAddMessage("(QccVIDRWMHEncode): Error calling QccBitBufferEnd()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&motion_vector_horizontal);
  QccIMGImageComponentFree(&motion_vector_vertical);
  QccIMGImageComponentFree(&reconstructed_frame);
  QccWAVWaveletRedundantDWT2DFree(current_frame_rdwt,
                                  num_rows,
                                  num_levels);
  QccWAVWaveletRedundantDWT2DFree(reference_frame_rdwt,
                                  reference_num_rows,
                                  num_levels);
  return(return_value);
}  


int QccVIDRWMHDecode(QccIMGImageSequence *image_sequence,
                     const QccFilter *filter1,
                     const QccFilter *filter2,
                     const QccFilter *filter3,
                     int subpixel_accuracy,
                     int blocksize,
                     QccBitBuffer *input_buffer,
                     int target_bit_cnt,
                     int num_levels,
                     const QccWAVWavelet *wavelet,
                     const QccString mv_filename,
                     int quiet)
{
  int return_value;
  int num_rows, num_cols;
  int reference_num_rows, reference_num_cols;
  QccIMGImageComponent *reconstructed_frame;
  QccMatrix *compensated_frame_rdwt = NULL;
  QccMatrix *reference_frame_rdwt = NULL;
  QccIMGImageComponent motion_vector_horizontal;
  QccIMGImageComponent motion_vector_vertical;
  int frame;
  
  QccIMGImageComponentInitialize(&motion_vector_horizontal);
  QccIMGImageComponentInitialize(&motion_vector_vertical);
  
  image_sequence->current_frame_num = image_sequence->start_frame_num;
  
  if (QccIMGImageGetSize(&image_sequence->current_frame,
                         &num_rows, &num_cols))
    {
      QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccIMGImageGetSize()");
      goto Error;
    }
  
  reconstructed_frame = &image_sequence->current_frame.Y;
  
  motion_vector_horizontal.num_rows =
    num_rows / blocksize;
  motion_vector_horizontal.num_cols =
    num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_horizontal))
    {
      QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  motion_vector_vertical.num_rows =
    num_rows / blocksize;
  motion_vector_vertical.num_cols =
    num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_vertical))
    {
      QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }

  if ((compensated_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(num_rows,
                                        num_cols,
                                        num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccWAVWaveletRedundantDWT2DAlloc");
      goto Error;
    }
  if ((reference_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(reference_num_rows,
                                        reference_num_cols,
                                        num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccWAVWaveletRedundantDWT2DAlloc");
      goto Error;
    }
  
  if (!quiet)
    {
      printf("===========================================================\n");
      printf("\n");
      printf("  RWMH -- decoding sequence:\n      %s\n         to\n      %s\n\n",
             input_buffer->filename,
             image_sequence->filename);
      printf("         Frame size: %d x %d\n",
             num_cols, num_rows);
      printf("        Start frame: %d\n",
             image_sequence->start_frame_num);
      printf("          End frame: %d\n\n",
             image_sequence->end_frame_num);
      printf("         Block size: %d\n\n",
             blocksize);
      printf("  Subpixel accuracy: ");
      switch (subpixel_accuracy)
        {
        case QCCVID_ME_FULLPIXEL:
          printf("full-pixel\n");
          break;
        case QCCVID_ME_HALFPIXEL:
          printf("half-pixel\n");
          break;
        case QCCVID_ME_QUARTERPIXEL:
          printf("quarter-pixel\n");
          break;
        case QCCVID_ME_EIGHTHPIXEL:
          printf("eighth-pixel\n");
          break;
        }
      printf("\n");
      printf("===========================================================\n");
      printf("\n");
    }
  
  if (!quiet)
    printf("  Frame %d\n",
           image_sequence->start_frame_num);
  
  if (QccVIDRWMHDecodeFrame(reconstructed_frame,
                            compensated_frame_rdwt,
                            reference_frame_rdwt,
                            num_levels,
                            blocksize,
                            subpixel_accuracy,
                            filter1,
                            filter2,
                            filter3,
                            target_bit_cnt,
                            wavelet,
                            input_buffer,
                            0,
                            NULL,
                            NULL))
    {
      QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccVIDRWMHDecodeFrame()");
      goto Error;
    }
  
  if (QccIMGImageSequenceWriteFrame(image_sequence))
    {
      QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccIMGImageSequenceWriteFrame()");
      goto Error;
    }
  
  for (frame = image_sequence->start_frame_num + 1;
       frame <= image_sequence->end_frame_num;
       frame++)
    {
      if (QccIMGImageSequenceIncrementFrameNum(image_sequence))
        {
          QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccIMGImageSequenceIncrementFrameNum()");
          goto Error;
        }
      
      if (!quiet)
        printf("  Frame %d\n", image_sequence->current_frame_num);
      
      if (mv_filename != NULL)
        if (QccVIDMotionVectorsReadFile(&motion_vector_horizontal,
                                        &motion_vector_vertical,
                                        mv_filename,
                                        frame))
          {
            QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccVIDMotionVectorsReadFile()");
            goto Error;
          }

      if (QccVIDRWMHDecodeFrame(reconstructed_frame,
                                compensated_frame_rdwt,
                                reference_frame_rdwt,
                                num_levels,
                                blocksize,
                                subpixel_accuracy,
                                filter1,
                                filter2,
                                filter3,
                                target_bit_cnt,
                                wavelet,
                                input_buffer,
                                (mv_filename != NULL),
                                &motion_vector_horizontal,
                                &motion_vector_vertical))
        {
          QccErrorAddMessage("(QccVIDRWMHDecode): Error calling QccVIDRWMHDecodeFrame()");
          goto Error;
        }
      
      if (QccIMGImageSequenceWriteFrame(image_sequence))
        {
          QccErrorAddMessage("(QccVIDRWMHDecode(): Error calling QccIMGImageSequenceWriteFrame()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&motion_vector_horizontal);
  QccIMGImageComponentFree(&motion_vector_vertical);
  QccWAVWaveletRedundantDWT2DFree(compensated_frame_rdwt,
                                  num_rows,
                                  num_levels);
  QccWAVWaveletRedundantDWT2DFree(reference_frame_rdwt,
                                  reference_num_rows,
                                  num_levels);
  return(return_value);
}

#else
int QccVIDRWMHEncode(QccIMGImageSequence *image_sequence,
                     const QccFilter *filter1,
                     const QccFilter *filter2,
                     const QccFilter *filter3,
                     int subpixel_accuracy,
                     int blocksize,
                     QccBitBuffer *output_buffer,
                     int num_levels,
                     int target_bit_cnt,
                     const QccWAVWavelet *wavelet,
                     const QccString mv_filename,
                     int read_motion_vectors,
                     int quiet)
{
  QccErrorAddMessage("(QccVIDRWMHEncode): Optional QccSPIHT module is not available -- RWMH coder is consequently not supported");
  return(1);
}


int QccVIDRWMHDecodeHeader(QccBitBuffer *input_buffer,
                           int *num_rows,
                           int *num_cols,
                           int *start_frame_num,
                           int *end_frame_num,
                           int *num_levels,
                           int *blocksize,
                           int *target_bit_cnt)
{
  QccErrorAddMessage("(QccVIDRWMHDecodeHeader): Optional QccSPIHT module is not available -- RWMH coder is consequently not supported");
  return(1);
}


int QccVIDRWMHDecode(QccIMGImageSequence *image_sequence,
                     const QccFilter *filter1,
                     const QccFilter *filter2,
                     const QccFilter *filter3,
                     int subpixel_accuracy,
                     int blocksize,
                     QccBitBuffer *input_buffer,
                     int target_bit_cnt,
                     int num_levels,
                     const QccWAVWavelet *wavelet,
                     const QccString mv_filename,
                     int quiet)
{
  QccErrorAddMessage("(QccVIDRWMHDecode): Optional QccSPIHT module is not available -- RWMH coder is consequently not supported");
  return(1);
}


#endif
