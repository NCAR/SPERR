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


int QccFilterInitialize(QccFilter *filter)
{
  if (filter == NULL)
    return(0);
  
  filter->causality = QCCFILTER_CAUSAL;
  filter->length = 0;
  filter->coefficients = NULL;

  return(0);
}


int QccFilterAlloc(QccFilter *filter)
{
  if (filter == NULL)
    return(0);
  
  if ((filter->coefficients == NULL) &&
      (filter->length > 0))
    if ((filter->coefficients = QccVectorAlloc(filter->length)) == NULL)
      {
        QccErrorAddMessage("(QccFilterAlloc): Error calling QccVectorAlloc()");
        return(1);
      }
  
  return(0);
}


void QccFilterFree(QccFilter *filter)
{
  if (filter == NULL)
    return;
  
  QccVectorFree(filter->coefficients);
  filter->coefficients = NULL;
}


int QccFilterCopy(QccFilter *filter1,
                  const QccFilter *filter2)
{
  int index;
  
  if (filter1 == NULL)
    return(0);
  if (filter2 == NULL)
    return(0);
  if (filter2->coefficients == NULL)
    return(0);
  
  if (filter1->coefficients == NULL)
    {
      filter1->length = filter2->length;
      if (QccFilterAlloc(filter1))
        {
          QccErrorAddMessage("(QccFilterCopy): Error calling QccFilterAlloc()");
          return(1);
        }
    }
  else
    if (filter1->length != filter2->length)
      {
        QccErrorAddMessage("(QccFilterCopy): Filters have different lengths");
        return(1);
      }
  
  filter1->causality = filter2->causality;
  
  for (index = 0; index < filter1->length; index++)
    filter1->coefficients[index] = filter2->coefficients[index];
  
  return(0);
}


int QccFilterReversal(const QccFilter *filter1,
                      QccFilter *filter2)
{
  int index;
  
  if (filter1 == NULL)
    return(0);
  if (filter2 == NULL)
    return(0);
  if (filter1->coefficients == NULL)
    return(0);
  if (filter2->coefficients == NULL)
    return(0);
  
  if (filter1->length != filter2->length)
    {
      QccErrorAddMessage("(QccFilterReversal): Filters have different lengths");
      return(1);
    }
  
  switch (filter1->causality)
    {
    case QCCFILTER_CAUSAL:
      filter2->causality = QCCFILTER_ANTICAUSAL;
      for (index = 0; index < filter2->length; index++)
        filter2->coefficients[index] =
          filter1->coefficients[filter1->length - 1 - index];
      break;
    case QCCFILTER_ANTICAUSAL:
      filter2->causality = QCCFILTER_CAUSAL;
      for (index = 0; index < filter2->length; index++)
        filter2->coefficients[index] =
          filter1->coefficients[filter1->length - 1 - index];
      break;
    case QCCFILTER_SYMMETRICWHOLE:
      filter2->causality = QCCFILTER_SYMMETRICWHOLE;
      for (index = 0; index < filter2->length; index++)
        filter2->coefficients[index] =
          filter1->coefficients[index];
      break;
    case QCCFILTER_SYMMETRICHALF:
      filter2->causality = QCCFILTER_SYMMETRICHALF;
      for (index = 0; index < filter2->length; index++)
        filter2->coefficients[index] =
          filter1->coefficients[index];
      break;
    default:
      QccErrorAddMessage("(QccFilterReversal): Undefined filter causality");
      return(1);
    }
  
  return(0);
}


int QccFilterAlternateSignFlip(QccFilter *filter)
{
  int index;
  
  if (filter == NULL)
    return(0);
  if (filter->coefficients == NULL)
    return(0);
  
  for (index = 0; index < filter->length; index += 2)
    filter->coefficients[index] *= -1;
  
  return(0);
}


