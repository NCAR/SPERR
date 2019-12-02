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


#define QCCVIDSPATIALBLOCK_WINDOWSIZE 15


typedef struct
{
  int intraframe;
  int motion_vector_bits;
  int intraframe_bits;
} QccVIDSpatialBlockStatistics;


static void QccVIDSpatialBlockPrintStatistics(const
                                              QccVIDSpatialBlockStatistics
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


static int QccVIDSpatialBlockMotionCompensation(QccIMGImageComponent
                                                *current_frame,
                                                QccIMGImageComponent
                                                *reference_frame,
                                                QccIMGImageComponent
                                                *horizontal_motion,
                                                QccIMGImageComponent
                                                *vertical_motion,
                                                int blocksize,
                                                int subpixel_accuracy)
{
  int return_value;
  QccIMGImageComponent predicted_frame;
  int row, col;

  QccIMGImageComponentInitialize(&predicted_frame);

  predicted_frame.num_rows = current_frame->num_rows;
  predicted_frame.num_cols = current_frame->num_cols;
  if (QccIMGImageComponentAlloc(&predicted_frame))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockMotionCompensation): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }

  if (QccVIDMotionEstimationCreateCompensatedFrame(&predicted_frame,
                                                   reference_frame,
                                                   horizontal_motion,
                                                   vertical_motion,
                                                   blocksize,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockMotionCompensation): Error calling QccVIDMotionEstimationCreateCompensatedFrame()");
      goto Error;
    }

  for (row = 0; row < current_frame->num_rows; row++)
    for (col = 0; col < current_frame->num_cols; col++)
      current_frame->image[row][col] -= predicted_frame.image[row][col];

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&predicted_frame);
  return(return_value);
}


static int QccVIDSpatialBlockInverseMotionCompensation(QccIMGImageComponent
                                                       *current_frame,
                                                       QccIMGImageComponent
                                                       *reference_frame,
                                                       QccIMGImageComponent
                                                       *horizontal_motion,
                                                       QccIMGImageComponent
                                                       *vertical_motion,
                                                       int blocksize,
                                                       int subpixel_accuracy)
{
  int return_value;
  QccIMGImageComponent predicted_frame;
  int row, col;

  QccIMGImageComponentInitialize(&predicted_frame);

  predicted_frame.num_rows = current_frame->num_rows;
  predicted_frame.num_cols = current_frame->num_cols;
  if (QccIMGImageComponentAlloc(&predicted_frame))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockInverseMotionCompensation): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }

  if (QccVIDMotionEstimationCreateCompensatedFrame(&predicted_frame,
                                                   reference_frame,
                                                   horizontal_motion,
                                                   vertical_motion,
                                                   blocksize,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockInverseMotionCompensation): Error calling QccVIDMotionEstimationCreateCompensatedFrame()");
      goto Error;
    }

  for (row = 0; row < current_frame->num_rows; row++)
    for (col = 0; col < current_frame->num_cols; col++)
      current_frame->image[row][col] += predicted_frame.image[row][col];

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&predicted_frame);
  return(return_value);
}


static int QccVIDSpatialBlockEncodeHeader(QccBitBuffer *output_buffer,
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
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, start_frame_num))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, end_frame_num))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, blocksize))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, num_levels))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(output_buffer, target_bit_cnt))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferFlush(output_buffer))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeHeader): Error calling QccBitBufferFlush()");
      return(1);
    }
  
  return(0);
}


static int QccVIDSpatialBlockDecodeFrame();

