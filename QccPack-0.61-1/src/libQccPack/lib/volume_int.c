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


QccVolumeInt QccVolumeIntAlloc(int num_frames,
                               int num_rows,
                               int num_cols)
{
  QccVolumeInt volume;
  int frame;
  
  if ((num_frames <=0) || (num_rows <= 0) || (num_cols <= 0))
    return(NULL);
  
  if ((volume =
       (QccVolumeInt)malloc(sizeof(QccMatrixInt) * num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccVolumeIntAlloc): Error allocating memory");
      return(NULL);
    }
  for (frame = 0; frame < num_frames; frame++)
    if ((volume[frame] = QccMatrixIntAlloc(num_rows, num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccVolumeIntAlloc): Error calling QccMatrixIntAlloc()");
        QccFree(volume);
        return(NULL);
      }
  
  return(volume);
}


void QccVolumeIntFree(QccVolumeInt volume,
                      int num_frames,
                      int num_rows)
{
  int frame;
  
  if (volume != NULL)
    {
      for (frame = 0; frame < num_frames; frame++)
        QccMatrixIntFree(volume[frame], num_rows);
      
      QccFree(volume);
    }
}


int QccVolumeIntZero(QccVolumeInt volume,
                     int num_frames,
                     int num_rows,
                     int num_cols)
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


QccVolumeInt QccVolumeIntResize(QccVolumeInt volume,
                                int num_frames,
                                int num_rows,
                                int num_cols,
                                int new_num_frames,
                                int new_num_rows,
                                int new_num_cols)
{
  int frame;
  QccVolumeInt new_volume;
  
  if (volume == NULL)
    {
      if ((new_volume = QccVolumeIntAlloc(new_num_frames,
                                          new_num_rows,
                                          new_num_cols)) == NULL)
        {
          QccErrorAddMessage("(QccVolumeIntResize): Error calling QccVolumeIntAlloc()");
          return(NULL);
        }
      QccVolumeIntZero(volume, new_num_frames, new_num_rows, new_num_cols);
      
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
           (QccVolumeInt)realloc(volume,
                                 sizeof(QccMatrixInt) * new_num_frames)) ==
          NULL)
        {
          QccErrorAddMessage("(QccVolumeIntResize): Error reallocating memory");
          return(NULL);
        }
      for (frame = num_frames; frame < new_num_frames; frame++)
        {
          if ((new_volume[frame] = QccMatrixIntAlloc(new_num_rows,
                                                     new_num_cols)) == NULL)
            {
              QccErrorAddMessage("(QccVolumeIntAlloc): Error QccMatrixIntAlloc()");
              QccFree(new_volume);
              return(NULL);
            }
          QccMatrixIntZero(new_volume[frame], new_num_rows, new_num_cols);
        }
    }
  else
    new_volume = volume;
  
  for (frame = 0; frame < num_frames; frame++)
    if ((new_volume[frame] = QccMatrixIntResize(new_volume[frame],
                                                num_rows,
                                                num_cols,
                                                new_num_rows,
                                                new_num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccVolumeIntAlloc): Error calling QccMatrixIntResize()");
        QccFree(new_volume);
        return(NULL);
      }

  return(new_volume);
}


int QccVolumeIntCopy(QccVolumeInt volume1,
                     const QccVolumeInt volume2,
                     int num_frames,
                     int num_rows,
                     int num_cols)
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


int QccVolumeIntMaxValue(const QccVolumeInt volume,
                         int num_frames,
                         int num_rows,
                         int num_cols)
{
  int frame;
  int max_value = -MAXINT;
  int tmp;
  
  if (volume != NULL)
    for (frame = 0; frame < num_frames; frame++)
      {
        tmp = QccMatrixIntMaxValue(volume[frame], num_rows, num_cols);
        if (tmp > max_value)
          max_value = tmp;
      }

  return(max_value);
}


int QccVolumeIntMinValue(const QccVolumeInt volume,
                         int num_frames,
                         int num_rows,
                         int num_cols)
{
  int frame;
  int min_value = MAXINT;
  int tmp;
  
  if (volume != NULL)
    for (frame = 0; frame < num_frames; frame++)
      {
        tmp = QccMatrixIntMinValue(volume[frame], num_rows, num_cols);
        if (tmp < min_value)
          min_value = tmp;
      }

  return(min_value);
}


int QccVolumeIntPrint(const QccVolumeInt volume,
                      int num_frames,
                      int num_rows,
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
            printf("%d ", volume[frame][row][col]);
          printf("\n");
        }

      printf("============================================================\n");
    }

  return(0);
}


int QccVolumeIntAdd(QccVolumeInt volume1,
                    const QccVolumeInt volume2,
                    int num_frames,
                    int num_rows,
                    int num_cols)
{
  int frame;
  
  if (volume1 == NULL)
    return(0);
  if (volume2 == NULL)
    return(0);
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccMatrixIntAdd(volume1[frame], volume2[frame], num_rows, num_cols))
      {
        QccErrorAddMessage("(QccVolumeIntAdd): Error calling QccVectorAdd()");
        return(1);
      }
  
  return(0);
}


int QccVolumeIntSubtract(QccVolumeInt volume1,
                         const QccVolumeInt volume2,
                         int num_frames,
                         int num_rows,
                         int num_cols)
{
  int frame;
  
  if (volume1 == NULL)
    return(0);
  if (volume2 == NULL)
    return(0);
  
  for (frame = 0; frame < num_frames; frame++)
    if (QccMatrixIntSubtract(volume1[frame],
                             volume2[frame],
                             num_rows,
                             num_cols))
      {
        QccErrorAddMessage("(QccVolumeIntSubtract): Error calling QccMatrixIntSubtract()");
        return(1);
      }
  
  return(0);
}


double QccVolumeIntMean(const QccVolumeInt volume,
                        int num_frames,
                        int num_rows,
                        int num_cols)
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


double QccVolumeIntVariance(const QccVolumeInt volume,
                            int num_frames,
                            int num_rows,
                            int num_cols)
{
  double variance = 0.0;
  double mean;
  int frame, row, col;

  mean = QccVolumeIntMean(volume, num_frames, num_rows, num_cols);

  for (frame = 0; frame < num_frames; frame++)
    for (row = 0; row < num_rows; row++)
      for (col = 0; col < num_cols; col++)
        variance +=
          (volume[frame][row][col] - mean) *
          (volume[frame][row][col] - mean);
  
  variance /= (num_frames * num_rows * num_cols);

  return(variance);
}
