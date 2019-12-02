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


QccVectorInt QccVectorIntAlloc(int vector_dimension)
{
  QccVectorInt vector;

  if (vector_dimension <= 0)
    return(NULL);

  if ((vector = (QccVectorInt)calloc(vector_dimension, sizeof(int))) == NULL)
    QccErrorAddMessage("(QccVectorIntAlloc): Error allocating memory");

  return(vector);
}


void QccVectorIntFree(QccVectorInt vector)
{
  if (vector != NULL)
    QccFree(vector);
}


int QccVectorIntZero(QccVectorInt vector, int vector_dimension)
{
  int component;
  
  if ((vector == NULL) || (vector_dimension <= 0))
    return(0);
  for (component = 0; component < vector_dimension; component++)
    vector[component] = 0;
  
  return(0);
}


QccVectorInt QccVectorIntResize(QccVectorInt vector,
                                int vector_dimension,
                                int new_vector_dimension)
{
  int index;
  QccVectorInt new_vector = NULL;
  
  if ((vector_dimension < 0) || (new_vector_dimension < 0))
    return(vector);
  
  if (new_vector_dimension == vector_dimension)
    return(vector);
  
  if (vector == NULL)
    {
      vector_dimension = 0;
      if ((new_vector = QccVectorIntAlloc(new_vector_dimension)) == NULL)
        {
          QccErrorAddMessage("(QccVectorIntResize): Error calling QccVectorIntAlloc()");
          return(NULL);
        }
    }
  else
    {
      if ((new_vector = 
           (QccVectorInt)realloc((void *)vector,
                                 sizeof(int) * new_vector_dimension)) == NULL)
        {
          QccErrorAddMessage("(QccVectorIntRealloc): Error reallocating memory");
          return(NULL);
        }
    }
  
  for (index = vector_dimension; index < new_vector_dimension; index++)
    new_vector[index] = 0;
  
  return(new_vector);
}


double QccVectorIntMean(const QccVectorInt vector,
                        int vector_dimension)
{
  double mean = 0.0;
  int component;

  if ((vector == NULL) || (vector_dimension <= 0))
    return((double)0.0);

  for (component = 0; component < vector_dimension; component++)
    mean += vector[component];

  mean /= vector_dimension;

  return(mean);
}


double QccVectorIntVariance(const QccVectorInt vector,
                            int vector_dimension)
{
  double mean;
  double variance = 0;
  int component;

  if ((vector == NULL) || (vector_dimension <= 0))
    return((double)0.0);

  mean = QccVectorIntMean(vector, vector_dimension);

  for (component = 0; component < vector_dimension; component++)
    variance += (vector[component] - mean) * (vector[component] - mean);

  variance /= vector_dimension;

  return(variance);
}


int QccVectorIntAdd(QccVectorInt vector1,
                    const QccVectorInt vector2,
                    int vector_dimension)
{
  int component;

  if ((vector1 == NULL) || (vector2 == NULL) || (vector_dimension <= 0))
    return(0);
  for (component = 0; component < vector_dimension; component++)
    vector1[component] += vector2[component];

  return(0);
}


int QccVectorIntSubtract(QccVectorInt vector1,
                         const QccVectorInt vector2,
                         int vector_dimension)
{
  int component;

  if ((vector1 == NULL) || (vector2 == NULL) || (vector_dimension <= 0))
    return(0);

  for (component = 0; component < vector_dimension; component++)
    vector1[component] -= vector2[component];

  return(0);
}


int QccVectorIntScalarMult(QccVectorInt vector,
                           int s,
                           int vector_dimension)
{
  int component;

  if ((vector == NULL) || (vector_dimension <= 0))
    return(0);

  for (component = 0; component < vector_dimension; component++)
    vector[component] *= s;

  return(0);
}


int QccVectorIntCopy(QccVectorInt vector1,
                     const QccVectorInt vector2,
                     int vector_dimension)
{
  int component;

  if ((vector1 == NULL) || (vector2 == NULL) || (vector_dimension <= 0))
    return(0);

  for (component = 0; component < vector_dimension; component++)
    vector1[component] = vector2[component];

  return(0);
}


double QccVectorIntNorm(const QccVectorInt vector,
                        int vector_dimension)
{
  int component;
  double squared_norm = 0.0;

  if ((vector == NULL) || (vector_dimension <= 0))
    return(squared_norm);

  for (component = 0; component < vector_dimension; component++)
    squared_norm += vector[component] * vector[component];

  return(sqrt(squared_norm));
}


int QccVectorIntDotProduct(const QccVectorInt vector1,
                           const QccVectorInt vector2,
                           int vector_dimension)
{
  int component;
  int dot_product = 0;

  if ((vector1 == NULL) ||
      (vector2 == NULL) ||
      (vector_dimension <= 0))
    return(0);

  for (component = 0; component < vector_dimension; component++)
    dot_product += vector1[component] * vector2[component];

  return(dot_product);
}