int QccFilterRead(FILE *infile,
                  QccFilter *filter)
{
  int index;
  
  if (infile == NULL)
    return(0);
  if (filter == NULL)
    return(0);
  
  fscanf(infile, "%d", &(filter->causality));
  if (ferror(infile) || feof(infile))
    goto Error;
  
  if (QccFileSkipWhiteSpace(infile, 0))
    goto Error;
  
  fscanf(infile, "%d", &(filter->length));
  if (ferror(infile) || feof(infile))
    goto Error;
  
  if (QccFileSkipWhiteSpace(infile, 0))
    goto Error;
  
  if (QccFilterAlloc(filter))
    {
      QccErrorAddMessage("(QccFilterRead): Error calling QccFilterAlloc()");
      return(1);
    }
  
  for (index = 0; index < filter->length; index++)
    {
      fscanf(infile, "%lf",
             &(filter->coefficients[index]));
      if (ferror(infile) || feof(infile))
        {
          QccFilterFree(filter);
          goto Error;
        }
    }
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccFilterRead): Error reading filter");
  return(1);
  
}


int QccFilterWrite(FILE *outfile,
                   const QccFilter *filter)
{
  int index;
  
  if (outfile == NULL)
    return(0);
  if (filter == NULL)
    return(0);
  
  fprintf(outfile, "%d\n%d\n",
          filter->causality,
          filter->length);
  
  for (index = 0; index < filter->length; index++)
    fprintf(outfile, "% 16.9e\n",
            filter->coefficients[index]);
  
  if (ferror(outfile))
    {
      QccErrorAddMessage("(QccFilterWrite): Error writing filter coefficients");
      return(1);
    }
  
  return(0);
}


int QccFilterPrint(const QccFilter *filter)
{
  if (filter == NULL)
    return(0);
  
  printf("  Causality: ");
  switch (filter->causality)
    {
    case QCCFILTER_CAUSAL:
      printf(" causal\n");
      break;
    case QCCFILTER_ANTICAUSAL:
      printf(" anti-causal\n");
      break;
    case QCCFILTER_SYMMETRICWHOLE:
      printf(" whole-sample symmetric\n");
      break;
    case QCCFILTER_SYMMETRICHALF:
      printf(" half-sample symmetric\n");
      break;
    }
  
  printf("  Coefficients: \n    ");
  QccVectorPrint(filter->coefficients, filter->length);
  
  return(0);
}


static int QccFilterVectorPeriodicExtension(const QccVector input_signal,
                                            QccVector output_signal,
                                            int length,
                                            const QccFilter *filter)
{
  int input_index1;
  int input_index2;
  int output_index;
  int filter_index;


  switch (filter->causality)
    {
    case QCCFILTER_CAUSAL:
      for (output_index = 0; output_index < length; output_index++)
        {
          output_signal[output_index] = 0;
          for (filter_index = 0; filter_index < filter->length; filter_index++)
            {
              input_index1 = 
                QccMathModulus(output_index - filter_index,
                               length);
              
              output_signal[output_index] +=
                input_signal[input_index1] * 
                filter->coefficients[filter_index];
              
            }
        }
      break;
      
    case QCCFILTER_ANTICAUSAL:
      for (output_index = 0; output_index < length; output_index++)
        {
          output_signal[output_index] = 0;
          for (filter_index = 0; filter_index < filter->length; filter_index++)
            {
              input_index1 = 
                QccMathModulus(output_index + filter_index,
                               length);
              
              output_signal[output_index] +=
                input_signal[input_index1] * 
                filter->coefficients[filter->length - 1 - filter_index];
            }
        }
      break;
      
    case QCCFILTER_SYMMETRICWHOLE:
      for (output_index = 0; output_index < length; output_index++)
        {
          output_signal[output_index] = 
            input_signal[output_index] * filter->coefficients[0];
          for (filter_index = 1; filter_index < filter->length; filter_index++)
            {
              input_index1 = 
                QccMathModulus(output_index - filter_index,
                               length);
              input_index2 = 
                QccMathModulus(output_index + filter_index,
                               length);
              
              output_signal[output_index] +=
                (input_signal[input_index1] + input_signal[input_index2]) * 
                filter->coefficients[filter_index];
            }
        }
      break;
      
    case QCCFILTER_SYMMETRICHALF:
      for (output_index = 0; output_index < length; output_index++)
        for (filter_index = 0; filter_index < filter->length; filter_index++)
          {
            input_index1 = 
              QccMathModulus(output_index - filter_index,
                             length);
            input_index2 = 
              QccMathModulus(output_index + filter_index + 1,
                             length);
            
            output_signal[output_index] +=
              (input_signal[input_index1] + input_signal[input_index2]) * 
              filter->coefficients[filter_index];
          }
      break;
      
    default:
      QccErrorAddMessage("(QccFilterVectorPeriodicExtension): Undefined filter causality (%d)",
                         filter->causality);
      return(1);
    }
  
  return(0);
}


