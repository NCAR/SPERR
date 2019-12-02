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


double QccHYPImageCubeMeanSAM(const QccIMGImageCube *image_cube1,
                              const QccIMGImageCube *image_cube2)
{
  int frame, row, col;
  double mean_sam = 0.0;
  QccVector vector1 = NULL;
  QccVector vector2 = NULL;

  if (image_cube1 == NULL)
    goto Return;
  if (image_cube2 == NULL)
    goto Return;
  if ((image_cube1->volume == NULL) || (image_cube2->volume == NULL))
    goto Return;

  if ((image_cube1->num_frames != image_cube2->num_frames) ||
      (image_cube1->num_rows != image_cube2->num_rows) ||
      (image_cube1->num_cols != image_cube2->num_cols))
    goto Return;

  if ((vector1 = QccVectorAlloc(image_cube1->num_frames)) == NULL)
    goto Return;
  if ((vector2 = QccVectorAlloc(image_cube1->num_frames)) == NULL)
    goto Return;

  for (row = 0; row < image_cube1->num_rows; row++)
    for (col = 0; col < image_cube1->num_cols; col++)
      {
        for (frame = 0; frame < image_cube1->num_frames; frame++)
          {
            vector1[frame] = image_cube1->volume[frame][row][col];
            vector2[frame] = image_cube2->volume[frame][row][col];
          }

        mean_sam +=
          QccVectorAngle(vector1,
                         vector2,
                         image_cube1->num_frames,
                         1) /
          image_cube1->num_rows /
          image_cube1->num_cols;
      }

 Return:
  QccVectorFree(vector1);
  QccVectorFree(vector2);
  return(mean_sam);
}
