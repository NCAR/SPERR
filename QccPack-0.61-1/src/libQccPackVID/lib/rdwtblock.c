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


#define QCCVIDRDWTBLOCK_WINDOWSIZE 15


typedef struct
{
  int blocksize;
  QccMatrix block;
} QccVIDRDWTBlockBlock;


typedef struct
{
  int intraframe;
  int motion_vector_bits;
  int intraframe_bits;
} QccVIDRDWTBlockStatistics;


static void QccVIDRDWTBlockBlockInitialize(QccVIDRDWTBlockBlock *block)
{
  block->blocksize = 0;
  block->block = NULL;
}


static int QccVIDRDWTBlockBlockAlloc(QccVIDRDWTBlockBlock *block)
{
  if ((block->block =
       QccMatrixAlloc(block->blocksize, block->blocksize)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRDWTBlockBlockAlloc): Error calling QccMatrixAlloc()");
      return(1);
    }

  return(0);
}


static void QccVIDRDWTBlockBlockFree(QccVIDRDWTBlockBlock *block)
{
  QccMatrixFree(block->block, block->blocksize);
}


static void QccVIDRDWTBlockPrintStatistics(const QccVIDRDWTBlockStatistics
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


static double QccVIDRDWTBlockMAE(QccVIDRDWTBlockBlock *block1,
                                 QccVIDRDWTBlockBlock *block2,
                                 int num_levels)
{
  int row, col;
  double mae = 0;
  int level;
  int index_skip = 1;
  double scale_factor;

  for (level = 1; level <= num_levels; level++)
    {
      index_skip = (1 << level);
      scale_factor = 1.0 / index_skip;

      for (row = 0; row < (block1->blocksize / index_skip);
           row++)
        for (col = 0; col < (block1->blocksize / index_skip);
             col++)
          {
            //HL
            mae += scale_factor *
              fabs(block1->block
                   [row][block1->blocksize / index_skip + col] -
                   block2->block
                   [row][block1->blocksize / index_skip + col]);
            //LH
            mae += scale_factor *
              fabs(block1->block
                   [block1->blocksize / index_skip + row][col] -
                   block2->block
                   [block1->blocksize / index_skip + row][col]); 
            //HH
            mae += scale_factor *
              fabs(block1->block
                   [block1->blocksize / index_skip + row]
                   [block1->blocksize / index_skip + col] -
                   block2->block
                   [block1->blocksize / index_skip + row]
                   [block1->blocksize / index_skip + col]);
          }
    }
  
  //LL  
  scale_factor = 1.0 / index_skip;

  for (row = 0; row < (block1->blocksize / index_skip); row++)
    for (col = 0; col < (block1->blocksize / index_skip); col++)
      mae += scale_factor *
        fabs(block1->block[row][col] - block2->block[row][col]);
  
  return(mae);
}


static double QccVIDRDWTBlockExtractCoefficient(QccWAVSubbandPyramid
                                                *subband_pyramid,
                                                int row,
                                                int col)
{
  if (row < 0)
    row = 0;
  else
    if (row >= subband_pyramid->num_rows)
      row = subband_pyramid->num_rows - 1;
  if (col < 0)
    col = 0;
  else
    if (col >= subband_pyramid->num_cols)
      col = subband_pyramid->num_cols - 1;

  return(subband_pyramid->matrix[row][col]);
}


static int QccVIDRDWTBlockExtractBlock(QccWAVSubbandPyramid *subband_pyramid,
                                       QccVIDRDWTBlockBlock *wavelet_block,
                                       int ref_offset_row, 
                                       int ref_offset_col,
                                       int num_rows, 
                                       int num_cols,
                                       int num_levels)
{
  int row, col;
  int offset_row, offset_col;
  int level;
  int index_skip;
  int pattern;
  int row_pattern, col_pattern;
  int v, h;
  
  index_skip = (1 << num_levels);
  offset_row = ref_offset_row / index_skip;
  offset_col = ref_offset_col / index_skip;
  row_pattern = QccMathModulus(ref_offset_row, index_skip);
  col_pattern = QccMathModulus(ref_offset_col, index_skip);
  
  for (level = 1; level <= num_levels; level++)
    {
      index_skip = (1 << level);
      offset_row = ref_offset_row / index_skip;
      offset_col = ref_offset_col / index_skip;
      pattern = (1 << (level - 1));
      v = (row_pattern & pattern) / pattern;
      h = (col_pattern & pattern) / pattern;
      
      for (row = 0; row < (wavelet_block->blocksize / index_skip);
           row++)
	for (col = 0; col < (wavelet_block->blocksize / index_skip);
             col++)
          {
	    //HL
	    wavelet_block->block
              [row][wavelet_block->blocksize / index_skip + col] =
              QccVIDRDWTBlockExtractCoefficient(subband_pyramid,
                                                offset_row + row,
                                                num_cols / index_skip +
                                                offset_col + col + h);
	    //LH
	    wavelet_block->block
              [wavelet_block->blocksize / index_skip + row][col] =
	      QccVIDRDWTBlockExtractCoefficient(subband_pyramid,
                                                num_rows / index_skip +
                                                offset_row + row + v,
                                                offset_col + col);
	    //HH
	    wavelet_block->block
              [wavelet_block->blocksize / index_skip + row]
              [wavelet_block->blocksize / index_skip + col] =
	      QccVIDRDWTBlockExtractCoefficient(subband_pyramid,
                                                num_rows / index_skip +
                                                offset_row + row + v,
                                                num_cols / index_skip +
                                                offset_col + col + h);
	  }
    }
  
  //LL
  for (row = 0; row < (wavelet_block->blocksize / index_skip); row++)
    for (col = 0; col < (wavelet_block->blocksize / index_skip); col++)
      wavelet_block->block[row][col] =
        QccVIDRDWTBlockExtractCoefficient(subband_pyramid,
                                          offset_row + row,
                                          offset_col + col);
  
  return(0);
}


static int QccVIDRDWTBlockInsertBlock(QccWAVSubbandPyramid *subband_pyramid,
                                      QccVIDRDWTBlockBlock *wavelet_block,
                                      int ref_offset_row, 
                                      int ref_offset_col,
                                      int num_rows,
                                      int num_cols,
                                      int num_levels)
{
  int row, col;
  int offset_row, offset_col;
  int level;
  int index_skip;
  
  index_skip = (1 << num_levels);
  offset_row = ref_offset_row / index_skip;
  offset_col = ref_offset_col / index_skip;

  for (level = 1; level <= num_levels; level++)
    {
      index_skip = (1 << level);
      offset_row = ref_offset_row / index_skip;
      offset_col = ref_offset_col / index_skip;
      
      for (row = 0; row < (wavelet_block->blocksize / index_skip);
           row++)
	for (col = 0; col < (wavelet_block->blocksize / index_skip);
             col++)
          {
	    //HL
	    subband_pyramid->matrix
              [offset_row + row][num_cols / index_skip + offset_col + col] =
	      wavelet_block->block
              [row][wavelet_block->blocksize / index_skip + col];
	    //LH
	    subband_pyramid->matrix
              [num_rows / index_skip + offset_row + row][offset_col + col] = 
	      wavelet_block->block
              [wavelet_block->blocksize / index_skip + row][col];
	    //HH
	    subband_pyramid->matrix
              [num_rows / index_skip + offset_row + row]
              [num_cols / index_skip + offset_col + col] =
	      wavelet_block->block
              [wavelet_block->blocksize / index_skip + row]
              [wavelet_block->blocksize / index_skip + col];
	  }
    }
  
  //LL
  for (row = 0; row < (wavelet_block->blocksize / index_skip); row++)
    for (col = 0; col < (wavelet_block->blocksize / index_skip); col++)
      subband_pyramid->matrix[offset_row + row][offset_col + col] =
        wavelet_block->block[row][col];
  
  return(0);
}


static QccWAVSubbandPyramid ****QccVIDRDWTBlockRDWTPhasesAlloc(int num_levels,
                                                               int num_rows,
                                                               int num_cols,
                                                               int subpixel_accuracy)
{
  QccWAVSubbandPyramid ****rdwt_phases = NULL;
  int num_phases;
  int phase_row, phase_col;
  int num_subpixels;
  int subpixel_row, subpixel_col;
  
  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      num_subpixels = 1;
      break;
    case QCCVID_ME_HALFPIXEL:
      num_subpixels = 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      num_subpixels = 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      num_subpixels = 8;
      break;
    default:
      QccErrorAddMessage("(QccVIDRDWTBlockRDWTPhasesAlloc): Unrecognized subpixel accuracy");
      return(NULL);
    }

  num_phases = (1 << num_levels);
  
  if ((rdwt_phases =
       (QccWAVSubbandPyramid ****)calloc(num_subpixels,
                                         sizeof(QccWAVSubbandPyramid ***))) ==
      NULL)
    {
      QccErrorAddMessage("(QccVIDRDWTBlockRDWTPhasesAlloc): Error allocating memory");
      return(NULL);
    }

  for (subpixel_row = 0; subpixel_row < num_subpixels; subpixel_row++)
    {
      if ((rdwt_phases[subpixel_row] =
           (QccWAVSubbandPyramid ***)calloc(num_subpixels,
                                            sizeof(QccWAVSubbandPyramid **)))
          == NULL)
        {
          QccErrorAddMessage("(QccVIDRDWTBlockRDWTPhasesAlloc): Error allocating memory");
          return(NULL);
        }
      
      for (subpixel_col = 0; subpixel_col < num_subpixels; subpixel_col++)
        {
          if ((rdwt_phases[subpixel_row][subpixel_col] =
               (QccWAVSubbandPyramid **)calloc(num_phases,
                                               sizeof(QccWAVSubbandPyramid *)))
              == NULL)
            {
              QccErrorAddMessage("(QccVIDRDWTBlockRDWTPhasesAlloc): Error allocating memory");
              return(NULL);
            }
          
          for (phase_row = 0; phase_row < num_phases; phase_row++)
            if ((rdwt_phases[subpixel_row][subpixel_col][phase_row] =
                 (QccWAVSubbandPyramid *)calloc(num_phases,
                                                sizeof(QccWAVSubbandPyramid)))
                == NULL)
              {
                QccErrorAddMessage("(QccVIDRDWTBlockRDWTPhasesAlloc): Error allocating memory");
                return(NULL);
              }
        }
    }
  
  for (subpixel_row = 0; subpixel_row < num_subpixels; subpixel_row++)
    for (subpixel_col = 0; subpixel_col < num_subpixels; subpixel_col++)
      for (phase_row = 0; phase_row < num_phases; phase_row++)
        for (phase_col = 0; phase_col < num_phases; phase_col++)
          {
            QccWAVSubbandPyramidInitialize(&rdwt_phases
                                           [subpixel_row][subpixel_col]
                                           [phase_row][phase_col]);
            rdwt_phases
              [subpixel_row][subpixel_col][phase_row][phase_col].num_rows =
              num_rows;
            rdwt_phases
              [subpixel_row][subpixel_col][phase_row][phase_col].num_cols =
              num_cols;
            rdwt_phases
              [subpixel_row][subpixel_col][phase_row][phase_col].num_levels =
              num_levels;
            if (QccWAVSubbandPyramidAlloc(&rdwt_phases
                                          [subpixel_row][subpixel_col]
                                          [phase_row][phase_col]))
              {
                QccErrorAddMessage("(QccVIDRDWTBlockRDWTPhasesAlloc): Error calling QccWAVSubbandPyramidAlloc()");
                return(NULL);
              }
          }
  
  return(rdwt_phases);
}