int QccFilterCalcSymmetricExtension(int index, int length, int symmetry)
{
  if (symmetry == QCCFILTER_SYMMETRICWHOLE)
    {
      if (length > 2)
        {
          if ((index < 0) || (index >= (2*length - 2)))
            index = QccMathModulus(index, 2*length - 2);
          if (index >= length)
            index = 2*length - index - 2;
        }
      else
        if ((index < 0) || (index >= length))
          index = QccMathModulus(index, length);
    }
  else
    {
      if (length > 1)
        {
          if ((index < 0) || (index >= 2*length))
            index = QccMathModulus(index, 2*length);
          if (index >= length)
            index = 2*length - index - 1;
        }
      else
        index = 0;
    }
  return(index);
}


static int QccFilterVectorSymmetricExtension(const QccVector input_signal,
                                             QccVector output_signal,
                                             int length,
                                             const QccFilter *filter)
{
  int input_index1;
  int input_index2;
  int output_index;
  int filter_index;

  switch (filter->causality)
    {
    case QCCFILTER_CAUSAL:
      for (output_index = 0; output_index < length; output_index++)
        {
          output_signal[output_index] = 0;
          for (filter_index = 0; filter_index < filter->length; filter_index++)
            {
              input_index1 = 
                QccFilterCalcSymmetricExtension(output_index - filter_index,
                                                length,
                                                QCCFILTER_SYMMETRICWHOLE);
              
              output_signal[output_index] +=
                input_signal[input_index1] * 
                filter->coefficients[filter_index];
            }
        }
      break;
      
    case QCCFILTER_ANTICAUSAL:
      for (output_index = 0; output_index < length; output_index++)
        {
          output_signal[output_index] = 0;
          for (filter_index = 0; filter_index < filter->length; filter_index++)
            {
              input_index1 = 
                QccFilterCalcSymmetricExtension(output_index + filter_index,
                                                length,
                                                QCCFILTER_SYMMETRICWHOLE);
              
              output_signal[output_index] +=
                input_signal[input_index1] * 
                filter->coefficients[filter->length - 1 - filter_index];
            }
        }
      break;
      
    case QCCFILTER_SYMMETRICWHOLE:
      for (output_index = 0; output_index < length; output_index++)
        {
          output_signal[output_index] = 
            input_signal[output_index] * filter->coefficients[0];
          for (filter_index = 1; filter_index < filter->length; filter_index++)
            {
              input_index1 = 
                QccFilterCalcSymmetricExtension(output_index - filter_index,
                                                length,
                                                QCCFILTER_SYMMETRICWHOLE);
              input_index2 = 
                QccFilterCalcSymmetricExtension(output_index + filter_index,
                                                length,
                                                QCCFILTER_SYMMETRICWHOLE);
              
              output_signal[output_index] +=
                (input_signal[input_index1] + input_signal[input_index2]) * 
                filter->coefficients[filter_index];
            }
        }
      break;
      
    case QCCFILTER_SYMMETRICHALF:
      for (output_index = 0; output_index < length; output_index++)
        for (filter_index = 0; filter_index < filter->length; filter_index++)
          {
            input_index1 = 
              QccFilterCalcSymmetricExtension(output_index - filter_index,
                                              length,
                                              QCCFILTER_SYMMETRICHALF);
            input_index2 = 
              QccFilterCalcSymmetricExtension(output_index + filter_index + 1,
                                              length,
                                              QCCFILTER_SYMMETRICHALF);
            
            output_signal[output_index] +=
              (input_signal[input_index1] + input_signal[input_index2]) * 
              filter->coefficients[filter_index];
        }
      break;
      
    default:
      QccErrorAddMessage("(QccFilterVectorSymmetricExtension): Undefined filter causality (%d)",
                         filter->causality);
      return(1);
    }
  
  return(0);
}


