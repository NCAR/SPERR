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


int QccWAVVectorFilterInitialize(QccWAVVectorFilter *vector_filter)
{
  if (vector_filter == NULL)
    return(0);
  
  vector_filter->dimension = 0;
  vector_filter->causality = QCCWAVVECTORFILTER_CAUSAL;
  vector_filter->length = 0;
  vector_filter->coefficients = NULL;
  
  return(0);
}


int QccWAVVectorFilterAlloc(QccWAVVectorFilter *vector_filter)
{
  int coefficient;
  
  if (vector_filter == NULL)
    return(0);
  
  if ((vector_filter->coefficients != NULL) ||
      (vector_filter->length <= 0) ||
      (vector_filter->dimension <= 0))
    return(0);
  
  if ((vector_filter->coefficients =
       (QccMatrix *)malloc(sizeof(QccMatrix) * vector_filter->length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVVectorFilterAlloc): Error allocating memory");
      return(1);
    }
  
  for (coefficient = 0; coefficient < vector_filter->length; coefficient++)
    vector_filter->coefficients[coefficient] = NULL;
  
  for (coefficient = 0; coefficient < vector_filter->length; coefficient++)
    if ((vector_filter->coefficients[coefficient] =
         QccMatrixAlloc(vector_filter->dimension,
                        vector_filter->dimension)) == NULL)
      {
        QccErrorAddMessage("(QccWAVVectorFilterAlloc): Error calling QccMatrixAlloc()");
        QccWAVVectorFilterFree(vector_filter);
        return(1);
      }
  
  return(0);
}


void QccWAVVectorFilterFree(QccWAVVectorFilter *vector_filter)
{
  int coefficient;
  
  if (vector_filter == NULL)
    return;
  
  if (vector_filter->coefficients != NULL)
    {
      for (coefficient = 0; coefficient < vector_filter->length; coefficient++)
        QccMatrixFree(vector_filter->coefficients[coefficient],
                      vector_filter->dimension);
      QccFree(vector_filter->coefficients);

      vector_filter->coefficients = NULL;
    }
  
}


int QccWAVVectorFilterCopy(QccWAVVectorFilter *vector_filter1,
                           const QccWAVVectorFilter *vector_filter2,
                           int transpose)
{
  int coefficient;
  
  if (vector_filter1 == NULL)
    return(0);
  if (vector_filter2 == NULL)
    return(0);
  if ((vector_filter2->coefficients == NULL) ||
      (vector_filter2->length <= 0))
    return(0);
  
  if (vector_filter1->coefficients == NULL)
    {
      vector_filter1->length = vector_filter2->length;
      vector_filter1->dimension = vector_filter2->dimension;
      if (QccWAVVectorFilterAlloc(vector_filter1))
        {
          QccErrorAddMessage("(QccWAVVectorFilterCopy): Error calling QccWAVVectorFilterAlloc()");
          return(1);
        }
    }
  else
    {
      if (vector_filter1->length != vector_filter2->length)
        {
          QccErrorAddMessage("(QccWAVVectorFilterCopy): Filters have different lengths");
          return(1);
        }
      if (vector_filter1->dimension != vector_filter2->dimension)
        {
          QccErrorAddMessage("(QccWAVVectorFilterCopy): Filters have different dimensions");
          return(1);
        }
    }
  
  vector_filter1->causality = vector_filter2->causality;
  
  for (coefficient = 0; coefficient < vector_filter1->length; coefficient++)
    if (transpose)
      {
        if (QccMatrixTranspose(vector_filter2->coefficients[coefficient],
                               vector_filter1->coefficients[coefficient],
                               vector_filter1->dimension,
                               vector_filter1->dimension))
          {
            QccErrorAddMessage("(QccWAVVectorFilterCopy): Error calling QccMatrixTranspose()");
            return(1);
          }
      }
    else
      if (QccMatrixCopy(vector_filter1->coefficients[coefficient],
                        vector_filter2->coefficients[coefficient],
                        vector_filter1->dimension,
                        vector_filter1->dimension))
        {
          QccErrorAddMessage("(QccWAVVectorFilterCopy): Error calling QccMatrixCopy()");
          return(1);
        }
  
  return(0);
}


int QccWAVVectorFilterReversal(const QccWAVVectorFilter *vector_filter1,
                               QccWAVVectorFilter *vector_filter2)
{
  int coefficient;
  
  if (vector_filter1 == NULL)
    return(0);
  if (vector_filter2 == NULL)
    return(0);
  if (vector_filter1->coefficients == NULL)
    return(0);
  if (vector_filter2->coefficients == NULL)
    return(0);
  
  if (vector_filter1->length != vector_filter2->length)
    {
      QccErrorAddMessage("(QccWAVVectorFilterReversal): Filters have different lengths");
      return(1);
    }
  
  if (vector_filter1->dimension != vector_filter2->dimension)
    {
      QccErrorAddMessage("(QccWAVVectorFilterReversal): Filters have different dimensions");
      return(1);
    }
  
  switch (vector_filter1->causality)
    {
    case QCCWAVVECTORFILTER_CAUSAL:
      vector_filter2->causality = QCCWAVVECTORFILTER_ANTICAUSAL;
      for (coefficient = 0; coefficient < vector_filter2->length;
           coefficient++)
        if (QccMatrixCopy(vector_filter2->coefficients[coefficient],
                          vector_filter1->coefficients[vector_filter1->length -
                                                      1 - coefficient],
                          vector_filter1->dimension,
                          vector_filter1->dimension))
          {
            QccErrorAddMessage("(QccWAVVectorFilterReversal): Error calling QccMatrixCopy()");
            return(1);
          }
      break;

    case QCCWAVVECTORFILTER_ANTICAUSAL:
      vector_filter2->causality = QCCWAVVECTORFILTER_CAUSAL;
      for (coefficient = 0; coefficient < vector_filter2->length;
           coefficient++)
        if (QccMatrixCopy(vector_filter2->coefficients[coefficient],
                          vector_filter1->coefficients
                          [vector_filter1->length - 1 - coefficient],
                          vector_filter1->dimension,
                          vector_filter1->dimension))
          {
            QccErrorAddMessage("(QccWAVVectorFilterReversal): Error calling QccMatrixCopy()");
            return(1);
          }
      break;
      
    default:
      QccErrorAddMessage("(QccWAVVectorFilterReversal): Undefined vector_filter causality");
      return(1);
    }
  
  return(0);
}


int QccWAVVectorFilterRead(FILE *infile,
                           QccWAVVectorFilter *vector_filter)
{
  int coefficient;
  int row, col;
  
  if (infile == NULL)
    return(0);
  if (vector_filter == NULL)
    return(0);
  
  fscanf(infile, "%d", &(vector_filter->dimension));
  if (ferror(infile) || feof(infile))
    goto Error;
  
  if (QccFileSkipWhiteSpace(infile, 0))
    goto Error;
  
  fscanf(infile, "%d", &(vector_filter->length));
  if (ferror(infile) || feof(infile))
    goto Error;
  
  if (QccFileSkipWhiteSpace(infile, 0))
    goto Error;
  
  fscanf(infile, "%d", &(vector_filter->causality));
  if (ferror(infile) || feof(infile))
    goto Error;
  
  if (QccFileSkipWhiteSpace(infile, 0))
    goto Error;
  
  if (QccWAVVectorFilterAlloc(vector_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterRead): Error calling QccWAVVectorFilterAlloc()");
      return(1);
    }
  
  for (coefficient = 0; coefficient < vector_filter->length; coefficient++)
    for (row = 0; row < vector_filter->dimension; row++)
      for (col = 0; col < vector_filter->dimension; col++)
        {
          fscanf(infile, "%lf",
                 &(vector_filter->coefficients[coefficient][row][col]));
          if (ferror(infile) || feof(infile))
            {
              QccWAVVectorFilterFree(vector_filter);
              goto Error;
            }
        }
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccWAVVectorFilterRead): Error reading vector_filter");
  return(1);
  
}


