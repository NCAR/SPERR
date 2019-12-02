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


QccECCFieldElement QccECCFieldExponential(int exponent,
                                          const QccECCField *field)
{
  return(field->exponential_table
         [QccMathModulus(exponent,
                         (1 << field->size_exponent) - 1)]);
}


int QccECCFieldLogarithm(QccECCFieldElement element, const QccECCField *field)
{
  return(field->log_table[element]);
}


QccECCFieldElement QccECCFieldAdd(QccECCFieldElement element1,
                                  QccECCFieldElement element2,
                                  const QccECCField *field)
{
  return((element1 ^ element2) & field->mask);
}


QccECCFieldElement QccECCFieldMultiply(QccECCFieldElement element1,
                                       QccECCFieldElement element2,
                                       const QccECCField *field)
{
  if (!element1 || !element2)
    return((QccECCFieldElement)0);

  return(QccECCFieldExponential(QccECCFieldLogarithm(element1, field) +
                                QccECCFieldLogarithm(element2, field),
                                field));
}


QccECCFieldElement QccECCFieldDivide(QccECCFieldElement element1,
                                     QccECCFieldElement element2,
                                     const QccECCField *field)
{
  QccECCFieldElement reciprocal;
  
  if (!element2)
    return(0);
  
  reciprocal = QccECCFieldExponential(-QccECCFieldLogarithm(element2,
                                                            field),
                                      field);
  return(QccECCFieldMultiply(element1, reciprocal, field));
}


static int QccECCFieldMatrixGaussianForwardElimination(QccECCFieldMatrix
                                                       matrix,
                                                       int num_rows, 
                                                       int num_cols,
                                                       const
                                                       QccECCField *field)
{
  QccECCFieldElement temp_element;
  QccECCFieldElement multiplier;
  int row, col;
  int index;

  for (index = 0; index < num_rows - 1; index++)
    {
      for (row = index; row < num_rows; row++)
        if (matrix[row][index])
          break;
      
      if (row >= num_rows)
        {
          QccErrorAddMessage("(QccECCFieldMatrixGaussianForwardElimination): Singular matrix detected");
          return(1);
        }
      
      if (row != index)
        for (col = 0; col < num_cols; col++)
          {
            temp_element = matrix[row][col];
            matrix[row][col] = matrix[index][col];
            matrix[index][col] = temp_element;
          }
      
      for (row = index + 1; row < num_rows; row++)
        if (matrix[row][index])
          {
            multiplier = QccECCFieldDivide(matrix[row][index],
                                           matrix[index][index],
                                           field);
            for (col = 0; col < num_cols; col++)
              matrix[row][col] =
                QccECCFieldAdd(matrix[row][col],
                               QccECCFieldMultiply(matrix[index][col],
                                                   multiplier,
                                                   field),
                               field);
          }
    }

  return(0);
}


static int QccECCFieldMatrixGaussianBackwardElimination(QccECCFieldMatrix
                                                        matrix,
                                                        int num_rows, 
                                                        int num_cols,
                                                        const 
                                                        QccECCField *field)
{
  QccECCFieldElement multiplier;
  int row, col;
  int index;

  for (index = num_rows - 1; index; index--)
    for (row = index - 1; row >= 0; row--)
      if (matrix[row][index])
        {
          multiplier = QccECCFieldDivide(matrix[row][index],
                                         matrix[index][index],
                                         field);
          for (col = 0; col < num_cols; col++)
            matrix[row][col] =
              QccECCFieldAdd(matrix[row][col],
                             QccECCFieldMultiply(matrix[index][col],
                                                 multiplier,
                                                 field),
                             field);
        }

  return(0);
}


int QccECCFieldMatrixGaussianElimination(QccECCFieldMatrix matrix,
                                         int num_rows, int num_cols,
                                         const QccECCField *field)
{
  int index;
  QccECCFieldElement reciprocal;
  int col;

  if (matrix == NULL)
    return(0);
  if (field == NULL)
    return(0);

  if (num_cols < num_rows)
    {
      QccErrorAddMessage("(QccECCFieldMatrixGaussianElimination): Matrix is too narrow");
      return(1);
    }

  if (QccECCFieldMatrixGaussianForwardElimination(matrix,
                                                  num_rows, num_cols,
                                                  field))
    {
      QccErrorAddMessage("(QccECCFieldMatrixGaussianElimination): Error calling QccECCFieldMatrixGaussianForwardElimination()");
      return(1);
    }

  if (QccECCFieldMatrixGaussianBackwardElimination(matrix,
                                                   num_rows, num_cols,
                                                   field))
    {
      QccErrorAddMessage("(QccECCFieldMatrixGaussianElimination): Error calling QccECCFieldMatrixGaussianBackwardElimination()");
      return(1);
    }

  for (index = 0; index < num_rows; index++)
    if (matrix[index][index] != 1)
      {      
        reciprocal = 
          QccECCFieldExponential(-QccECCFieldLogarithm(matrix[index][index],
                                                       field),
                                 field);
        for (col = 0; col < num_cols; col++)
          matrix[index][col] = 
            QccECCFieldMultiply(matrix[index][col],
                                reciprocal,
                                field);
      }

  return(0);
}
