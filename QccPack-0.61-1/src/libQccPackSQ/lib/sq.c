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


static int QccSQInverseScalarQuantizationGeneral(int partition,
                                                 const QccSQScalarQuantizer 
                                                 *quantizer,
                                                 double *value)
{
  if (quantizer->levels == NULL)
    return(0);

  if (partition < 0)
    return(0);

  if (partition < quantizer->num_levels)
    *value = quantizer->levels[partition];
  else
    {
      QccErrorAddMessage("(QccSQInverseScalarQuantizer): Partition value (%d) is beyond number of levels (%d) in scalar quantizer",
                         partition, quantizer->num_levels);
      return(1);
    }

  return(0);
}


static int QccSQScalarQuantizationGeneral(double value,
                                          const
                                          QccSQScalarQuantizer *quantizer,
                                          double *distortion,
                                          int *partition)
{
  int index;

  if (quantizer->boundaries == NULL)
    return(0);
  if (quantizer->num_levels < 1)
    return(0);

  for (index = 0; index < quantizer->num_levels - 1; index++)
    if (value <= quantizer->boundaries[index])
      goto Return;

  index = quantizer->num_levels - 1;

 Return:
  if (partition != NULL)
    *partition = index;
  if ((distortion != NULL) && (quantizer->levels != NULL))
    *distortion = (value - quantizer->levels[index]) *
      (value - quantizer->levels[index]);

  return(0);
}


static int QccSQInverseScalarQuantizationUniform(int partition,
                                                 const QccSQScalarQuantizer
                                                 *quantizer,
                                                 double *value)
{
  *value = 
    partition * quantizer->stepsize;
  
  return(0);
}


static int QccSQScalarQuantizationUniform(double value,
                                          const
                                          QccSQScalarQuantizer *quantizer,
                                          double *distortion,
                                          int *partition)
{
  int index;
  double dist;
  double quantized_value;

  if (quantizer->stepsize)
    index = (int)rint(value / quantizer->stepsize);
  else
    index = 0;

  if (partition != NULL)
    *partition = index;

  if (distortion != NULL)
    {
      QccSQInverseScalarQuantizationUniform(index,
                                            quantizer,
                                            &quantized_value);
      dist = value - quantized_value;
      *distortion = dist * dist;
    }

  return(0);
}


static int QccSQInverseScalarQuantizationDeadZone(int partition,
                                                  const QccSQScalarQuantizer
                                                  *quantizer,
                                                  double *value)
{
  if (!partition)
    *value = 0;
  else
    if (partition < 0)
      *value = 
        -((((-partition) - 1) + 0.5) * quantizer->stepsize +
          quantizer->deadzone/2);
    else
      *value = 
        ((partition - 1) + 0.5) * quantizer->stepsize +
        quantizer->deadzone/2;
  
  return(0);
}


static int QccSQScalarQuantizationDeadZone(double value,
                                           const
                                           QccSQScalarQuantizer *quantizer,
                                           double *distortion,
                                           int *partition)
{
  int index;
  double dist;
  double quantized_value;

  if (fabs(value) <= quantizer->deadzone/2)
    index = 0;
  else
    if (quantizer->stepsize)
      if (value < 0) 
        index = 
          -(int)ceil(((-value) - quantizer->deadzone/2) /
                     quantizer->stepsize);
      else
        index = 
          (int)ceil((value - quantizer->deadzone/2) /
                    quantizer->stepsize);
  
    else
      index = 0;

  if (partition != NULL)
    *partition = index;

  if (distortion != NULL)
    {
      QccSQInverseScalarQuantizationDeadZone(index,
                                             quantizer,
                                             &quantized_value);
      dist = value - quantized_value;
      *distortion = dist * dist;
    }

  return(0);
}


