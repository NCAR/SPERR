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
 *  This code was written by Justin T. Rucker <jtr9@msstate.edu>
 */


#include "libQccPack.h"


static QccIMGImageVolume QccIMGImageVolumeAlloc(int num_frames,
                                                int num_rows,
                                                int num_cols)
{
  QccIMGImageVolume new_volume;
  
  if ((num_frames <=0) || (num_rows <= 0) || (num_cols <= 0))
    return(NULL);
  
  if ((new_volume =
       (QccIMGImageVolume)QccVolumeAlloc(num_frames,
                                         num_rows,
                                         num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageVolumeAlloc): Error allocating memory");
      return(NULL);
    }
  
  return(new_volume);
}


static void QccIMGImageVolumeFree(QccIMGImageVolume image_volume,
                                  int num_frames,
                                  int num_rows)
{
  QccVolumeFree((QccVolume)image_volume, num_frames, num_rows);
}


int QccIMGImageCubeInitialize(QccIMGImageCube *image_cube)
{
  if (image_cube == NULL)
    return(0);
  
  QccStringMakeNull(image_cube->filename);
  QccStringCopy(image_cube->magic_num, QCCIMGIMAGECUBE_MAGICNUM);
  QccGetQccPackVersion(&image_cube->major_version,
                       &image_cube->minor_version,
                       NULL);
  image_cube->num_frames = 0;
  image_cube->num_rows = 0;
  image_cube->num_cols = 0;
  image_cube->min_val = 0.0;
  image_cube->max_val = 0.0;
  image_cube->volume = NULL;
  
  return(0);
}


int QccIMGImageCubeAlloc(QccIMGImageCube *image_cube)
{
  if (image_cube == NULL)
    return(0);
  
  if (image_cube->volume == NULL)
    {
      if ((image_cube->num_frames > 0) ||
          (image_cube->num_rows > 0) ||
          (image_cube->num_cols > 0))
        {
          if ((image_cube->volume =
               QccIMGImageVolumeAlloc(image_cube->num_frames,
                                      image_cube->num_rows,
                                      image_cube->num_cols)) == NULL)
            {
              QccErrorAddMessage("(QccIMGImageCubeAlloc): Error calling QccIMGImageVolumeAlloc()");
              return(1);
            }
        }
      else
        image_cube->volume = NULL;
    }
  
  return(0);
}


void QccIMGImageCubeFree(QccIMGImageCube *image_cube)
{
  if (image_cube == NULL)
    return;
  
  if (image_cube->volume != NULL)
    {
      QccIMGImageVolumeFree(image_cube->volume,
                            image_cube->num_frames,
                            image_cube->num_rows);
      image_cube->volume = NULL;
    }
}


int QccIMGImageCubePrint(const QccIMGImageCube *image_cube)
{
  int frame, row, col;
  
  if (QccFilePrintFileInfo(image_cube->filename,
                           image_cube->magic_num,
                           image_cube->major_version,
                           image_cube->minor_version))
    return(1);
  
  printf("Size: %dx%dx%d\n",
         image_cube->num_cols,
         image_cube->num_rows,
         image_cube->num_frames);
  
  printf("Min. value: % 10.4f\n", image_cube->min_val);
  printf("Max. value: % 10.4f\n", image_cube->max_val);
  printf("\nImage volume:\n\n");
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    {
      for (row = 0; row < image_cube->num_rows; row++)
        {
          for (col = 0; col < image_cube->num_cols; col++)
            {
              printf("% 10.4f ", image_cube->volume[frame][row][col]);
            }
          printf("\n");
        }
      printf("============================================================\n");
    }

  return(0);
}


int QccIMGImageCubeSetMin(QccIMGImageCube *image_cube)
{
  
  int frame, row, col;
  double min = MAXDOUBLE;
  
  if (image_cube == NULL)
    return(0);
  if (image_cube->volume == NULL)
    return(0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        if (image_cube->volume[frame][row][col] < min)
          min = image_cube->volume[frame][row][col];
  
  image_cube->min_val = min;
  
  return(0);
}


int QccIMGImageCubeSetMax(QccIMGImageCube *image_cube)
{
  double max = -MAXDOUBLE;
  int frame, row, col;
  
  if (image_cube == NULL)
    return(0);
  if (image_cube->volume == NULL)
    return(0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        if (image_cube->volume[frame][row][col] > max)
          max = image_cube->volume[frame][row][col];
  
  image_cube->max_val = max;
  
  return(0);
}


int QccIMGImageCubeSetMaxMin(QccIMGImageCube *image_cube)
{
  if (QccIMGImageCubeSetMax(image_cube))
    {
      QccErrorAddMessage("(QccIMGImageCubeSetMaxMin): Error calling QccIMGImageCubeSetMax()");
      return(1);
    }
  
  if (QccIMGImageCubeSetMin(image_cube))
    {
      QccErrorAddMessage("(QccIMGImageCubeSetMaxMin): Error calling QccIMGImageCubeSetMin()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageCubeResize(QccIMGImageCube *image_cube,
                          int num_frames,
                          int num_rows,
                          int num_cols)
{
  if (image_cube == NULL)
    return(0);

  if ((image_cube->volume =
       (QccIMGImageVolume)QccVolumeResize((QccVolume)image_cube->volume,
                                          image_cube->num_frames,
                                          image_cube->num_rows,
                                          image_cube->num_cols,
                                          num_frames,
                                          num_rows,
                                          num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageCubeResize): Error calling QccVolumeResize()");
      return(1);
    }

  image_cube->num_frames = num_frames;
  image_cube->num_rows = num_rows;
  image_cube->num_cols = num_cols;
  
  QccIMGImageCubeSetMaxMin(image_cube);

  return(0);
}


static int QccIMGImageCubeReadHeader(FILE *infile,
                                     QccIMGImageCube *image_cube)
{
  if ((infile == NULL) || (image_cube == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             image_cube->magic_num,
                             &image_cube->major_version,
                             &image_cube->minor_version))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading magic number in %s",
                         image_cube->filename);
      return(1);
    }
  
  if (strcmp(image_cube->magic_num, QCCIMGIMAGECUBE_MAGICNUM))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): %s is not of image_cube (%s) type",
                         image_cube->filename, QCCIMGIMAGECUBE_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(image_cube->num_cols));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of columns in %s",
                         image_cube->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of columns in %s",
                         image_cube->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(image_cube->num_rows));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of rows in %s",
                         image_cube->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of rows in %s",
                         image_cube->filename);
      return(1);
    }
  fscanf(infile, "%d", &(image_cube->num_frames));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of frames in %s",
                         image_cube->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of frames in %s",
                         image_cube->filename);
      return(1);
    }
  
  fscanf(infile, "%lf", &(image_cube->min_val));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of rows in %s",
                         image_cube->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of rows in %s",
                         image_cube->filename);
      return(1);
    }
  
  fscanf(infile, "%lf%*1[\n]", &(image_cube->max_val));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageCubeReadHeader): Error reading number of rows in %s",
                         image_cube->filename);
      return(1);
    }
  
  return(0);
}