static int QccVIDSpatialBlockEncodeFrame(QccIMGImageComponent *current_frame,
                                         QccIMGImageComponent *reference_frame,
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
                                         QccVIDSpatialBlockStatistics
                                         *statistics,
                                         const QccFilter *filter1,
                                         const QccFilter *filter2,
                                         const QccFilter *filter3)
{
  int return_value;
  int intraframe;
  int start_position;
  
  start_position = output_buffer->bit_cnt;
  
  intraframe =
    (motion_vector_horizontal == NULL) || (motion_vector_vertical == NULL);
  
  if (statistics != NULL)
    statistics->intraframe = intraframe;
  
  if (!intraframe)
    {
      if (!read_motion_vectors)
        {
          if (QccVIDMotionEstimationFullSearch(current_frame,
                                               reference_frame,
                                               motion_vector_horizontal,
                                               motion_vector_vertical,
                                               blocksize,
                                               QCCVIDSPATIALBLOCK_WINDOWSIZE,
                                               subpixel_accuracy))
            {
              QccErrorAddMessage("(QccVIDSpatialBlockEncodeFrame): Error calling QccVIDMotionEstimationFullSearch()");
              goto Error;
            }
          
          if (QccVIDMotionVectorsEncode(motion_vector_horizontal,
                                        motion_vector_vertical,
                                        NULL,
                                        subpixel_accuracy,
                                        output_buffer))
            {
              QccErrorAddMessage("(QccVIDSpatialBlockEncodeFrame): Error calling QccVIDMotionVectorsEncode()");
              goto Error;
            }
        }      
      
      if (QccVIDSpatialBlockMotionCompensation(current_frame,
                                               reference_frame,
                                               motion_vector_horizontal,
                                               motion_vector_vertical,
                                               blocksize,
                                               subpixel_accuracy))
	{
	  QccErrorAddMessage("(QccVIDSpatialBlockEncodeFrame): Error calling QccVIDSpatialBlockMotionCompensation()");
	  goto Error;
	}
      
      if (statistics != NULL)
        statistics->motion_vector_bits =
          output_buffer->bit_cnt - start_position;
      
      if (output_buffer->bit_cnt > start_position + target_bit_cnt)
	{
	  QccErrorAddMessage("(QccVIDSpatialBlockEncodeFrame): Rate is not sufficient to store all motion vectors for frame");
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
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeFrame): Error calling QccSPIHTEncode()");
      goto Error;
    }
  
  if (statistics != NULL)
    statistics->intraframe_bits =
      output_buffer->bit_cnt - start_position - statistics->motion_vector_bits;
  
  if (QccBitBufferFlush(output_buffer))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeFrame): Error calling QccBitBufferFlush()");
      goto Error;
    }
  
  if (QccVIDSpatialBlockDecodeFrame(current_frame,
                                    reference_frame,
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
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeFrame): Error calling QccVIDSpatialBlockDecodeFrame()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccVIDSpatialBlockDecodeHeader(QccBitBuffer *input_buffer,
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
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_rows != NULL)
    *num_rows = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_cols != NULL)
    *num_cols = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (start_frame_num != NULL)
    *start_frame_num = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (end_frame_num != NULL)
    *end_frame_num = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (blocksize != NULL)
    *blocksize = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (num_levels != NULL)
    *num_levels = val;
  
  if (QccBitBufferGetInt(input_buffer, &val))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  if (target_bit_cnt != NULL)
    *target_bit_cnt = val;
  
  if (QccBitBufferFlush(input_buffer))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeHeader): Error calling QccBitBufferFlush()");
      return(1);
    }
  
  return(0);
}


