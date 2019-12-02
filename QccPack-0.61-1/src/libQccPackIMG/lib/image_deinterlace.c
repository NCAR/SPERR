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



int QccIMGImageComponentDeinterlace(const
                                    QccIMGImageComponent *input_component,
                                    QccIMGImageComponent *output_component1,
                                    QccIMGImageComponent *output_component2)
{
  int num_rows, num_cols;
  int row, col;
  int prev_row, next_row;

  if ((input_component == NULL) ||
      (output_component1 == NULL) ||
      (output_component2 == NULL))
    return(0);

  if ((input_component->image == NULL) ||
      (output_component1->image == NULL) ||
      (output_component2->image == NULL))
    return(0);

  num_rows = input_component->num_rows;
  num_cols = input_component->num_cols;

  if ((num_rows != output_component1->num_rows) || 
      (num_rows != output_component2->num_rows) ||
      (num_cols != output_component1->num_cols) ||
      (num_cols != output_component2->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentDeinterlace): All images must have same size");
      return(1);
    }

  /*  First output frame  */

  /*  Top field  */
  for (row = 0; row < num_rows; row += 2)
    for (col = 0; col < num_cols; col++)
      output_component1->image[row][col] =
        input_component->image[row][col];
  
  /*  Bottom field  */
  for (row = 1; row < num_rows; row += 2)
    {
      prev_row = row - 1;
      next_row = (row + 1 < num_rows) ? row + 1 : prev_row;
      for (col = 0; col < num_cols; col++)
        output_component1->image[row][col] =
          0.5*(input_component->image[prev_row][col] +
               input_component->image[next_row][col]);
    }

  /*  Second output frame  */

  /*  Bottom field  */
  for (row = 1; row < num_rows; row += 2)
    for (col = 0; col < num_cols; col++)
      output_component2->image[row][col] =
        input_component->image[row][col];
  
  /*  Top field  */
  for (row = 0; row < num_rows; row += 2)
    {
      prev_row = row - 1;
      next_row = row + 1;
      prev_row = (prev_row >= 0) ? prev_row : next_row;
      next_row = (next_row < num_rows) ? next_row : prev_row;
      for (col = 0; col < num_cols; col++)
        output_component2->image[row][col] =
          0.5*(input_component->image[prev_row][col] +
               input_component->image[next_row][col]);
    }

  QccIMGImageComponentSetMin(output_component1);
  QccIMGImageComponentSetMin(output_component2);
  QccIMGImageComponentSetMax(output_component1);
  QccIMGImageComponentSetMax(output_component2);

  return(0);
}


int QccIMGImageDeinterlace(const QccIMGImage *input_image, 
                           QccIMGImage *output_image1,
                           QccIMGImage *output_image2)
{
  if ((input_image == NULL) || (output_image1 == NULL) ||
      (output_image2 == NULL))
    return(0);

  if (QccIMGImageComponentDeinterlace(&(input_image->Y),
                                      &(output_image1->Y),
                                      &(output_image2->Y)))
    {
      QccErrorAddMessage("(QccIMGImageDeinterlace): Error calling QccIMGImageComponentDeinterlace()");
      return(1);
    }

  if (QccIMGImageComponentDeinterlace(&(input_image->U),
                                      &(output_image1->U),
                                      &(output_image2->U)))
    {
      QccErrorAddMessage("(QccIMGImageDeinterlace): Error calling QccIMGImageComponentDeinterlace()");
      return(1);
    }

  if (QccIMGImageComponentDeinterlace(&(input_image->V),
                                      &(output_image1->V),
                                      &(output_image2->V)))
    {
      QccErrorAddMessage("(QccIMGImageDeinterlace): Error calling QccIMGImageComponentDeinterlace()");
      return(1);
    }

  return(0);
}


int QccIMGImageSequenceDeinterlace(QccIMGImageSequence *input_sequence,
                                   QccIMGImageSequence *output_sequence,
                                   int supersample)
{
  QccIMGImage next_frame;
  int num_rows_Y, num_cols_Y;
  int num_rows_U, num_cols_U;
  int num_rows_V, num_cols_V;
  int current_frame;

  if ((input_sequence == NULL) || (output_sequence == NULL))
    return(0);

  if (QccIMGImageSequenceStartRead(input_sequence))
    {
      QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageSequenceStartRead()");
      return(1);
    }

  next_frame.Y.image = next_frame.U.image = 
    next_frame.V.image = NULL;
  next_frame.image_type = input_sequence->current_frame.image_type;

  if (QccIMGImageGetSizeYUV(&(input_sequence->current_frame),
                            &num_rows_Y, &num_cols_Y,
                            &num_rows_U, &num_cols_U,
                            &num_rows_V, &num_cols_V))
    {
      QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageGetSizeYUV()");
      return(1);
    }
  if (QccIMGImageSetSizeYUV(&(next_frame),
                            num_rows_Y, num_cols_Y,
                            num_rows_U, num_cols_U,
                            num_rows_V, num_cols_V))
    {
      QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageSetSizeYUV()");
      return(1);
    }

  if (QccIMGImageAlloc(&next_frame))
    {
      QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageSequenceStartRead()");
      return(1);
    }

  output_sequence->start_frame_num = input_sequence->start_frame_num;
  output_sequence->end_frame_num = 
    (supersample) ? input_sequence->end_frame_num :
    input_sequence->start_frame_num +
    2*QccIMGImageSequenceLength(input_sequence) - 1;

  
  output_sequence->current_frame_num = output_sequence->start_frame_num - 1;
  input_sequence->current_frame_num = input_sequence->start_frame_num - 1;

  for (current_frame = input_sequence->start_frame_num;
       current_frame <= input_sequence->end_frame_num;
       current_frame++)
    {
      if (QccIMGImageSequenceIncrementFrameNum(input_sequence))
        {
          QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageSequenceIncrementFrameNum()");
          return(1);
        }
      if (QccIMGImageSequenceIncrementFrameNum(output_sequence))
        {
          QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageSequenceIncrementFrameNum()");
          return(1);
        }

      if (QccIMGImageSequenceReadFrame(input_sequence))
        {
          QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageSequenceReadFrame()");
          return(1);
        }

      if (QccIMGImageDeinterlace(&(input_sequence->current_frame),
                                 &(output_sequence->current_frame),
                                 &(next_frame)))
        {
          QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageDeinterlace()");
          return(1);
        }

      if (QccIMGImageSequenceWriteFrame(output_sequence))
        {
          QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImagesequenceWriteFrame()");
          return(1);
        }

      if (!supersample)
        {
          if (QccIMGImageSequenceIncrementFrameNum(output_sequence))
            {
              QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageSequenceIncrementFrameNum()");
              return(1);
            }

          if (QccIMGImageCopy(&(output_sequence->current_frame), 
                              &(next_frame)))
            {
              QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImageCopy()");
              return(1);
            }
          
          if (QccIMGImageSequenceWriteFrame(output_sequence))
            {
              QccErrorAddMessage("(QccIMGImageSequenceDeinterlace): Error calling QccIMGImagesequenceWriteFrame()");
              return(1);
            }
        }

    }

  QccIMGImageFree(&next_frame);

  return(0);
}
