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


int QccVQMultiStageVQTraining(const QccDataset *dataset,
                              QccVQMultiStageCodebook *multistage_codebook,
                              int num_iterations,
                              double threshold,
                              QccVQDistortionMeasure distortion_measure,
                              QccVQGeneralizedLloydCentroids
                              centroid_calculation,
                              int verbose)
{
  int return_value;
  int block_size;
  int stage;
  int *partition = NULL;
  QccDataset residual;
  QccDataset reconstruction;

  if (dataset == NULL)
    return(0);
  if (multistage_codebook == NULL)
    return(0);

  QccDatasetInitialize(&residual);
  QccDatasetInitialize(&reconstruction);

  for (stage = 0; stage < multistage_codebook->num_codebooks; stage++)
    if (dataset->vector_dimension !=
        multistage_codebook->codebooks[stage].codeword_dimension)
      {
        QccErrorAddMessage("(QccVQMultiStageVQTraining): All codebooks must have same vector dimension as dataset");
        goto Error;
      }

  block_size = QccDatasetGetBlockSize(dataset);

  if ((partition = (int *)malloc(sizeof(int) * block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVQMultiStageVQTraining): Error allocating memory");
      goto Error;
    }

  residual.num_vectors = block_size;
  residual.vector_dimension = dataset->vector_dimension;
  residual.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  QccStringMakeNull(residual.filename);
  if (QccDatasetAlloc(&residual))
    {
      QccErrorAddMessage("(QccVQMultiStageVQTraining): Error calling QccDatasetAlloc()");
      goto Error;
    }

  reconstruction.num_vectors = block_size;
  reconstruction.vector_dimension = dataset->vector_dimension;
  reconstruction.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  QccStringMakeNull(reconstruction.filename);
  if (QccDatasetAlloc(&reconstruction))
    {
      QccErrorAddMessage("(QccVQMultiStageVQTraining): Error calling QccDatasetAlloc()");
      goto Error;
    }

  if (QccDatasetCopy(&residual, dataset))
    {
      QccErrorAddMessage("(QccVQMultiStageVQTraining): Error calling QccDatasetCopy()");
      goto Error;
    }

  for (stage = 0; stage < multistage_codebook->num_codebooks; stage++)
    {
      if (verbose)
        printf("MSVQ Stage: %d\n", stage);

      if (QccVQGeneralizedLloydVQTraining(&residual,
                                          &multistage_codebook->codebooks
                                          [stage],
                                          num_iterations,
                                          threshold,
                                          distortion_measure,
                                          centroid_calculation,
                                          verbose))
        {
          QccErrorAddMessage("(QccVQMultiStageVQTraining): Error calling QccVQGeneralizedLloydVQTraining()");
          goto Error;
        }

      if (QccVQVectorQuantization(&residual,
                                  &multistage_codebook->codebooks[stage],
                                  NULL, partition,
                                  distortion_measure))
        {
          QccErrorAddMessage("(QccVQMultiStageVQTraining): Error calling QccVQVectorQuantization()");
          goto Error;
        }

      if (QccVQInverseVectorQuantization(partition,
                                         &multistage_codebook->codebooks
                                         [stage],
                                         &reconstruction))
        {
          QccErrorAddMessage("(QccVQMultiStageVQTraining): Error calling QccVQInverseVectorQuantization()");
          goto Error;
        }

      if (QccMatrixSubtract(residual.vectors,
                            reconstruction.vectors,
                            block_size,
                            residual.vector_dimension))
        {
          QccErrorAddMessage("(QccVQMultiStageVQTraining): Error calling QccMatrixSubtract()");
          goto Error;
        }

    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (partition != NULL)
    QccFree(partition);
  QccDatasetFree(&residual);
  QccDatasetFree(&reconstruction);
  return(return_value);
}


int QccVQMultiStageVQEncode(const QccDataset *dataset,
                            const QccVQMultiStageCodebook *multistage_codebook,
                            int *partition,
                            QccVQDistortionMeasure distortion_measure)
{
  int return_value;
  int stage;
  int *current_partition;
  int block_size;
  QccDataset residual;
  QccDataset reconstruction;

  if (dataset == NULL)
    return(0);
  if (multistage_codebook == NULL)
    return(0);
  if (partition == NULL)
    return(0);

  QccDatasetInitialize(&residual);
  QccDatasetInitialize(&reconstruction);

  block_size = QccDatasetGetBlockSize(dataset);

  residual.num_vectors = block_size;
  residual.vector_dimension = dataset->vector_dimension;
  if (QccDatasetAlloc(&residual))
    {
      QccErrorAddMessage("(QccVQMultiStageVQEncode): Error calling QccDatasetAlloc()");
      goto Error;
    }

  reconstruction.num_vectors = block_size;
  reconstruction.vector_dimension = dataset->vector_dimension;
  if (QccDatasetAlloc(&reconstruction))
    {
      QccErrorAddMessage("(QccVQMultiStageVQEncode): Error calling QccDatasetAlloc()");
      goto Error;
    }

  if (QccDatasetCopy(&residual, dataset))
    {
      QccErrorAddMessage("(QccVQMultiStageVQEncode): Error calling QccDatasetCopy()");
      goto Error;
    }

  for (stage = 0; stage < multistage_codebook->num_codebooks; stage++)
    {
      current_partition =
        &partition[stage * block_size];

      if (QccVQVectorQuantization(&residual,
                                  &multistage_codebook->codebooks[stage],
                                  NULL,
                                  current_partition,
                                  distortion_measure))
        {
          QccErrorAddMessage("(QccVQMultiStageVQEncode): Error calling QccVQVectorQuantization()");
          goto Error;
        }
      
      if (QccVQInverseVectorQuantization(current_partition,
                                         &multistage_codebook->codebooks
                                         [stage],
                                         &reconstruction))
        {
          QccErrorAddMessage("(QccVQMultiStageVQEncode): Error calling QccVQInverseVectorQuantization()");
          goto Error;
        }
      
      if (QccMatrixSubtract(residual.vectors,
                            reconstruction.vectors,
                            block_size,
                            residual.vector_dimension))
        {
          QccErrorAddMessage("(QccVQMultiStageVQEncode): Error calling QccMatrixSubtract()");
          goto Error;
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccDatasetFree(&residual);
  QccDatasetFree(&reconstruction);
  return(return_value);
}


int QccVQMultiStageVQDecode(const int *partition, 
                            const
                            QccVQMultiStageCodebook *multistage_codebook, 
                            QccDataset *dataset,
                            int num_stages_to_decode)
{
  int return_value;
  int stage;
  int block_size;
  const int *current_partition;
  QccDataset reconstruction;
  int vector;

  if (partition == NULL)
    return(0);
  if (multistage_codebook == NULL)
    return(0);
  if (dataset == NULL)
    return(0);

  QccDatasetInitialize(&reconstruction);

  block_size = QccDatasetGetBlockSize(dataset);

  reconstruction.num_vectors = block_size;
  reconstruction.vector_dimension = dataset->vector_dimension;
  if (QccDatasetAlloc(&reconstruction))
    {
      QccErrorAddMessage("(QccVQMultiStageVQDecode): Error calling QccDatasetAlloc()");
      goto Error;
    }

  for (vector = 0; vector < block_size; vector++)
    QccVectorZero(dataset->vectors[vector], dataset->vector_dimension);

  for (stage = 0; stage < num_stages_to_decode; stage++)
    {
      current_partition =
        &partition[stage * block_size];

      if (QccVQInverseVectorQuantization(current_partition,
                                         &multistage_codebook->codebooks
                                         [stage],
                                         &reconstruction))
        {
          QccErrorAddMessage("(QccVQMultiStageVQDecode): Error calling QccVQInverseVectorQuantization()");
          return(1);
        }

      if (QccMatrixAdd(dataset->vectors,
                       reconstruction.vectors,
                       block_size,
                       dataset->vector_dimension))
        {
          QccErrorAddMessage("(QccVQMultiStageVQDecode): Error calling QccMatrixAdd()");
          return(1);
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccDatasetFree(&reconstruction);
  return(return_value);
}

