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


static double QccVQDistortionMeasureSquaredError(const QccVector vector1,
                                                 const QccVector vector2,
                                                 int vector_dimension,
                                                 int index)
{
  return(QccVectorSquareDistance(vector1, vector2,
                                 vector_dimension));
}


int QccVQVectorQuantizeVector(const QccVector vector,
                              const QccVQCodebook *codebook,
                              double *distortion,
                              int *partition,
                              QccVQDistortionMeasure distortion_measure)
{
  int codeword;
  int winner;
  double current_distance;
  double smallest_distance;
  int vector_dimension;
  
  if (vector == NULL)
    return(0);
  if (codebook == NULL)
    return(0);

  if (distortion_measure == NULL)
    distortion_measure = QccVQDistortionMeasureSquaredError;

  vector_dimension = codebook->codeword_dimension;
  
  smallest_distance = MAXDOUBLE;
  winner = 0;
      
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    {
      current_distance = 
        distortion_measure(vector,
                           codebook->codewords[codeword],
                           vector_dimension,
                           0);
      
      if (current_distance < smallest_distance)
        {
          smallest_distance = current_distance;
          winner = codeword;
        }
    }
      
  if (distortion != NULL)
    *distortion = smallest_distance;
      
  if (partition != NULL)
    *partition = winner;
  
  return(0);
}


int QccVQVectorQuantization(const QccDataset *dataset,
                            const QccVQCodebook *codebook,
                            QccVector distortion,
                            int *partition,
                            QccVQDistortionMeasure distortion_measure)
{
  int codeword;
  int winner;
  double current_distance;
  double smallest_distance;
  int vector;
  int vector_dimension;
  int num_vectors;
  
  if (dataset == NULL)
    return(0);
  if (codebook == NULL)
    return(0);

  num_vectors = QccDatasetGetBlockSize(dataset);
  
  if ((vector_dimension = dataset->vector_dimension) !=
      codebook->codeword_dimension)
    {
      QccErrorAddMessage("(QccVQVectorQuantization): vector dimensions of codebook and data are different");
      return(1);
    }
  
  if (distortion_measure == NULL)
    distortion_measure = QccVQDistortionMeasureSquaredError;

  for (vector = 0; vector < num_vectors; vector++)
    {
      smallest_distance = MAXDOUBLE;
      winner = 0;
      
      for (codeword = 0; codeword < codebook->num_codewords; codeword++)
        {
          current_distance = 
            distortion_measure(dataset->vectors[vector],
                               codebook->codewords[codeword],
                               vector_dimension,
                               vector);
          
          if (current_distance < smallest_distance)
            {
              smallest_distance = current_distance;
              winner = codeword;
            }
        }
      
      if (distortion != NULL)
        distortion[vector] = smallest_distance;
      
      if (partition != NULL)
        partition[vector] = winner;
    }
  
  return(0);
}


int QccVQInverseVectorQuantization(const int *partition, 
                                   const QccVQCodebook *codebook, 
                                   QccDataset *dataset)
{
  int vector;
  int num_vectors;
  int vector_dimension;
  
  if ((dataset == NULL) || (codebook == NULL) || (partition == NULL))
    return(0);
  
  num_vectors = QccDatasetGetBlockSize(dataset);
  
  if ((vector_dimension = dataset->vector_dimension) != 
      codebook->codeword_dimension)
    {
      QccErrorAddMessage("(QccVQInverseVectorQuantization): vector dimensions of codebook and data are different");
      return(1);
    }
  
  if (codebook->num_codewords > 0)
    for (vector = 0; vector < num_vectors; vector++)
      {
        if ((partition[vector] < codebook->num_codewords) &&
            (partition[vector] >= 0))
          QccVectorCopy(dataset->vectors[vector], 
                        codebook->codewords[partition[vector]],
                        vector_dimension);
        else
          {
            QccErrorAddMessage("(QccVQInverseVectorQuantization): Partition value is out of range for number of codewords in codebook");
            return(1);
          }
      }
  else
    for (vector = 0; vector < num_vectors; vector++)
      QccVectorZero(dataset->vectors[vector], 
                    vector_dimension);
    
  return(0);
}