static void QccVIDRDWTBlockRDWTPhasesFree(QccWAVSubbandPyramid ****rdwt_phases,
                                          int num_levels,
                                          int subpixel_accuracy)
{
  int num_subpixels;
  int subpixel_row, subpixel_col;
  int num_phases;
  int phase_row, phase_col;
  
  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      num_subpixels = 1;
      break;
    case QCCVID_ME_HALFPIXEL:
      num_subpixels = 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      num_subpixels = 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      num_subpixels = 8;
      break;
    default:
      return;
    }

  num_phases = (1 << num_levels);
  
  if (rdwt_phases != NULL)
    {
      for (subpixel_row = 0; subpixel_row < num_subpixels; subpixel_row++)
        if (rdwt_phases[subpixel_row] != NULL)
          {
            for (subpixel_col = 0; subpixel_col < num_subpixels;
                 subpixel_col++)
              if (rdwt_phases[subpixel_row][subpixel_col] != NULL)
                {
                  for (phase_row = 0; phase_row < num_phases; phase_row++)
                    if (rdwt_phases[subpixel_row][subpixel_col][phase_row]
                        != NULL)
                      {
                        for (phase_col = 0; phase_col < num_phases;
                             phase_col++)
                          QccWAVSubbandPyramidFree(&rdwt_phases
                                                   [subpixel_row][subpixel_col]
                                                   [phase_row][phase_col]);
                        QccFree(rdwt_phases[subpixel_row][subpixel_col]
                                [phase_row]);
                      }
                  QccFree(rdwt_phases[subpixel_row][subpixel_col]);
                }
            QccFree(rdwt_phases[subpixel_row]);
          }
      QccFree(rdwt_phases);  
    }
}


