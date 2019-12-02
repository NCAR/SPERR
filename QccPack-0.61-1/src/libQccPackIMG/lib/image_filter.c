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



int QccIMGImageComponentFilterSeparable(const 
                                        QccIMGImageComponent *input_image,
                                        QccIMGImageComponent *output_image,
                                        const QccFilter *horizontal_filter,
                                        const QccFilter *vertical_filter,
                                        int boundary_extension)
{
  if ((input_image == NULL) || (output_image == NULL))
    return(0);
  if ((horizontal_filter == NULL) || (vertical_filter == NULL))
    return(0);
  
  if ((input_image->image == NULL) || (output_image->image == NULL))
    return(0);
  
  if ((input_image->num_rows != output_image->num_rows) ||
      (input_image->num_cols != output_image->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentFilterSeparable): Error input and output images have different sizes");
      return(1);
    }

  if (QccFilterMatrixSeparable(input_image->image,
                               output_image->image,
                               input_image->num_rows,
                               input_image->num_cols,
                               horizontal_filter,
                               vertical_filter,
                               boundary_extension))
    {
      QccErrorAddMessage("(QccIMGImageComponentFilterSeparable): Error calling QccFilterMatrixSeparable()");
      return(1);
    }

  return(0);
}


int QccIMGImageFilterSeparable(const QccIMGImage *input_image,
                               QccIMGImage *output_image,
                               const QccFilter *horizontal_filter,
                               const QccFilter *vertical_filter,
                               int boundary_extension)
{
  if ((input_image == NULL) || (output_image == NULL))
    return(0);
  if ((horizontal_filter == NULL) || (vertical_filter == NULL))
    return(0);
  
  if (QccIMGImageComponentFilterSeparable(&(input_image->Y),
                                          &(output_image->Y),
                                          horizontal_filter,
                                          vertical_filter,
                                          boundary_extension))
    {
      QccErrorAddMessage("(QccIMGImageFilterSeparable): Error calling QccIMGImageComponentFilterSeparable()");
      return(1);
    }
  
  if (QccIMGImageComponentFilterSeparable(&(input_image->U),
                                          &(output_image->U),
                                          horizontal_filter,
                                          vertical_filter,
                                          boundary_extension))
    {
      QccErrorAddMessage("(QccIMGImageFilterSeparable): Error calling QccIMGImageComponentFilterSeparable()");
      return(1);
    }
  
  if (QccIMGImageComponentFilterSeparable(&(input_image->V),
                                          &(output_image->V),
                                          horizontal_filter,
                                          vertical_filter,
                                          boundary_extension))
    {
      QccErrorAddMessage("(QccIMGImageFilterSeparable): Error calling QccIMGImageComponentFilterSeparable()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageComponentFilter2D(const QccIMGImageComponent *input_image,
                                 QccIMGImageComponent *output_image,
                                 const QccIMGImageComponent *filter)
{
  int return_value;
  int input_row, input_col;
  int output_row, output_col;
  int filter_row, filter_col;
  int filter_row_center, filter_col_center;

  if (input_image == NULL)
    return(0);
  if (output_image == NULL)
    return(0);
  if (filter == NULL)
    return(0);

  if (input_image->image == NULL)
    return(0);
  if (output_image->image == NULL)
    return(0);
  if (filter->image == NULL)
    return(0);

  if ((filter->num_rows <= 0) ||
      (!(filter->num_rows & 1)) ||
      (filter->num_cols <= 0) ||
      (!(filter->num_cols & 1)))
    {
      QccErrorAddMessage("(QccIMGImageComponentFilter2D): Filter size must have odd number of rows and columns");
      goto Error;
    }

  if ((output_image->num_rows != input_image->num_rows) ||
      (output_image->num_cols != input_image->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentFilter2D): input and output images must have same size");
      goto Error;
    }

  filter_row_center = filter->num_rows/2;
  filter_col_center = filter->num_cols/2;

  for (output_row = 0; output_row < output_image->num_rows; output_row++)
    for (output_col = 0; output_col < output_image->num_cols; output_col++)
      {
        output_image->image[output_row][output_col] = 0;
        for (filter_row = filter->num_rows - 1;
             filter_row >= 0; filter_row--)
          for (filter_col = filter->num_cols - 1;
               filter_col >= 0; filter_col--)
            {
              input_row =
                output_row - (filter_row - filter_row_center);
              input_col =
                output_col - (filter_col - filter_col_center);
              if (input_row < 0)
                input_row = -input_row;
              else
                if (input_row >= input_image->num_rows)
                  input_row = input_image->num_rows -
                    (input_row - input_image->num_rows) - 2;
              if (input_col < 0)
                input_col = -input_col;
              else
                if (input_col >= input_image->num_cols)
                  input_col = input_image->num_cols -
                    (input_col - input_image->num_cols) - 2;
              output_image->image[output_row][output_col] +=
                filter->image[filter_row][filter_col] *
                input_image->image[input_row][input_col];
            }
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccIMGImageFilter2D(const QccIMGImage *input_image,
                        QccIMGImage *output_image,
                        const QccIMGImageComponent *filter)
{
  if ((input_image == NULL) || (output_image == NULL))
    return(0);
  if (filter == NULL)
    return(0);
  
  if (QccIMGImageComponentFilter2D(&(input_image->Y),
                                   &(output_image->Y),
                                   filter))
    {
      QccErrorAddMessage("(QccIMGImageFilter2D): Error calling QccIMGImageComponentFilter2D()");
      return(1);
    }

  if (QccIMGImageComponentFilter2D(&(input_image->U),
                                   &(output_image->U),
                                   filter))
    {
      QccErrorAddMessage("(QccIMGImageFilter2D): Error calling QccIMGImageComponentFilter2D()");
      return(1);
    }

  if (QccIMGImageComponentFilter2D(&(input_image->V),
                                   &(output_image->V),
                                   filter))
    {
      QccErrorAddMessage("(QccIMGImageFilter2D): Error calling QccIMGImageComponentFilter2D()");
      return(1);
    }
  
  return(0);
}