int QccVectorIntSquareDistance(const QccVectorInt vector1,
                               const QccVectorInt vector2, 
                               int vector_dimension)
{
  int diff, distance = 0;
  int component;

  if ((vector1 == NULL) || (vector2 == NULL) || (vector_dimension <= 0))
    return((int)0);

  for (component = 0; component < vector_dimension; component++)
    {
      diff = 
        vector1[component] - vector2[component];
      distance += diff * diff;
    }

  return(distance);
}


int QccVectorIntSumComponents(const QccVectorInt vector,
                              int vector_dimension)
{
  int component;
  int sum = 0;

  if (vector == NULL)
    return(0);

  for (component = 0; component < vector_dimension; component++)
    sum += vector[component];

  return(sum);
}


int QccVectorIntMaxValue(const QccVectorInt vector,
                         int vector_dimension,
                         int *winner)
{
  int component;
  int max_value = -MAXINT;
  int max_index = 0;

  if (vector != NULL)
    for (component = 0; component < vector_dimension; component++)
      if (vector[component] > max_value)
        {
          max_value = vector[component];
          max_index = component;
        }

  if (winner != NULL)
    *winner = max_index;

  return(max_value);
}


int QccVectorIntMinValue(const QccVectorInt vector,
                         int vector_dimension,
                         int *winner)
{
  int component;
  int min_value = MAXINT;
  int min_index = 0;

  if (vector != NULL)
    for (component = 0; component < vector_dimension; component++)
      if (vector[component] < min_value)
        {
          min_value = vector[component];
          min_index = component;
        }

  if (winner != NULL)
    *winner = min_index;

  return(min_value);
}




int QccVectorIntPrint(const QccVectorInt vector,
                      int vector_dimension)
{
  int component;

  if (vector == NULL)
    return(0);

  printf("< ");

  for (component = 0; component < vector_dimension; component++)
    printf("%d ", vector[component]);

  printf(">\n");

  return(0);
}


/*  Based on QS2I1D, part of the SLATEC library  */

#define QccQuickSortSwap(a,b) {int temp;temp=(a);(a)=(b);(b)=temp;}

static int QccVectorIntQuickSortAscending(QccVectorInt A,
                                          int low,
                                          int high)
{
  int low_pnt, high_pnt, pivot_position;
  int pivot;

  pivot_position = low + (high - low)/2;
  pivot = A[pivot_position];

  /*  If first element of array is greater than it, interchange with it.  */
  if (A[low] > pivot)
    QccQuickSortSwap(A[pivot_position], A[low]);

  /*  If last element of array is less than it, swap with it.  */
  if (A[high] < pivot)
    {
      QccQuickSortSwap(A[pivot_position], A[high]);

      /*  If first element of array is greater than it, swap with it.  */
      if (A[low] > A[pivot_position])
        QccQuickSortSwap(A[pivot_position], A[low]);
    }
  
  low_pnt = low;
  high_pnt = high;
  while (low_pnt < high_pnt)
    {
      /*  Find an element in the second half of the array which is
          smaller than it.  */
      do
        high_pnt--;
      while (A[high_pnt] > pivot);
      
      /*  Find an element in the first half of the array which is
          greater than it.  */
      do
        low_pnt++;
      while (A[low_pnt] < pivot);
      
      /*  Interchange these elements.  */
      if (low_pnt <= high_pnt)
        QccQuickSortSwap(A[high_pnt], A[low_pnt]);
    }

  if (high_pnt + 1 < high)
    QccVectorIntQuickSortAscending(A, high_pnt + 1, high);
  if (low < high_pnt)
    QccVectorIntQuickSortAscending(A, low, high_pnt);

  return(0);
}