int QccSQScalarQuantization(double value,
                            const QccSQScalarQuantizer *quantizer,
                            double *distortion,
                            int *partition)
{
  if (quantizer == NULL)
    return(0);

  switch (quantizer->type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      QccSQScalarQuantizationGeneral(value,
                                     quantizer,
                                     distortion,
                                     partition);
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:
      QccSQScalarQuantizationUniform(value,
                                     quantizer,
                                     distortion,
                                     partition);
      break;

    case QCCSQSCALARQUANTIZER_DEADZONE:
      QccSQScalarQuantizationDeadZone(value,
                                      quantizer,
                                      distortion,
                                      partition);
      break;
    }

  return(0);
}


int QccSQInverseScalarQuantization(int partition,
                                   const QccSQScalarQuantizer *quantizer,
                                   double *value)
{
  if (quantizer == NULL)
    return(0);
  if (value == NULL)
    return(0);
  
  switch (quantizer->type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      QccSQInverseScalarQuantizationGeneral(partition,
                                            quantizer,
                                            value);
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:
      QccSQInverseScalarQuantizationUniform(partition,
                                            quantizer,
                                            value);
      break;

    case QCCSQSCALARQUANTIZER_DEADZONE:
      QccSQInverseScalarQuantizationDeadZone(partition,
                                             quantizer,
                                             value);
      break;
    }

  return(0);
}


int QccSQScalarQuantizeVector(const QccVector data,
                              int data_length,
                              const QccSQScalarQuantizer *quantizer,
                              QccVector distortion,
                              int *partition)
{
  int index;

  if (data == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);

  if (data_length < 1)
    return(0);

  if (distortion == NULL)
    if (partition == NULL)
      {
        for (index = 0; index < data_length; index++)
          if (QccSQScalarQuantization(data[index],
                                      quantizer,
                                      NULL, 
                                      NULL))
            {
              QccErrorAddMessage("(QccSQScalarQuantizerVector): Error calling QccSQScalarQuantization()");
              return(1);
            }
      }
    else
      {
        for (index = 0; index < data_length; index++)
          {
            if (QccSQScalarQuantization(data[index],
                                        quantizer,
                                        NULL, 
                                        &(partition[index])))
              {
                QccErrorAddMessage("(QccSQScalarQuantizerVector): Error calling QccSQScalarQuantization()");
                return(1);
              }
          }
      }
  else
    if (partition == NULL)
      {
        for (index = 0; index < data_length; index++)
          if (QccSQScalarQuantization(data[index],
                                      quantizer,
                                      &(distortion[index]), 
                                      NULL))
            {
              QccErrorAddMessage("(QccSQScalarQuantizerVector): Error calling QccSQScalarQuantization()");
              return(1);
            }
      }
    else
      {
        for (index = 0; index < data_length; index++)
          if (QccSQScalarQuantization(data[index],
                                      quantizer,
                                      &(distortion[index]), 
                                      &(partition[index])))
            {
              QccErrorAddMessage("(QccSQScalarQuantizerVector): Error calling QccSQScalarQuantization()");
              return(1);
            }
      }

  return(0);
}