static int QccVIDRDWTBlockExtractPhases(QccMatrix *rdwt,
                                        QccWAVSubbandPyramid ****phases,
                                        int num_rows,
                                        int num_cols,
                                        int reference_num_rows,
                                        int reference_num_cols,
                                        int num_levels,
                                        int subpixel_accuracy,
                                        const QccWAVWavelet *wavelet)
{
  int return_value;
  int num_subpixels;
  int subpixel_row, subpixel_col;
  int num_phases;
  int phase_row, phase_col;
  QccMatrix *rdwt2 = NULL;
  int num_subbands;
  int subband;
  int row, col;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);

  if ((rdwt2 =
       QccWAVWaveletRedundantDWT2DAlloc(num_rows, num_cols, num_levels)) ==
      NULL)
    {
      QccErrorAddMessage("(QccWAVRDWTBlockExtractPhases): Error calling QccWAVWaveletRedundantDWT2DAlloc()");
      goto Error;
    }

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      num_subpixels = 1;
      break;
    case QCCVID_ME_HALFPIXEL:
      num_subpixels = 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      num_subpixels = 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      num_subpixels = 8;
      break;
    default:
      QccErrorAddMessage("(QccVIDRDWTBlockExtractPhases): Unrecognized subpixel accuracy");
      goto Error;
    }

  num_phases = (1 << num_levels);
  
  for (subpixel_row = 0; subpixel_row < num_subpixels; subpixel_row++)
    for (subpixel_col = 0; subpixel_col < num_subpixels; subpixel_col++)
      {
        for (subband = 0; subband < num_subbands; subband++)
          for (row = 0; row < num_rows; row++)
            for (col = 0; col < num_cols; col++)
              rdwt2[subband][row][col] =
                rdwt[subband]
                [row * num_subpixels + subpixel_row]
                [col * num_subpixels + subpixel_col];

        for (phase_row = 0; phase_row < num_phases; phase_row++)
          for (phase_col = 0; phase_col < num_phases; phase_col++)
            {
              phases[subpixel_row][subpixel_col]
                [phase_row][phase_col].subsample_pattern_row = phase_row;
              phases[subpixel_row][subpixel_col]
                [phase_row][phase_col].subsample_pattern_col = phase_col;
              
              if (QccWAVSubbandPyramidRedundantDWTSubsample(rdwt2,
                                                            &phases
                                                            [subpixel_row]
                                                            [subpixel_col]
                                                            [phase_row]
                                                            [phase_col],
                                                            wavelet))
                {
                  QccErrorAddMessage("(QccWAVRDWTBlockExtractPhases): Error calling QccWAVSubbandPyramidRedundantDWTSubsample()");
                  goto Error;
                }
            }
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVWaveletRedundantDWT2DFree(rdwt2, num_rows, num_levels);
  return(return_value);
}


static int QccVIDRDWTBlockCreateReferenceFrame(QccMatrix
                                               *reconstructed_frame_rdwt,
                                               QccMatrix
                                               *reference_frame_rdwt,
                                               int num_rows,
                                               int num_cols,
                                               int reference_num_rows,
                                               int reference_num_cols,
                                               int num_levels,
                                               int subpixel_accuracy,
                                               const QccFilter *filter1,
                                               const QccFilter *filter2,
                                               const QccFilter *filter3)
{
  int subband;
  int num_subbands;
  QccIMGImageComponent reconstructed_subband;
  QccIMGImageComponent reference_subband;

  QccIMGImageComponentInitialize(&reconstructed_subband);
  QccIMGImageComponentInitialize(&reference_subband);

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
          QccErrorAddMessage("(QccVIDRDWTBlockCreateReferenceFrame): Error calling QccVIDMotionEstimationCreateReferenceFrame()");
          return(1);
        }
    }

  return(0);
}


