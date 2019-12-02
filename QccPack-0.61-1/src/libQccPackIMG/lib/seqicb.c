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


int QccIMGImageSequenceToImageCube(QccIMGImageSequence *image_sequence,
                                   QccIMGImageCube *image_cube)
{
  int frame, row, col;
  int num_rows, num_cols;
  int current_frame;

  if ((image_sequence == NULL) || (image_cube == NULL))
    return(0);

  if (QccIMGImageGetSize(&(image_sequence->current_frame),
                         &num_rows, &num_cols))
    {
      QccErrorAddMessage("(QccIMGImageSequenceToImageCube): Error calling QccIMGImageGetSize()");
      return(1);
    }

  if ((num_rows != image_cube->num_rows) ||
      (num_cols != image_cube->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageSequenceToImageCube): Image cube must have same number of rows and columns as frames in sequence");
      return(1);
    }
  if (QccIMGImageColor(&(image_sequence->current_frame)))
    {
      QccErrorAddMessage("(QccIMGImageSequenceToImageCube): All frames of image sequence must be grayscale");
      return(1);
    }

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      image_cube->volume[0][row][col] =
        image_sequence->current_frame.Y.image[row][col];

  for (frame = 1, current_frame = image_sequence->start_frame_num + 1;
       current_frame <= image_sequence->end_frame_num;
       current_frame++, frame++)
    {
      if (QccIMGImageSequenceIncrementFrameNum(image_sequence))
        {
          QccErrorAddMessage("(QccIMGImageSequenceToImageCube): Error calling QccIMGImageSequenceIncrementFrameNum()");
          return(1);
        }
      if (QccIMGImageSequenceReadFrame(image_sequence))
        {
          QccErrorAddMessage("(QccIMGImageSequenceToImageCube): Error calling QccIMGImageSequenceReadFrame()");
          return(1);
        }

      if ((image_sequence->current_frame.Y.num_rows != num_rows) ||
          (image_sequence->current_frame.Y.num_cols != num_cols))
        {
          QccErrorAddMessage("(QccIMGImageSequenceToImageCube): All frames of sequence must have same size");
          return(1);
        }

      if (QccIMGImageColor(&(image_sequence->current_frame)))
        {
          QccErrorAddMessage("(QccIMGImageSequenceToImageCube): All frames of image sequence must be grayscale");
          return(1);
        }

      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          image_cube->volume[frame][row][col] =
            image_sequence->current_frame.Y.image[row][col];
    }

  if (QccIMGImageCubeSetMaxMin(image_cube))
    {
      QccErrorAddMessage("(QccIMGImageSequenceToImageCube): Error calling QccIMGImageCubeSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccIMGImageCubeToImageSequence(const QccIMGImageCube *image_cube,
                                   QccIMGImageSequence *image_sequence)
{
  int frame, row, col;

  if ((image_sequence == NULL) || (image_cube == NULL))
    return(0);

  image_sequence->current_frame_num =
    image_sequence->start_frame_num;

  for (frame = 0; frame < image_cube->num_frames; frame++)
    {
      for (row = 0; row < image_cube->num_rows; row++)
        for (col = 0; col < image_cube->num_cols; col++)
          image_sequence->current_frame.Y.image[row][col] =
            image_cube->volume[frame][row][col];

      if (QccIMGImageSequenceWriteFrame(image_sequence))
        {
          QccErrorAddMessage("(QccIMGImageCubeToImageSequence): Error calling QccIMGImageSequenceWriteFrame()");
          return(1);
        }

      if (QccIMGImageSequenceIncrementFrameNum(image_sequence))
        {
          QccErrorAddMessage("(QccIMGImageCubeToImageSequence): Error calling QccIMGImageSequenceIncrementFrameNum()");
          return(1);
        }
    }
  
  return(0);
}
