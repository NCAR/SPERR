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

int QccVQEntropyConstrainedVQ(const QccDataset *dataset,
                              const QccVQCodebook *codebook,
                              double lambda,
                              QccVector distortion,
                              int *partition,
                              QccVQDistortionMeasure
                              distortion_measure)
{
  int codebook_size, vector_dimension, block_size;
  int current_vector;
  int codeword;
  double min_J, J;
  int winner;
  double distance, winning_distance;
  
  if ((dataset == NULL) || (codebook == NULL))
    return(0);
  
  if (dataset->vectors == NULL)
    return(0);
  
  if (distortion_measure == NULL)
    distortion_measure = (QccVQDistortionMeasure)QccVectorSquareDistance;

  vector_dimension = dataset->vector_dimension;
  if (vector_dimension != codebook->codeword_dimension)
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQ): Dataset %s and codebook %s have different vector dimensions",
                         dataset->filename, codebook->filename);
      return(1);
    }
  block_size = QccDatasetGetBlockSize(dataset);
  codebook_size = codebook->num_codewords;
  
  for (current_vector = 0; current_vector < block_size; current_vector++)
    {
      min_J = MAXDOUBLE;
      winner = 0;
      winning_distance = MAXDOUBLE;
      
      for (codeword = 0; codeword < codebook_size; codeword++)
        if (codebook->codeword_probs[codeword] > 0)
          {
            distance = 
              distortion_measure(dataset->vectors[current_vector],
                                 codebook->codewords[codeword],
                                 vector_dimension,
                                 current_vector);
            J = distance + lambda * codebook->codeword_codelengths[codeword];
            
            if (J < min_J)
              {
                min_J = J;
                winner = codeword;
                winning_distance = distance;
              }
          }
      
      if (distortion != NULL)
        distortion[current_vector] = winning_distance;
      if (partition != NULL)
        partition[current_vector] = winner;
    }
  
  return(0);
}


static int QccVQEntropyConstrainedVQTrainingIteration(const
                                                      QccDataset *dataset,
                                                      QccVQCodebook *codebook,
                                                      double lambda,
                                                      QccVector distortion,
                                                      int *partition,
                                                      QccVQDistortionMeasure
                                                      distortion_measure)
{
  int return_value;
  int vector_dimension;
  int num_vectors;
  QccVector mean_vector;
  int *codeword_cnt = NULL;
  int codeword;
  int current_vector;
  
  vector_dimension = dataset->vector_dimension;
  num_vectors = QccDatasetGetBlockSize(dataset);
  
  if ((mean_vector = QccVectorAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTrainingIteration): Error allocating memory");
      goto Error;
    }
  if ((codeword_cnt = 
       (int *)calloc(codebook->num_codewords, sizeof(int))) == NULL)
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTrainingIteration): Error allocating memory");
      goto Error;
    }
  
  /*  Find new centroids  */
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    {
      QccVectorZero(mean_vector, vector_dimension);
      for (current_vector = 0; current_vector < num_vectors;
           current_vector++)
        if (partition[current_vector] == codeword)
          {
            QccVectorAdd(mean_vector, dataset->vectors[current_vector],
                         vector_dimension);
            codeword_cnt[codeword]++;
          }
      
      codebook->codeword_probs[codeword] =
        (double)codeword_cnt[codeword]/num_vectors;
      
      if (codeword_cnt[codeword])
        QccVectorScalarMult(mean_vector, 
                            (double)(1.0/(double)codeword_cnt[codeword]),
                            vector_dimension);
      
      QccVectorCopy((QccVector)codebook->codewords[codeword],
                    mean_vector, vector_dimension);
    }
  
  QccVQCodebookSetCodewordLengths(codebook);
  
  /*  Find new partitions  */
  if (QccVQEntropyConstrainedVQ(dataset, codebook, lambda, 
                                distortion, partition, distortion_measure))
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTrainingIteration): Error calling QccVQEntropyConstrainedVQ()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(mean_vector);
  if (codeword_cnt != NULL) 
    QccFree(codeword_cnt);
  return(return_value);
}


