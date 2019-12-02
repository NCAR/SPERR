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
 *
 * Written by
 *
 * Jing Zhang, at Mississippi State University, 2008 
 *
 */


#include "libQccPack.h"


int QccHYPrkltInitialize(QccHYPrklt *rklt)
{
  if (rklt == NULL)
    return(0);
  
  rklt->num_bands = 0;
  rklt->mean = NULL;
  rklt->matrix = NULL;
  rklt->P = NULL;
  rklt->L = NULL;
  rklt->U = NULL;
  rklt->S = NULL;
  
  rklt->factored = 0;

  return(0);
}


int QccHYPrkltAlloc(QccHYPrklt *rklt)
{
  if (rklt == NULL)
    return(0);
  
  if (rklt->mean == NULL)
    {
      if (rklt->num_bands > 0)
        {
          if ((rklt->mean = 
               QccVectorIntAlloc(rklt->num_bands)) == NULL)
            {
              QccErrorAddMessage("(QccHYPrkltAlloc): Error calling QccVectorIntAlloc()");
              return(1);
            }
        }
      else
        rklt->mean = NULL;
    }
  
  if (rklt->matrix == NULL)
    {
      if (rklt->num_bands > 0)
        {
          if ((rklt->matrix = 
               QccMatrixAlloc(rklt->num_bands, 
                              rklt->num_bands)) == NULL)
            {
              QccErrorAddMessage("(QccHYPrkltAlloc): Error calling QccMatrixAlloc()");
              return(1);
            }
        }
      else
        rklt->matrix = NULL;
    }

  if (rklt->P == NULL)
    {
      if (rklt->num_bands > 0)
        {
          if ((rklt->P = 
               QccMatrixAlloc(rklt->num_bands, 
                              rklt->num_bands)) == NULL)
            {
              QccErrorAddMessage("(QccHYPrkltAlloc): Error calling QccMatrixAlloc()");
              return(1);
            }
        }
      else
        rklt->P = NULL;
    }

  if (rklt->L == NULL)
    {
      if (rklt->num_bands > 0)
        {
          if ((rklt->L = 
               QccMatrixAlloc(rklt->num_bands, 
                              rklt->num_bands)) == NULL)
            {
              QccErrorAddMessage("(QccHYPrkltAlloc): Error calling QccMatrixAlloc()");
              return(1);
            }
        }
      else
        rklt->L = NULL;
    }

  if (rklt->U == NULL)
    {
      if (rklt->num_bands > 0)
        {
          if ((rklt->U = 
               QccMatrixAlloc(rklt->num_bands, 
                              rklt->num_bands)) == NULL)
            {
              QccErrorAddMessage("(QccHYPrkltAlloc): Error calling QccMatrixAlloc()");
              return(1);
            }
        }
      else
        rklt->U = NULL;
    }

  if (rklt->S == NULL)
    {
      if (rklt->num_bands > 0)
        {
          if ((rklt->S= 
               QccMatrixAlloc(rklt->num_bands, 
                              rklt->num_bands)) == NULL)
            {
              QccErrorAddMessage("(QccHYPrkltAlloc): Error calling QccMatrixAlloc()");
              return(1);
            }
        }
      else
        rklt->S = NULL;
    }

  return(0);
}


void QccHYPrkltFree(QccHYPrklt *rklt)
{
  if (rklt == NULL)
    return;
  
  if (rklt->mean != NULL)
    {
      QccVectorIntFree(rklt->mean);
      rklt->mean = NULL;
    }
  
  if (rklt->matrix != NULL)
    {
      QccMatrixFree(rklt->matrix, 
                    rklt->num_bands);
      rklt->matrix = NULL;
    }

  if (rklt->P != NULL)
    {
      QccMatrixFree(rklt->P, 
                    rklt->num_bands);
      rklt->P = NULL;
    }

  if (rklt->L != NULL)
    {
      QccMatrixFree(rklt->L, 
                    rklt->num_bands);
      rklt->L = NULL;
    }

  if (rklt->U != NULL)
    {
      QccMatrixFree(rklt->U, 
                    rklt->num_bands);
      rklt->U = NULL;
    }

  if (rklt->S != NULL)
    {
      QccMatrixFree(rklt->S, 
                    rklt->num_bands);
      rklt->S = NULL;
    }
}


