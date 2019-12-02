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


int QccVIDMotionEstimationExtractBlock(const QccIMGImageComponent *image,
                                       double row,
                                       double col,
                                       QccMatrix block,
                                       int block_size,
                                       int subpixel_accuracy)
{
  int block_row, block_col;
  int step;
  int row2, col2;
  int extraction_row, extraction_col;

  if (image == NULL)
    return(0);
  if (image->image == NULL)
    return(0);
  if (block == NULL)
    return(0);

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
      QccErrorAddMessage("(QccVIDMotionEstimationExtractBlock): Unrecognized subpixel accuracy (%d)",
                         subpixel_accuracy);
      return(1);
    }

  for (block_row = 0; block_row < block_size; block_row++)
    for (block_col = 0; block_col < block_size; block_col++)
      {
        extraction_row = block_row * step + row2;
        extraction_col = block_col * step + col2;

        if (extraction_row < 0)
          extraction_row = 0;
        else
          if (extraction_row >= image->num_rows)
            extraction_row = image->num_rows - 1;

        if (extraction_col < 0)
          extraction_col = 0;
        else
          if (extraction_col >= image->num_cols)
            extraction_col = image->num_cols - 1;

        block[block_row][block_col] =
          image->image[extraction_row][extraction_col];
      }

  return(0);
}


int QccVIDMotionEstimationInsertBlock(QccIMGImageComponent *image,
                                      double row,
                                      double col,
                                      const QccMatrix block,
                                      int block_size,
                                      int subpixel_accuracy)
{
  int block_row, block_col;
  int step;
  int row2, col2;
  int insertion_row, insertion_col;

  if (image == NULL)
    return(0);
  if (image->image == NULL)
    return(0);
  if (block == NULL)
    return(0);

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
      QccErrorAddMessage("(QccVIDMotionEstimationInsertBlock): Unrecognized subpixel accuracy (%d)",
                         subpixel_accuracy);
      return(1);
    }

  for (block_row = 0; block_row < block_size; block_row++)
    for (block_col = 0; block_col < block_size; block_col++)
      {
        insertion_row = block_row * step + row2;
        insertion_col = block_col * step + col2;

        if ((insertion_row >= 0) &&
            (insertion_row < image->num_rows) &&
            (insertion_col >= 0) &&
            (insertion_col < image->num_cols))
          image->image[insertion_row][insertion_col] =
            block[block_row][block_col];
      }

  return(0);
}


double QccVIDMotionEstimationMAE(QccMatrix current_block,
                                 QccMatrix reference_block,
                                 QccMatrix weights,
                                 int block_size)
{
  double mae = 0.0;
  int block_row, block_col;

  if (current_block == NULL)
    return(0.0);
  if (reference_block == NULL)
    return(0.0);

  if (weights == NULL)
    for (block_row = 0; block_row < block_size; block_row++)
      for (block_col = 0; block_col < block_size; block_col++)
        mae +=
          fabs(current_block[block_row][block_col] -
               reference_block[block_row][block_col]);
  else
    for (block_row = 0; block_row < block_size; block_row++)
      for (block_col = 0; block_col < block_size; block_col++)
        mae +=
          weights[block_row][block_col] *
          fabs(current_block[block_row][block_col] -
               reference_block[block_row][block_col]);

  return(mae / block_size / block_size);
}


static int QccVIDMotionEstimationWindowSearch(QccMatrix current_block,
                                              QccMatrix reference_block,
                                              int block_size,
                                              const QccIMGImageComponent
                                              *reference_frame,
                                              double search_row,
                                              double search_col,
                                              double window_size,
                                              double search_step,
                                              int subpixel_accuracy,
                                              double *motion_vector_horizontal,
                                              double *motion_vector_vertical)
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
        
        if (QccVIDMotionEstimationExtractBlock(reference_frame,
                                               reference_frame_row,
                                               reference_frame_col,
                                               reference_block,
                                               block_size,
                                               subpixel_accuracy))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationWindowSearch): Error calling QccVIDMotionEstimationExtractBlock()");
            return(1);
          }
        
        current_mae = QccVIDMotionEstimationMAE(current_block,
                                                reference_block,
                                                NULL,
                                                block_size);
        
        if (current_mae < min_mae)
          {
            min_mae = current_mae;
            *motion_vector_horizontal = u;
            *motion_vector_vertical = v;
          }
      }

  return(0);
}