int QccVQEntropyConstrainedVQTraining(const QccDataset *dataset,
                                      QccVQCodebook *codebook,
                                      double lambda,
                                      int num_iterations,
                                      double threshold,
                                      QccVQDistortionMeasure
                                      distortion_measure)
{
  int return_value;
  QccVector distortion = NULL;
  int *partition = NULL;
  int iteration;
  double D, R, J;
  double previous_J = MAXDOUBLE;
  int block_size;
  int codebook_size;
  QccVector probs;
  QccVQCodebook new_codebook;
  int codeword, new_codeword;
  
  QccVQCodebookInitialize(&new_codebook);

  if (dataset->vector_dimension != codebook->codeword_dimension)
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): codebook %s and dataset %s do not have the same vector dimension",
                         codebook->filename, dataset->filename);
      goto Error;
    }
  
  codebook_size = codebook->num_codewords;
  
  block_size = QccDatasetGetBlockSize(dataset);
  if ((distortion = QccVectorAlloc(block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error allocating memory");
      goto Error;
    }
  
  if ((partition = (int *)malloc(sizeof(int)*block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error allocating memory");
      goto Error;
    }
  if ((probs = QccVectorAlloc(codebook_size)) == NULL)
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  if (QccVQEntropyConstrainedVQ(dataset, codebook, lambda,
                                distortion, partition,
                                distortion_measure))
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error calling QccVQVectorQuantization()");
      goto Error;
    }

  D = QccVectorMean(distortion, block_size);
  QccVectorGetSymbolProbs(partition, block_size,
                             probs, codebook_size);
  R = QccENTEntropy(probs, codebook_size);
  previous_J = D + lambda*R;
  
  if (previous_J != 0.0)
    {
      if (num_iterations != 0)
        {
          for (iteration = 0; iteration < num_iterations; iteration++)
            if (QccVQEntropyConstrainedVQTrainingIteration(dataset, codebook, 
                                                           lambda,
                                                           distortion, 
                                                           partition,
                                                           distortion_measure))
              {
                QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error calling QccVQEntropyConstrainedVQTrainingIteration()");
                goto Error;
              }
        }
      else
        for (;;)
          {
            {
              int i;
              int nullcws;
              for (nullcws = 0, i = 0; i < codebook->num_codewords; i++)
                if (codebook->codeword_probs[i] != 0)
                  nullcws++;
            }
            
            if (QccVQEntropyConstrainedVQTrainingIteration(dataset, codebook, 
                                                           lambda,
                                                           distortion, 
                                                           partition,
                                                           distortion_measure))
              {
                QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error calling QccVQEntropyConstrainedVQTrainingIteration()");
                goto Error;
              }
            
            D = QccVectorMean(distortion, block_size);
            QccVectorGetSymbolProbs(partition, block_size,
                                    probs, codebook_size);
            
            R = QccENTEntropy(probs, codebook_size);
            J = D + lambda*R;
            
            if ((((previous_J - J)/previous_J) < threshold) ||
                (J == 0.0))
              break;
            previous_J = J;
          }
    }
  
  for (codeword = 0, new_codebook.num_codewords = 0; codeword < codebook_size;
       codeword++)
    if (codebook->codeword_probs[codeword] > 0)
      new_codebook.num_codewords++;
  
  new_codebook.codeword_dimension = codebook->codeword_dimension;

  if (QccVQCodebookAlloc(&new_codebook))
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error calling QccVQCodebookAlloc()");
      goto Error;
    }
  
  for (codeword = 0, new_codeword = 0; codeword < codebook_size;
       codeword++)
    if (codebook->codeword_probs[codeword] > 0)
      {
        QccVectorCopy((QccVector)new_codebook.codewords[new_codeword],
                      (QccVector)codebook->codewords[codeword],
                      codebook->codeword_dimension);
        new_codebook.codeword_probs[new_codeword] = 
          codebook->codeword_probs[codeword];
        new_codeword++;
      }

  QccVQCodebookFree(codebook);
  codebook->num_codewords = new_codebook.num_codewords;
  codebook->codewords = new_codebook.codewords;
  codebook->codeword_probs = new_codebook.codeword_probs;
  codebook->codeword_codelengths = new_codebook.codeword_codelengths;

  if (QccVQCodebookSetIndexLength(codebook))
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error calling QccVQCodebookSetIndexLength()");
      goto Error;
    }

  if (QccVQCodebookSetCodewordLengths(codebook))
    {
      QccErrorAddMessage("(QccVQEntropyConstrainedVQTraining): Error calling QccVQCodebookSetCodewordLengths()");
      goto Error;
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