static int QccVIDRDWTBlockMotionEstimationSearch(QccWAVSubbandPyramid
                                                 ****reference_frame_phases,
                                                 QccVIDRDWTBlockBlock
                                                 *current_block,
                                                 QccVIDRDWTBlockBlock
                                                 *reference_block,
                                                 int num_rows,
                                                 int num_cols,
                                                 int num_levels,
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
  int subsample_pattern_row, subsample_pattern_col;
  int num_phases;
  double current_mae;
  double min_mae = MAXDOUBLE;
  int integer_row, integer_col;
  int subpixel_row, subpixel_col;
  int num_subpixels;
  
  num_phases = (1 << num_levels);
  
  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      num_subpixels = 1;
      break;
    case QCCVID_ME_HALFPIXEL:
      num_subpixels = 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      num_subpixels = 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      num_subpixels = 8;
      break;
    default:
      QccErrorAddMessage("(QccVIDRDWTBlockMotionEstimationSearch): Unrecognized subpixel accuracy");
      return(1);
    }
  
  for (v = -window_size; v <= window_size; v += search_step)
    for (u = -window_size; u <= window_size; u += search_step)
      {
        reference_frame_row = search_row + v;
        reference_frame_col = search_col + u;
        
        integer_row = (int)floor(reference_frame_row);
        integer_col = (int)floor(reference_frame_col);
        subpixel_row =
          (int)((reference_frame_row - integer_row) * num_subpixels);
        subpixel_col =
          (int)((reference_frame_col - integer_col) * num_subpixels);
        
        subsample_pattern_row = QccMathModulus(integer_row, num_phases);
        subsample_pattern_col = QccMathModulus(integer_col, num_phases);
        
        if (QccVIDRDWTBlockExtractBlock(&reference_frame_phases
                                        [subpixel_row][subpixel_col]
                                        [subsample_pattern_row]
                                        [subsample_pattern_col],
                                        reference_block,
                                        integer_row,
                                        integer_col,
                                        num_rows,
                                        num_cols,
                                        num_levels))
          {
            QccErrorAddMessage("(QccVIDRDWTBlockMotionEstimationSearch): Error calling QccVIDRDWTBlockExtractBlock()");
            return(1);
          }
        
        current_mae =
          QccVIDRDWTBlockMAE(current_block,
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


static int QccVIDRDWTBlockMotionEstimation(QccWAVSubbandPyramid
                                           ****reference_frame_phases,
                                           QccWAVSubbandPyramid
                                           *current_subband_pyramid,
                                           QccIMGImageComponent
                                           *horizontal_motion,
                                           QccIMGImageComponent
                                           *vertical_motion,
                                           int blocksize,
                                           int subpixel_accuracy)
{
  int row, col;
  int num_rows, num_cols;
  int num_levels;
  double u = 0;
  double v = 0;
  QccVIDRDWTBlockBlock current_block;
  QccVIDRDWTBlockBlock reference_block;
  int return_value;
  double search_row, search_col;
  double current_window_size;
  double final_search_step;
  double current_search_step;
  int mv_row, mv_col;
  
  QccVIDRDWTBlockBlockInitialize(&current_block);
  QccVIDRDWTBlockBlockInitialize(&reference_block);
  
  current_block.blocksize = blocksize;
  if (QccVIDRDWTBlockBlockAlloc(&current_block))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockMotionEstimation): Error calling QccVIDRDWTBlockBlockAlloc()");
      goto Error;
    }
  reference_block.blocksize = blocksize;
  if (QccVIDRDWTBlockBlockAlloc(&reference_block))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockMotionEstimation): Error calling QccVIDRDWTBlockBlockAlloc()");
      goto Error;
    }
  
  num_rows = current_subband_pyramid->num_rows;
  num_cols = current_subband_pyramid->num_cols;
  num_levels = current_subband_pyramid->num_levels;
  
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
      QccErrorAddMessage("(QccVIDRDWTBlockMotionEstimation): Unrecognized subpixel accuracy");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row += blocksize)
    for (col = 0; col < num_cols; col += blocksize)
      {
        if (QccVIDRDWTBlockExtractBlock(current_subband_pyramid,
                                        &current_block,
                                        row,
                                        col,
                                        num_rows,
                                        num_cols,
                                        num_levels))
	  {
            QccErrorAddMessage("(QccVIDRDWTBlockMotionEstimation): Error calling QccVIDRDWTBlockExtractBlock()");
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
              (double)QCCVIDRDWTBLOCK_WINDOWSIZE : current_search_step;
            
            if (QccVIDRDWTBlockMotionEstimationSearch(reference_frame_phases,
                                                      &current_block,
                                                      &reference_block,
                                                      num_rows,
                                                      num_cols,
                                                      num_levels,
                                                      search_row,
                                                      search_col,
                                                      current_window_size,
                                                      current_search_step,
                                                      subpixel_accuracy,
                                                      &u,
                                                      &v))
              {
                QccErrorAddMessage("(QccVIDRDWTBlockMotionEstimation): Error calling QccVIDRDWTBlockMotionEstimationSearch()");
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
  QccVIDRDWTBlockBlockFree(&current_block);
  QccVIDRDWTBlockBlockFree(&reference_block);
  return(return_value);
}


static int QccVIDRDWTBlockMotionCompensation(QccWAVSubbandPyramid
                                             ****reference_frame_phases,
                                             QccWAVSubbandPyramid
                                             *current_subband_pyramid,
                                             QccIMGImageComponent
                                             *horizontal_motion,
                                             QccIMGImageComponent
                                             *vertical_motion,
                                             int blocksize,
                                             int subpixel_accuracy)
{
  int row, col;
  int num_rows, num_cols;
  int num_levels;
  int num_phases;
  int subsample_pattern_row, subsample_pattern_col;
  double reference_frame_row, reference_frame_col;
  int integer_row, integer_col;
  int subpixel_row, subpixel_col;
  int num_subpixels;
  QccVIDRDWTBlockBlock reference_block;
  QccWAVSubbandPyramid predicted_frame;
  int return_value;

  QccVIDRDWTBlockBlockInitialize(&reference_block);
  QccWAVSubbandPyramidInitialize(&predicted_frame);

  reference_block.blocksize = blocksize;
  if (QccVIDRDWTBlockBlockAlloc(&reference_block))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Error calling QccVIDRDWTBlockBlockAlloc()");
      goto Error;
    }

  num_rows = current_subband_pyramid->num_rows;
  num_cols = current_subband_pyramid->num_cols;
  num_levels = current_subband_pyramid->num_levels;
  num_phases = (1 << num_levels);

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      num_subpixels = 1;
      break;
    case QCCVID_ME_HALFPIXEL:
      num_subpixels = 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      num_subpixels = 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      num_subpixels = 8;
      break;
    default:
      QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Unrecognized subpixel accuracy");
      return(1);
    }

  predicted_frame.num_levels = num_levels;
  predicted_frame.num_rows = num_rows;
  predicted_frame.num_cols = num_cols;
  if (QccWAVSubbandPyramidAlloc(&predicted_frame))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }

  for (row = 0; row < num_rows; row += blocksize)
    for (col = 0; col < num_cols; col += blocksize)
      {
        reference_frame_col =
          horizontal_motion->image
          [row / blocksize]
          [col / blocksize] + col;
        reference_frame_row = 
          vertical_motion->image
          [row / blocksize]
          [col / blocksize] + row;

        integer_row = (int)floor(reference_frame_row);
        integer_col = (int)floor(reference_frame_col);
        subpixel_row =
          (int)((reference_frame_row - integer_row) * num_subpixels);
        subpixel_col =
          (int)(fabs(reference_frame_col - integer_col) * num_subpixels);

        subsample_pattern_row = QccMathModulus(integer_row, num_phases);
        subsample_pattern_col = QccMathModulus(integer_col, num_phases);
	
        if (QccVIDRDWTBlockExtractBlock(&reference_frame_phases
                                        [subpixel_row][subpixel_col]
                                        [subsample_pattern_row]
                                        [subsample_pattern_col],
                                        &reference_block,
                                        integer_row,
                                        integer_col,
                                        num_rows,
                                        num_cols,
                                        num_levels))
	  {
            QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Error calling ExtractWaveletBlock()");
	    goto Error;
	  }
        
        if (QccVIDRDWTBlockInsertBlock(&predicted_frame,
                                       &reference_block,
                                       row,
                                       col,
                                       num_rows,
                                       num_cols,
                                       num_levels))
	  {
            QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Error calling QccVIDRDWTBlockInsertBlock()");
	    goto Error;
	  }
      }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      current_subband_pyramid->matrix[row][col] -=
        predicted_frame.matrix[row][col];
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVIDRDWTBlockBlockFree(&reference_block);
  QccWAVSubbandPyramidFree(&predicted_frame);
  return(return_value);
}


static int QccVIDRDWTBlockInverseMotionCompensation(QccWAVSubbandPyramid
                                                    ****reference_frame_phases,
                                                    QccWAVSubbandPyramid
                                                    *current_subband_pyramid,
                                                    QccIMGImageComponent
                                                    *horizontal_motion,
                                                    QccIMGImageComponent
                                                    *vertical_motion,
                                                    int blocksize,
                                                    int subpixel_accuracy)
{
  int row, col;
  int num_rows, num_cols;
  int num_levels;
  int num_phases;
  int subsample_pattern_row, subsample_pattern_col;
  double reference_frame_row, reference_frame_col;
  int integer_row, integer_col;
  int subpixel_row, subpixel_col;
  int num_subpixels;
  QccVIDRDWTBlockBlock reference_block;
  QccWAVSubbandPyramid predicted_frame;
  int return_value;

  QccVIDRDWTBlockBlockInitialize(&reference_block);
  QccWAVSubbandPyramidInitialize(&predicted_frame);

  reference_block.blocksize = blocksize;
  if (QccVIDRDWTBlockBlockAlloc(&reference_block))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Error calling QccVIDRDWTBlockBlockAlloc()");
      goto Error;
    }

  num_rows = current_subband_pyramid->num_rows;
  num_cols = current_subband_pyramid->num_cols;
  num_levels = current_subband_pyramid->num_levels;
  num_phases = (1 << num_levels);

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      num_subpixels = 1;
      break;
    case QCCVID_ME_HALFPIXEL:
      num_subpixels = 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      num_subpixels = 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      num_subpixels = 8;
      break;
    default:
      QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Unrecognized subpixel accuracy");
      return(1);
    }

  predicted_frame.num_levels = num_levels;
  predicted_frame.num_rows = num_rows;
  predicted_frame.num_cols = num_cols;
  if (QccWAVSubbandPyramidAlloc(&predicted_frame))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }

  for (row = 0; row < num_rows; row += blocksize)
    for (col = 0; col < num_cols; col += blocksize)
      {
        reference_frame_col =
          horizontal_motion->image
          [row / blocksize]
          [col / blocksize] + col;
        reference_frame_row = 
          vertical_motion->image
          [row / blocksize]
          [col / blocksize] + row;

        integer_row = (int)floor(reference_frame_row);
        integer_col = (int)floor(reference_frame_col);
        subpixel_row =
          (int)((reference_frame_row - integer_row) * num_subpixels);
        subpixel_col =
          (int)(fabs(reference_frame_col - integer_col) * num_subpixels);

        subsample_pattern_row = QccMathModulus(integer_row, num_phases);
        subsample_pattern_col = QccMathModulus(integer_col, num_phases);
	
        if (QccVIDRDWTBlockExtractBlock(&reference_frame_phases
                                        [subpixel_row][subpixel_col]
                                        [subsample_pattern_row]
                                        [subsample_pattern_col],
                                        &reference_block,
                                        integer_row,
                                        integer_col,
                                        num_rows,
                                        num_cols,
                                        num_levels))
	  {
            QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Error calling ExtractWaveletBlock()");
	    goto Error;
	  }
        
        if (QccVIDRDWTBlockInsertBlock(&predicted_frame,
                                       &reference_block,
                                       row,
                                       col,
                                       num_rows,
                                       num_cols,
                                       num_levels))
	  {
            QccErrorAddMessage("(QccVIDRDWTBlockInverseMotionCompensation): Error calling QccVIDRDWTBlockInsertBlock()");
	    goto Error;
	  }
      }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      current_subband_pyramid->matrix[row][col] +=
        predicted_frame.matrix[row][col];
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVIDRDWTBlockBlockFree(&reference_block);
  QccWAVSubbandPyramidFree(&predicted_frame);
  return(return_value);
}


