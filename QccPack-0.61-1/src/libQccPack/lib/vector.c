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


QccVector QccVectorAlloc(int vector_dimension)
{
  QccVector vector;

  if (vector_dimension <= 0)
    return(NULL);

  if ((vector = (QccVector)calloc(vector_dimension, sizeof(double))) == NULL)
    QccErrorAddMessage("(QccVectorAlloc): Error allocating memory");

  return(vector);
}


void QccVectorFree(QccVector vector)
{
  if (vector != NULL)
    QccFree(vector);
}


int QccVectorZero(QccVector vector, int vector_dimension)
{
  int component;

  if ((vector == NULL) || (vector_dimension <= 0))
    return(0);
  for (component = 0; component < vector_dimension; component++)
    vector[component] = 0;

  return(0);
}


QccVector QccVectorResize(QccVector vector,
                          int vector_dimension,
                          int new_vector_dimension)
{
  int index;
  QccVector new_vector = NULL;

  if ((vector_dimension < 0) || (new_vector_dimension < 0))
    return(vector);

  if (new_vector_dimension == vector_dimension)
    return(vector);

  if (vector == NULL)
    {
      vector_dimension = 0;
      if ((new_vector = QccVectorAlloc(new_vector_dimension)) == NULL)
        {
          QccErrorAddMessage("(QccVectorResize): Error calling QccVectorAlloc()");
          return(NULL);
        }
    }
  else
    {
      if ((new_vector = 
           (QccVector)realloc((void *)vector,
                              sizeof(double) * new_vector_dimension)) == NULL)
        {
          QccErrorAddMessage("(QccVectorRealloc): Error reallocating memory");
          return(NULL);
        }
    }
  
  for (index = vector_dimension; index < new_vector_dimension; index++)
    new_vector[index] = 0;

  return(new_vector);
}


double QccVectorMean(const QccVector vector, int vector_dimension)
{
  double mean = 0;
  int component;

  if ((vector == NULL) || (vector_dimension <= 0))
    return((double)0.0);

  for (component = 0; component < vector_dimension; component++)
    mean += vector[component];

  mean /= vector_dimension;

  return(mean);
}


double QccVectorVariance(const QccVector vector, int vector_dimension)
{
  double mean;
  double variance = 0;
  int component;

  if ((vector == NULL) || (vector_dimension <= 0))
    return((double)0.0);

  mean = QccVectorMean(vector, vector_dimension);

  for (component = 0; component < vector_dimension; component++)
    variance += (vector[component] - mean) * (vector[component] - mean);

  variance /= vector_dimension;

  return(variance);
}


int QccVectorAdd(QccVector vector1, const QccVector vector2,
                 int vector_dimension)
{
  int component;

  if ((vector1 == NULL) || (vector2 == NULL) || (vector_dimension <= 0))
    return(0);
  for (component = 0; component < vector_dimension; component++)
    vector1[component] += vector2[component];

  return(0);
}


int QccVectorSubtract(QccVector vector1, const QccVector vector2,
                      int vector_dimension)
{
  int component;

  if ((vector1 == NULL) || (vector2 == NULL) || (vector_dimension <= 0))
    return(0);
  for (component = 0; component < vector_dimension; component++)
    vector1[component] -= vector2[component];
  return(0);
}


int QccVectorScalarMult(QccVector vector, double s, int vector_dimension)
{
  int component;

  if ((vector == NULL) || (vector_dimension <= 0))
    return(0);
  for (component = 0; component < vector_dimension; component++)
    vector[component] *= s;
  return(0);
}


int QccVectorCopy(QccVector vector1, const QccVector vector2,
                  int vector_dimension)
{
  int component;

  if ((vector1 == NULL) || (vector2 == NULL) || (vector_dimension <= 0))
    return(0);

  for (component = 0; component < vector_dimension; component++)
    vector1[component] = vector2[component];

  return(0);
}


double QccVectorNorm(const QccVector vector, int vector_dimension)
{
  int component;
  double squared_norm = 0.0;

  if ((vector == NULL) || (vector_dimension <= 0))
    return(squared_norm);

  for (component = 0; component < vector_dimension; component++)
    squared_norm += vector[component]*vector[component];

  return(sqrt(squared_norm));
}