static int QccHYPrkltMean(const QccVolumeInt image,
                          int num_bands,
                          int num_rows,
                          int num_cols,
                          QccVector mean)
{
  int row, col, band;

  for (band = 0; band < num_bands; band++)
    {
      mean[band] = 0;
      
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          mean[band] +=
            image[band][row][col];
      
      mean[band] /= (num_rows * num_cols);
    }
  
  return(0);
}


static int QccHYPrkltCovariance(const QccVolumeInt image,
                                int num_bands,
                                int num_rows,
                                int num_cols,
                                const QccVector mean,
                                QccMatrix covariance)
{
  int row, col;
  int covariance_row, covariance_col;

  for (covariance_row = 0; covariance_row < num_bands; covariance_row++)
    for (covariance_col = 0; covariance_col < num_bands; covariance_col++)
      if (covariance_col >= covariance_row)
        {
          covariance[covariance_row][covariance_col] = 0;
          
          for (row = 0; row < num_rows; row++)
            for (col = 0; col < num_cols; col++)
              covariance[covariance_row][covariance_col] +=
                image[covariance_row][row][col] *
                image[covariance_col][row][col];
          
          covariance[covariance_row][covariance_col] /=
            (num_rows * num_cols);
          covariance[covariance_row][covariance_col] -=
            mean[covariance_row] * mean[covariance_col];
        }
      else
        covariance[covariance_row][covariance_col] =
          covariance[covariance_col][covariance_row];
  
  return(0);
}