int QccVIDMotionEstimationFullSearch(const QccIMGImageComponent *current_frame,
                                     const QccIMGImageComponent
                                     *reference_frame,
                                     QccIMGImageComponent
                                     *motion_vectors_horizontal,
                                     QccIMGImageComponent
                                     *motion_vectors_vertical,
                                     int block_size,
                                     int window_size,
                                     int subpixel_accuracy)
{
  int return_value;
  QccMatrix current_block = NULL;
  QccMatrix reference_block = NULL;
  int block_row, block_col;
  double u = 0;
  double v = 0;
  double current_search_step;
  double final_search_step;
  int mv_row, mv_col;
  double search_row, search_col;
  double current_window_size;

  if (current_frame == NULL)
    return(0);
  if (reference_frame == NULL)
    return(0);
  if (motion_vectors_horizontal == NULL)
    return(0);
  if (motion_vectors_vertical == NULL)
    return(0);

  if (current_frame->image == NULL)
    return(0);
  if (reference_frame->image == NULL)
    return(0);
  if (motion_vectors_horizontal->image == NULL)
    return(0);
  if (motion_vectors_vertical->image == NULL)
    return(0);

  if ((motion_vectors_horizontal->num_rows !=
       current_frame->num_rows / block_size) ||
      (motion_vectors_horizontal->num_cols !=
       current_frame->num_cols / block_size))
    {
      QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Motion-vector field is inconsistent with current-frame size");
      goto Error;
    }

  if ((motion_vectors_vertical->num_rows !=
       current_frame->num_rows / block_size) ||
      (motion_vectors_vertical->num_cols !=
       current_frame->num_cols / block_size))
    {
      QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Motion-vector field is inconsistent with current-frame size");
      goto Error;
    }

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      if ((reference_frame->num_rows != current_frame->num_rows) ||
          (reference_frame->num_cols != current_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Reference-frame size is inconsistent with current-frame size for full-pixel motion estimation");
          goto Error;
        }
      final_search_step = 1.0;
      break;
    case QCCVID_ME_HALFPIXEL:
      if ((reference_frame->num_rows != 2 * current_frame->num_rows) ||
          (reference_frame->num_cols != 2 * current_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Reference-frame size is inconsistent with current-frame size for half-pixel motion estimation");
          goto Error;
        }
      final_search_step = 0.5;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      if ((reference_frame->num_rows != 4 * current_frame->num_rows) ||
          (reference_frame->num_cols != 4 * current_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Reference-frame size is inconsistent with current-frame size for quarter-pixel motion estimation");
          goto Error;
        }
      final_search_step = 0.25;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      if ((reference_frame->num_rows != 8 * current_frame->num_rows) ||
          (reference_frame->num_cols != 8 * current_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Reference-frame size is inconsistent with current-frame size for eighth-pixel motion estimation");
          goto Error;
        }
      final_search_step = 0.125;
      break;
    default:
      QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Unrecognized motion-estimation accuracy (%d)",
                         subpixel_accuracy);
      goto Error;
    }

  if ((current_block =
       QccMatrixAlloc(block_size, block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((reference_block =
       QccMatrixAlloc(block_size, block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Error calling QccMatrixAlloc()");
      goto Error;
    }

  for (block_row = 0;
       block_row < current_frame->num_rows; block_row += block_size)
    for (block_col = 0;
         block_col < current_frame->num_cols; block_col += block_size)
      {
        if (QccVIDMotionEstimationExtractBlock(current_frame,
                                               (double)block_row,
                                               (double)block_col,
                                               current_block,
                                               block_size,
                                               QCCVID_ME_FULLPIXEL))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Error calling QccVIDMotionEstimationExtractBlock()");
            goto Error;
          }

        mv_row = block_row / block_size;
        mv_col = block_col / block_size;

        motion_vectors_horizontal->image[mv_row][mv_col] = 0.0;
        motion_vectors_vertical->image[mv_row][mv_col] = 0.0;

        for (current_search_step = 1.0;
             current_search_step >= final_search_step;
             current_search_step /= 2)
          {
            search_row = block_row + 
              motion_vectors_vertical->image[mv_row][mv_col];
            search_col = block_col + 
              motion_vectors_horizontal->image[mv_row][mv_col];

            current_window_size =
              (current_search_step == 1.0) ?
              (double)window_size : current_search_step;

            if (QccVIDMotionEstimationWindowSearch(current_block,
                                                   reference_block,
                                                   block_size,
                                                   reference_frame,
                                                   search_row,
                                                   search_col,
                                                   current_window_size,
                                                   current_search_step,
                                                   subpixel_accuracy,
                                                   &u,
                                                   &v))
              {
                QccErrorAddMessage("(QccVIDMotionEstimationFullSearch): Error calling QccVIDMotionEstimationWindowSearch()");
                goto Error;
              }

            motion_vectors_horizontal->image[mv_row][mv_col] += u;
            motion_vectors_vertical->image[mv_row][mv_col] += v;
          }
      }
      
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(current_block, block_size);
  QccMatrixFree(reference_block, block_size);
  return(return_value);
}


int QccVIDMotionEstimationCalcReferenceFrameSize(int num_rows,
                                                 int num_cols,
                                                 int *reference_num_rows,
                                                 int *reference_num_cols,
                                                 int subpixel_accuracy)
{
  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      *reference_num_rows = num_rows;
      *reference_num_cols = num_cols;
      break;
    case QCCVID_ME_HALFPIXEL:
      *reference_num_rows = num_rows * 2;
      *reference_num_cols = num_cols * 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      *reference_num_rows = num_rows * 4;
      *reference_num_cols = num_cols * 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      *reference_num_rows = num_rows * 8;
      *reference_num_cols = num_cols * 8;
      break;
    default:
      QccErrorAddMessage("(QccVIDMotionEstimationCalcReferenceFrameSize): Unrecognized subpixel accuracy (%d)",
                         subpixel_accuracy);
      return(1);
    }

  return(0);
}


int QccVIDMotionEstimationCreateReferenceFrame(const QccIMGImageComponent
                                               *current_frame,
                                               QccIMGImageComponent
                                               *reference_frame,
                                               int subpixel_accuracy,
                                               const QccFilter *filter1,
                                               const QccFilter *filter2,
                                               const QccFilter *filter3)
{
  int return_value;
  QccIMGImageComponent reference_frame2;
  QccIMGImageComponent reference_frame3;

  QccIMGImageComponentInitialize(&reference_frame2);
  QccIMGImageComponentInitialize(&reference_frame3);

  if (current_frame == NULL)
    return(0);
  if (reference_frame == NULL)
    return(0);

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      if ((reference_frame->num_rows != current_frame->num_rows) ||
          (reference_frame->num_cols != current_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Reference-frame size is inconsistent with current-frame size for full-pixel motion estimation");
          goto Error;
        }
      if (QccIMGImageComponentCopy(reference_frame,
                                   current_frame))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentCopy()");
          goto Error;
        }
      break;
    case QCCVID_ME_HALFPIXEL:
      if ((reference_frame->num_rows != 2 * current_frame->num_rows) ||
          (reference_frame->num_cols != 2 * current_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Reference-frame size is inconsistent with current-frame size for half-pixel motion estimation");
          goto Error;
        }
      if (filter1 == NULL)
        {
          if (QccIMGImageComponentInterpolateBilinear(current_frame,
                                                      reference_frame))
            {
              QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateBilinear()");
              goto Error;
            }
        }
      else
        if (QccIMGImageComponentInterpolateFilter(current_frame,
                                                  reference_frame,
                                                  filter1))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateFilter()");
            goto Error;
          }
      break;
    case QCCVID_ME_QUARTERPIXEL:
      if ((reference_frame->num_rows != 4 * current_frame->num_rows) ||
          (reference_frame->num_cols != 4 * current_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Reference-frame size is inconsistent with current-frame size for quarter-pixel motion estimation");
          goto Error;
        }
      reference_frame2.num_rows = 2 * current_frame->num_rows;
      reference_frame2.num_cols = 2 * current_frame->num_cols;
      if (QccIMGImageComponentAlloc(&reference_frame2))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
      if (filter1 == NULL)
        {
          if (QccIMGImageComponentInterpolateBilinear(current_frame,
                                                      &reference_frame2))
            {
              QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateBilinear()");
              goto Error;
            }
        }
      else
        if (QccIMGImageComponentInterpolateFilter(current_frame,
                                                  &reference_frame2,
                                                  filter1))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateFilter()");
            goto Error;
          }
      if (filter2 == NULL)
        {
          if (QccIMGImageComponentInterpolateBilinear(&reference_frame2,
                                                      reference_frame))
            {
              QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateBilinear()");
              goto Error;
            }
        }
      else
        if (QccIMGImageComponentInterpolateFilter(&reference_frame2,
                                                  reference_frame,
                                                  filter2))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateFilter()");
            goto Error;
          }
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      if ((reference_frame->num_rows != 8 * current_frame->num_rows) ||
          (reference_frame->num_cols != 8 * current_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Reference-frame size is inconsistent with current-frame size for eighth-pixel motion estimation");
          goto Error;
        }
      reference_frame2.num_rows = 2 * current_frame->num_rows;
      reference_frame2.num_cols = 2 * current_frame->num_cols;
      if (QccIMGImageComponentAlloc(&reference_frame2))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
      reference_frame3.num_rows = 4 * current_frame->num_rows;
      reference_frame3.num_cols = 4 * current_frame->num_cols;
      if (QccIMGImageComponentAlloc(&reference_frame3))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
      if (filter1 == NULL)
        {
          if (QccIMGImageComponentInterpolateBilinear(current_frame,
                                                      &reference_frame2))
            {
              QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateBilinear()");
              goto Error;
            }
        }
      else
        if (QccIMGImageComponentInterpolateFilter(current_frame,
                                                  &reference_frame2,
                                                  filter1))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateFilter()");
            goto Error;
          }
      if (filter2 == NULL)
        {
          if (QccIMGImageComponentInterpolateBilinear(&reference_frame2,
                                                      &reference_frame3))
            {
              QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateBilinear()");
              goto Error;
            }
        }
      else
        if (QccIMGImageComponentInterpolateFilter(&reference_frame2,
                                                  &reference_frame3,
                                                  filter2))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateFilter()");
            goto Error;
          }
      if (filter3 == NULL)
        {
          if (QccIMGImageComponentInterpolateBilinear(&reference_frame3,
                                                      reference_frame))
            {
              QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateBilinear()");
              goto Error;
            }
        }
      else
        if (QccIMGImageComponentInterpolateFilter(&reference_frame3,
                                                  reference_frame,
                                                  filter3))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Error calling QccIMGImageComponentInterpolateFilter()");
            goto Error;
          }
      break;
    default:
      QccErrorAddMessage("(QccVIDMotionEstimationCreateReferenceFrame): Unrecognized motion-estimation accuracy (%d)",
                         subpixel_accuracy);
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&reference_frame2);
  QccIMGImageComponentFree(&reference_frame3);
  return(return_value);
}


int QccVIDMotionEstimationCreateCompensatedFrame(QccIMGImageComponent
                                                 *motion_compensated_frame,
                                                 const QccIMGImageComponent
                                                 *reference_frame,
                                                 const QccIMGImageComponent
                                                 *motion_vectors_horizontal,
                                                 const QccIMGImageComponent
                                                 *motion_vectors_vertical,
                                                 int block_size,
                                                 int subpixel_accuracy)
{
  int return_value;
  QccMatrix reference_block = NULL;
  int block_row, block_col;
  int mv_row, mv_col;

  if (motion_compensated_frame == NULL)
    return(0);
  if (reference_frame == NULL)
    return(0);
  if (motion_vectors_horizontal == NULL)
    return(0);
  if (motion_vectors_vertical == NULL)
    return(0);

  if (motion_compensated_frame->image == NULL)
    return(0);
  if (reference_frame->image == NULL)
    return(0);
  if (motion_vectors_horizontal->image == NULL)
    return(0);
  if (motion_vectors_vertical->image == NULL)
    return(0);

  if ((motion_vectors_horizontal->num_rows !=
       motion_compensated_frame->num_rows / block_size) ||
      (motion_vectors_horizontal->num_cols !=
       motion_compensated_frame->num_cols / block_size))
    {
      QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Motion-vector field is inconsistent with current-frame size");
      goto Error;
    }

  if ((motion_vectors_vertical->num_rows !=
       motion_compensated_frame->num_rows / block_size) ||
      (motion_vectors_vertical->num_cols !=
       motion_compensated_frame->num_cols / block_size))
    {
      QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Motion-vector field is inconsistent with current-frame size");
      goto Error;
    }

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      if ((reference_frame->num_rows != motion_compensated_frame->num_rows) ||
          (reference_frame->num_cols != motion_compensated_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Reference-frame size is inconsistent with current-frame size for full-pixel motion estimation");
          goto Error;
        }
      break;
    case QCCVID_ME_HALFPIXEL:
      if ((reference_frame->num_rows !=
           2 * motion_compensated_frame->num_rows) ||
          (reference_frame->num_cols !=
           2 * motion_compensated_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Reference-frame size is inconsistent with current-frame size for half-pixel motion estimation");
          goto Error;
        }
      break;
    case QCCVID_ME_QUARTERPIXEL:
      if ((reference_frame->num_rows !=
           4 * motion_compensated_frame->num_rows) ||
          (reference_frame->num_cols !=
           4 * motion_compensated_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Reference-frame size is inconsistent with current-frame size for quarter-pixel motion estimation");
          goto Error;
        }
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      if ((reference_frame->num_rows !=
           8 * motion_compensated_frame->num_rows) ||
          (reference_frame->num_cols !=
           8 * motion_compensated_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Reference-frame size is inconsistent with current-frame size for eighth-pixel motion estimation");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Unrecognized motion-estimation accuracy (%d)",
                         subpixel_accuracy);
      goto Error;
    }

  if ((reference_block = QccMatrixAlloc(block_size, block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Error calling QccMatrixAlloc()");
      goto Error;
    }

  for (block_row = 0;
       block_row < motion_compensated_frame->num_rows;
       block_row += block_size)
    for (block_col = 0;
         block_col < motion_compensated_frame->num_cols;
         block_col += block_size)
      {
        mv_row = block_row / block_size;
        mv_col = block_col / block_size;

        if (QccVIDMotionEstimationExtractBlock(reference_frame,
                                               (double)block_row +
                                               motion_vectors_vertical->image
                                               [mv_row][mv_col],
                                               (double)block_col +
                                               motion_vectors_horizontal->image
                                               [mv_row][mv_col],
                                               reference_block,
                                               block_size,
                                               subpixel_accuracy))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Error calling QccVIDMotionEstimationExtractBlock()");
            goto Error;
          }

        if (QccVIDMotionEstimationInsertBlock(motion_compensated_frame,
                                              (double)block_row,
                                              (double)block_col,
                                              reference_block,
                                              block_size,
                                              QCCVID_ME_FULLPIXEL))
          {
            QccErrorAddMessage("(QccVIDMotionEstimationCreateCompensatedFrame): Error calling QccVIDMotionEstimationInsertBlock()");
            goto Error;
          }
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(reference_block, block_size);
  return(return_value);
}
