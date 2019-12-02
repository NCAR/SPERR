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


QccVolume QccVolumeAlloc(int num_frames, int num_rows, int num_cols)
{
  QccVolume volume;
  int frame;
  
  if ((num_frames <=0) || (num_rows <= 0) || (num_cols <= 0))
    return(NULL);
  
  if ((volume =
       (QccVolume)malloc(sizeof(QccMatrix) * num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccVolumeAlloc): Error allocating memory");
      return(NULL);
    }
  for (frame = 0; frame < num_frames; frame++)
    if ((volume[frame] = QccMatrixAlloc(num_rows, num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccVolumeAlloc): Error calling QccMatrixAlloc()");
        QccFree(volume);
        return(NULL);
      }
  
  return(volume);
}


void QccVolumeFree(QccVolume volume, int num_frames, int num_rows)
{
  int frame;
  
  if (volume != NULL)
    {
      for (frame = 0; frame < num_frames; frame++)
        QccMatrixFree(volume[frame], num_rows);
      
      QccFree(volume);
    }
}


int QccVolumeZero(QccVolume volume,int num_frames, int num_rows, int num_cols)
{
  int frame, row, col;
  
  if (volume == NULL)
    return(0);

  for (frame = 0; frame < num_frames; frame++)
    for (row = 0; row < num_rows; row++)
      for (col = 0; col < num_cols; col++)
        volume[frame][row][col] = 0;
  
  return(0);
}


QccVolume QccVolumeResize(QccVolume volume,
                          int num_frames,
                          int num_rows,
                          int num_cols,
                          int new_num_frames,
                          int new_num_rows,
                          int new_num_cols)
{
  int frame;
  QccVolume new_volume;

  if (volume == NULL)
    {
      if ((new_volume = QccVolumeAlloc(new_num_frames,
                                       new_num_rows,
                                       new_num_cols)) == NULL)
        {
          QccErrorAddMessage("(QccVolumeResize): Error calling QccVolumeAlloc()");
          return(NULL);
        }
      QccVolumeZero(volume, new_num_frames, new_num_rows, new_num_cols);

      return(new_volume);
    }

  if ((num_frames < 0) || (new_num_frames < 0) ||
      (num_rows < 0) || (new_num_rows < 0) ||
      (num_cols < 0) || (new_num_cols < 0))
    return(volume);

  if ((new_num_frames == num_frames) &&
      (new_num_rows == num_rows) &&
      (new_num_cols == num_cols))
    return(volume);

  if (new_num_frames != num_frames)
    {
      if ((new_volume =
           (QccVolume)realloc(volume, sizeof(QccMatrix) * new_num_frames)) ==
          NULL)
        {
          QccErrorAddMessage("(QccVolumeResize): Error reallocating memory");
          return(NULL);
        }
      for (frame = num_frames; frame < new_num_frames; frame++)
        {
          if ((new_volume[frame] = QccMatrixAlloc(new_num_rows,
                                                  new_num_cols)) == NULL)
            {
              QccErrorAddMessage("(QccVolumeAlloc): Error QccMatrixAlloc()");
              QccFree(new_volume);
              return(NULL);
            }
          QccMatrixZero(new_volume[frame], new_num_rows, new_num_cols);
        }
    }
  else
    new_volume = volume;
  
  for (frame = 0; frame < num_frames; frame++)
    if ((new_volume[frame] = QccMatrixResize(new_volume[frame],
                                             num_rows,
                                             num_cols,
                                             new_num_rows,
                                             new_num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccVolumeAlloc): Error calling QccMatrixResize()");
        QccFree(new_volume);
        return(NULL);
      }

  return(new_volume);
}


int QccVolumeCopy(QccVolume volume1, const QccVolume volume2,
                  int num_frames, int num_rows, int num_cols)
{
  int frame, row, col;
  
  if ((volume1 == NULL) ||
      (volume2 == NULL) ||
      (num_frames <= 0) ||
      (num_rows <= 0) ||
      (num_cols <= 0))
    return(0);

  for (frame = 0; frame < num_frames; frame++)
    for (row = 0; row < num_rows; row++)
      for (col = 0; col < num_cols; col++)
        volume1[frame][row][col] = volume2[frame][row][col];
  
  return(0);
}


double QccVolumeMaxValue(const QccVolume volume,
                         int num_frames, int num_rows, int num_cols)
{
  int frame;
  double max_value = -MAXDOUBLE;
  double tmp;
  
  if (volume != NULL)
    for (frame = 0; frame < num_frames; frame++)
      {
        tmp = QccMatrixMaxValue(volume[frame], num_rows, num_cols);
        if (tmp > max_value)
          max_value = tmp;
      }

  return(max_value);
}


double QccVolumeMinValue(const QccVolume volume,
                         int num_frames, int num_rows, int num_cols)
{
  int frame;
  double min_value = MAXDOUBLE;
  double tmp;
  
  if (volume != NULL)
    for (frame = 0; frame < num_frames; frame++)
      {
        tmp = QccMatrixMinValue(volume[frame], num_rows, num_cols);
        if (tmp < min_value)
          min_value = tmp;
      }

  return(min_value);
}


int QccVolumePrint(const QccVolume volume, int num_frames, int num_rows,
                   int num_cols)
{
  int frame, row, col;
  
  if (volume == NULL)
    return(0);

  for (frame = 0; frame < num_frames; frame++)
    {
      for (row = 0; row < num_rows; row++)
        {
          for (col = 0; col < num_cols; col++)
            printf("%g ", volume[frame][row][col]);
          printf("\n");
        }

      printf("============================================================\n");
    }

  return(0);
}


int QccVolumeAdd(QccVolume volume1, const QccVolume volume2,
                 int num_frames, int num_rows, int num_cols)
{
  int frame;
  
  if (volume1 == NULL)
    return(0);
  if (volume2 == NULL)
    return(0);
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccMatrixAdd(volume1[frame], volume2[frame], num_rows, num_cols))
      {
        QccErrorAddMessage("(QccVolumeAdd): Error calling QccMatrixAdd()");
        return(1);
      }
  
  return(0);
}


int QccVolumeSubtract(QccVolume volume1, const QccVolume volume2,
                      int num_frames, int num_rows, int num_cols)
{
  int frame;
  
  if (volume1 == NULL)
    return(0);
  if (volume2 == NULL)
    return(0);
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccMatrixSubtract(volume1[frame], volume2[frame], num_rows, num_cols))
      {
        QccErrorAddMessage("(QccVolumeSubtract): Error calling QccMatrixSubtract()");
        return(1);
      }
  
  return(0);
}


double QccVolumeMean(const QccVolume volume,
                     int num_frames, int num_rows, int num_cols)
{
  double sum = 0.0;
  int frame, row, col;

  for (frame =0; frame < num_frames; frame++)
    for (row = 0; row < num_rows; row++)
      for (col = 0; col < num_cols; col++)
        sum += volume[frame][row][col];
  
  sum /= num_frames * num_rows * num_cols;
  
  return(sum);
}


double QccVolumeVariance(const QccVolume volume,
                         int num_frames,
                         int num_rows,
                         int num_cols)
{
  double variance = 0.0;
  double mean;
  int frame, row, col;

  mean = QccVolumeMean(volume, num_frames, num_rows, num_cols);

  for (frame = 0; frame < num_frames; frame++)
    for (row = 0; row < num_rows; row++)
      for (col = 0; col < num_cols; col++)
        variance +=
          (volume[frame][row][col] - mean) *
          (volume[frame][row][col] - mean);
  
  variance /= (num_frames * num_rows * num_cols);

  return(variance);
}