void QccVectorNormalize(QccVector vector, int vector_dimension)
{
  double norm;

  if ((vector == NULL) || (vector_dimension <= 0))
    return;

  norm = QccVectorNorm(vector, vector_dimension);

  if (norm == 0.0)
    return;

  QccVectorScalarMult(vector, 1/norm, vector_dimension);
}


double QccVectorDotProduct(const QccVector vector1, const QccVector vector2,
                           int vector_dimension)
{
  int component;
  double dot_product = 0.0;

  if ((vector1 == NULL) ||
      (vector2 == NULL) ||
      (vector_dimension <= 0))
    return(0.0);

  for (component = 0; component < vector_dimension; component++)
    dot_product += vector1[component] * vector2[component];

  return(dot_product);
}


double QccVectorAngle(const QccVector vector1,
                      const QccVector vector2,
                      int vector_dimension,
                      int degrees)
{
  double angle = 0.0;
  double dot_product;
  double norm1;
  double norm2;

  if ((vector1 == NULL) ||
      (vector2 == NULL) ||
      (vector_dimension <= 0))
    return(0.0);

  norm1 = QccVectorNorm(vector1, vector_dimension);
  norm2 = QccVectorNorm(vector2, vector_dimension);

  if ((norm1 == 0.0) || (norm2 == 0.0))
    return(0.0);

  dot_product = QccVectorDotProduct(vector1,
                                    vector2,
                                    vector_dimension);

  angle = acos(dot_product / norm1 / norm2);

  if (degrees)
    angle *= (180 / M_PI);

  return(angle);
}


double QccVectorSquareDistance(const QccVector vector1,
                               const QccVector vector2, 
                               int vector_dimension)
{
  double diff, distance = 0.0;
  int component;

  if ((vector1 == NULL) || (vector2 == NULL) || (vector_dimension <= 0))
    return((double)0.0);
  for (component = 0; component < vector_dimension; component++)
    {
      diff = 
        vector1[component] - vector2[component];
      distance += diff*diff;
    }
  return(distance);
}


double QccVectorSumComponents(const QccVector vector, int vector_dimension)
{
  int component;
  double sum = 0.0;

  if (vector == NULL)
    return((double)0.0);

  for (component = 0; component < vector_dimension; component++)
    sum += vector[component];

  return(sum);
}


