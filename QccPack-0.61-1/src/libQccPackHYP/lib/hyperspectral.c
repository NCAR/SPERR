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


int QccHYPImageCubeToColor(const QccIMGImageCube *image_cube,
                           QccIMGImage *image,
                           int red_band,
                           int green_band,
                           int blue_band)
{
  int return_value;
  int num_rowsY, num_colsY;
  int num_rowsU, num_colsU;
  int num_rowsV, num_colsV;
  QccIMGImageComponent red;
  QccIMGImageComponent green;
  QccIMGImageComponent blue;
  int row, col;

  QccIMGImageComponentInitialize(&red);
  QccIMGImageComponentInitialize(&green);
  QccIMGImageComponentInitialize(&blue);

  if (image_cube == NULL)
    return(0);
  if (image == NULL)
    return(0);

  if ((red_band < 0) || (red_band >= image_cube->num_frames))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Invalid red band");
      goto Error;
    }
  if ((green_band < 0) || (green_band >= image_cube->num_frames))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Invalid green band");
      goto Error;
    }
  if ((blue_band < 0) || (blue_band >= image_cube->num_frames))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Invalid blue band");
      goto Error;
    }

  if (!QccIMGImageColor(image))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Output image must be color");
      goto Error;
    }

  if (QccIMGImageGetSizeYUV(image,
                            &num_rowsY, &num_colsY,
                            &num_rowsU, &num_colsU,
                            &num_rowsV, &num_colsV))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageGetSizeYUV()");
      goto Error;
    }

  if ((num_rowsY != image_cube->num_rows) ||
      (num_rowsU != image_cube->num_rows) ||
      (num_rowsV != image_cube->num_rows) ||
      (num_colsY != image_cube->num_cols) ||
      (num_colsU != image_cube->num_cols) ||
      (num_colsV != image_cube->num_cols))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Each component of image must be same size (spatially) as that of image cube");
      goto Error;
    }

  red.num_rows = image_cube->num_rows;
  red.num_cols = image_cube->num_cols;
  if (QccIMGImageComponentAlloc(&red))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  green.num_rows = image_cube->num_rows;
  green.num_cols = image_cube->num_cols;
  if (QccIMGImageComponentAlloc(&green))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  blue.num_rows = image_cube->num_rows;
  blue.num_cols = image_cube->num_cols;
  if (QccIMGImageComponentAlloc(&blue))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }

  if (QccIMGImageCubeExtractFrame(image_cube,
                                  red_band,
                                  &red))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageCubeExtractFrame()");
      goto Error;
    }
  if (QccIMGImageCubeExtractFrame(image_cube,
                                  green_band,
                                  &green))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageCubeExtractFrame()");
      goto Error;
    }
  if (QccIMGImageCubeExtractFrame(image_cube,
                                  blue_band,
                                  &blue))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageCubeExtractFrame()");
      goto Error;
    }

  if (QccIMGImageComponentNormalize(&red))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageComponentNormalize()");
      goto Error;
    }
  if (QccIMGImageComponentNormalize(&green))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageComponentNormalize()");
      goto Error;
    }
  if (QccIMGImageComponentNormalize(&blue))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageComponentNormalize()");
      goto Error;
    }

  for (row = 0; row < image_cube->num_rows; row++)
    for (col = 0; col < image_cube->num_cols; col++)
      QccIMGImageRGBtoYUV(&(image->Y.image[row][col]),
                          &(image->U.image[row][col]),
                          &(image->V.image[row][col]),
                          red.image[row][col],
                          green.image[row][col],
                          blue.image[row][col]);
  
  if (QccIMGImageSetMaxMin(image))
    {
      QccErrorAddMessage("(QccHYPImageCubeToColor): Error calling QccIMGImageSetMaxMin()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&red);
  QccIMGImageComponentFree(&green);
  QccIMGImageComponentFree(&blue);
  return(return_value);
}
