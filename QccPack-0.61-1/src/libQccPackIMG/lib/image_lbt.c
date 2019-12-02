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


/* Written by VIJAY SHAH, at Mississippi State University, 2005 */


#include "libQccPack.h"

#define QCCIMGLBT_PREFILTER 0
#define QCCIMGLBT_POSTFILTER 1


static int QccIMGImageComponentLBTCreateDCTII(QccMatrix dct2_matrix, int M)
{
  double temp1;
  double temp2;
  int i, j;
  
  for (i = 0; i < M; i++)
    {
      temp1 = 2 * i + 1;
      for (j = 0; j < M; j++)
        {
          temp2 = j;
          dct2_matrix[i][j] =
            sqrt(2.0 / (double)M) *
            cos((temp1 * temp2 * M_PI) / (double)(2 * M));
          if (j == 0)
            dct2_matrix[i][j] = dct2_matrix[i][j] / sqrt(2.0);
        }
    }
  
  return(0);
}


static int QccIMGImageComponentLBTCreateDCTIV(QccMatrix dct4_matrix,
                                              int M)
{
  double temp1;
  double temp2;
  int i, j;
  
  for (i = 0; i < M; i++)
    {
      temp1 = 2 * i + 1;
      for (j = 0; j < M; j++)
        {
          temp2 = 2 * j + 1;
          dct4_matrix[i][j] =
            sqrt(2.0 / (double)M) *
            cos((temp1 * temp2 * M_PI) / (double)(4 * M));
        }
    }

  return(0);
}