double QccVectorMaxValue(const QccVector vector, int vector_dimension,
                         int *winner)
{
  int component;
  double max_value = -MAXDOUBLE;
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


double QccVectorMinValue(const QccVector vector, int vector_dimension,
                         int *winner)
{
  int component;
  double min_value = MAXDOUBLE;
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




int QccVectorPrint(const QccVector vector,
                   int vector_dimension)
{
  int component;

  if (vector == NULL)
    return(0);

  printf("< ");

  for (component = 0; component < vector_dimension; component++)
    printf("%g ", vector[component]);

  printf(">\n");

  return(0);
}


/*  Based on QS2I1D, part of the SLATEC library  */

#define QccQuickSortSwap(a,b) {double temp;temp=(a);(a)=(b);(b)=temp;}

static int QccVectorQuickSortAscending(QccVector A, int low, int high)
{
  int low_pnt, high_pnt, pivot_position;
  double pivot;

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
    QccVectorQuickSortAscending(A, high_pnt + 1, high);
  if (low < high_pnt)
    QccVectorQuickSortAscending(A, low, high_pnt);

  return(0);
}


static int QccVectorQuickSortAscendingWithAux(QccVector A, int low, 
                                              int high, int *auxiliary_list)
{
  int low_pnt, high_pnt, pivot_position;
  double pivot;

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
          QccQuickSortSwap(auxiliary_list[pivot_position], auxiliary_list[low]);
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
    QccVectorQuickSortAscendingWithAux(A, high_pnt + 1, high, auxiliary_list);

  if (low < high_pnt)
    QccVectorQuickSortAscendingWithAux(A, low, high_pnt, auxiliary_list);

  return(0);
}


int QccVectorSortComponents(const QccVector vector, QccVector sorted_vector,
                            int vector_dimension, int sort_direction,
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
      if (QccVectorQuickSortAscending(sorted_vector, 0, vector_dimension - 1))
        {
          QccErrorAddMessage("(QccVectorSortComponents): Error calling QccVectorQuickSortAscending()");
          return(1);
        }
    }
  else
    if (QccVectorQuickSortAscendingWithAux(sorted_vector, 0, 
                                           vector_dimension - 1,
                                           auxiliary_list))
      {
        QccErrorAddMessage("(QccVectorSortComponents): Error calling QccVectorQuickSortAscendingWithIndex()");
        return(1);
      }

  if (sort_direction == QCCVECTOR_SORTDESCENDING)
    for (component = 0; component < vector_dimension; component++)
      sorted_vector[component] = -sorted_vector[component];

  return(0);
}


int QccVectorGetSymbolProbs(const int *symbol_list, int symbol_list_length,
                            QccVector probs, 
                            int num_symbols)
{
  int symbol;

  for (symbol = 0; symbol < num_symbols; symbol++)
    probs[symbol] = (double)0.0;

  for (symbol = 0; symbol < symbol_list_length; symbol++)
    if (symbol_list[symbol] >= num_symbols)
      {
        QccErrorAddMessage("(QccVectorGetSymbolProbs): Symbol value out of range");
        return(1);
      }
    else
      probs[symbol_list[symbol]] += 1.0;

  for (symbol = 0; symbol < num_symbols; symbol++)
    probs[symbol] /= (double)symbol_list_length;

  return(0);
}


int QccVectorMoveComponentToFront(QccVector vector, int vector_dimension,
                                  int index)
{
  int component;
  double tmp;

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


int QccVectorSubsample(const QccVector input_signal,
                       int input_length,
                       QccVector output_signal,
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
      QccErrorAddMessage("(QccVectorSubsample): Undefined sampling (%d)",
                         sampling_flag);
      return(1);
    }

  return(0);
}


int QccVectorUpsample(const QccVector input_signal,
                      int input_length,
                      QccVector output_signal,
                      int output_length,
                      int sampling_flag)
{
  int index;

  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);

  QccVectorZero(output_signal, output_length);

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
      QccErrorAddMessage("(QccVectorUpsample): Undefined sampling (%d)",
                         sampling_flag);
      return(1);
    }

  return(0);
}


int QccVectorDCT(const QccVector input_signal,
                 QccVector output_signal,
                 int signal_length)
{
  double C1, C2, C3;
  int n, k;

  if ((input_signal == NULL) ||
      (output_signal == NULL))
    return(0);
  if (signal_length <= 0)
    return(0);

  C1 = 1.0/sqrt((double)signal_length);
  C2 = sqrt(2.0/signal_length);
  C3 = (double)M_PI/2/signal_length;

  output_signal[0] = 0.0;
  for (n = 0; n < signal_length; n++)
    output_signal[0] += input_signal[n];
  output_signal[0] *= C1;

  for (k = 1; k < signal_length; k++)
    {
      output_signal[k] = 0.0;
      for (n = 0; n < signal_length; n++)
        output_signal[k] +=
          input_signal[n] * cos(C3 * (2*n + 1) * k);
      output_signal[k] *= C2;
    }

  return(0);
}


int QccVectorInverseDCT(const QccVector input_signal,
                        QccVector output_signal,
                        int signal_length)
{
  double C1, C2, C3;
  int n, k;

  if ((input_signal == NULL) ||
      (output_signal == NULL))
    return(0);
  if (signal_length <= 0)
    return(0);

  C1 = 1.0/sqrt((double)signal_length);
  C2 = sqrt(2.0/signal_length);
  C3 = (double)M_PI/2/signal_length;

  for (n = 0; n < signal_length; n++)
    {
      output_signal[n] = C1 * input_signal[0];
      for (k = 1; k < signal_length; k++)
        output_signal[n] +=
          C2 * input_signal[k] * cos(C3 * (2*n + 1) * k);
    }

  return(0);
}