static int QccVIDRDWTBlockEncodeHeader(QccBitBuffer *output_buffer,
                                       int num_rows,
                                       int num_cols,
                                       int start_frame_num,
                                       int end_frame_num,
                                       int blocksize,
                                       int num_levels,
                                       int target_bit_cnt)
{
  if (QccBitBufferPutInt(output_buffer, num_rows))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }

  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }

  if (QccBitBufferPutInt(output_buffer, start_frame_num))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }

  if (QccBitBufferPutInt(output_buffer, end_frame_num))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }

  if (QccBitBufferPutInt(output_buffer, blocksize))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }

  if (QccBitBufferPutInt(output_buffer, num_levels))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }

  if (QccBitBufferPutInt(output_buffer, target_bit_cnt))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }

  if (QccBitBufferFlush(output_buffer))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeHeader): Error calling QccBitBufferFlush()");
      return(1);
    }

  return(0);
}


static int QccVIDRDWTBlockDecodeFrame();
                                      
static int QccVIDRDWTBlockEncodeFrame(QccWAVSubbandPyramid *current_frame,
                                      QccMatrix *reconstructed_frame_rdwt,
                                      QccMatrix *reference_frame_rdwt,
                                      QccWAVSubbandPyramid
                                      ****reference_frame_phases,
                                      int blocksize,
                                      int num_levels,
                                      int subpixel_accuracy,
                                      const QccWAVWavelet *wavelet,
                                      QccBitBuffer *output_buffer,
                                      QccBitBuffer *input_buffer,
                                      QccVIDMotionVectorsTable *mvd_table,
                                      QccIMGImageComponent
                                      *motion_vector_horizontal,
                                      QccIMGImageComponent
                                      *motion_vector_vertical,
                                      int read_motion_vectors,
                                      int target_bit_cnt,
                                      QccVIDRDWTBlockStatistics *statistics,
                                      const QccFilter *filter1,
                                      const QccFilter *filter2,
                                      const QccFilter *filter3)
{
  int return_value;
  int intraframe;
  double image_mean;
  int start_position;

  start_position = output_buffer->bit_cnt;

  intraframe =
    (motion_vector_horizontal == NULL) || (motion_vector_vertical == NULL);

  if (statistics != NULL)
    statistics->intraframe = intraframe;

  if (intraframe)
    if (QccWAVSubbandPyramidSubtractMean(current_frame,
                                         &image_mean,
                                         NULL))
      {
        QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccWAVSubbandPyramidSubtractMean()");
        goto Error;
      }

  if (QccWAVSubbandPyramidDWT(current_frame,
                              num_levels,
                              wavelet))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccWAVSubbandPyramidDWT()");
      goto Error;
    }
  
  if (!intraframe)
    {
      if (!read_motion_vectors)
        {
          if (QccVIDRDWTBlockMotionEstimation(reference_frame_phases,
                                              current_frame,
                                              motion_vector_horizontal,
                                              motion_vector_vertical,
                                              blocksize,
                                              subpixel_accuracy))
            {
              QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccVIDRDWTBlockMotionEstimation()");
              goto Error;
            }
          
          if (QccVIDMotionVectorsEncode(motion_vector_horizontal,
                                        motion_vector_vertical,
                                        NULL,
                                        subpixel_accuracy,
                                        output_buffer))
            {
              QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccVIDMotionVectorsEncode()");
              goto Error;
            }
        }      
      
      if (QccVIDRDWTBlockMotionCompensation(reference_frame_phases,
                                            current_frame,
                                            motion_vector_horizontal,
                                            motion_vector_vertical,
                                            blocksize,
                                            subpixel_accuracy))
	{
	  QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccVIDRDWTBlockMotionCompensation()");
	  goto Error;
	}
      
      if (statistics != NULL)
        statistics->motion_vector_bits =
          output_buffer->bit_cnt - start_position;

      if (output_buffer->bit_cnt > start_position + target_bit_cnt)
	{
	  QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Rate is not sufficient to store all motion vectors for frame");
	  goto Error;
	}

      image_mean = 0;
    }
  else
    if (statistics != NULL)
      statistics->motion_vector_bits = 0;

  if (QccSPIHTEncode2(current_frame,
                      NULL,
                      image_mean,
                      output_buffer,
                      start_position + target_bit_cnt,
                      1,
                      NULL))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccSPIHTEncode2()");
      goto Error;
    }
  
  if (statistics != NULL)
    statistics->intraframe_bits =
      output_buffer->bit_cnt - start_position - statistics->motion_vector_bits;

  if (QccBitBufferFlush(output_buffer))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccBitBufferFlush()");
      goto Error;
    }

  if (QccVIDRDWTBlockDecodeFrame(current_frame,
                                 reconstructed_frame_rdwt,
                                 reference_frame_rdwt,
                                 reference_frame_phases,
                                 blocksize,
                                 num_levels,
                                 subpixel_accuracy,
                                 target_bit_cnt,
                                 wavelet,
                                 input_buffer,
                                 mvd_table,
                                 read_motion_vectors,
                                 motion_vector_horizontal,
                                 motion_vector_vertical,
                                 filter1,
                                 filter2,
                                 filter3))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccVIDRDWTBlockDecodeFrame()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccVIDRDWTBlockDecodeHeader(QccBitBuffer *input_buffer,
                                int *num_rows,
                                int *num_cols,
                                int *start_frame_num,
                                int *end_frame_num,
                                int *blocksize,
                                int *num_levels,
                                int *target_bit_cnt)
{
  int val;

  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_rows != NULL)
    *num_rows = val;

  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_cols != NULL)
    *num_cols = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (start_frame_num != NULL)
    *start_frame_num = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (end_frame_num != NULL)
    *end_frame_num = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (blocksize != NULL)
    *blocksize = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_levels != NULL)
    *num_levels = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (target_bit_cnt != NULL)
    *target_bit_cnt = val;
  
  if (QccBitBufferFlush(input_buffer))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeHeader): Error calling QccBitBufferFlush()");
      return(1);
    }

  return(0);
}