int QccWAVVectorFilterWrite(FILE *outfile,
                            const QccWAVVectorFilter *vector_filter)
{
  int coefficient;
  int row, col;
  
  if (outfile == NULL)
    return(0);
  if (vector_filter == NULL)
    return(0);
  
  fprintf(outfile, "%d\n%d\n%d\n",
          vector_filter->dimension,
          vector_filter->length,
          vector_filter->causality);
  
  for (coefficient = 0; coefficient < vector_filter->length; coefficient++)
    for (row = 0; row < vector_filter->dimension; row++)
      for (col = 0; col < vector_filter->dimension; col++)
        fprintf(outfile, "% 16.9e\n",
                vector_filter->coefficients[coefficient][row][col]);
  
  if (ferror(outfile))
    {
      QccErrorAddMessage("(QccWAVVectorFilterWrite): Error writing vector_filter coefficients");
      return(1);
    }
  
  return(0);
}


int QccWAVVectorFilterPrint(const QccWAVVectorFilter *vector_filter)
{
  int coefficient;
  
  if (vector_filter == NULL)
    return(0);
  
  printf("  Causality: ");
  switch (vector_filter->causality)
    {
    case QCCWAVVECTORFILTER_CAUSAL:
      printf(" causal\n");
      break;
    case QCCWAVVECTORFILTER_ANTICAUSAL:
      printf(" anti-causal\n");
      break;
    }
  
  if (vector_filter->coefficients == NULL)
    return(0);

  printf("  Coefficients: \n    ");
  
  for (coefficient = 0; coefficient < vector_filter->length; coefficient++)
    {
      printf("    Matrix Coefficient %d\n", coefficient);
      QccMatrixPrint(vector_filter->coefficients[coefficient],
                     vector_filter->dimension,
                     vector_filter->dimension);
    }
  
  return(0);
}


