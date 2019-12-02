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


static int QccVQGeneralizedLloydSquaredErrorCentroids(const
                                                      QccDataset *dataset,
                                                      QccVQCodebook *codebook,
                                                      const int *partition)
{
  int codeword;
  int vector_dimension;
  int current_vector;
  int num_vectors;
  int codeword_cnt;

  vector_dimension = dataset->vector_dimension;
  num_vectors = QccDatasetGetBlockSize(dataset);

  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    {
      codeword_cnt = 0;
      QccVectorZero((QccVector)codebook->codewords[codeword],
                    vector_dimension);

      for (current_vector = 0; current_vector < num_vectors;
           current_vector++)
        if (partition[current_vector] == codeword)
          {
            QccVectorAdd((QccVector)codebook->codewords[codeword],
                         dataset->vectors[current_vector],
                         vector_dimension);
            codeword_cnt++;
          }

      if (codeword_cnt)
        QccVectorScalarMult((QccVector)codebook->codewords[codeword], 
                            1.0 / (double)codeword_cnt,
                            vector_dimension);
      else
        QccVectorCopy((QccVector)codebook->codewords[codeword],
                      dataset->vectors[(int)
                                      (floor(QccMathRand() * 
                                             (num_vectors - 1)))],
                      vector_dimension);
    }

  return(0);
}


static int
QccVQGeneralizedLloydVQTrainingIteration(const QccDataset *dataset,
                                         QccVQCodebook *codebook,
                                         QccVector distortion,
                                         int *partition,
                                         QccVQDistortionMeasure
                                         distortion_measure,
                                         QccVQGeneralizedLloydCentroids
                                         centroid_calculation)
{

  if (centroid_calculation == NULL)
    centroid_calculation = QccVQGeneralizedLloydSquaredErrorCentroids;

  if (centroid_calculation(dataset,
                           codebook,
                           partition))
    {
      QccErrorAddMessage("(QccVQGeneralizedLloydVQTrainingIteration): Error calling calculate_centroids()");
      goto Error;
    }

  if (QccVQVectorQuantization(dataset,
                              codebook,
                              distortion,
                              partition,
                              distortion_measure))
    {
      QccErrorAddMessage("(QccVQGeneralizedLloydVQTrainingIteration): Error calling QccVQVectorQuantization()");
      goto Error;
    }

  return(0);

 Error:
  return(1);
}


int QccVQGeneralizedLloydVQTraining(const QccDataset *dataset,
                                    QccVQCodebook *codebook,
                                    int num_iterations,
                                    double threshold,
                                    QccVQDistortionMeasure distortion_measure,
                                    QccVQGeneralizedLloydCentroids
                                    centroid_calculation,
                                    int verbose)
{
  QccVector distortion = NULL;
  int *partition = NULL;
  int iteration;
  double D;
  double previous_D;
  int block_size;
  int return_value;

  if (dataset == NULL)
    return(0);
  if (codebook == NULL)
    return(0);

  if (dataset->vector_dimension != codebook->codeword_dimension)
    {
      QccErrorAddMessage("(QccVQGeneralizedLloydVQTraining): codebook %s and dataset %s do not have the same vector dimension",
                         codebook->filename, dataset->filename);
      goto Error;
    }

  block_size = QccDatasetGetBlockSize(dataset);
  if ((distortion = QccVectorAlloc(block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVQGeneralizedLloydVQTraining): Error allocating memory");
      goto Error;
    }

  if ((partition = (int *)malloc(sizeof(int)*block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVQGeneralizedLloydVQTraining): Error allocating memory");
      goto Error;
    }

  if (QccVQVectorQuantization(dataset, codebook, distortion, partition,
                              distortion_measure))
    {
      QccErrorAddMessage("(QccVQGeneralizedLloydVQTraining): Error calling QccVQVectorQuantization()");
      goto Error;
    }
  previous_D = QccVectorMean(distortion, block_size);

  if (previous_D != 0)
    {
      if (num_iterations != 0)
        {
          for (iteration = 0; iteration < num_iterations; iteration++)
            {
              if (verbose)
                printf("GLA Iteration: %d\n", iteration);
              if (QccVQGeneralizedLloydVQTrainingIteration(dataset,
                                                           codebook,
                                                           distortion,
                                                           partition,
                                                           distortion_measure,
                                                           centroid_calculation))
                {
                  QccErrorAddMessage("(QccVQGeneralizedLloydVQTraining): Error calling QccVQGeneralizedLloydVQTrainingIteration()");
                  goto Error;
                }
            }
        }
      else
        for (iteration = 0; ; iteration++)
          {
            if (QccVQGeneralizedLloydVQTrainingIteration(dataset,
                                                         codebook,
                                                         distortion,
                                                         partition,
                                                         distortion_measure,
                                                         centroid_calculation))
              {
                QccErrorAddMessage("(QccVQGeneralizedLloydVQTraining): Error calling QccVQGeneralizedLloydVQTrainingIteration()");
                goto Error;
              }
            
            D = QccVectorMean(distortion, block_size);
            if (verbose)
              printf("GLA Iteration: %d, distortion = %g\n",
                     iteration, D);
            if ((((previous_D - D)/previous_D) < threshold) ||
                (D == 0.0))
              break;
            previous_D = D;
          }
    }
  
  if (partition != NULL)
    {
      if (QccVQCodebookSetProbsFromPartitions(codebook,
                                              partition, block_size))
        {
          QccErrorAddMessage("(QccVQGeneralizedLloydVQTraining): Error calling QccVQCodebookSetProbsFromPartitions()");
          goto Error;
        }
      QccVQCodebookSetCodewordLengths(codebook);
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(distortion);
  if (partition != NULL)
    QccFree(partition);
  return(return_value);
}