int QccFilterVector(const QccVector input_signal,
                    QccVector output_signal,
                    int length,
                    const QccFilter *filter,
                    int boundary_extension)
{
  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (filter == NULL)
    return(0);
  if (!(filter->length) || (filter->coefficients == NULL))
    return(0);
  
  switch (boundary_extension)
    {
    case QCCFILTER_SYMMETRIC_EXTENSION:
      if (QccFilterVectorSymmetricExtension(input_signal,
                                            output_signal,
                                            length,
                                            filter))
        {
          QccErrorAddMessage("(QccFilterVector): Error calling QccFilterVectorPeriodicExtension()");
          return(1);
        }
      break;
      
    case QCCFILTER_PERIODIC_EXTENSION:
      if (QccFilterVectorPeriodicExtension(input_signal,
                                           output_signal,
                                           length,
                                           filter))
        {
          QccErrorAddMessage("(QccFilterVector): Error calling QccFilterVectorSymmetricExtension()");
          return(1);
        }
      break;
      
    default:
      QccErrorAddMessage("(QccFilterVector): Undefined boundary extension (%d)",
                         boundary_extension);
      return(1);
    }
  
  return(0);
}


int QccFilterMultiRateFilterVector(const QccVector input_signal,
                                   int input_length,
                                   QccVector output_signal,
                                   int output_length,
                                   const QccFilter *filter,
                                   int input_sampling,
                                   int output_sampling,
                                   int boundary_extension)
{
  QccVector input_signal2 = NULL;
  QccVector output_signal2 = NULL;
  int input_length2;
  int output_length2;
  int input_allocated = 0;
  int output_allocated = 0;
  int return_value;
  
  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (filter == NULL)
    return(0);
  if (!(filter->length) || (filter->coefficients == NULL))
    return(0);

  switch (input_sampling)
    {
    case QCCFILTER_SAMESAMPLING:
      input_length2 = input_length;
      input_signal2 = input_signal;
      input_allocated = 0;
      break;
      
    case QCCFILTER_SUBSAMPLEEVEN:
      input_length2 = (int)ceil((double)input_length / 2);
      if ((input_signal2 = QccVectorAlloc(input_length2)) == NULL)
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorAlloc()");
          goto Error;
        }
      input_allocated = 1;
      if (QccVectorSubsample(input_signal,
                             input_length,
                             input_signal2,
                             input_length2,
                             QCCVECTOR_EVEN))
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorSubsample()");
          goto Error;
        }
      break;

    case QCCFILTER_SUBSAMPLEODD:
      input_length2 = (int)floor((double)input_length / 2);
      if ((input_signal2 = QccVectorAlloc(input_length2)) == NULL)
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorAlloc()");
          goto Error;
        }
      input_allocated = 1;
      if (QccVectorSubsample(input_signal,
                             input_length,
                             input_signal2,
                             input_length2,
                             QCCVECTOR_ODD))
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorSubsample()");
          goto Error;
        }
      break;
      
    case QCCFILTER_UPSAMPLEEVEN:
      input_length2 = input_length * 2;
      if ((input_signal2 = QccVectorAlloc(input_length2)) == NULL)
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorAlloc()");
          goto Error;
        }
      input_allocated = 1;
      if (QccVectorUpsample(input_signal,
                            input_length,
                            input_signal2,
                            input_length2,
                            ((input_sampling == QCCFILTER_UPSAMPLEEVEN) ?
                             QCCVECTOR_EVEN : QCCVECTOR_ODD)))
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorUpsample()");
          goto Error;
        }
      break;
      
    case QCCFILTER_UPSAMPLEODD:
      input_length2 = input_length * 2 + 1;
      if ((input_signal2 = QccVectorAlloc(input_length2)) == NULL)
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorAlloc()");
          goto Error;
        }
      input_allocated = 1;
      if (QccVectorUpsample(input_signal,
                            input_length,
                            input_signal2,
                            input_length2,
                            ((input_sampling == QCCFILTER_UPSAMPLEEVEN) ?
                             QCCVECTOR_EVEN : QCCVECTOR_ODD)))
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorUpsample()");
          goto Error;
        }
      break;
      
    default:
      QccErrorAddMessage("(QccFilterMultiRateFilterVector): Undefined input sampling (%d)",
                         input_sampling);
      goto Error;
    }
  
  switch (output_sampling)
    {
    case QCCFILTER_SAMESAMPLING:
      output_length2 = QccMathMin(input_length2, output_length);
      output_signal2 = output_signal;
      output_allocated = 0;
      break;
    case QCCFILTER_SUBSAMPLEEVEN:
    case QCCFILTER_SUBSAMPLEODD:
      output_length2 = input_length2;
      if ((output_signal2 = QccVectorAlloc(output_length2)) == NULL)
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorAlloc()");
          goto Error;
        }
      output_allocated = 1;
      break;
    case QCCFILTER_UPSAMPLEEVEN:
    case QCCFILTER_UPSAMPLEODD:
      output_length2 = input_length2;
      if ((output_signal2 = QccVectorAlloc(output_length2)) == NULL)
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorAlloc()");
          goto Error;
        }
      output_allocated = 1;
      break;
    default:
      QccErrorAddMessage("(QccFilterMultiRateFilterVector): Undefined output sampling (%d)",
                         output_sampling);
      goto Error;
    }
  
  if (QccFilterVector(input_signal2,
                      output_signal2,
                      output_length2,
                      filter,
                      boundary_extension))
    {
      QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccFilterVector()");
      goto Error;
    }
  
  switch (output_sampling)
    {
    case QCCFILTER_SUBSAMPLEEVEN:
    case QCCFILTER_SUBSAMPLEODD:
      if (QccVectorSubsample(output_signal2,
                             output_length2,
                             output_signal,
                             output_length,
                             ((output_sampling == QCCFILTER_SUBSAMPLEEVEN) ?
                              QCCVECTOR_EVEN : QCCVECTOR_ODD)))
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorSubsample()");
          goto Error;
        }
      break;
    case QCCFILTER_UPSAMPLEEVEN:
    case QCCFILTER_UPSAMPLEODD:
      if (QccVectorUpsample(output_signal2,
                            output_length2,
                            output_signal,
                            output_length,
                            ((output_sampling == QCCFILTER_UPSAMPLEEVEN) ?
                             QCCVECTOR_EVEN : QCCVECTOR_ODD)))
        {
          QccErrorAddMessage("(QccFilterMultiRateFilterVector): Error calling QccVectorUpsample()");
          goto Error;
        }
      break;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (input_allocated)
    QccVectorFree(input_signal2);
  if (output_allocated)
    QccVectorFree(output_signal2);
  return(return_value);
}


