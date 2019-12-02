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


int QccIMGImageComponentToDataset(const QccIMGImageComponent *image_component, 
                                  QccDataset *dataset, 
                                  int tile_num_rows,
                                  int tile_num_cols)
{
  int tileimg_num_rows, tileimg_num_cols;
  int tileimg_col_cnt, tileimg_row_cnt;
  int col_cnt, row_cnt;
  int vector;
  int block_size;
  QccIMGImageArray image_array;
  int img_num_rows, img_num_cols;
  
  if (image_component == NULL)
    return(0);
  
  image_array = image_component->image;
  img_num_rows = image_component->num_rows;
  img_num_cols = image_component->num_cols;
  
  tileimg_num_rows = img_num_rows / tile_num_rows;
  tileimg_num_cols = img_num_cols / tile_num_cols;
  
  if ((image_array == NULL) || (dataset == NULL))
    return(0);
  
  if (dataset->vectors == NULL)
    {
      dataset->num_vectors =
        tileimg_num_rows * tileimg_num_cols;
      dataset->vector_dimension =
        tile_num_rows * tile_num_cols;
      dataset->access_block_size =
        QCCDATASET_ACCESSWHOLEFILE;
      if (QccDatasetAlloc(dataset))
        {
          QccErrorAddMessage("(QccIMGImageComponentToDataset): Error calling QccDatasetAlloc()");
          return(1);
        }
    }
  
  if (dataset->vector_dimension != tile_num_cols * tile_num_rows)
    {
      QccErrorAddMessage("(QccIMGImageComponentToDataset): Dataset %s has vector dimension (%d) incompatible with requested tile size (%d x %d)",
                         dataset->filename,
                         dataset->vector_dimension,
                         tile_num_cols,
                         tile_num_rows);
      return(1);
    }
  
  block_size = QccDatasetGetBlockSize(dataset);
  
  if (block_size < tileimg_num_rows * tileimg_num_cols)
    {
      QccErrorAddMessage("(QccIMGImageComponentToDataset): Block size (%d vectors) of dataset %s is too small for image of size %d x %d with tile size %d x %d",
                         block_size,
                         dataset->filename,
                         img_num_cols,
                         img_num_rows,
                         tile_num_cols,
                         tile_num_rows);
      return(1);
    }
  
  for (tileimg_row_cnt = 0, vector = 0; tileimg_row_cnt < tileimg_num_rows; 
       tileimg_row_cnt++)
    for (tileimg_col_cnt = 0; tileimg_col_cnt < tileimg_num_cols; 
         tileimg_col_cnt++, vector++)
      for (row_cnt = 0; row_cnt < tile_num_rows; row_cnt++)
        for (col_cnt = 0; col_cnt < tile_num_cols; col_cnt++)
          dataset->vectors[vector][row_cnt*tile_num_cols + col_cnt] =
            (double)image_array
            [tileimg_row_cnt*tile_num_rows + row_cnt]
            [tileimg_col_cnt*tile_num_cols + col_cnt];
  
  return(0);
}

int QccIMGDatasetToImageComponent(QccIMGImageComponent *image_component,
                                  const QccDataset *dataset,
                                  int tile_num_rows,
                                  int tile_num_cols)
{
  int vector;
  int tileimg_num_rows, tileimg_num_cols;
  int tileimg_col_cnt, tileimg_row_cnt;
  int col_cnt, row_cnt;
  int block_size;
  QccIMGImageArray image_array;
  int img_num_rows, img_num_cols;
  
  if (dataset == NULL)
    return(0);
  if (dataset->vectors == NULL)
    return(0);
  if (image_component == NULL)
    return(0);
  image_array = image_component->image;
  img_num_rows = image_component->num_rows;
  img_num_cols = image_component->num_cols;
  
  if (image_array == NULL)
    return(0);
  
  if (dataset->vector_dimension != tile_num_cols * tile_num_rows)
    {
      QccErrorAddMessage("(QccIMGDatasetToImageComponent): Dataset %s has vector dimension (%d) incompatible with requested tile size (%d x %d)",
                         dataset->filename,
                         dataset->vector_dimension,
                         tile_num_cols,
                         tile_num_rows);
      return(1);
    }
  
  tileimg_num_rows = img_num_rows / tile_num_rows;
  tileimg_num_cols = img_num_cols / tile_num_cols;
  block_size = QccDatasetGetBlockSize(dataset);
  
  if (block_size < tileimg_num_rows * tileimg_num_cols)
    {
      QccErrorAddMessage("(QccIMGDatasetToImageComponent): Block size (%d vectors) of dataset %s is too small to create image of %d x %d with tile size %d x %d",
                         block_size,
                         dataset->filename,
                         img_num_rows,
                         img_num_cols,
                         tile_num_rows,
                         tile_num_cols);
      return(1);
    }
  
  for (vector = 0, tileimg_row_cnt = 0; tileimg_row_cnt < tileimg_num_rows; 
       tileimg_row_cnt++)
    for (tileimg_col_cnt = 0; tileimg_col_cnt < tileimg_num_cols; 
         tileimg_col_cnt++, vector++)
      for (row_cnt = 0; row_cnt < tile_num_rows; row_cnt++)
        for (col_cnt = 0; col_cnt < tile_num_cols; col_cnt++)
          image_array
            [tileimg_row_cnt*tile_num_rows + row_cnt]
            [tileimg_col_cnt*tile_num_cols + col_cnt] =
            dataset->vectors[vector][row_cnt*tile_num_cols + col_cnt];
  
  QccIMGImageComponentSetMin(image_component);
  QccIMGImageComponentSetMax(image_component);
  
  return(0);
}
