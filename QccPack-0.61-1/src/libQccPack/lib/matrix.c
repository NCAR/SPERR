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


QccMatrix QccMatrixAlloc(int num_rows, int num_cols)
{
  QccMatrix matrix;
  int row;
  
  if ((num_rows <= 0) || (num_cols <= 0))
    return(NULL);

  if ((matrix = 
       (QccMatrix)malloc(sizeof(QccVector)*num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixAlloc): Error allocating memory");
      return(NULL);
    }
  
  for (row = 0; row < num_rows; row++)
    if ((matrix[row] = QccVectorAlloc(num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccMatrixAlloc): Error allocating memory");
        QccFree(matrix);
        return(NULL);
      }
  
  return(matrix);
}


void QccMatrixFree(QccMatrix matrix, int num_rows)
{
  int row;
  
  if (matrix != NULL)
    {
      for (row = 0; row < num_rows; row++)
        QccVectorFree(matrix[row]);

      QccFree(matrix);
    }
}


int QccMatrixZero(QccMatrix matrix, int num_rows, int num_cols)
{
  int row, col;

  if (matrix == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      matrix[row][col] = 0;

  return(0);
}


QccMatrix QccMatrixResize(QccMatrix matrix,
                          int num_rows,
                          int num_cols,
                          int new_num_rows,
                          int new_num_cols)
{
  int row;
  QccMatrix new_matrix;

  if (matrix == NULL)
    {
      if ((new_matrix = QccMatrixAlloc(new_num_rows,
                                       new_num_cols)) == NULL)
        {
          QccErrorAddMessage("(QccMatrixResize): Error calling QccMatrixAlloc()");
          return(NULL);
        }
      QccMatrixZero(matrix, new_num_rows, new_num_cols);

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
           (QccMatrix)realloc(matrix, sizeof(QccVector) * new_num_rows)) ==
          NULL)
        {
          QccErrorAddMessage("(QccMatrixResize): Error reallocating memory");
          return(NULL);
        }
      for (row = num_rows; row < new_num_rows; row++)
        {
          if ((new_matrix[row] = QccVectorAlloc(new_num_cols)) == NULL)
            {
              QccErrorAddMessage("(QccMatrixAlloc): Error QccVectorAlloc()");
              QccFree(new_matrix);
              return(NULL);
            }
          QccVectorZero(new_matrix[row], new_num_cols);
        }
    }
  else
    new_matrix = matrix;

  for (row = 0; row < num_rows; row++)
    if ((new_matrix[row] = QccVectorResize(new_matrix[row],
                                           num_cols,
                                           new_num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccMatrixAlloc): Error calling QccVectorResize()");
        QccFree(new_matrix);
        return(NULL);
      }

  return(new_matrix);
}


int QccMatrixCopy(QccMatrix matrix1, const QccMatrix matrix2,
                  int num_rows, int num_cols)
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


double QccMatrixMaxValue(const QccMatrix matrix,
                         int num_rows, int num_cols)
{
  int row;
  double max_value = -MAXDOUBLE;
  double tmp;
  
  if (matrix != NULL)
    for (row = 0; row < num_rows; row++)
      {
        tmp = QccVectorMaxValue(matrix[row], num_cols, NULL);
        if (tmp > max_value)
          max_value = tmp;
      }

  return(max_value);
}


double QccMatrixMinValue(const QccMatrix matrix,
                         int num_rows, int num_cols)
{
  int row;
  double min_value = MAXDOUBLE;
  double tmp;
  
  if (matrix != NULL)
    for (row = 0; row < num_rows; row++)
      {
        tmp = QccVectorMinValue(matrix[row], num_cols, NULL);
        if (tmp < min_value)
          min_value = tmp;
      }

  return(min_value);
}


int QccMatrixPrint(const QccMatrix matrix, int num_rows, int num_cols)
{
  int row, col;

  if (matrix == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    {
      for (col = 0; col < num_cols; col++)
        printf("%g ", matrix[row][col]);
      printf("\n");
    }

  return(0);
}


int QccMatrixRowExchange(QccMatrix matrix,
                         int num_cols,
                         int row1,
                         int row2)
{
  int col;
  double temp;
  
  if (matrix == NULL)
    return(0);
  
  for (col = 0; col < num_cols; col++)
    {
      temp = matrix[row1][col];	 
      matrix[row1][col] = matrix[row2][col]; 
      matrix[row2][col] = temp;
    }	
  
  return(0);
}


int QccMatrixColExchange(QccMatrix matrix,   
                         int num_rows,
                         int col1,
                         int col2)
{
  int row;
  double temp;
  
  if (matrix == NULL)
    return(0);
  
  for (row = 0; row < num_rows; row++)
    {
      temp = matrix[row][col1];	 
      matrix[row][col1] = matrix[row][col2]; 
      matrix[row][col2] = temp;
    }	

  return(0);
}


int QccMatrixIdentity(QccMatrix matrix, int num_rows, int num_cols)
{
  int row;
  
  if (matrix == NULL)
    return(0);
  
  QccMatrixZero(matrix, num_rows, num_cols);
  
  for (row = 0; row < num_rows; row++)		
    matrix[row][row] = 1;
  
  return(0);
}


int QccMatrixTranspose(const QccMatrix matrix1, QccMatrix matrix2,
                       int num_rows, int num_cols)
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


int QccMatrixAdd(QccMatrix matrix1, const QccMatrix matrix2,
                 int num_rows, int num_cols)
{
  int row;

  if (matrix1 == NULL)
    return(0);
  if (matrix2 == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    if (QccVectorAdd(matrix1[row], matrix2[row], num_cols))
      {
        QccErrorAddMessage("(QccMatrixAdd): Error calling QccVectorAdd()");
        return(1);
      }

  return(0);
}


int QccMatrixSubtract(QccMatrix matrix1, const QccMatrix matrix2,
                      int num_rows, int num_cols)
{
  int row;

  if (matrix1 == NULL)
    return(0);
  if (matrix2 == NULL)
    return(0);

  for (row = 0; row < num_rows; row++)
    if (QccVectorSubtract(matrix1[row], matrix2[row], num_cols))
      {
        QccErrorAddMessage("(QccMatrixSubtract): Error calling QccVectorSubtract()");
        return(1);
      }

  return(0);
}


double QccMatrixMean(const QccMatrix matrix,
                     int num_rows, int num_cols)
{
  double sum = 0.0;
  int row, col;
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      sum += matrix[row][col];
  
  sum /= num_rows * num_cols;
  
  return(sum);
}


double QccMatrixVariance(const QccMatrix matrix,
                         int num_rows, int num_cols)
{
  double variance = 0.0;
  double mean;
  int row, col;

  mean = QccMatrixMean(matrix, num_rows, num_cols);

  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      variance +=
        (matrix[row][col] - mean) *
        (matrix[row][col] - mean);

  variance /= (num_rows * num_cols);

  return(variance);
}


double QccMatrixMaxSignalPower(const QccMatrix matrix,
                               int num_rows, int num_cols)
{
  int current_row, component;
  double max_power = -1;
  double power;
  
  for (current_row = 0; current_row < num_rows; current_row++)
    {
      power = 0;
      for (component = 0; component < num_cols; component++)
        power += matrix[current_row][component]*
          matrix[current_row][component];
      if (power > max_power)
        max_power = power;
    }
  
  return(max_power);
  
}


int QccMatrixVectorMultiply(const QccMatrix matrix,
                            const QccVector vector1,
                            QccVector vector2,
                            int num_rows, int num_cols)
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
      QccVectorDotProduct(matrix[row], vector1, num_cols);

  return(0);
}


int QccMatrixMultiply(const QccMatrix matrix1,
                      int num_rows1,
                      int num_cols1,
                      const QccMatrix matrix2,
                      int num_rows2,
                      int num_cols2,
                      QccMatrix matrix3)
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
      QccErrorAddMessage("(QccMatrixMultiply): Inner matrix dimensions do not agree");
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


int QccMatrixDCT(const QccMatrix input_block,
                 QccMatrix output_block,
                 int num_rows, int num_cols)
{
  int return_value;
  QccVector col_vector1 = NULL;
  QccVector col_vector2 = NULL;
  int row, col;
  
  if ((input_block == NULL) ||
      (output_block == NULL))
    return(0);
  if ((num_rows <= 0) || (num_cols <= 0))
    return(0);
  
  if ((col_vector1 = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixDCT): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((col_vector2 = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixDCT): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    if (QccVectorDCT(input_block[row],
                     output_block[row],
                     num_cols))
      {
        QccErrorAddMessage("(QccMatrixDCT): Error calling QccVectorDCT()");
        goto Error;
      }
  
  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        col_vector1[row] = output_block[row][col];
      if (QccVectorDCT(col_vector1,
                       col_vector2,
                       num_rows))
        {
          QccErrorAddMessage("(QccMatrixDCT): Error calling QccVectorDCT()");
          goto Error;
        }
      for (row = 0; row < num_rows; row++)
        output_block[row][col] = col_vector2[row];
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(col_vector1);
  QccVectorFree(col_vector2);
  return(return_value);
}


int QccMatrixInverseDCT(const QccMatrix input_block,
                        QccMatrix output_block,
                        int num_rows, int num_cols)
{
  int return_value;
  QccVector col_vector1 = NULL;
  QccVector col_vector2 = NULL;
  int row, col;
  
  if ((input_block == NULL) ||
      (output_block == NULL))
    return(0);
  if ((num_rows <= 0) || (num_cols <= 0))
    return(0);
  
  if ((col_vector1 = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixInverseDCT): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((col_vector2 = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixInverseDCT): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    if (QccVectorInverseDCT(input_block[row],
                            output_block[row],
                            num_cols))
      {
        QccErrorAddMessage("(QccMatrixInverseDCT): Error calling QccVectorInverseDCT()");
        goto Error;
      }
  
  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        col_vector1[row] = output_block[row][col];
      if (QccVectorInverseDCT(col_vector1,
                              col_vector2,
                              num_rows))
        {
          QccErrorAddMessage("(QccMatrixInverseDCT): Error calling QccVectorInverseDCT()");
          goto Error;
        }
      for (row = 0; row < num_rows; row++)
        output_block[row][col] = col_vector2[row];
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(col_vector1);
  QccVectorFree(col_vector2);
  return(return_value);
}


int QccMatrixAddNoiseToRegion(QccMatrix matrix,
                              int num_rows, int num_cols,
                              int start_row, int start_col,
                              int region_num_rows,
                              int region_num_cols,
                              double noise_variance)
{
  int stop_row, stop_col;
  int row, col;
  double val;
  
  if (matrix == NULL)
    return(0);
  
  if ((region_num_rows <= 0) ||
      (region_num_cols <= 0))
    return(0);
  
  if (start_row < 0)
    start_row = 0;
  if (start_col < 0)
    start_col = 0;
  
  stop_row = start_row + region_num_rows - 1;
  stop_col = start_col + region_num_cols - 1;
  
  if (stop_row >= num_rows)
    stop_row = num_rows - 1;
  if (stop_col >= num_cols)
    stop_col = num_cols - 1;
  
  for (row = start_row; row <= stop_row; row++)
    for (col = start_col; col <= stop_col; col++)
      {
        /*
         * The following uses the fact that the variance of
         * a (zero-mean) uniform random variable on the range (-c, c) is
         *     var = (c^2)/3
         */
        
        /*  Uniform(0.0, 1.0), mean = 0.5  */
        val = QccMathRand();
        /*  Uniform(-1.0, 1.0), variance = 1/3, mean = 0.0  */
        val = (val - 0.5)*2.0;
        /*  Uniform(-c, c), Variance = noise_variance  */
        val *= sqrt(3.0*noise_variance);
        
        matrix[row][col] += val;
      }
  
  return(0);
}


int QccMatrixInverse(const QccMatrix matrix,
                     QccMatrix matrix_inverse,
                     int num_rows,
                     int num_cols)
{
#ifdef QCC_USE_GSL

  int return_value;
  gsl_matrix *A = NULL;
  gsl_permutation *P = NULL;
  gsl_matrix *A_inverse = NULL;
  int row, col;
  int signum;
  double determinant;

  if (matrix == NULL)
    return(0);
  if (matrix_inverse == NULL)
    return(0);

  if (num_rows != num_cols)
    {
      QccErrorAddMessage("(QccMatrixInverse): Matrix is not square");
      return(1);
    }

  if ((A = gsl_matrix_alloc(num_rows, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixInverse): Error allocating memory");
      goto Error;
    }
  if ((P = gsl_permutation_alloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixInverse): Error allocating memory");
      goto Error;
    }
  if ((A_inverse = gsl_matrix_alloc(num_rows, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixInverse): Error allocating memory");
      goto Error;
    }

  gsl_matrix_set_zero(A);
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      gsl_matrix_set(A, row, col, matrix[row][col]);

  if (gsl_linalg_LU_decomp(A, P, &signum))
    {
      QccErrorAddMessage("(QccMatrixInverse): Error performing LU decomposition");
      goto Error;
    }

  determinant = gsl_linalg_LU_det(A, signum);
  if (fabs(determinant) < QCCMATHEPS)
    {
      QccErrorAddMessage("(QccMatrixInverse): Matrix must be nonsingular");
      goto Error;
    }

  if (gsl_linalg_LU_invert(A, P, A_inverse))
    {
      QccErrorAddMessage("(QccMatrixInverse): Error performing LU matrix inverse");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      matrix_inverse[row][col] = gsl_matrix_get(A_inverse, row, col);
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  gsl_matrix_free(A);
  gsl_permutation_free(P);
  gsl_matrix_free(A_inverse);
  return(return_value);

#else

  QccErrorAddMessage("(QccMatrixInverse): GSL support is not available -- matrix inverse is thus not supported");
  return(1);

#endif
}


int QccMatrixSVD(const QccMatrix A,
                 int num_rows,
                 int num_cols,
                 QccMatrix U,
                 QccVector S,
                 QccMatrix V)
{
#ifdef QCC_USE_GSL

  int return_value;
  gsl_matrix *A2 = NULL;
  gsl_matrix *V2 = NULL;
  gsl_vector *S2 = NULL;
  gsl_vector *work = NULL;
  int row, col;
  int num_rows2;

  if (A == NULL)
    return(0);

  if (num_rows >= num_cols)
    num_rows2 = num_rows;
  else
    num_rows2 = num_cols;

  if ((A2 = gsl_matrix_alloc(num_rows2, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixSVD): Error allocating memory");
      goto Error;
    }
  if ((V2 = gsl_matrix_alloc(num_cols, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixSVD): Error allocating memory");
      goto Error;
    }
  if ((S2 = gsl_vector_alloc(num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixSVD): Error allocating memory");
      goto Error;
    }
  if ((work = gsl_vector_alloc(num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixSVD): Error allocating memory");
      goto Error;
    }
  
  gsl_matrix_set_zero(A2);
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      gsl_matrix_set(A2, row, col, A[row][col]);
  
  if (gsl_linalg_SV_decomp(A2, V2, S2, work))
    {
      QccErrorAddMessage("(QccMatrixSVD): Error performing SVD");
      goto Error;
    }
  
  if (U != NULL)
    for (row = 0; row < num_rows; row++)
      for (col = 0; col < num_cols; col++)
        U[row][col] = gsl_matrix_get(A2, row, col);
  
  if (S != NULL)
    for (col = 0; col < num_cols; col++)
      S[col] = gsl_vector_get(S2, col);
  
  if (V != NULL)
    for (row = 0; row < num_cols; row++)
      for (col = 0; col < num_cols; col++)
        V[row][col] = gsl_matrix_get(V2, row, col);

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  gsl_matrix_free(A2);
  gsl_matrix_free(V2);
  gsl_vector_free(S2);
  gsl_vector_free(work);
  return(return_value);

#else

  QccErrorAddMessage("(QccMatrixSVD): GSL support is not available -- SVD is thus not supported");
  return(1);

#endif
}


int QccMatrixOrthogonalize(const QccMatrix matrix1,
                           int num_rows,
                           int num_cols,
                           QccMatrix matrix2,
                           int *num_cols2)
{
  int return_value;
  QccVector eigenvalues = NULL;
  int row, col;
  int num_nonzero_eigenvectors;

  if ((eigenvalues = QccVectorAlloc(num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixOrthogonalize): Error calling QccVectorAlloc()");
      goto Error;
    }

  if (QccMatrixSVD(matrix1,
                   num_rows,
                   num_cols,
                   matrix2,
                   eigenvalues,
                   NULL))
    {
      QccErrorAddMessage("(QccMatrixOrthogonalize): Error calling QccMatrixSVD()");
      goto Error;
    }

  num_nonzero_eigenvectors = 0;
  for (col = 0; col < num_cols; col++)
    if (eigenvalues[col] < num_rows * eigenvalues[0] * QCCMATHEPS)
      for (row = 0; row < num_rows; row++)
        matrix2[row][col] = 0;
    else
      num_nonzero_eigenvectors++;

  if (num_cols2 != NULL)
    *num_cols2 = num_nonzero_eigenvectors;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(eigenvalues);
  return(return_value);
}


int QccMatrixNullspace(const QccMatrix matrix1,
                       int num_rows,
                       int num_cols,
                       QccMatrix matrix2,
                       int *num_cols2)
{
  int return_value;
  QccVector eigenvalues = NULL;
  QccMatrix V = NULL;
  int row, col;
  int index;

  if ((eigenvalues = QccVectorAlloc(num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixOrthogonalize): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((V = QccMatrixAlloc(num_cols,
                          num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccMatrixOrthogonalize): Error calling QccMatrixAlloc()");
      goto Error;
    }

  if (QccMatrixSVD(matrix1,
                   num_rows,
                   num_cols,
                   NULL,
                   eigenvalues,
                   V))
    {
      QccErrorAddMessage("(QccMatrixOrthogonalize): Error calling QccMatrixSVD()");
      goto Error;
    }

  QccMatrixZero(matrix2, num_cols, num_cols);

  index = 0;
  for (col = 0; col < num_cols; col++)
    if (eigenvalues[col] < num_cols * eigenvalues[0] * QCCMATHEPS)
      {
        for (row = 0; row < num_cols; row++)
          matrix2[row][index] = V[row][col];
        index++;
      }
  
  if (num_cols2 != NULL)
    *num_cols2 = index;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(eigenvalues);
  QccMatrixFree(V, num_cols);
  return(return_value);
}