static int QccIMGImageCubeReadData(FILE *infile,
                                   QccIMGImageCube *image_cube)
{
  int frame, row, col;
  
  if ((infile == NULL) ||
      (image_cube == NULL))
    return(0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        if (QccFileReadDouble(infile,
                              &(image_cube->volume[frame][row][col])))
          {
            QccErrorAddMessage("(QccIMGImageCubeReadData): Error calling QccFileReadDouble()",
                               image_cube->filename);
            return(1);
          }
  
  return(0);
}


int QccIMGImageCubeRead(QccIMGImageCube *image_cube)
{
  FILE *infile = NULL;
  
  if (image_cube == NULL)
    return(0);
  
  if ((infile = QccFileOpen(image_cube->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageCubeRead): Error opening %s for reading",
                         image_cube->filename);
      return(1);
    }
  
  if (QccIMGImageCubeReadHeader(infile, image_cube))
    {
      QccErrorAddMessage("(QccIMGImageCubeRead): Error calling QccIMGImageCubeReadHeader()");
      return(1);
    }
  
  if (QccIMGImageCubeAlloc(image_cube))
    {
      QccErrorAddMessage("(QccIMGImageCubeRead): Error calling QccIMGImageCubeAlloc()");
      return(1);
    }
  
  if (QccIMGImageCubeReadData(infile, image_cube))
    {
      QccErrorAddMessage("(QccIMGImageCubeRead): Error calling QccIMGImageCubeReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


static int QccIMGImageCubeWriteHeader(FILE *outfile,
                                      const QccIMGImageCube *image_cube)
{
  if ((outfile == NULL) || (image_cube == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCIMGIMAGECUBE_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d %d %d\n",
          image_cube->num_cols,
          image_cube->num_rows,
          image_cube->num_frames);
  if (ferror(outfile))
    goto Error;
  
  fprintf(outfile, "% 16.9e % 16.9e\n",
          image_cube->min_val,
          image_cube->max_val);
  if (ferror(outfile))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccIMGImageCubeWriteHeader): Error writing header to %s",
                     image_cube->filename);
  return(1);
}


static int QccIMGImageCubeWriteData(FILE *outfile,
                                    const QccIMGImageCube *image_cube)
{
  int frame, row, col;
  
  if ((image_cube == NULL) ||
      (outfile == NULL))
    return(0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        if (QccFileWriteDouble(outfile,
                               image_cube->volume[frame][row][col]))
          {
            QccErrorAddMessage("(QccIMGImageCubeWriteData): Error calling QccFileWriteDouble()");
            return(1);
          }
  
  return(0);
}


int QccIMGImageCubeWrite(const QccIMGImageCube *image_cube)
{
  FILE *outfile;
  
  if (image_cube == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(image_cube->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageCubeWrite): Error opening %s for writing",
                         image_cube->filename);
      return(1);
    }
  
  if (QccIMGImageCubeWriteHeader(outfile, image_cube))
    {
      QccErrorAddMessage("(QccIMGImageCubeWrite): Error calling QccIMGImageCubeWriteHeader()");
      return(1);
    }
  if (QccIMGImageCubeWriteData(outfile, image_cube))
    {
      QccErrorAddMessage("(QccIMGImageCubeWrite): Error calling QccIMGImageCubeWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


int QccIMGImageCubeClip(QccIMGImageCube *image_cube)
{
  int frame, row, col;
  
  if (image_cube == NULL)
    return(0);
  if (image_cube->volume == NULL)
    return(0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        image_cube->volume[frame][row][col] =
          QccIMGImageComponentClipPixel(image_cube->volume[frame][row][col]);
  
  if (QccIMGImageCubeSetMaxMin(image_cube))
    {
      QccErrorAddMessage("(QccIMGImageCubeClip): Error calling QccIMGImageCubeSetMaxMin()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageCubeNormalize(QccIMGImageCube *image_cube,
                             double upper_bound,
                             double lower_bound)
{
  int frame, row, col;
  double range;
  double min;
  
  if (image_cube == NULL)
    return(0);
  if (image_cube->volume == NULL)
    return(0);
  
  range = image_cube->max_val - image_cube->min_val;
  min = image_cube->min_val;
  
  if (range == 0.0)
    return(0);

  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        image_cube->volume[frame][row][col] =
          (upper_bound - lower_bound) *
          (image_cube->volume[frame][row][col] - min) /
          range + lower_bound;
  
  QccIMGImageCubeSetMin(image_cube);
  QccIMGImageCubeSetMax(image_cube);
  
  return(0);
}


int QccIMGImageCubeAbsoluteValue(QccIMGImageCube *image_cube)
{
  int frame, row, col;
  
  if (image_cube == NULL)
    return(0);
  if (image_cube->volume == NULL)
    return(0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        image_cube->volume[frame][row][col] =
          fabs(image_cube->volume[frame][row][col]);
  
  QccIMGImageCubeSetMin(image_cube);
  QccIMGImageCubeSetMax(image_cube);
  
  return(0);
}


double QccIMGImageCubeMean(const QccIMGImageCube *image_cube)
{
  double sum = 0.0;
  int frame, row, col;
  
  if (image_cube == NULL)
    return(0.0);
  if (image_cube->volume == NULL)
    return(0.0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        sum += image_cube->volume[frame][row][col] /
          image_cube->num_rows / image_cube->num_cols / image_cube->num_frames;
  
  return(sum);
}


double QccIMGImageCubeShapeAdaptiveMean(const QccIMGImageCube *image_cube,
                                        const QccIMGImageCube *alpha_mask)
{
  double sum = 0.0;
  int frame, row, col;
  int cnt = 0;
  
  if (image_cube == NULL)
    return(0.0);
  if (image_cube->volume == NULL)
    return(0.0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        if (!QccAlphaTransparent(alpha_mask->volume[frame][row][col]))
          cnt++;

  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        if (!QccAlphaTransparent(alpha_mask->volume[frame][row][col]))
          sum += image_cube->volume[frame][row][col] / cnt;
  
  return(sum);
}


double QccIMGImageCubeVariance(const QccIMGImageCube *image_cube)
{
  double sum = 0.0;
  double mean;
  int frame, row, col;
  
  if (image_cube == NULL)
    return(0.0);
  if (image_cube->volume == NULL)
    return(0.0);
  
  mean = QccIMGImageCubeMean(image_cube);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        sum +=
          (image_cube->volume[frame][row][col] - mean)*
          (image_cube->volume[frame][row][col] - mean) /
          image_cube->num_rows / image_cube->num_cols / image_cube->num_frames;

  return(sum);
}


double QccIMGImageCubeShapeAdaptiveVariance(const QccIMGImageCube *image_cube,
                                            const QccIMGImageCube *alpha_mask)
{
  double sum = 0.0;
  double mean;
  int frame, row, col;
  int cnt = 0;
  
  if (image_cube == NULL)
    return(0.0);
  if (image_cube->volume == NULL)
    return(0.0);
  
  if (alpha_mask == NULL)
    return(QccIMGImageCubeVariance(image_cube));
  if (alpha_mask->volume == NULL)
    return(QccIMGImageCubeVariance(image_cube));
  
  mean = QccIMGImageCubeShapeAdaptiveMean(image_cube, alpha_mask);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        if (!QccAlphaTransparent(alpha_mask->volume[frame][row][col]))
          cnt++;

  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        if (!QccAlphaTransparent(alpha_mask->volume[frame][row][col]))
          sum +=
            (image_cube->volume[frame][row][col] - mean) *
            (image_cube->volume[frame][row][col] - mean) / cnt;

  return(sum);
}


int QccIMGImageCubeSubtractMean(QccIMGImageCube *image_cube,
                                double *mean,
                                const QccSQScalarQuantizer *quantizer)
{
  int frame, row, col;
  int partition;
  
  double mean1;
  
  if (image_cube == NULL)
    return(0);
  if (image_cube->volume == NULL)
    return(0);
  
  mean1 = QccIMGImageCubeMean(image_cube);
  
  if (quantizer != NULL)
    {
      if (QccSQScalarQuantization(mean1, quantizer, NULL, &partition))
        {
          QccErrorAddMessage("(QccIMGImageCubeSubtractMean): Error calling QccSQScalarQuantization()");
          return(1);
        }
      if (QccSQInverseScalarQuantization(partition, quantizer, &mean1))
        {
          QccErrorAddMessage("(QccIMGImageCubeSubtractMean): Error calling QccSQInverseScalarQuantization()");
          return(1);
        }
    }
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        image_cube->volume[frame][row][col] -= mean1;
  
  if (mean != NULL)
    *mean = mean1;
  
  QccIMGImageCubeSetMin(image_cube);
  QccIMGImageCubeSetMax(image_cube);
  
  return(0);
}


int QccIMGImageCubeAddMean(QccIMGImageCube *image_cube,
                           double mean)
{
  int frame, row, col;
  
  if (image_cube == NULL)
    return(0);
  if (image_cube->volume == NULL)
    return(0);
  
  for (frame = 0; frame < image_cube->num_frames; frame++)
    for (row = 0; row < image_cube->num_rows; row++)
      for (col = 0; col < image_cube->num_cols; col++)
        image_cube->volume[frame][row][col] += mean;
  
  QccIMGImageCubeSetMin(image_cube);
  QccIMGImageCubeSetMax(image_cube);
  
  return(0);
}


double QccIMGImageCubeMSE(const QccIMGImageCube *image_cube1,
                          const QccIMGImageCube *image_cube2)
{
  double sum = 0.0;
  int frame, row, col;
  
  if (image_cube1 == NULL)
    return(0.0);
  if (image_cube2 == NULL)
    return(0.0);
  if ((image_cube1->volume == NULL) || (image_cube2->volume == NULL))
    return(0.0);
  
  for (frame = 0; frame < image_cube1->num_frames; frame++)
    for (row = 0; row < image_cube1->num_rows; row++)
      for (col = 0; col < image_cube1->num_cols; col++)
        sum +=
          (image_cube1->volume[frame][row][col] -
           image_cube2->volume[frame][row][col]) *
          (image_cube1->volume[frame][row][col] -
           image_cube2->volume[frame][row][col]) /
          image_cube1->num_rows /
          image_cube1->num_cols /
          image_cube1->num_frames;

  return(sum);
}


double QccIMGImageCubeShapeAdaptiveMSE(const QccIMGImageCube
                                       *image_cube1,
                                       const QccIMGImageCube
                                       *image_cube2,
                                       const QccIMGImageCube
                                       *alpha_mask)
{
  double sum = 0.0;
  int frame, row, col;
  int cnt = 0;
  
  if (image_cube1 == NULL)
    return(0.0);
  if (image_cube2 == NULL)
    return(0.0);
  if ((image_cube1->volume == NULL) || (image_cube2->volume == NULL))
    return(0.0);
  
  if (alpha_mask == NULL)
    return(QccIMGImageCubeMSE(image_cube1,
                              image_cube2));
  if (alpha_mask->volume == NULL)
    return(QccIMGImageCubeMSE(image_cube1,
                              image_cube2));
  
  for (frame = 0; frame < image_cube1->num_frames; frame++)
    for (row = 0; row < image_cube1->num_rows; row++)
      for (col = 0; col < image_cube1->num_cols; col++)
        if (!QccAlphaTransparent(alpha_mask->volume[frame][row][col]))
          cnt++;
  
  for (frame = 0; frame < image_cube1->num_frames; frame++)
    for (row = 0; row < image_cube1->num_rows; row++)
      for (col = 0; col < image_cube1->num_cols; col++)
        if (!QccAlphaTransparent(alpha_mask->volume[frame][row][col]))
          sum +=
            (image_cube1->volume[frame][row][col] -
             image_cube2->volume[frame][row][col]) *
            (image_cube1->volume[frame][row][col] -
             image_cube2->volume[frame][row][col]) / cnt;

  return(sum);
}


double QccIMGImageCubeMAE(const QccIMGImageCube *image_cube1,
                          const QccIMGImageCube *image_cube2)
{
  int frame, row, col;
  double mae = 0;
  
  if (image_cube1 == NULL)
    return(0.0);
  if (image_cube2 == NULL)
    return(0.0);
  if ((image_cube1->volume == NULL) || (image_cube2->volume == NULL))
    return(0.0);
  
  for (frame = 0; frame < image_cube1->num_frames; frame++)
    for (row = 0; row < image_cube1->num_rows; row++)
      for (col = 0; col < image_cube1->num_cols; col++)
        mae = QccMathMax(mae,
                         fabs(image_cube1->volume[frame][row][col] -
                              image_cube2->volume[frame][row][col]));

  return(mae);
}


double QccIMGImageCubeShapeAdaptiveMAE(const QccIMGImageCube
                                       *image_cube1,
                                       const QccIMGImageCube
                                       *image_cube2,
                                       const QccIMGImageCube
                                       *alpha_mask)
{
  int frame, row, col;
  double mae = 0;
  
  if (image_cube1 == NULL)
    return(0.0);
  if (image_cube2 == NULL)
    return(0.0);
  if ((image_cube1->volume == NULL) || (image_cube2->volume == NULL))
    return(0.0);
  
  if (alpha_mask == NULL)
    return(QccIMGImageCubeMAE(image_cube1,
                              image_cube2));
  if (alpha_mask->volume == NULL)
    return(QccIMGImageCubeMAE(image_cube1,
                              image_cube2));
  
  for (frame = 0; frame < image_cube1->num_frames; frame++)
    for (row = 0; row < image_cube1->num_rows; row++)
      for (col = 0; col < image_cube1->num_cols; col++)
        if (!QccAlphaTransparent(alpha_mask->volume[frame][row][col]))
          mae = QccMathMax(mae,
                           fabs(image_cube1->volume[frame][row][col] -
                                image_cube2->volume[frame][row][col]));
  
  return(mae);
}


int QccIMGImageCubeExtractBlock(const QccIMGImageCube *image_cube,
                                int image_frame,
                                int image_row,
                                int image_col,
                                QccVolume block,
                                int block_num_frames,
                                int block_num_rows,
                                int block_num_cols)
{
  int block_frame, block_row, block_col;
  int current_image_frame, current_image_row, current_image_col;
  
  for (block_frame = 0; block_frame < block_num_frames; block_frame++)
    for (block_row = 0; block_row < block_num_rows; block_row++)
      for (block_col = 0; block_col < block_num_cols; block_col++)
        {
          current_image_frame = image_frame + block_frame;
          current_image_row = image_row + block_row;
          current_image_col = image_col + block_col;
          block[block_frame][block_row][block_col] =
            ((current_image_frame >= 0) &&
             (current_image_frame < image_cube->num_frames) &&
             (current_image_row >= 0) &&
             (current_image_row < image_cube->num_rows) &&
             (current_image_col >= 0) &&
             (current_image_col < image_cube->num_cols)) ?
            image_cube->volume
            [current_image_frame][current_image_row][current_image_col] : 0;
        }
  
  return(0);
}


int QccIMGImageCubeInsertBlock(QccIMGImageCube *image_cube,
                               int image_frame,
                               int image_row,
                               int image_col,
                               const QccVolume block,
                               int block_num_frames,
                               int block_num_rows,
                               int block_num_cols)
{
  int block_frame, block_row, block_col;
  int current_image_frame, current_image_row, current_image_col;
  
  for (block_frame = 0; block_frame < block_num_frames; block_frame++)
    for (block_row = 0; block_row < block_num_rows; block_row++)
      for (block_col = 0; block_col < block_num_cols; block_col++)
        {
          current_image_frame = image_frame + block_frame;
          current_image_row = image_row + block_row;
          current_image_col = image_col + block_col;
          if ((current_image_frame >= 0) &&
              (current_image_frame < image_cube->num_frames) &&
              (current_image_row >= 0) &&
              (current_image_row < image_cube->num_rows) &&
              (current_image_col >= 0) &&
              (current_image_col < image_cube->num_cols))
            image_cube->volume
              [current_image_frame][current_image_row][current_image_col] =
              block[block_frame][block_row][block_col];
        }
  
  return(0);
}


int QccIMGImageCubeCopy(QccIMGImageCube *image_cube1,
                        const QccIMGImageCube *image_cube2)
{
  int frame, row, col;
  
  if ((image_cube1 == NULL) ||
      (image_cube2 == NULL))
    return(0);
  
  if ((image_cube2->volume == NULL) ||
      (image_cube2->num_frames <= 0) ||
      (image_cube2->num_rows <= 0) ||
      (image_cube2->num_cols <= 0))
    return(0);
  
  if (image_cube1->volume == NULL)
    {
      image_cube1->num_frames = image_cube2->num_frames;
      image_cube1->num_rows = image_cube2->num_rows;
      image_cube1->num_cols = image_cube2->num_cols;
      if (QccIMGImageCubeAlloc(image_cube1))
        {
          QccErrorAddMessage("(QccIMGImageCubeCopy): Error calling QccIMGImageCubeAlloc()");
          return(1);
        }
    }
  else
    {
      if ((image_cube1->num_frames != image_cube2->num_frames) ||
          (image_cube1->num_rows != image_cube2->num_rows) ||
          (image_cube1->num_cols != image_cube2->num_cols))
        {
          QccErrorAddMessage("(QccIMGImageCubeCopy): Image-cube arrays have different sizes");
          return(1);
        }
    }

  for (frame = 0; frame < image_cube1->num_frames; frame++)
    for (row = 0; row < image_cube1->num_rows; row++)
      for (col = 0; col < image_cube1->num_cols; col++)
        image_cube1->volume[frame][row][col] =
          image_cube2->volume[frame][row][col];
  
  QccIMGImageCubeSetMin(image_cube1);
  QccIMGImageCubeSetMax(image_cube1);
  
  return(0);
  
}


int QccIMGImageCubeAdd(const QccIMGImageCube *image_cube1,
                       const QccIMGImageCube *image_cube2,
                       QccIMGImageCube *image_cube3)
{
  int frame, row, col;
  
  if (image_cube1 == NULL)
    return(0);
  if (image_cube2 == NULL)
    return(0);
  if (image_cube3 == NULL)
    return(0);
  
  if (image_cube1->volume == NULL)
    return(0);
  if (image_cube2->volume == NULL)
    return(0);
  if (image_cube3->volume == NULL)
    return(0);
  
  if ((image_cube1->num_frames != image_cube2->num_frames) ||
      (image_cube1->num_rows != image_cube2->num_rows) ||
      (image_cube1->num_cols != image_cube2->num_cols) ||
      (image_cube1->num_frames != image_cube3->num_frames) ||
      (image_cube1->num_rows != image_cube3->num_rows) ||
      (image_cube1->num_cols != image_cube3->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageCubeSubtract): Image cubes must have same number of rows, columns, and frames");
      return(1);
    }
  
  for (frame = 0; frame < image_cube1->num_frames; frame++)
    for (row = 0; row < image_cube1->num_rows; row++)
      for (col = 0; col < image_cube1->num_cols; col++)
        image_cube3->volume[frame][row][col] =
          image_cube1->volume[frame][row][col] +
          image_cube2->volume[frame][row][col];
  
  QccIMGImageCubeSetMin(image_cube3);
  QccIMGImageCubeSetMax(image_cube3);
  
  return(0);
}


int QccIMGImageCubeSubtract(const QccIMGImageCube *image_cube1,
                            const QccIMGImageCube *image_cube2,
                            QccIMGImageCube *image_cube3)
{
  int frame, row, col;
  
  if (image_cube1 == NULL)
    return(0);
  if (image_cube2 == NULL)
    return(0);
  if (image_cube3 == NULL)
    return(0);
  
  if (image_cube1->volume == NULL)
    return(0);
  if (image_cube2->volume == NULL)
    return(0);
  if (image_cube3->volume == NULL)
    return(0);
  
  if ((image_cube1->num_frames != image_cube2->num_frames) ||
      (image_cube1->num_rows != image_cube2->num_rows) ||
      (image_cube1->num_cols != image_cube2->num_cols) ||
      (image_cube1->num_frames != image_cube3->num_frames) ||
      (image_cube1->num_rows != image_cube3->num_rows) ||
      (image_cube1->num_cols != image_cube3->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageCubeSubtract): Image cubes must have same number of rows, columns, and frames");
      return(1);
    }
  
  for (frame = 0; frame < image_cube1->num_frames; frame++)
    for (row = 0; row < image_cube1->num_rows; row++)
      for (col = 0; col < image_cube1->num_cols; col++)
        image_cube3->volume[frame][row][col] =
          image_cube1->volume[frame][row][col] -
          image_cube2->volume[frame][row][col];
  
  QccIMGImageCubeSetMin(image_cube3);
  QccIMGImageCubeSetMax(image_cube3);
  
  return(0);
}


int QccIMGImageCubeExtractFrame(const QccIMGImageCube *image_cube,
                                int frame,
                                QccIMGImageComponent *image_component)
{
  int row, col;

  if (image_cube == NULL)
    return(0);
  if (image_component == NULL)
    return(0);

  if ((image_cube->num_rows != image_component->num_rows) ||
      (image_cube->num_cols != image_component->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageCubeExtractFrame): Image-component size not the same as image-cube size");
      return(1);
    }

  if ((frame < 0) || (frame >= image_cube->num_frames))
    {
      QccErrorAddMessage("(QccIMGImageCubeExtractFrame): Invalid frame number");
      return(1);
    }

  for (row = 0; row < image_cube->num_rows; row++)
    for (col = 0; col < image_cube->num_cols; col++)
      image_component->image[row][col] =
        image_cube->volume[frame][row][col];

  if (QccIMGImageComponentSetMaxMin(image_component))
    {
      QccErrorAddMessage("(QccIMGImageCubeExtractFrame): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}