static int QccVectorIntQuickSortAscendingWithAux(QccVectorInt A,
                                                 int low, 
                                                 int high,
                                                 int *auxiliary_list)
{
  int low_pnt, high_pnt, pivot_position;
  int pivot;

  pivot_position = low + (high - low)/2;
  pivot = A[pivot_position];

  /*  If first element of array is greater than it, interchange with it.  */
  if (A[low] > pivot)
    {
      QccQuickSortSwap(A[pivot_position], A[low]);
      QccQuickSortSwap(auxiliary_list[pivot_position], auxiliary_list[low]);
    }

  /*  If last element of array is less than it, swap with it.  */
  if (A[high] < pivot)
    {
      QccQuickSortSwap(A[pivot_position], A[high]);
      QccQuickSortSwap(auxiliary_list[pivot_position], auxiliary_list[high]);

      /*  If first element of array is greater than it, swap with it.  */
      if (A[low] > A[pivot_position])
        {
          QccQuickSortSwap(A[pivot_position], A[low]);
          QccQuickSortSwap(auxiliary_list[pivot_position],
                           auxiliary_list[low]);
        }
    }
  
  low_pnt = low;
  high_pnt = high;
  while (low_pnt < high_pnt)
    {
      /*  Find an element in the second half of the array which is
          smaller than it.  */
      do
        high_pnt--;
      while (A[high_pnt] > pivot);
      
      /*  Find an element in the first half of the array which is
          greater than it.  */
      do
        low_pnt++;
      while (A[low_pnt] < pivot);
      
      /*  Interchange these elements.  */
      if (low_pnt <= high_pnt)
        {
          QccQuickSortSwap(A[high_pnt], A[low_pnt]);
          QccQuickSortSwap(auxiliary_list[high_pnt], auxiliary_list[low_pnt]);
        }
    }

  if (high_pnt + 1 < high)
    QccVectorIntQuickSortAscendingWithAux(A, high_pnt + 1, high,
                                          auxiliary_list);

  if (low < high_pnt)
    QccVectorIntQuickSortAscendingWithAux(A, low, high_pnt, auxiliary_list);

  return(0);
}


int QccVectorIntSortComponents(const QccVectorInt vector,
                               QccVectorInt sorted_vector,
                               int vector_dimension,
                               int sort_direction,
                               int *auxiliary_list)
{
  int component;

  if ((vector == NULL) || (sorted_vector == NULL) ||
      (vector_dimension <= 0))
    return(0);

  if (sort_direction == QCCVECTOR_SORTDESCENDING)
    for (component = 0; component < vector_dimension; component++)
      sorted_vector[component] = -vector[component];
  else
    for (component = 0; component < vector_dimension; component++)
      sorted_vector[component] = vector[component];

  if (auxiliary_list == NULL)
    {
      if (QccVectorIntQuickSortAscending(sorted_vector, 0,
                                         vector_dimension - 1))
        {
          QccErrorAddMessage("(QccVectorIntSortComponents): Error calling QccVectorIntQuickSortAscending()");
          return(1);
        }
    }
  else
    if (QccVectorIntQuickSortAscendingWithAux(sorted_vector, 0, 
                                              vector_dimension - 1,
                                              auxiliary_list))
      {
        QccErrorAddMessage("(QccVectorIntSortComponents): Error calling QccVectorIntQuickSortAscendingWithIndex()");
        return(1);
      }

  if (sort_direction == QCCVECTOR_SORTDESCENDING)
    for (component = 0; component < vector_dimension; component++)
      sorted_vector[component] = -sorted_vector[component];

  return(0);
}


int QccVectorIntMoveComponentToFront(QccVectorInt vector,
                                     int vector_dimension,
                                     int index)
{
  int component;
  int tmp;

  if (!index)
    return(0);
  if (index >= vector_dimension)
    return(1);

  tmp = vector[index];
  for (component = index; component; component--)
    vector[component] = vector[component - 1];
  vector[0] = tmp;

  return(0);
}


int QccVectorIntSubsample(const QccVectorInt input_signal,
                          int input_length,
                          QccVectorInt output_signal,
                          int output_length,
                          int sampling_flag)
{
  int index;

  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);

  switch (sampling_flag)
    {
    case QCCVECTOR_EVEN:
      for (index = 0;
           (index < output_length) && ((index * 2) < input_length);
           index++)
        output_signal[index] = input_signal[index * 2];
      break;
    case QCCVECTOR_ODD:
      for (index = 0;
           (index < output_length) && ((index * 2 + 1) < input_length);
           index++)
        output_signal[index] = input_signal[index * 2 + 1];
      break;
    default:
      QccErrorAddMessage("(QccVectorIntSubsample): Undefined sampling (%d)",
                         sampling_flag);
      return(1);
    }

  return(0);
}


int QccVectorIntUpsample(const QccVectorInt input_signal,
                         int input_length,
                         QccVectorInt output_signal,
                         int output_length,
                         int sampling_flag)
{
  int index;

  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);

  QccVectorIntZero(output_signal, output_length);

  switch (sampling_flag)
    {
    case QCCVECTOR_EVEN:
      for (index = 0;
           (index < input_length) && ((index * 2) < output_length);
           index++)
        output_signal[index * 2] = input_signal[index];
      break;
    case QCCVECTOR_ODD:
      for (index = 0;
           (index < input_length) && ((index * 2 + 1) < output_length);
           index++)
        output_signal[index * 2 + 1] = input_signal[index];
      break;
    default:
      QccErrorAddMessage("(QccVectorIntUpsample): Undefined sampling (%d)",
                         sampling_flag);
      return(1);
    }

  return(0);
}