static int QccVIDRDWTBlockDecodeFrame(QccWAVSubbandPyramid *current_frame,
                                      QccMatrix *reconstructed_frame_rdwt,
                                      QccMatrix *reference_frame_rdwt,
                                      QccWAVSubbandPyramid
                                      ****reference_frame_phases,
                                      int blocksize,
                                      int num_levels,
                                      int subpixel_accuracy,
                                      int target_bit_cnt,
                                      const QccWAVWavelet *wavelet,
                                      QccBitBuffer *input_buffer,
                                      const QccVIDMotionVectorsTable
                                      *mvd_table,
                                      int read_motion_vectors,
                                      QccIMGImageComponent
                                      *motion_vector_horizontal,
                                      QccIMGImageComponent
                                      *motion_vector_vertical,
                                      const QccFilter *filter1,
                                      const QccFilter *filter2,
                                      const QccFilter *filter3)
{
  int return_value;
  int num_rows;
  int num_cols;
  int reference_num_rows;
  int reference_num_cols;
  double image_mean;
  int max_coefficient_bits;
  int arithmetic_coded;
  int intraframe;
  int start_position;

  start_position = input_buffer->bit_cnt;

  num_rows = current_frame->num_rows;
  num_cols = current_frame->num_cols;
  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }
  
  intraframe =
    (motion_vector_horizontal == NULL) || (motion_vector_vertical == NULL);

  if ((!intraframe) && (!read_motion_vectors))
    if (QccVIDMotionVectorsDecode(motion_vector_horizontal,
                                  motion_vector_vertical,
                                  NULL,
                                  subpixel_accuracy,
                                  input_buffer))
      {
        QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccVIDMotionVectorsDecode()");
        goto Error;
      }

  current_frame->num_levels = num_levels;
  if (QccMatrixZero(current_frame->matrix,
                    num_rows, num_cols))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccMatrixZero()");
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
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccSPIHTDecodeHeader()");
      goto Error;
    }

  if (QccSPIHTDecode2(input_buffer,
                      current_frame,
                      NULL,
                      max_coefficient_bits,
                      start_position + target_bit_cnt,
                      arithmetic_coded))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame):Error calling QccSPIHTDecode2()");
      goto Error;
    }
  
  if (QccBitBufferFlush(input_buffer))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame):Error calling QccBitBufferFlush()");
      goto Error;
    }

  if (!intraframe)
    if (QccVIDRDWTBlockInverseMotionCompensation(reference_frame_phases,
                                                 current_frame,
                                                 motion_vector_horizontal,
                                                 motion_vector_vertical,
                                                 blocksize,
                                                 subpixel_accuracy))
      {
	QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccVIDRDWTBlockInverseMotionCompensation()");
        goto Error;
      }
  
  if (QccWAVSubbandPyramidInverseDWT(current_frame,
                                     wavelet))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccWAVSubbandPyramidInverseDWT()");
      return(1);
    }
  
  if (intraframe)
    if (QccWAVSubbandPyramidAddMean(current_frame,
                                    image_mean))
      {
        QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccWAVSubbandPyramidAddMean()");
        return(1);
      }

  if (QccWAVWaveletRedundantDWT2D(current_frame->matrix,
				  reconstructed_frame_rdwt,
				  num_rows,
				  num_cols,
				  num_levels,
                                  wavelet))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccWAVWaveletRedundantDWT2D()");
      goto Error;
    }
  
  if (QccVIDRDWTBlockCreateReferenceFrame(reconstructed_frame_rdwt,
                                          reference_frame_rdwt,
                                          num_rows,
                                          num_cols,
                                          reference_num_rows,
                                          reference_num_cols,
                                          num_levels,
                                          subpixel_accuracy,
                                          filter1,
                                          filter2,
                                          filter3))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccVIDRDWTBlockCreateReferenceFrame()");
      goto Error;
    }

  if (QccVIDRDWTBlockExtractPhases(reference_frame_rdwt,
                                   reference_frame_phases,
                                   num_rows,
                                   num_cols,
                                   reference_num_rows,
                                   reference_num_cols,
                                   num_levels,
                                   subpixel_accuracy,
                                   wavelet))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecodeFrame): Error calling QccVIDRDWTBlockExtractPhases()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccVIDRDWTBlockEncode(QccIMGImageSequence *image_sequence,
                          const QccFilter *filter1,
                          const QccFilter *filter2,
                          const QccFilter *filter3,
                          int subpixel_accuracy,
                          QccBitBuffer *output_buffer,
                          int blocksize,
                          int num_levels,
                          int target_bit_cnt,
                          const QccWAVWavelet *wavelet,
                          const QccString mv_filename,
                          int read_motion_vectors,
                          int quiet)
{
  int return_value;
  QccWAVSubbandPyramid current_frame;
  QccMatrix *reconstructed_frame_rdwt = NULL;
  QccMatrix *reference_frame_rdwt = NULL;
  QccWAVSubbandPyramid ****reference_frame_phases = NULL;
  QccIMGImageComponent motion_vector_horizontal;
  QccIMGImageComponent motion_vector_vertical;
  QccVIDMotionVectorsTable mvd_table;
  int frame = 0;
  int num_rows, num_cols;
  int reference_num_rows, reference_num_cols;
  QccBitBuffer input_buffer;
  QccVIDRDWTBlockStatistics statistics;

  QccBitBufferInitialize(&input_buffer);
  QccWAVSubbandPyramidInitialize(&current_frame);
  QccIMGImageComponentInitialize(&motion_vector_horizontal);
  QccIMGImageComponentInitialize(&motion_vector_vertical);
  QccVIDMotionVectorsTableInitialize(&mvd_table);

  if (QccIMGImageSequenceStartRead(image_sequence))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccIMGImageSequenceStartRead()");
      goto Error;
    }

  if (QccIMGImageGetSize(&image_sequence->current_frame,
                         &num_rows, &num_cols))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccIMGImageGetSize()");
      goto Error;
    }
  
  if ((num_rows % blocksize) || (num_cols % blocksize))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Image size is not an integer multiple of block size");
      goto Error;
    }

  if (blocksize < (1 << num_levels))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Block size is too small for specified number of levels");
      goto Error;
    }

  current_frame.num_rows = num_rows;
  current_frame.num_cols = num_cols;
  current_frame.num_levels = 0;
  current_frame.matrix = image_sequence->current_frame.Y.image;

  motion_vector_horizontal.num_rows =
    num_rows / blocksize;
  motion_vector_horizontal.num_cols =
    num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_horizontal))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  motion_vector_vertical.num_rows =
    num_rows / blocksize;
  motion_vector_vertical.num_cols =
    num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_vertical))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }

  if ((reconstructed_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(num_rows,
                                        num_cols,
                                        num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccWAVWaveletRedundantDWT2DAlloc()");
      goto Error;
    }
  
  if ((reference_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(reference_num_rows,
                                        reference_num_cols,
                                        num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccWAVWaveletRedundantDWT2DAlloc()");
      goto Error;
    }
  
  if ((reference_frame_phases =
       QccVIDRDWTBlockRDWTPhasesAlloc(num_levels,
                                      num_rows,
                                      num_cols,
                                      subpixel_accuracy)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccVIDRDWTBlockRDWTPhasesAlloc()");
      goto Error;
    }

  QccStringCopy(input_buffer.filename, output_buffer->filename);
  input_buffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&input_buffer))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccBitBufferStart()");
      goto Error;
    }
  
  if (QccVIDRDWTBlockEncodeHeader(output_buffer,
                                  num_rows,
                                  num_cols,
                                  image_sequence->start_frame_num,
                                  image_sequence->end_frame_num,
                                  blocksize,
                                  num_levels,
                                  target_bit_cnt))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccVIDRDWTBlockEncodeHeader()");
      goto Error;
    }
  
  if (QccVIDRDWTBlockDecodeHeader(&input_buffer,
                                  NULL, NULL, NULL, NULL, NULL, NULL, NULL))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncodeFrame): Error calling QccVIDRDWTBlockDecodeHeader()");
      goto Error;
    }
  
  if (QccVIDMotionVectorsTableCreate(&mvd_table))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccVIDMotionVectorsTableCreate()");
      goto Error;
    }
  
  if (!quiet)
    {
      printf("===========================================================\n");
      printf("\n");
      printf("  RDWT Block -- encoding sequence:\n      %s\n         to\n      %s\n\n",
             image_sequence->filename,
             output_buffer->filename);
      printf("   Frame size: %d x %d\n",
             num_cols, num_rows);
      printf("  Start frame: %d\n",
             image_sequence->start_frame_num);
      printf("    End frame: %d\n\n",
             image_sequence->end_frame_num);
      printf("===========================================================\n");
      printf("\n");
    }

  if (!quiet)
    printf("  Frame: %d\n",
           image_sequence->start_frame_num);

  if (QccVIDRDWTBlockEncodeFrame(&current_frame,
                                 reconstructed_frame_rdwt,
                                 reference_frame_rdwt,
                                 reference_frame_phases,
                                 blocksize,
                                 num_levels,
                                 subpixel_accuracy,
                                 wavelet,
                                 output_buffer,
                                 &input_buffer,
                                 &mvd_table,
                                 NULL,
                                 NULL,
                                 0,
                                 target_bit_cnt,
                                 &statistics,
                                 filter1,
                                 filter2,
                                 filter3))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccVIDRDWTBlockEncodeFrame()");
      goto Error;
    }
  
  if (!quiet)
    QccVIDRDWTBlockPrintStatistics(&statistics);

  for (frame = image_sequence->start_frame_num + 1;
       frame <= image_sequence->end_frame_num;
       frame++)
    {
      if (!quiet)
        printf("  Frame: %d\n", frame);
      
      if (QccIMGImageSequenceIncrementFrameNum(image_sequence))
	{
	  QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccIMGImageSequenceIncrementFrameNum()");
	  goto Error;
	}
      
      if (QccIMGImageSequenceReadFrame(image_sequence))
	{
	  QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccIMGImageSequenceReadFrame()");
	  goto Error;
	}
      
      if ((mv_filename != NULL) && read_motion_vectors)
        if (QccVIDMotionVectorsReadFile(&motion_vector_horizontal,
                                        &motion_vector_vertical,
                                        mv_filename,
                                        frame))
          {
            QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccVIDMotionVectorsReadFile()");
            goto Error;
          }

      if (QccVIDRDWTBlockEncodeFrame(&current_frame,
                                     reconstructed_frame_rdwt,
                                     reference_frame_rdwt,
                                     reference_frame_phases,
                                     blocksize,
                                     num_levels,
                                     subpixel_accuracy,
                                     wavelet,
                                     output_buffer,
                                     &input_buffer,
                                     &mvd_table,
                                     &motion_vector_horizontal,
                                     &motion_vector_vertical,
                                     read_motion_vectors,
                                     target_bit_cnt,
                                     &statistics,
                                     filter1,
                                     filter2,
                                     filter3))
	{
	  QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccVIDRDWTBlockEncodeFrame()");
	  goto Error;
	}

      if ((mv_filename != NULL) && (!read_motion_vectors))
        if (QccVIDMotionVectorsWriteFile(&motion_vector_horizontal,
                                         &motion_vector_vertical,
                                         mv_filename,
                                         frame))
          {
            QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccVIDMotionVectorsWriteFile()");
            goto Error;
          }

      if (!quiet)
        QccVIDRDWTBlockPrintStatistics(&statistics);
    }
  
  if (QccBitBufferEnd(&input_buffer))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockEncode): Error calling QccBitBufferEnd()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&motion_vector_horizontal);
  QccIMGImageComponentFree(&motion_vector_vertical);
  QccWAVWaveletRedundantDWT2DFree(reconstructed_frame_rdwt,
                                  num_rows, num_levels);
  QccWAVWaveletRedundantDWT2DFree(reference_frame_rdwt,
                                  reference_num_rows, num_levels);
  QccVIDRDWTBlockRDWTPhasesFree(reference_frame_phases,
                                num_levels,
                                subpixel_accuracy);
  QccVIDMotionVectorsTableFree(&mvd_table);
  return(return_value);
}  