int QccHYPrkltTrain(const QccVolumeInt image,
                    int num_bands,
                    int num_rows,
                    int num_cols,
                    QccHYPrklt *rklt)
{
  int return_value;
  int row, col;
  QccMatrix covariance = NULL;
  QccVector mean = NULL;

  if (image == NULL)
    return(0);
  if (rklt == NULL)
    return(0);
  
  if (num_bands != rklt->num_bands)
    {
      QccErrorAddMessage("(QccHYPrkltTrain): Number of bands in KLT (%d) does not match that of image cube (%d)",
                         rklt->num_bands,
                         num_bands);
      goto Error;
    }
  
  if ((mean = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltTrain): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  if (QccHYPrkltMean(image, num_bands, num_rows, num_cols, mean))
    {
      QccErrorAddMessage("(QccHYPrkltTrain): Error calling QccHYPkltMean()");
      goto Error;
    }
  
  if ((covariance = QccMatrixAlloc(num_bands, num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltTrain): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if (QccHYPrkltCovariance(image, num_bands, num_rows, num_cols,
                           mean, covariance))
    {
      QccErrorAddMessage("(QccHYPrkltTrain): Error calling QccHYPrkltCovariance()");
      goto Error;
    }

  if (QccMatrixSVD(covariance,
                   num_bands,
                   num_bands,
                   rklt->matrix,
                   NULL,
                   NULL))
    {
      QccErrorAddMessage("(QccHYPrkltTrain): Error calling QccMatrixSVD()");
      goto Error;
    }
  
  for (row = 0; row < num_bands; row++)
    {
      for (col = 0; col < num_bands; col++)
        rklt->matrix[row][col] = (float)rklt->matrix[row][col];
      rklt->mean[row] = (int)rint(mean[row]);
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(covariance, num_bands);
  return(return_value);
}


int QccHYPrkltFactorization(QccHYPrklt *rklt)
{
  int i, j, k;
  int row = 0;
  int col = 0;
  double minvalue, s;
  QccMatrix matrix_temp1 = NULL;
  QccMatrix matrix_temp2 = NULL;
  QccVolume L_temp = NULL;
  QccVectorInt P_temp = NULL;
  QccMatrix S_temp = NULL;
  
  if (rklt == NULL)
    return(0);
  if (rklt->matrix == NULL)
    return(0);
  if (rklt->P == NULL)
    return(0);
  if (rklt->L == NULL)
    return(0);
  if (rklt->U == NULL)
    return(0);
  if (rklt->S == NULL)
    return(0);

  if ((matrix_temp1 = QccMatrixAlloc(rklt->num_bands, rklt->num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltFactorization): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((matrix_temp2 = QccMatrixAlloc(rklt->num_bands, rklt->num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltFactorization): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((P_temp = QccVectorIntAlloc(rklt->num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltFactorization): Error calling QccVectorIntAlloc()");
      goto Error;
    }
  if ((L_temp = QccVolumeAlloc(rklt->num_bands - 1, rklt->num_bands,
                               rklt->num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltFactorization): Error calling QccVolumeAlloc()");
      goto Error;
    }
  if ((S_temp = QccMatrixAlloc(rklt->num_bands, rklt->num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltFactorization): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  QccMatrixTranspose(rklt->matrix, matrix_temp2, rklt->num_bands, rklt->num_bands);
  QccMatrixCopy(matrix_temp1, matrix_temp2, rklt->num_bands, rklt->num_bands);
  
  QccMatrixIdentity(rklt->P, rklt->num_bands, rklt->num_bands);	 
  QccMatrixIdentity(rklt->S, rklt->num_bands, rklt->num_bands);
  
  for (k = 0; k < rklt->num_bands - 1; k++)
    QccMatrixIdentity(L_temp[k], rklt->num_bands, rklt->num_bands);	
  
  // K step
  for (k = 0; k < rklt->num_bands - 1; k++)
    {
      minvalue = MAXDOUBLE;
      
      for (i = k; i < rklt->num_bands; i++)
        for (j = k + 1; j < rklt->num_bands; j++)
          {
            s = (matrix_temp1[i][k] - 1.0) / matrix_temp1[i][j];
            
            if (fabs(s) < fabs(minvalue))
              {
                minvalue = s;
                
                row = i;
                col = j;  
              }
          }
      
      P_temp[k] = row;
      
      // P matrix
      QccMatrixColExchange(rklt->P, rklt->num_bands, k, row);		
      
      
      // S Matrix
      QccMatrixIdentity(S_temp, rklt->num_bands, rklt->num_bands);	     
      S_temp[col][k] = -1 * minvalue;
      
      if (k == 0)
        rklt->S[col][k] = minvalue;
      else
        for (j = 0; j < k + 1; j++)
          rklt->S[col][j] = rklt->S[col][j] + minvalue * rklt->S[k][j];
      
      // matrix = P * matrix
      QccMatrixRowExchange(matrix_temp1, rklt->num_bands, k, P_temp[k]);	
      
      // L[k][][] matrix	
      for(i = k + 1; i < rklt->num_bands; i++)
        L_temp[k][i][k] = minvalue * matrix_temp1[i][col] - matrix_temp1[i][k];
      
      
      // matrix =  P(k) * A(k-1) * S(k) 
      QccMatrixMultiply(matrix_temp1, rklt->num_bands, rklt->num_bands,
                        S_temp, rklt->num_bands, rklt->num_bands,
                        matrix_temp2);
      
      // U(k) Matrix U(k) = A(k) = L(k) * P(k) * A(k-1) * S(k) 
      QccMatrixMultiply(L_temp[k], rklt->num_bands, rklt->num_bands,
                        matrix_temp2, rklt->num_bands, rklt->num_bands,
                        matrix_temp1);
    }
  
  // U Matrix
  QccMatrixCopy(rklt->U, matrix_temp1, rklt->num_bands, rklt->num_bands);
  
  // L Matrix   
  for (k = 0; k < rklt->num_bands - 2 ; k++)
    for (i = k + 1; i < rklt->num_bands - 1; i++)
      { 
        QccMatrixRowExchange(L_temp[k], rklt->num_bands, i, P_temp[i]);
        QccMatrixColExchange(L_temp[k], rklt->num_bands, i, P_temp[i]);
      }
  
  QccMatrixCopy(rklt->L, L_temp[0], rklt->num_bands, rklt->num_bands);	
  for (k = 1; k < rklt->num_bands - 1; k++)
    {
      QccMatrixMultiply(L_temp[k], rklt->num_bands, rklt->num_bands, 
                        rklt->L,  rklt->num_bands, rklt->num_bands,
                        matrix_temp2);
      QccMatrixCopy(rklt->L, matrix_temp2, rklt->num_bands, rklt->num_bands);
    }
  if (QccMatrixInverse(matrix_temp2, rklt->L, rklt->num_bands, rklt->num_bands))
    {
      QccErrorAddMessage("(QccHYPrkltFactorization): Error calling QccMatrixInverse()");
      goto Error;
    }

  rklt->factored = 1;

 Error:
  QccMatrixFree(matrix_temp1, rklt->num_bands);
  QccMatrixFree(matrix_temp2, rklt->num_bands);
  QccVectorIntFree(P_temp);
  QccVolumeFree(L_temp, rklt->num_bands - 1, rklt->num_bands);
  QccMatrixFree(S_temp, rklt->num_bands);
  
  return(0);
}


static int QccHYPrkltApplyTransform(QccVector X,
                                    QccVector Y,
                                    const QccHYPrklt *rklt)
{
  int m, n;
  double tmp;
  int N = rklt->num_bands;

  for (m = N - 1; m > 0; m--)
    {
      tmp = 0;
      for (n = 0; n < m; n++)
        tmp = tmp + rklt->S[m][n]*X[n];
      Y[m] = X[m] + rint(tmp);
    }
  Y[0] = X[0];
  
  for(m = 0; m < N - 1; m++)
    {
      tmp = 0;
      for (n = m + 1; n < N; n++)
        tmp = tmp + rklt->U[m][n]*Y[n];
      Y[m] = Y[m] + rint(tmp);
    }
  Y[N-1] *= rint(rklt->U[N-1][N-1]);
  
  for (m = N - 1; m > 0; m--)
    {
      tmp = 0;
      for (n = 0; n < m; n++)
        tmp = tmp + rklt->L[m][n]*Y[n];
      Y[m] = Y[m] + rint(tmp);
    }
  
  QccMatrixVectorMultiply(rklt->P, Y, X, N, N);
  for (m = 0; m < N; m++)
    Y[m] = X[m];
  
  return(0);
}


static int QccHYPrkltApplyInverseTransform(QccVector Y,
                                           QccVector X,
                                           const QccMatrix Pinv,
                                           const QccHYPrklt *rklt)
{
  int m, n;
  double tmp;
  int N = rklt->num_bands;

  QccMatrixVectorMultiply(Pinv, Y, X, N, N);
  
  for (m = 1; m < N; m++)
    {
      tmp = 0;
      for (n = 0; n < m; n++)
        tmp = tmp + rklt->L[m][n]*X[n];
      X[m] = X[m] - rint(tmp);
    }
  
  X[N-1] /= rint(rklt->U[N-1][N-1]);
  for (m = N - 2; m >= 0; m--)
    {
      tmp = 0;
      for (n = m + 1; n < N; n++)
        tmp = tmp + rklt->U[m][n]*X[n];
      X[m] = X[m] - rint(tmp);
    }
  
  for (m = 1; m < N; m++)
    {
      tmp = 0;
      for (n = 0; n < m; n++)
        tmp = tmp + rklt->S[m][n]*X[n];
      X[m] = X[m] - rint(tmp);
    }
  
  return(0);
}


int QccHYPrkltTransform(QccVolumeInt image,
                        int num_bands,
                        int num_rows,
                        int num_cols,
                        const QccHYPrklt *rklt)
{
  int return_value;
  int band;
  int row, col;
  QccVector temp1 = NULL;
  QccVector temp2 = NULL;
  
  if (image == NULL)
    return(0);
  if (rklt == NULL)
    return(0);
  if (rklt->P == NULL)
    return(0);
  if (rklt->L == NULL)
    return(0);
  if (rklt->U == NULL)
    return(0);
  if (rklt->S == NULL)
    return(0);

  if (!rklt->factored)
    {
      QccErrorAddMessage("(QccHYPrkltTransform): RKLT must be factored first");
      goto Error;
    }

  if (rklt->num_bands != num_bands)
    {
      QccErrorAddMessage("(QccHYPrkltTransform): Number of bands in RKLT (%d) does not match that of image cube (%d)",
                         rklt->num_bands,
                         num_bands);
      goto Error;
    }
  
  if ((temp1 = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltTransform): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((temp2 = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltTransform): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        for (band = 0; band < num_bands; band++)
          temp1[band] = 
            (double)(image[band][row][col] - rklt->mean[band]);

        if (QccHYPrkltApplyTransform(temp1,
                                     temp2,
                                     rklt))
          {
            QccErrorAddMessage("(QccHYPrkltTransform): Error calling QccHYPrkltApplyTransform()");
            goto Error;
          }
        
        for (band = 0; band < num_bands; band++)
          image[band][row][col] = (int)temp2[band];
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp1);
  QccVectorFree(temp2);
  return(return_value);
}


int QccHYPrkltInverseTransform(QccVolumeInt image,
                               int num_bands,
                               int num_rows,
                               int num_cols,
                               const QccHYPrklt *rklt)
{
  int return_value;
  int band;
  int row, col;
  QccVector temp1 = NULL;
  QccVector temp2 = NULL;
  QccMatrix Pinv = NULL;
  
  if (image == NULL)
    return(0);
  if (rklt == NULL)
    return(0);
  if (rklt->P == NULL)
    return(0);
  if (rklt->L == NULL)
    return(0);
  if (rklt->U == NULL)
    return(0);
  if (rklt->S == NULL)
    return(0);

  if (!rklt->factored)
    {
      QccErrorAddMessage("(QccHYPrkltInverseTransform): RKLT must be factored first");
      goto Error;
    }
  
  if (rklt->num_bands != num_bands)
    {
      QccErrorAddMessage("(QccHYPrkltInverseTransform): Number of bands in RKLT (%d) does not match that of image cube (%d)",
                         rklt->num_bands,
                         num_bands);
      goto Error;
    }
  
  if ((temp1 = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltInverseTransform): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((temp2 = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltInverseTransform): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((Pinv = QccMatrixAlloc(num_bands, num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPrkltInverseTransform): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if (QccMatrixInverse(rklt->P, Pinv, num_bands, num_bands))
    {
      QccErrorAddMessage("(QccHYPrkltInverseTransform): Error calling QccMatrixInverse()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        for (band = 0; band < num_bands; band++)
          temp1[band] = (double)image[band][row][col];
        if (QccHYPrkltApplyInverseTransform(temp1,
                                            temp2,
                                            Pinv,
                                            rklt))
          {
            QccErrorAddMessage("(QccHYPrkltInverseTransform): Error calling InverseIntegerTransform()");
            goto Error;
          }
        
        for (band = 0; band < num_bands; band++)
          image[band][row][col] =
            (int)(temp2[band] + rklt->mean[band]);
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp1);
  QccVectorFree(temp2);
  QccMatrixFree(Pinv, num_bands);
  return(return_value);
}
