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


int QccWAVShapeAdaptiveMaskBAR(const QccMatrix mask,
                               int num_rows,
                               int num_cols,
                               double *bar_value)
{
  double bar = 0;
  int return_value = 0;
  int row, col;
  int boundary_cnt = 0;
  int area_cnt = 0;
  QccMatrix boundary_mask = NULL;

  if (bar_value == NULL)
    goto Return;
  if (mask == NULL)
    goto Return;

  if ((boundary_mask = QccMatrixAlloc(num_rows, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVShapeAdaptiveMaskBAR): Error calling QccMatrixAlloc()");
      goto Error;
    }

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        boundary_mask[row][col] = 0;
        if (!QccAlphaTransparent(mask[row][col]))
          area_cnt++;
      }
  
  for (row = 0; row < num_rows; row++)
    for (col = 1; col < num_cols; col++)
      if (mask[row][col] != mask[row][col - 1])
        {
          if (!QccAlphaTransparent(mask[row][col]))
            boundary_mask[row][col] = 1;
          else
            boundary_mask[row][col - 1] = 1;
        }
  
  for (col = 0; col < num_cols; col++)
    for (row = 1; row < num_rows; row++)
      if (mask[row][col] != mask[row - 1][col])
        {
          if (!QccAlphaTransparent(mask[row][col]))
            boundary_mask[row][col] = 1;
          else
            boundary_mask[row - 1][col] = 1;
        }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      if (boundary_mask[row][col] > 0)
        boundary_cnt++;

  bar = (double)boundary_cnt / area_cnt;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (bar_value != NULL)
    *bar_value = bar;
  QccMatrixFree(boundary_mask, num_rows);
  return(return_value);
}


int QccWAVShapeAdaptiveMaskBoundingBox(const QccWAVSubbandPyramid *mask,
                                       int subband,
                                       int *bounding_box_origin_row,
                                       int *bounding_box_origin_col,
                                       int *bounding_box_num_rows,
                                       int *bounding_box_num_cols)
{
  int min_row = MAXINT;
  int min_col = MAXINT;
  int max_row = -MAXINT;
  int max_col = -MAXINT;
  int origin_row;
  int origin_col;
  int num_rows;
  int num_cols;
  int row;
  int col;
  int totally_transparent = 1;

  if (mask == NULL)
    return(0);

  if (QccWAVSubbandPyramidSubbandOffsets(mask,
                                         subband,
                                         &origin_row,
                                         &origin_col))
    {
      QccErrorAddMessage("(QccWAVShapeAdaptiveMaskBoundingBox): Error calling QccWAVSubbandPyramidSubbandOffsets()");
      return(1);
    }
  if (QccWAVSubbandPyramidSubbandSize(mask,
                                      subband,
                                      &num_rows,
                                      &num_cols))
    {
      QccErrorAddMessage("(QccWAVShapeAdaptiveMaskBoundingBox): Error calling QccWAVSubbandPyramidSubbandSize()");
      return(1);
    }

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      if (!QccAlphaTransparent(mask->matrix[row + origin_row]
                               [col + origin_col]))
        {
          totally_transparent = 0;
          max_row = QccMathMax(max_row, row + origin_row);
          max_col = QccMathMax(max_col, col + origin_col);

          min_row = QccMathMin(min_row, row + origin_row);
          min_col = QccMathMin(min_col, col + origin_col);
        }
        
  if (totally_transparent)
    return(2);

  if (bounding_box_origin_row != NULL)
    *bounding_box_origin_row = min_row;
  if (bounding_box_origin_col != NULL)
    *bounding_box_origin_col = min_col;

  if (bounding_box_num_rows != NULL)
    *bounding_box_num_rows = max_row - min_row + 1;
  if (bounding_box_num_cols != NULL)
    *bounding_box_num_cols = max_col - min_col + 1;
  
  return(0);
}
