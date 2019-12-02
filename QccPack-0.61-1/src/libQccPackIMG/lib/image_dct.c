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


#ifndef QCC_USE_GSL


int QccIMGImageComponentDCT(const QccIMGImageComponent *input_component,
                            QccIMGImageComponent *output_component,
                            int block_num_rows,
                            int block_num_cols)
{
  int return_value;
  QccMatrix input_block = NULL;
  QccMatrix output_block = NULL;
  int image_num_rows, image_num_cols;
  int row, col;

  if (input_component == NULL)
    return(0);
  if (output_component == NULL)
    return(0);
  if (input_component->image == NULL)
    return(0);
  if (output_component->image == NULL)
    return(0);
  if ((block_num_rows <= 0) || (block_num_cols <= 0))
    return(0);
  
  image_num_rows = input_component->num_rows;
  image_num_cols = input_component->num_cols;
  
  if ((output_component->num_rows != image_num_rows) ||
      (output_component->num_cols != image_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Image components must have same size");
      goto Error;
    }
  
  if ((image_num_rows % block_num_rows) ||
      (image_num_cols % block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Size of image components is not an integer multiple of the block size");
      goto Error;
    }
  
  if ((input_block = QccMatrixAlloc(block_num_rows, block_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((output_block = QccMatrixAlloc(block_num_rows, block_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  for (row = 0; row < image_num_rows; row += block_num_rows)
    for (col = 0; col < image_num_cols; col += block_num_cols)
      {
        if (QccIMGImageComponentExtractBlock(input_component,
                                             row, col,
                                             input_block,
                                             block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccIMGImageComponentExtractBlock()");
            goto Error;
          }

        if (QccMatrixDCT(input_block, output_block,
                         block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccMatrixDCT()");
            goto Error;
          }

        if (QccIMGImageComponentInsertBlock(output_component,
                                            row, col,
                                            output_block,
                                            block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccIMGImageComponentInsertBlock()");
            goto Error;
          }
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(input_block, block_num_rows);
  QccMatrixFree(output_block, block_num_rows);
  return(return_value);
}


int QccIMGImageComponentInverseDCT(const QccIMGImageComponent *input_component,
                                   QccIMGImageComponent *output_component,
                                   int block_num_rows,
                                   int block_num_cols)
{
  int return_value;
  QccMatrix input_block = NULL;
  QccMatrix output_block = NULL;
  int image_num_rows, image_num_cols;
  int row, col;
  
  if (input_component == NULL)
    return(0);
  if (output_component == NULL)
    return(0);
  if (input_component->image == NULL)
    return(0);
  if (output_component->image == NULL)
    return(0);
  if ((block_num_rows <= 0) || (block_num_cols <= 0))
    return(0);
  
  image_num_rows = input_component->num_rows;
  image_num_cols = input_component->num_cols;
  
  if ((output_component->num_rows != image_num_rows) ||
      (output_component->num_cols != image_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Image components must have same size");
      goto Error;
    }
  
  if ((image_num_rows % block_num_rows) ||
      (image_num_cols % block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Size of image components is not an integer multiple of the block size");
      goto Error;
    }
  
  if ((input_block = QccMatrixAlloc(block_num_rows, block_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((output_block = QccMatrixAlloc(block_num_rows, block_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  for (row = 0; row < image_num_rows; row += block_num_rows)
    for (col = 0; col < image_num_cols; col += block_num_cols)
      {
        if (QccIMGImageComponentExtractBlock(input_component,
                                             row, col,
                                             input_block,
                                             block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccIMGImageComponentExtractBlock()");
            goto Error;
          }

        if (QccMatrixInverseDCT(input_block,
                                output_block,
                                block_num_rows,
                                block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccMatrixInverseDCT()");
            goto Error;
          }

        if (QccIMGImageComponentInsertBlock(output_component,
                                            row, col,
                                            output_block,
                                            block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccIMGImageComponentInsertBlock()");
            goto Error;
          }
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(input_block, block_num_rows);
  QccMatrixFree(output_block, block_num_rows);
  return(return_value);
}


#else


int QccIMGImageComponentDCT(const QccIMGImageComponent *input_component,
                            QccIMGImageComponent *output_component,
                            int block_num_rows,
                            int block_num_cols)
{
  int return_value;
  QccMatrix input_block = NULL;
  QccMatrix output_block = NULL;
  int image_num_rows, image_num_cols;
  int row, col;
  QccFastDCT transform_horizontal;
  QccFastDCT transform_vertical;

  QccFastDCTInitialize(&transform_horizontal);
  QccFastDCTInitialize(&transform_vertical);

  if (input_component == NULL)
    return(0);
  if (output_component == NULL)
    return(0);
  if (input_component->image == NULL)
    return(0);
  if (output_component->image == NULL)
    return(0);
  if ((block_num_rows <= 0) || (block_num_cols <= 0))
    return(0);
  
  image_num_rows = input_component->num_rows;
  image_num_cols = input_component->num_cols;
  
  if ((output_component->num_rows != image_num_rows) ||
      (output_component->num_cols != image_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Image components must have same size");
      goto Error;
    }
  
  if ((image_num_rows % block_num_rows) ||
      (image_num_cols % block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Size of image components is not an integer multiple of the block size");
      goto Error;
    }
  
  if ((input_block = QccMatrixAlloc(block_num_rows, block_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((output_block = QccMatrixAlloc(block_num_rows, block_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if (QccFastDCTCreate(&transform_horizontal,
                       block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccFastDCTCreate()");
      goto Error;
    }
  if (QccFastDCTCreate(&transform_vertical,
                       block_num_rows))
    {
      QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccFastDCTCreate()");
      goto Error;
    }

  for (row = 0; row < image_num_rows; row += block_num_rows)
    for (col = 0; col < image_num_cols; col += block_num_cols)
      {
        if (QccIMGImageComponentExtractBlock(input_component,
                                             row, col,
                                             input_block,
                                             block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccIMGImageComponentExtractBlock()");
            goto Error;
          }

        if (QccFastDCTForwardTransform2D(input_block,
                                         output_block,
                                         block_num_rows,
                                         block_num_cols,
                                         &transform_horizontal,
                                         &transform_vertical))
          {
            QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccMatrixDCT()");
            goto Error;
          }

        if (QccIMGImageComponentInsertBlock(output_component,
                                            row, col,
                                            output_block,
                                            block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentDCT): Error calling QccIMGImageComponentInsertBlock()");
            goto Error;
          }
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(input_block, block_num_rows);
  QccMatrixFree(output_block, block_num_rows);
  QccFastDCTFree(&transform_horizontal);
  QccFastDCTFree(&transform_vertical);
  return(return_value);
}


int QccIMGImageComponentInverseDCT(const QccIMGImageComponent *input_component,
                                   QccIMGImageComponent *output_component,
                                   int block_num_rows,
                                   int block_num_cols)
{
  int return_value;
  QccMatrix input_block = NULL;
  QccMatrix output_block = NULL;
  int image_num_rows, image_num_cols;
  int row, col;
  QccFastDCT transform_horizontal;
  QccFastDCT transform_vertical;

  QccFastDCTInitialize(&transform_horizontal);
  QccFastDCTInitialize(&transform_vertical);

  
  if (input_component == NULL)
    return(0);
  if (output_component == NULL)
    return(0);
  if (input_component->image == NULL)
    return(0);
  if (output_component->image == NULL)
    return(0);
  if ((block_num_rows <= 0) || (block_num_cols <= 0))
    return(0);
  
  image_num_rows = input_component->num_rows;
  image_num_cols = input_component->num_cols;
  
  if ((output_component->num_rows != image_num_rows) ||
      (output_component->num_cols != image_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Image components must have same size");
      goto Error;
    }
  
  if ((image_num_rows % block_num_rows) ||
      (image_num_cols % block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Size of image components is not an integer multiple of the block size");
      goto Error;
    }
  
  if ((input_block = QccMatrixAlloc(block_num_rows, block_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((output_block = QccMatrixAlloc(block_num_rows, block_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if (QccFastDCTCreate(&transform_horizontal,
                       block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccFastDCTCreate()");
      goto Error;
    }
  if (QccFastDCTCreate(&transform_vertical,
                       block_num_rows))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccFastDCTCreate()");
      goto Error;
    }

  for (row = 0; row < image_num_rows; row += block_num_rows)
    for (col = 0; col < image_num_cols; col += block_num_cols)
      {
        if (QccIMGImageComponentExtractBlock(input_component,
                                             row, col,
                                             input_block,
                                             block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccIMGImageComponentExtractBlock()");
            goto Error;
          }

        if (QccFastDCTInverseTransform2D(input_block,
                                         output_block,
                                         block_num_rows,
                                         block_num_cols,
                                         &transform_horizontal,
                                         &transform_vertical))
          {
            QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccFastDCTInverseTransform2D()");
            goto Error;
          }

        if (QccIMGImageComponentInsertBlock(output_component,
                                            row, col,
                                            output_block,
                                            block_num_rows, block_num_cols))
          {
            QccErrorAddMessage("(QccIMGImageComponentInverseDCT): Error calling QccIMGImageComponentInsertBlock()");
            goto Error;
          }
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(input_block, block_num_rows);
  QccMatrixFree(output_block, block_num_rows);
  QccFastDCTFree(&transform_horizontal);
  QccFastDCTFree(&transform_vertical);
  return(return_value);
}


#endif


int QccIMGImageDCT(const QccIMGImage *input_image,
                   QccIMGImage *output_image,
                   int block_num_rows, int block_num_cols)
{
  if (input_image == NULL)
    return(0);
  if (output_image == NULL)
    return(0);
  if ((block_num_rows <= 0) || (block_num_cols <= 0))
    return(0);
  
  if (QccIMGImageComponentDCT(&(input_image->Y),
                              &(output_image->Y),
                              block_num_rows, block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageDCT): Error calling QccIMGImageComponentDCT()");
      return(1);
    }
  
  if (QccIMGImageComponentDCT(&(input_image->U),
                              &(output_image->U),
                              block_num_rows, block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageDCT): Error calling QccIMGImageComponentDCT()");
      return(1);
    }
  if (QccIMGImageComponentDCT(&(input_image->V),
                              &(output_image->V),
                              block_num_rows, block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageDCT): Error calling QccIMGImageComponentDCT()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageInverseDCT(const QccIMGImage *input_image,
                          QccIMGImage *output_image,
                          int block_num_rows, int block_num_cols)
{
  if (input_image == NULL)
    return(0);
  if (output_image == NULL)
    return(0);
  if ((block_num_rows <= 0) || (block_num_cols <= 0))
    return(0);
  
  if (QccIMGImageComponentInverseDCT(&(input_image->Y),
                                     &(output_image->Y),
                                     block_num_rows, block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageInverseDCT): Error calling QccIMGImageComponentInverseDCT()");
      return(1);
    }
  
  if (QccIMGImageComponentInverseDCT(&(input_image->U),
                                     &(output_image->U),
                                     block_num_rows, block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageInverseDCT): Error calling QccIMGImageComponentInverseDCT()");
      return(1);
    }
  if (QccIMGImageComponentInverseDCT(&(input_image->V),
                                     &(output_image->V),
                                     block_num_rows, block_num_cols))
    {
      QccErrorAddMessage("(QccIMGImageInverseDCT): Error calling QccIMGImageComponentInverseDCT()");
      return(1);
    }
  
  return(0);
}