static int QccIMGImageComponentLBTCreateV(QccMatrix v_filter,
                                          int N,
                                          int M,
                                          double s1)
{
  int return_value;
  QccMatrix dct2_matrix = NULL;
  QccMatrix dct4_matrix = NULL;
  QccMatrix I = NULL;
  QccMatrix J = NULL;
  QccMatrix S = NULL;
  QccMatrix temp_V = NULL;
  QccMatrix temp1_V = NULL;
  QccMatrix temp2_V = NULL;
  int mn;
  int i, j;
  
  mn = (int)((M - (2 * N)) / 2);
  
  if ((dct4_matrix = QccMatrixAlloc(N, N)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((dct2_matrix = QccMatrixAlloc(N, N)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((I = QccMatrixAlloc(N, N)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((J = QccMatrixAlloc(N, N)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((S = QccMatrixAlloc(N, N)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((temp_V = QccMatrixAlloc(N, N)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((temp1_V = QccMatrixAlloc(N, N)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((temp2_V = QccMatrixAlloc(N, N)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixAlloc()");
      goto Error;
    } 
  
  for (i = 0; i < N; i++)
    for (j = 0;j < N; j++)
      if (i == j)
        {
          J[i][N - j - 1] = 1;
          S[i][j] = 1;
        }
      else
        {
          J[i][N - j - 1] = 0;
          S[i][j] = 0;
        }

  S[0][0] = s1;

  if (QccIMGImageComponentLBTCreateDCTIV(dct4_matrix, N))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccIMGImageComponentLBTCreateDCTIV()");
      goto Error;
    } 

  if (QccIMGImageComponentLBTCreateDCTII(dct2_matrix, N))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccIMGImageComponentLBTCreateDCTII()");
      goto Error;
    } 
  
  if (QccMatrixMultiply(J, N, N,
                        dct2_matrix, N, N,
                        temp_V))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixMultiply()");
      goto Error;
    } 
  
  if (QccMatrixMultiply(temp_V, N, N,
                        S, N, N,
                        temp1_V))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixMultiply()");
      goto Error;
    } 
  
  if (QccMatrixMultiply(dct4_matrix, N, N,
                        J, N, N,
                        temp_V))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixMultiply()");
      goto Error;
    } 
  
  if (QccMatrixMultiply(temp1_V, N, N,
                        temp_V, N, N,
                        temp2_V))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreateV): Error calling QccMatrixMultiply()");
      goto Error;
    } 
  
  for (i = 0; i < N + mn; i++)
    for (j = 0; j < N + mn; j++)
      if ((i < N) && (j < N))
        v_filter[i][j] = temp2_V[i][j];
      else
        if ((i >= N) && (i == j))
          v_filter[i][j] = 1;
        else
          v_filter[i][j] = 0;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(dct2_matrix, N); 
  QccMatrixFree(dct4_matrix, N); 
  QccMatrixFree(I, N); 
  QccMatrixFree(J, N); 
  QccMatrixFree(S, N); 
  QccMatrixFree(temp_V, N); 
  QccMatrixFree(temp1_V, N); 
  QccMatrixFree(temp2_V, N); 

  return(return_value);
} 


static int QccIMGImageComponentLBTCreateFilter(QccMatrix p_filter,
                                               int N,
                                               int f_size,
                                               double s1,
                                               int transform_direction)
{
  int return_value;
  int M;
  int i, j;
  QccMatrix v_filter = NULL;
  QccMatrix bf1 = NULL;
  QccMatrix bf2 = NULL;
  QccMatrix temp_P = NULL;
  
  M = f_size / 2;
  
  if ((v_filter = QccMatrixAlloc(M, M)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreate): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((bf1 = QccMatrixAlloc(f_size, f_size)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreate): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((bf2 = QccMatrixAlloc(f_size, f_size)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreate): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((temp_P = QccMatrixAlloc(f_size, f_size)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreate): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  QccMatrixZero(bf1, f_size, f_size);
  QccMatrixZero(bf2, f_size, f_size);
  
  for (i = 0; i < f_size; i++)
    for (j = 0; j < f_size; j++)
      if (i == j)
        {
          if(i >= M)
            bf1[i][j] = -1;
          else
            bf1[i][j] = 1;
          
          bf1[i][f_size - j - 1] = 1;
        }
  
  if (QccIMGImageComponentLBTCreateV(v_filter, N, f_size, s1))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreate): Error calling QccIMGImageComponentLBTCreateV()");
      goto Error;
    }
  
  if (transform_direction == QCCIMGLBT_POSTFILTER)
    if (QccMatrixInverse(v_filter, v_filter, M, M))
      {
        QccErrorAddMessage("(QccIMGImageComponentLBTCreate): Error calling QccMatrixInverse()");
        goto Error;
      }
  
  for (i = 0; i < f_size; i++)
    for (j = 0; j < f_size; j++)
      {
        if ((i < M) && (j < M))
          {
            if (i == j)
              bf2[i][j] = 1;
          }
        if ((i >= M) && (j >= M))
          bf2[i][j] = v_filter[i - M][j - M];
      }
  
  if (QccMatrixMultiply(bf1, f_size, f_size,
                        bf2, f_size, f_size,
                        temp_P))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreate): Error calling QccMatrixMultiply()");
      goto Error;
    }
  
  if (QccMatrixMultiply(temp_P, f_size, f_size,
                        bf1, f_size, f_size,
                        p_filter))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTCreate): Error calling QccMatrixMultiply()");
      goto Error;
    }
  
  for (i = 0; i < f_size; i++)
    for (j = 0; j < f_size; j++)
      p_filter[i][j] = p_filter[i][j] * 0.5;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(v_filter, M);
  QccMatrixFree(bf1, f_size);
  QccMatrixFree(bf2, f_size);
  QccMatrixFree(temp_P, f_size);
  return(return_value);
} 


static int QccIMGImageComponentLBTProcess(const QccMatrix input_image,
                                          const QccMatrix p_filter,
                                          QccMatrix output_image,
                                          int num_row,
                                          int num_cols,
                                          int f_size)
{
  int return_value;
  int row, col;
  int i;
  QccMatrix temp_image = NULL;
  QccVector temp_vector = NULL;
  QccVector filter_vector = NULL;
  
  if ((temp_image = QccMatrixAlloc(num_row, num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTProcess): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((temp_vector = QccVectorAlloc(f_size)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTProcess): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((filter_vector = QccVectorAlloc(f_size)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBTProcess): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  for (col = 0; col < num_cols; col++)
    for (row = 0; row < f_size / 2; row++)
      {
        temp_image[row][col] = input_image[row][col];  
        temp_image[num_row - row - 1][col] =
          input_image[num_row - row - 1][col];
      }
  
  for (col = 0; col < num_cols; col++)
    for (row = f_size / 2; row < num_row - (f_size / 2); row += f_size)
      {
        for (i = 0; i < f_size; i++)
          temp_vector[i] = input_image[row + i][col];
        
        if (QccMatrixVectorMultiply(p_filter,
                                    temp_vector,
                                    filter_vector,
                                    f_size,
                                    f_size))
          { 
            QccErrorAddMessage("(QccIMGImageComponentLBTProcess): Error calling QccMatrixVectorMultiply()");
            goto Error;
          }
        
        for (i = 0; i < f_size; i++)
          temp_image[row + i][col] = filter_vector[i];
      }
  
  for (row = 0; row < num_row; row++)
    for (col = 0; col < f_size / 2; col++)
      {
        output_image[row][col] = temp_image[row][col];  
        output_image[row][num_cols - col - 1] =
          temp_image[row][num_cols - col - 1];
      }
  
  for (row = 0; row < num_row; row++)
    for (col = f_size / 2; col < num_cols - (f_size / 2); col += f_size)
      {
        for (i = 0; i < f_size; i++)
          temp_vector[i] = temp_image[row][col + i];
        
        if (QccMatrixVectorMultiply(p_filter,
                                    temp_vector,
                                    filter_vector,
                                    f_size,
                                    f_size))
          {
            QccErrorAddMessage("(QccIMGImageComponentLBTProcess): Error calling QccVectorAdd()");
            goto Error;
          }
        for (i = 0; i < f_size; i++)
          output_image[row][col + i] = filter_vector[i];
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(temp_image, num_row);
  QccVectorFree(temp_vector);
  QccVectorFree(filter_vector);
  return(return_value);
} 


int QccIMGImageComponentLBT(const QccIMGImageComponent *image1,
                            QccIMGImageComponent *image2,
                            int overlap_sample,
                            int block_size,
                            double smooth_factor)
{
  int return_value;
  QccMatrix prefilter = NULL;
  
  if (image1 == NULL)
    return(0);
  if (image2 == NULL)
    return(0);

  if ((image1->num_rows != image2->num_rows) ||
      (image1->num_cols != image2->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBT): Images must have the same size");
      goto Error;
    }

  if ((prefilter = QccMatrixAlloc(block_size, block_size)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentLBT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if (QccIMGImageComponentLBTCreateFilter(prefilter,
                                          overlap_sample,
                                          block_size,
                                          smooth_factor,
                                          QCCIMGLBT_PREFILTER))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBT): Error calling QccIMGImageComponentLBTCreateFilter()");
      goto Error;
    }
  
  if (QccIMGImageComponentLBTProcess(image1->image,
                                     prefilter,
                                     image2->image,
                                     image1->num_rows,
                                     image1->num_cols,
                                     block_size))
    {
      QccErrorAddMessage("(QccIMGImageComponentLBT): Error calling QccIMGImageComponentLBTProcess()");
      goto Error;
    }
  
  if (QccIMGImageComponentDCT(image2,
                              image2,
                              block_size,
                              block_size))
    {
      QccErrorAddMessage("(QccWAVdcttceEncodeDCT): Error calling QccIMGImageComponentDCT()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(prefilter, block_size);
  return(return_value);
} 


int QccIMGImageComponentInverseLBT(const QccIMGImageComponent *image1,
                                   QccIMGImageComponent *image2,
                                   int overlap_sample,
                                   int block_size,
                                   double smooth_factor)
{
  int return_value;
  QccMatrix postfilter = NULL;
  
  if (image1 == NULL)
    return(0);
  if (image2 == NULL)
    return(0);

  if ((image1->num_rows != image2->num_rows) ||
      (image1->num_cols != image2->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseLBT): Images must have the same size");
      goto Error;
    }

  if ((postfilter = QccMatrixAlloc(block_size, block_size)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseLBT): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if (QccIMGImageComponentInverseDCT(image1,
                                     image2,
                                     block_size,
                                     block_size))
    {
      QccErrorAddMessage("(QccWAVdcttceDecodeInverseLBT): Error calling QccIMGImageComponentInverseDCT()");
      goto Error;
    }
  
  if (QccIMGImageComponentLBTCreateFilter(postfilter,
                                          overlap_sample,
                                          block_size,
                                          smooth_factor,
                                          QCCIMGLBT_POSTFILTER))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseLBT): Error calling QccIMGImageComponentLBTCreateFilter()");
      goto Error;
    }
  
  if (QccIMGImageComponentLBTProcess(image2->image,
                                     postfilter,
                                     image2->image,
                                     image2->num_rows,
                                     image2->num_cols,
                                     block_size))
    {
      QccErrorAddMessage("(QccIMGImageComponentInverseLBT): Error calling QccIMGImageComponentLBTProcess()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(postfilter, block_size);
  return(return_value);
}