int QccSQInverseScalarQuantizeVector(const int *partition,
                                     const QccSQScalarQuantizer *quantizer,
                                     QccVector data,
                                     int data_length)
{
  int index;

  if (partition == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (data == NULL)
    return(0);

  for (index = 0; index < data_length; index++)
    if (QccSQInverseScalarQuantization(partition[index],
                                       quantizer,
                                       &(data[index])))
      {
        QccErrorAddMessage("(QccSQInverseScalarQuantizerVector): Error calling QccSQInverseScalarQuantization()");
        return(1);
      }

  return(0);
}


int QccSQScalarQuantizeDataset(const QccDataset *dataset,
                               const QccSQScalarQuantizer *quantizer,
                               QccVector distortion,
                               int *partition)
{
  int vector;
  int distortion_index;
  int block_size;

  if (dataset == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  
  block_size = QccDatasetGetBlockSize(dataset);

  for (vector = 0, distortion_index = 0; 
       vector < block_size; 
       vector++, distortion_index += dataset->vector_dimension)
    if (QccSQScalarQuantizeVector(dataset->vectors[vector],
                                  dataset->vector_dimension,
                                  quantizer,
                                  (distortion == NULL) ? NULL :
                                  (QccVector)&(distortion[distortion_index]),
                                  (partition == NULL) ? NULL :
                                  &(partition[distortion_index])))
      {
        QccErrorAddMessage("(QccSQScalarQuantizerDataset): Error calling QccSQScalarQuantizeVector()");
        return(1);
      }

  return(0);
}


int QccSQInverseScalarQuantizeDataset(const int *partition,
                                      const QccSQScalarQuantizer *quantizer,
                                      QccDataset *dataset)
{
  int vector;
  int partition_index;
  int block_size;

  if (partition == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (dataset == NULL)
    return(0);

  block_size = QccDatasetGetBlockSize(dataset);

  for (vector = 0, partition_index = 0; 
       vector < block_size; 
       vector++, partition_index += dataset->vector_dimension)
    if (QccSQInverseScalarQuantizeVector(&(partition[partition_index]),
                                         quantizer,
                                         dataset->vectors[vector],
                                         dataset->vector_dimension))
      {
        QccErrorAddMessage("(QccSQInverseScalarQuantizeDataset): Error calling QccSQInverseScalarQuantizeVector()");
        return(1);
      }

  return(0);
}


int QccSQUniformMakeQuantizer(QccSQScalarQuantizer *quantizer,
                              double max_value,
                              double min_value,
                              int overload_region)
{
  int level;
  double stepsize;

  if (quantizer == NULL)
    return(0);

  if (max_value <= min_value)
    {
      QccErrorAddMessage("(QccSQUniformMakeQuantizer): Max value (%f) must be larger than min value (%f)",
                         max_value, min_value);
      return(1);
    }

  quantizer->type = QCCSQSCALARQUANTIZER_GENERAL;

  if (quantizer->num_levels < 1)
    return(0);

  stepsize = (max_value - min_value) / quantizer->num_levels;

  if (overload_region)
    {
      quantizer->num_levels += 2;
      min_value -= stepsize;
      max_value += stepsize;
    }
  if (QccSQScalarQuantizerAlloc(quantizer))
    {
      QccErrorAddMessage("(QccSQUniformMakeQuantizer): Error calling QccSQScalarQuantizerAlloc()");
      return(1);
    }
  for (level = 0; level < quantizer->num_levels - 1; level++)
    {
      quantizer->levels[level] = 
        min_value + stepsize*level + stepsize/2;
      quantizer->boundaries[level] =
        min_value + stepsize*(level + 1);
    }
  quantizer->levels[quantizer->num_levels - 1] =
    max_value - stepsize/2;

  for (level = 0; level < quantizer->num_levels; level++)
    quantizer->probs[level] = 1.0 / quantizer->num_levels;

  return(0);
}


/*
 *  Refer to:
 *    A. Gersho and R. M. Gray, "Vector Quantization and Signal Compression,"
 *      Kluwer Academic Publishers, Boston, 1992, pp. 156-161.
 */
double QccSQULawExpander(double value,
                         double V, double u)
{
  double rv;

  if ((u <= 0.0) || (V <= 0.0))
    return(0.0);

  rv = (V/u)*(exp(fabs(value)*log(1 + u)/V) - 1);

  if (value < 0.0)
    rv *= -1.0;

  return(rv);
}


int QccSQULawMakeQuantizer(QccSQScalarQuantizer *quantizer,
                           double u,
                           double max_value,
                           double min_value)
{
  int level;
  double center_value;
  double V;

  if (quantizer == NULL)
    return(0);

  if (u < 0.0)
    {
      QccErrorAddMessage("(QccSQULawMakeQuantizer): u must be nonnegative");
      return(1);
    }

  if (max_value <= min_value)
    {
      QccErrorAddMessage("(QccSQULawMakeQuantizer): Max value (%f) must be larger than min value (%f)",
                         max_value, min_value);
      return(1);
    }

  quantizer->type = QCCSQSCALARQUANTIZER_GENERAL;

  if (quantizer->num_levels < 1)
    return(0);

  center_value = (max_value + min_value)/2;
  V = fabs(max_value - center_value);

  if (QccSQUniformMakeQuantizer(quantizer,
                                V, -V,
                                QCCSQ_NOOVERLOAD))
    {
      QccErrorAddMessage("(QccMakeULawQuantizer): Error calling QccSQUniformMakeQuantizer()");
      return(1);
    }

  if (u > 0.0)
    {
      for (level = 0; level < quantizer->num_levels - 1; level++)
        {
          quantizer->levels[level] =
            QccSQULawExpander(quantizer->levels[level], V, u) +
            center_value;
          quantizer->boundaries[level] =
            QccSQULawExpander(quantizer->boundaries[level], V, u) +
            center_value;
        }
      
      quantizer->levels[quantizer->num_levels - 1] =
        QccSQULawExpander(quantizer->levels[quantizer->num_levels - 1],
                          V, u) + center_value;
    }
  else
    {
      for (level = 0; level < quantizer->num_levels - 1; level++)
        {
          quantizer->levels[level] +=
            center_value;
          quantizer->boundaries[level] +=
            center_value;
        }
      
      quantizer->levels[quantizer->num_levels - 1] +=
        center_value;
    }

  return(0);
}


/*
 *  Refer to:
 *    A. Gersho and R. M. Gray, "Vector Quantization and Signal Compression,"
 *      Kluwer Academic Publishers, Boston, 1992, pp. 156-161.
 */
double QccSQALawExpander(double value,
                         double V, double A)
{
  double rv;

  if ((A < 1.0) || (V <= 0.0))
    return(0.0);
  
  if (fabs(value) <= V/(1 + log(A)))
    rv = fabs(value)*(1 + log(A))/A;
  else
    rv = (V/A)*exp(fabs(value)*(1 + log(A))/V - 1);

  if (value < 0.0)
    rv *= -1.0;

  return(rv);
}


int QccSQALawMakeQuantizer(QccSQScalarQuantizer *quantizer,
                           double A,
                           double max_value,
                           double min_value)
{
  int level;
  double center_value;
  double V;

  if (quantizer == NULL)
    return(0);
  if (A < 1.0)
    {
      QccErrorAddMessage("(QccSQALawMakeQuantizer): A must be 1.0 or greater");
      return(1);
    }

  if (max_value <= min_value)
    {
      QccErrorAddMessage("(QccSQALawMakeQuantizer): Max value (%f) must be larger than min value (%f)",
                         max_value, min_value);
      return(1);
    }

  quantizer->type = QCCSQSCALARQUANTIZER_GENERAL;

  if (quantizer->num_levels < 1)
    return(0);

  center_value = (max_value + min_value)/2;
  V = fabs(max_value - center_value);

  if (QccSQUniformMakeQuantizer(quantizer,
                                V, -V,
                                QCCSQ_NOOVERLOAD))
    {
      QccErrorAddMessage("(QccMakeALawQuantizer): Error calling QccSQUniformMakeQuantizer()");
      return(1);
    }

  for (level = 0; level < quantizer->num_levels - 1; level++)
    {
      quantizer->levels[level] =
        QccSQALawExpander(quantizer->levels[level], V, A) +
        center_value;
      quantizer->boundaries[level] =
        QccSQALawExpander(quantizer->boundaries[level], V, A) +
        center_value;
    }

  quantizer->levels[quantizer->num_levels - 1] =
    QccSQALawExpander(quantizer->levels[quantizer->num_levels - 1],
                      V, A) + center_value;

  return(0);
}