int QccFilterMatrixSeparable(const QccMatrix input_matrix,
                             QccMatrix output_matrix,
                             int num_rows,
                             int num_cols,
                             const QccFilter *horizontal_filter,
                             const QccFilter *vertical_filter,
                             int boundary_extension)
{
  int return_value;
  QccVector input_column = NULL;
  QccVector output_column = NULL;
  int row, col;

  if (input_matrix == NULL)
    return(0);
  if (output_matrix == NULL)
    return(0);
  if (horizontal_filter == NULL)
    return(0);
  if (vertical_filter == NULL)
    return(0);

  if ((input_column = QccVectorAlloc(num_rows)) == NULL)
    {

        QccErrorAddMessage("(QccFilterMatrixSeparable): Error calling QccVectorAlloc()");
        goto Error;
    }
  if ((output_column = QccVectorAlloc(num_rows)) == NULL)
    {
        QccErrorAddMessage("(QccFilterMatrixSeparable): Error calling QccVectorAlloc()");
        goto Error;
    }

  for (row = 0; row < num_rows; row++)
    if (QccFilterVector(input_matrix[row],
                        output_matrix[row],
                        num_cols,
                        horizontal_filter,
                        boundary_extension))
      {
        QccErrorAddMessage("(QccFilterMatrixSeparable): Error calling QccFilterVector()");
        goto Error;
      }

  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        input_column[row] = output_matrix[row][col];

      if (QccFilterVector(input_column,
                          output_column,
                          num_rows,
                          vertical_filter,
                          boundary_extension))
        {
          QccErrorAddMessage("(QccFilterMatrixSeparable): Error calling QccFilterVector()");
          goto Error;
        }
      
      for (row = 0; row < num_rows; row++)
        output_matrix[row][col] = output_column[row];
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(input_column);
  QccVectorFree(output_column);
  return(return_value);
}