int QccVIDRDWTBlockDecode(QccIMGImageSequence *image_sequence,
                          const QccFilter *filter1,
                          const QccFilter *filter2,
                          const QccFilter *filter3,
                          int subpixel_accuracy,
                          QccBitBuffer *input_buffer,
                          int target_bit_cnt,
                          int blocksize,
                          int num_levels,
                          const QccWAVWavelet *wavelet,
                          const QccString mv_filename,
                          int quiet)
{
  int return_value;
  int num_rows, num_cols;
  int reference_num_rows, reference_num_cols;
  QccWAVSubbandPyramid current_frame;
  QccMatrix *reconstructed_frame_rdwt = NULL;
  QccMatrix *reference_frame_rdwt = NULL;
  QccWAVSubbandPyramid ****reference_frame_phases = NULL;
  QccIMGImageComponent motion_vector_horizontal;
  QccIMGImageComponent motion_vector_vertical;
  QccVIDMotionVectorsTable mvd_table;
  int frame;

  QccWAVSubbandPyramidInitialize(&current_frame);
  QccIMGImageComponentInitialize(&motion_vector_horizontal);
  QccIMGImageComponentInitialize(&motion_vector_vertical);
  QccVIDMotionVectorsTableInitialize(&mvd_table);
  
  image_sequence->current_frame_num = image_sequence->start_frame_num;

  if (QccIMGImageGetSize(&image_sequence->current_frame,
                         &num_rows, &num_cols))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccIMGImageGetSize()");
      goto Error;
    }

  current_frame.num_rows = num_rows;
  current_frame.num_cols = num_cols;
  current_frame.num_levels = num_levels;
  current_frame.matrix = image_sequence->current_frame.Y.image;

  motion_vector_horizontal.num_rows =
    num_rows / blocksize;
  motion_vector_horizontal.num_cols =
    num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_horizontal))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  motion_vector_vertical.num_rows =
    num_rows / blocksize;
  motion_vector_vertical.num_cols =
    num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_vertical))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }

  if ((reconstructed_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(num_rows,
                                        num_cols,
                                        num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccWAVWaveletRedundantDWT2DAlloc()");
      goto Error;
    }
  
  if ((reference_frame_rdwt =
       QccWAVWaveletRedundantDWT2DAlloc(reference_num_rows,
                                        reference_num_cols,
                                        num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccWAVWaveletRedundantDWT2DAlloc()");
      goto Error;
    }
  
  if ((reference_frame_phases =
       QccVIDRDWTBlockRDWTPhasesAlloc(num_levels,
                                      num_rows,
                                      num_cols,
                                      subpixel_accuracy)) == NULL)
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccWAVRDWTRDWTPhasesAlloc()");
      goto Error;
    }

  if (QccVIDMotionVectorsTableCreate(&mvd_table))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccVIDMotionVectorsTableCreate()");
      goto Error;
    }

  if (!quiet)
    {
      printf("===========================================================\n");
      printf("\n");
      printf("  RDWT Block -- decoding sequence:\n      %s\n         to\n      %s\n\n",
             input_buffer->filename,
             image_sequence->filename);
      printf("   Frame size: %d x %d\n",
             num_cols, num_rows);
      printf("  Start frame: %d\n",
             image_sequence->start_frame_num);
      printf("    End frame: %d\n\n",
             image_sequence->end_frame_num);
      printf("===========================================================\n");
      printf("\n");
    }

  if (!quiet)
    printf("  Frame %d\n",
           image_sequence->start_frame_num);
  
  if (QccVIDRDWTBlockDecodeFrame(&current_frame,
                                 reconstructed_frame_rdwt,
                                 reference_frame_rdwt,
                                 reference_frame_phases,
                                 blocksize,
                                 num_levels,
                                 subpixel_accuracy,
                                 target_bit_cnt,
                                 wavelet,
                                 input_buffer,
                                 &mvd_table,
                                 0,
                                 NULL,
                                 NULL,
                                 filter1,
                                 filter2,
                                 filter3))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccVIDRDWTBlockDecodeFrame()");
      goto Error;
    }
  
  if (QccIMGImageSequenceWriteFrame(image_sequence))
    {
      QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccIMGImageSequenceWriteFrame()");
      goto Error;
    }

  for (frame = image_sequence->start_frame_num + 1;
       frame <= image_sequence->end_frame_num;
       frame++)
    {
      if (QccIMGImageSequenceIncrementFrameNum(image_sequence))
	{
	  QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccIMGImageSequenceIncrementFrameNum()");
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
            QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccVIDMotionVectorsReadFile()");
            goto Error;
          }

      if (QccVIDRDWTBlockDecodeFrame(&current_frame,
                                     reconstructed_frame_rdwt,
                                     reference_frame_rdwt,
                                     reference_frame_phases,
                                     blocksize,
                                     num_levels,
                                     subpixel_accuracy,
                                     target_bit_cnt,
                                     wavelet,
                                     input_buffer,
                                     &mvd_table,
                                     (mv_filename != NULL),
                                     &motion_vector_horizontal,
                                     &motion_vector_vertical,
                                     filter1,
                                     filter2,
                                     filter3))
	{
	  QccErrorAddMessage("(QccVIDRDWTBlockDecode): Error calling QccVIDRDWTBlockDecodeFrame()");
          goto Error;
	}

      if (QccIMGImageSequenceWriteFrame(image_sequence))
	{
	  QccErrorAddMessage("(QccVIDRDWTBlockDecode(): Error calling QccIMGImageSequenceWriteFrame()");
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
  QccWAVWaveletRedundantDWT2DFree(reconstructed_frame_rdwt,
                                  num_rows, num_levels);
  QccWAVWaveletRedundantDWT2DFree(reference_frame_rdwt,
                                  reference_num_rows, num_levels);
  QccVIDRDWTBlockRDWTPhasesFree(reference_frame_phases,
                                num_levels,
                                subpixel_accuracy);
  QccVIDMotionVectorsTableFree(&mvd_table);
  return(return_value);
}

#else
int QccVIDRDWTBlockEncode(QccIMGImageSequence *image_sequence,
                          const QccFilter *filter1,
                          const QccFilter *filter2,
                          const QccFilter *filter3,
                          int subpixel_accuracy,
                          QccBitBuffer *output_buffer,
                          int blocksize,
                          int num_levels,
                          int target_bit_cnt,
                          const QccWAVWavelet *wavelet,
                          const QccString mv_filename,
                          int read_motion_vectors,
                          int quiet)
{
  QccErrorAddMessage("(QccVIDRDWTBlockEncode): Optional QccSPIHT module is not available -- RDWT-block coder is consequently not supported");
  return(1);
}


int QccVIDRDWTBlockDecodeHeader(QccBitBuffer *input_buffer,
                                int *num_rows,
                                int *num_cols,
                                int *start_frame_num,
                                int *end_frame_num,
                                int *blocksize,
                                int *num_levels,
                                int *target_bit_cnt)
{
  QccErrorAddMessage("(QccVIDRDWTBlockEncode): Optional QccSPIHT module is not available -- RDWT-block coder is consequently not supported");
  return(1);
}


int QccVIDRDWTBlockDecode(QccIMGImageSequence *image_sequence,
                          const QccFilter *filter1,
                          const QccFilter *filter2,
                          const QccFilter *filter3,
                          int subpixel_accuracy,
                          QccBitBuffer *input_buffer,
                          int target_bit_cnt,
                          int blocksize,
                          int num_levels,
                          const QccWAVWavelet *wavelet,
                          const QccString mv_filename,
                          int quiet)
{
  QccErrorAddMessage("(QccVIDRDWTBlockEncode): Optional QccSPIHT module is not available -- RDWT-block coder is consequently not supported");
  return(1);
}


#endif