static int QccVIDSpatialBlockDecodeFrame(QccIMGImageComponent *current_frame,
                                         QccIMGImageComponent
                                         *reference_frame,
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
  double image_mean;
  int max_coefficient_bits;
  int arithmetic_coded;
  int intraframe;
  int start_position;
  
  start_position = input_buffer->bit_cnt;
  
  num_rows = current_frame->num_rows;
  num_cols = current_frame->num_cols;
  
  intraframe =
    (motion_vector_horizontal == NULL) || (motion_vector_vertical == NULL);
  
  if ((!intraframe) && (!read_motion_vectors))
    if (QccVIDMotionVectorsDecode(motion_vector_horizontal,
                                  motion_vector_vertical,
                                  NULL,
                                  subpixel_accuracy,
                                  input_buffer))
      {
        QccErrorAddMessage("(QccVIDSpatialBlockDecodeFrame): Error calling QccVIDMotionVectorsDecode()");
        goto Error;
      }
  
  if (QccMatrixZero((QccMatrix)current_frame->image,
                    num_rows, num_cols))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeFrame): Error calling QccMatrixZero()");
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
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeFrame): Error calling QccSPIHTDecodeHeader()");
      goto Error;
    }
  
  if (QccSPIHTDecode(input_buffer,
                     current_frame,
                     NULL,
                     num_levels,
                     wavelet,
                     NULL,
                     image_mean,
                     max_coefficient_bits,
                     start_position + target_bit_cnt,
                     arithmetic_coded))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeFrame):Error calling QccSPIHTDecode()");
      goto Error;
    }
  
  if (QccBitBufferFlush(input_buffer))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeFrame):Error calling QccBitBufferFlush()");
      goto Error;
    }
  
  if (!intraframe)
    if (QccVIDSpatialBlockInverseMotionCompensation(current_frame,
                                                    reference_frame,
                                                    motion_vector_horizontal,
                                                    motion_vector_vertical,
                                                    blocksize,
                                                    subpixel_accuracy))
      {
	QccErrorAddMessage("(QccVIDSpatialBlockDecodeFrame): Error calling QccVIDSpatialBlockInverseMotionCompensation()");
        goto Error;
      }
  
  if (QccVIDMotionEstimationCreateReferenceFrame(current_frame,
                                                 reference_frame,
                                                 subpixel_accuracy,
                                                 filter1,
                                                 filter2,
                                                 filter3))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecodeFrame): Error calling QccVIDMotionEstimationCreateReferenceFrame()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccVIDSpatialBlockEncode(QccIMGImageSequence *image_sequence,
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
  QccIMGImageComponent current_frame;
  QccIMGImageComponent reference_frame;
  QccIMGImageComponent motion_vector_horizontal;
  QccIMGImageComponent motion_vector_vertical;
  QccVIDMotionVectorsTable mvd_table;
  int frame = 0;
  int num_rows, num_cols;
  int reference_num_rows, reference_num_cols;
  QccBitBuffer input_buffer;
  QccVIDSpatialBlockStatistics statistics;
  
  QccBitBufferInitialize(&input_buffer);
  QccIMGImageComponentInitialize(&current_frame);
  QccIMGImageComponentInitialize(&reference_frame);
  QccIMGImageComponentInitialize(&motion_vector_horizontal);
  QccIMGImageComponentInitialize(&motion_vector_vertical);
  QccVIDMotionVectorsTableInitialize(&mvd_table);
  
  if (QccIMGImageSequenceStartRead(image_sequence))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccIMGImageSequenceStartRead()");
      goto Error;
    }
  
  if (QccIMGImageGetSize(&image_sequence->current_frame,
                         &num_rows, &num_cols))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccIMGImageGetSize()");
      goto Error;
    }
  
  if ((num_rows % blocksize) || (num_cols % blocksize))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Image size is not an integer multiple of block size");
      goto Error;
    }

  current_frame.num_rows = num_rows;
  current_frame.num_cols = num_cols;
  current_frame.image = image_sequence->current_frame.Y.image;
  
  motion_vector_horizontal.num_rows = num_rows / blocksize;
  motion_vector_horizontal.num_cols = num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_horizontal))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  motion_vector_vertical.num_rows = num_rows / blocksize;
  motion_vector_vertical.num_cols = num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_vertical))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }
  
  reference_frame.num_rows = reference_num_rows;
  reference_frame.num_cols = reference_num_cols;
  if (QccIMGImageComponentAlloc(&reference_frame))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  QccStringCopy(input_buffer.filename, output_buffer->filename);
  input_buffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&input_buffer))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccBitBufferStart()");
      goto Error;
    }
  
  if (QccVIDSpatialBlockEncodeHeader(output_buffer,
                                     num_rows,
                                     num_cols,
                                     image_sequence->start_frame_num,
                                     image_sequence->end_frame_num,
                                     blocksize,
                                     num_levels,
                                     target_bit_cnt))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccVIDSpatialBlockEncodeHeader()");
      goto Error;
    }
  
  if (QccVIDSpatialBlockDecodeHeader(&input_buffer,
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncodeFrame): Error calling QccVIDSpatialBlockDecodeHeader()");
      goto Error;
    }
  
  if (QccVIDMotionVectorsTableCreate(&mvd_table))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccVIDMotionVectorsTableCreate()");
      goto Error;
    }
  
  if (!quiet)
    {
      printf("===========================================================\n");
      printf("\n");
      printf("  Spatial Block -- encoding sequence:\n      %s\n         to\n      %s\n\n",
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
  
  if (QccVIDSpatialBlockEncodeFrame(&current_frame,
                                    &reference_frame,
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
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccVIDSpatialBlockEncodeFrame()");
      goto Error;
    }
  
  if (!quiet)
    QccVIDSpatialBlockPrintStatistics(&statistics);
  
  for (frame = image_sequence->start_frame_num + 1;
       frame <= image_sequence->end_frame_num;
       frame++)
    {
      if (!quiet)
        printf("  Frame: %d\n", frame);
      
      if (QccIMGImageSequenceIncrementFrameNum(image_sequence))
	{
	  QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccIMGImageSequenceIncrementFrameNum()");
	  goto Error;
	}
      
      if (QccIMGImageSequenceReadFrame(image_sequence))
	{
	  QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccIMGImageSequenceReadFrame()");
	  goto Error;
	}
      
      if ((mv_filename != NULL) && read_motion_vectors)
        if (QccVIDMotionVectorsReadFile(&motion_vector_horizontal,
                                        &motion_vector_vertical,
                                        mv_filename,
                                        frame))
          {
            QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccVIDMotionVectorsReadFile()");
            goto Error;
          }
      
      if (QccVIDSpatialBlockEncodeFrame(&current_frame,
                                        &reference_frame,
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
	  QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccVIDSpatialBlockEncodeFrame()");
	  goto Error;
	}
      
      if ((mv_filename != NULL) && (!read_motion_vectors))
        if (QccVIDMotionVectorsWriteFile(&motion_vector_horizontal,
                                         &motion_vector_vertical,
                                         mv_filename,
                                         frame))
          {
            QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccVIDMotionVectorsWriteFile()");
            goto Error;
          }
      
      if (!quiet)
        QccVIDSpatialBlockPrintStatistics(&statistics);
    }
  
  if (QccBitBufferEnd(&input_buffer))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockEncode): Error calling QccBitBufferEnd()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&motion_vector_horizontal);
  QccIMGImageComponentFree(&motion_vector_vertical);
  QccIMGImageComponentFree(&reference_frame);
  QccVIDMotionVectorsTableFree(&mvd_table);
  return(return_value);
}  


int QccVIDSpatialBlockDecode(QccIMGImageSequence *image_sequence,
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
  QccIMGImageComponent current_frame;
  QccIMGImageComponent reference_frame;
  QccIMGImageComponent motion_vector_horizontal;
  QccIMGImageComponent motion_vector_vertical;
  QccVIDMotionVectorsTable mvd_table;
  int frame;
  
  QccIMGImageComponentInitialize(&current_frame);
  QccIMGImageComponentInitialize(&reference_frame);
  QccIMGImageComponentInitialize(&motion_vector_horizontal);
  QccIMGImageComponentInitialize(&motion_vector_vertical);
  QccVIDMotionVectorsTableInitialize(&mvd_table);
  
  image_sequence->current_frame_num = image_sequence->start_frame_num;
  
  if (QccIMGImageGetSize(&image_sequence->current_frame,
                         &num_rows, &num_cols))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccIMGImageGetSize()");
      goto Error;
    }
  
  current_frame.num_rows = num_rows;
  current_frame.num_cols = num_cols;
  current_frame.image = image_sequence->current_frame.Y.image;
  
  motion_vector_horizontal.num_rows = num_rows / blocksize;
  motion_vector_horizontal.num_cols = num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_horizontal))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  motion_vector_vertical.num_rows = num_rows / blocksize;
  motion_vector_vertical.num_cols = num_cols / blocksize;
  if (QccIMGImageComponentAlloc(&motion_vector_vertical))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  if (QccVIDMotionEstimationCalcReferenceFrameSize(num_rows,
                                                   num_cols,
                                                   &reference_num_rows,
                                                   &reference_num_cols,
                                                   subpixel_accuracy))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccVIDMotionEstimationCalcReferenceFrameSize()");
      goto Error;
    }
  
  reference_frame.num_rows = reference_num_rows;
  reference_frame.num_cols = reference_num_cols;
  if (QccIMGImageComponentAlloc(&reference_frame))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  if (QccVIDMotionVectorsTableCreate(&mvd_table))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccVIDMotionVectorsTableCreate()");
      goto Error;
    }
  
  if (!quiet)
    {
      printf("===========================================================\n");
      printf("\n");
      printf("  Spatial Block -- decoding sequence:\n      %s\n         to\n      %s\n\n",
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
  
  if (QccVIDSpatialBlockDecodeFrame(&current_frame,
                                    &reference_frame,
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
      QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccVIDSpatialBlockDecodeFrame()");
      goto Error;
    }
  
  if (QccIMGImageSequenceWriteFrame(image_sequence))
    {
      QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccIMGImageSequenceWriteFrame()");
      goto Error;
    }
  
  for (frame = image_sequence->start_frame_num + 1;
       frame <= image_sequence->end_frame_num;
       frame++)
    {
      if (QccIMGImageSequenceIncrementFrameNum(image_sequence))
	{
	  QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccIMGImageSequenceIncrementFrameNum()");
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
            QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccVIDMotionVectorsReadFile()");
            goto Error;
          }
      
      if (QccVIDSpatialBlockDecodeFrame(&current_frame,
                                        &reference_frame,
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
	  QccErrorAddMessage("(QccVIDSpatialBlockDecode): Error calling QccVIDSpatialBlockDecodeFrame()");
          goto Error;
	}
      
      if (QccIMGImageSequenceWriteFrame(image_sequence))
	{
	  QccErrorAddMessage("(QccVIDSpatialBlockDecode(): Error calling QccIMGImageSequenceWriteFrame()");
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
  QccIMGImageComponentFree(&reference_frame);
  QccVIDMotionVectorsTableFree(&mvd_table);
  return(return_value);
}

#else
int QccVIDSpatialBlockEncode(QccIMGImageSequence *image_sequence,
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
  QccErrorAddMessage("(QccVIDSpatialBlockEncode): Optional QccSPIHT module is not available -- spatial-block coder is consequently not supported");
  return(1);
}


int QccVIDSpatialBlockDecodeHeader(QccBitBuffer *input_buffer,
                                   int *num_rows,
                                   int *num_cols,
                                   int *start_frame_num,
                                   int *end_frame_num,
                                   int *blocksize,
                                   int *num_levels,
                                   int *target_bit_cnt)
{
  QccErrorAddMessage("(QccVIDSpatialBlockEncode): Optional QccSPIHT module is not available -- spatial-block coder is consequently not supported");
  return(1);
}


int QccVIDSpatialBlockDecode(QccIMGImageSequence *image_sequence,
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
  QccErrorAddMessage("(QccVIDSpatialBlockEncode): Optional QccSPIHT module is not available -- spatial-block coder is consequently not supported");
  return(1);
}


#endif
