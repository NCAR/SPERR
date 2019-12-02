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


QccMatrixInt QccMatrixIntAlloc(int num_rows,
                               int num_cols)
{
  QccMatrixInt matrix;
  int row;
  
  if ((num_rows <= 0) || (num_cols <= 0))
    return(NULL);

  if ((matrix = 
       (QccMatrixInt)malloc(sizeof(QccVectorInt) * num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixIntAlloc): Error allocating memory");
      return(NULL);
    }
  
  for (row = 0; row < num_rows; row++)
    if ((matrix[row] = QccVectorIntAlloc(num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccMatrixIntAlloc): Error allocating memory");
        QccFree(matrix);
        return(NULL);
      }
  
  return(matrix);
}


void QccMatrixIntFree(QccMatrixInt matrix,
                      int num_rows)
{
  int row;
  
  if (matrix != NULL)
    {
      for (row = 0; row < num_rows; row++)
        QccVectorIntFree(matrix[row]);

      QccFree(matrix);
    }
}


int QccMatrixIntZero(QccMatrixInt matrix,
                     int num_rows,
                     int num_cols)
{
  int row, col;

  if (matrix == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      matrix[row][col] = 0;

  return(0);
}


QccMatrixInt QccMatrixIntResize(QccMatrixInt matrix,
                          int num_rows,
                          int num_cols,
                          int new_num_rows,
                          int new_num_cols)
{
  int row;
  QccMatrixInt new_matrix;

  if (matrix == NULL)
    {
      if ((new_matrix = QccMatrixIntAlloc(new_num_rows,
                                          new_num_cols)) == NULL)
        {
          QccErrorAddMessage("(QccMatrixIntResize): Error calling QccMatrixIntAlloc()");
          return(NULL);
        }
      QccMatrixIntZero(matrix, new_num_rows, new_num_cols);

      return(new_matrix);
    }

  if ((num_rows < 0) || (new_num_rows < 0) ||
      (num_cols < 0) || (new_num_cols < 0))
    return(matrix);

  if ((new_num_rows == num_rows) && (new_num_cols == num_cols))
    return(matrix);

  if (new_num_rows != num_rows)
    {
      if ((new_matrix =
           (QccMatrixInt)realloc(matrix,
                                 sizeof(QccVectorInt) * new_num_rows)) ==
          NULL)
        {
          QccErrorAddMessage("(QccMatrixIntResize): Error reallocating memory");
          return(NULL);
        }
      for (row = num_rows; row < new_num_rows; row++)
        {
          if ((new_matrix[row] = QccVectorIntAlloc(new_num_cols)) == NULL)
            {
              QccErrorAddMessage("(QccMatrixIntAlloc): Error calling QccVectorIntAlloc()");
              QccFree(new_matrix);
              return(NULL);
            }
          QccVectorIntZero(new_matrix[row], new_num_cols);
        }
    }
  else
    new_matrix = matrix;

  for (row = 0; row < num_rows; row++)
    if ((new_matrix[row] = QccVectorIntResize(new_matrix[row],
                                              num_cols,
                                              new_num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccMatrixIntAlloc): Error calling QccVectorIntResize()");
        QccFree(new_matrix);
        return(NULL);
      }

  return(new_matrix);
}


int QccMatrixIntCopy(QccMatrixInt matrix1,
                     const QccMatrixInt matrix2,
                     int num_rows,
                     int num_cols)
{
  int row, col;

  if ((matrix1 == NULL) || 
      (matrix2 == NULL) ||
      (num_rows <= 0) ||
      (num_cols <= 0))
    return(0);

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      matrix1[row][col] = matrix2[row][col];

  return(0);
}


int QccMatrixIntMaxValue(const QccMatrixInt matrix,
                         int num_rows,
                         int num_cols)
{
  int row;
  int max_value = -MAXINT;
  int tmp;
  
  if (matrix != NULL)
    for (row = 0; row < num_rows; row++)
      {
        tmp = QccVectorIntMaxValue(matrix[row], num_cols, NULL);
        if (tmp > max_value)
          max_value = tmp;
      }

  return(max_value);
}


int QccMatrixIntMinValue(const QccMatrixInt matrix,
                         int num_rows,
                         int num_cols)
{
  int row;
  int min_value = MAXINT;
  int tmp;
  
  if (matrix != NULL)
    for (row = 0; row < num_rows; row++)
      {
        tmp = QccVectorIntMinValue(matrix[row], num_cols, NULL);
        if (tmp < min_value)
          min_value = tmp;
      }

  return(min_value);
}


int QccMatrixIntPrint(const QccMatrixInt matrix,
                      int num_rows,
                      int num_cols)
{
  int row, col;

  if (matrix == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    {
      for (col = 0; col < num_cols; col++)
        printf("%d ", matrix[row][col]);
      printf("\n");
    }

  return(0);
}


int QccMatrixIntTranspose(const QccMatrixInt matrix1,
                          QccMatrixInt matrix2,
                          int num_rows,
                          int num_cols)
{
  int row, col;

  if (matrix1 == NULL)
    return(0);
  if (matrix2 == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      matrix2[col][row] = matrix1[row][col];

  return(0);
}


int QccMatrixIntAdd(QccMatrixInt matrix1,
                    const QccMatrixInt matrix2,
                    int num_rows,
                    int num_cols)
{
  int row;

  if (matrix1 == NULL)
    return(0);
  if (matrix2 == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    if (QccVectorIntAdd(matrix1[row], matrix2[row], num_cols))
      {
        QccErrorAddMessage("(QccMatrixIntAdd): Error calling QccVectorIntAdd()");
        return(1);
      }

  return(0);
}


int QccMatrixIntSubtract(QccMatrixInt matrix1,
                         const QccMatrixInt matrix2,
                         int num_rows,
                         int num_cols)
{
  int row;

  if (matrix1 == NULL)
    return(0);
  if (matrix2 == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    if (QccVectorIntSubtract(matrix1[row], matrix2[row], num_cols))
      {
        QccErrorAddMessage("(QccMatrixIntSubtract): Error calling QccVectorIntSubtract()");
        return(1);
      }

  return(0);
}


double QccMatrixIntMean(const QccMatrixInt matrix,
                        int num_rows,
                        int num_cols)
{
  double sum = 0.0;
  int row, col;
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      sum += matrix[row][col];
  
  sum /= num_rows * num_cols;
  
  return(sum);
}


double QccMatrixIntVariance(const QccMatrixInt matrix,
                            int num_rows,
                            int num_cols)
{
  double variance = 0.0;
  double mean;
  int row, col;

  mean = QccMatrixIntMean(matrix, num_rows, num_cols);

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      variance +=
        (matrix[row][col] - mean) *
        (matrix[row][col] - mean);

  variance /= (num_rows * num_cols);

  return(variance);
}


int QccMatrixIntVectorMultiply(const QccMatrixInt matrix,
                               const QccVectorInt vector1,
                               QccVectorInt vector2,
                               int num_rows,
                               int num_cols)
{
  int row;

  if (matrix == NULL)
    return(0);
  if (vector1 == NULL)
    return(0);
  if (vector2 == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    vector2[row] =
      QccVectorIntDotProduct(matrix[row], vector1, num_cols);

  return(0);
}


int QccMatrixIntMultiply(const QccMatrixInt matrix1,
                         int num_rows1,
                         int num_cols1,
                         const QccMatrixInt matrix2,
                         int num_rows2,
                         int num_cols2,
                         QccMatrixInt matrix3)
{
  int row, col;
  int index;
  
  if (matrix1 == NULL)
    return(0);
  if (matrix2 == NULL)
    return(0);
  if (matrix3 == NULL)
    return(0);
  
  if (num_cols1 != num_rows2)
    {
      QccErrorAddMessage("(QccMatrixIntMultiply): Inner matrix dimensions do not agree");
      return(1);
    }
  
  for (row = 0; row < num_rows1; row++)
    for (col = 0; col < num_cols2; col++)
      {
        matrix3[row][col] = 0;
        for (index = 0; index < num_cols1; index++)
          matrix3[row][col] +=
            matrix1[row][index] * matrix2[index][col];
      }
  
  return(0);
}

