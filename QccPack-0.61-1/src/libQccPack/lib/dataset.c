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


int QccDatasetInitialize(QccDataset *dataset)
{
  if (dataset == NULL)
    return(0);

  QccStringMakeNull(dataset->filename);
  dataset->fileptr = NULL;
  QccStringCopy(dataset->magic_num, QCCDATASET_MAGICNUM);
  QccGetQccPackVersion(&dataset->major_version,
                       &dataset->minor_version,
                       NULL);
  dataset->num_vectors = 0;
  dataset->vector_dimension = 0;
  dataset->access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  dataset->num_blocks_accessed = 0;
  dataset->min_val = 0.0;
  dataset->max_val = 0.0;
  dataset->vectors = NULL;

  return(0);
}


int QccDatasetAlloc(QccDataset *dataset)
{
  int block_size;

  if (dataset == NULL)
    return(0);

  block_size = QccDatasetGetBlockSize(dataset);

  if (dataset->vectors == NULL)
    if ((dataset->vectors = 
         QccMatrixAlloc(block_size,
                        dataset->vector_dimension)) == NULL)
      {
        QccErrorAddMessage("(QccDatasetAlloc): Error calling QccMatrixAlloc()");
        return(1);
      }
      
  return(0);
}


void QccDatasetFree(QccDataset *dataset)
{
  if (dataset == NULL)
    return;

  QccMatrixFree(dataset->vectors, 
                QccDatasetGetBlockSize(dataset));
  dataset->vectors = NULL;

}


int QccDatasetGetBlockSize(const QccDataset *dataset)
{
  int block_size;

  if (dataset == NULL)
    return(0);

  block_size = 
    (dataset->access_block_size == QCCDATASET_ACCESSWHOLEFILE) ?
    dataset->num_vectors : dataset->access_block_size;
  
  return(block_size);
}


int QccDatasetPrint(const QccDataset *dataset)
{
  int component, vector;
  int block_size;
  
  if (QccFilePrintFileInfo(dataset->filename,
                           dataset->magic_num,
                           dataset->major_version,
                           dataset->minor_version))
    return(1);

  printf("Num of vectors: %d\n", dataset->num_vectors);
  printf("Vector dimension: %d\n", dataset->vector_dimension);
  printf("Minimum value: % 10.4f, maximum value: % 10.4f\n",
         dataset->min_val, dataset->max_val);
  block_size = QccDatasetGetBlockSize(dataset);
  if (dataset->access_block_size == QCCDATASET_ACCESSWHOLEFILE)
    printf("Access block size: whole file\n");
  else
    printf("Access block size: %d vectors\n",
           block_size);
  printf("Number of blocks accessed so far: %d\n",
         dataset->num_blocks_accessed);
  if (dataset->num_blocks_accessed)
    {
      printf("\nCurrent block:\n\n     Index\t\tVector\n\n");
      
      for (vector = 0; vector < block_size; vector++)
        {
          printf("%10d\t", vector);
          for (component = 0; component < dataset->vector_dimension; 
               component++)
            printf("% 10.4f ", dataset->vectors[vector][component]);
          printf("\n");
        }
    }
  fflush(stdout);
  return(0);
}


int QccDatasetCopy(QccDataset *dataset1, const QccDataset *dataset2)
{
  int block_size1 = 0;
  int block_size2 = 0;

  if (dataset1 == NULL)
    return(0);
  if (dataset2 == NULL)
    return(0);
  if (dataset2->vectors == NULL)
    return(0);

  block_size2 = QccDatasetGetBlockSize(dataset2);

  if ((block_size2 <= 0) ||
      (dataset2->vector_dimension <= 0))
    return(0);

  if (dataset1->vectors == NULL)
    {
      dataset1->access_block_size = block_size2;
      block_size1 = block_size2;
      dataset1->vector_dimension = dataset2->vector_dimension;
      dataset1->num_vectors = dataset2->num_vectors;

      if (QccDatasetAlloc(dataset1))
        {
          QccErrorAddMessage("(QccDatasetCopy): Error calling QccDatasetAlloc()");
          return(1);
        }
    }
  else
    {
      block_size1 = QccDatasetGetBlockSize(dataset1);
      if (block_size1 != block_size2)
        {
          QccErrorAddMessage("(QccDatasetCopy): block sizes of datasets differ");
          return(1);
        }
      if (dataset1->vector_dimension !=
          dataset2->vector_dimension)
        {
          QccErrorAddMessage("(QccDatasetCopy): vector dimensions of datasets differ");
          return(1);
        }
    }
  
  if (QccMatrixCopy(dataset1->vectors,
                    dataset2->vectors,
                    block_size1,
                    dataset1->vector_dimension))
    {
      QccErrorAddMessage("(QccDatasetCopy): Error calling QccMatrixCopy()");
      return(1);
    }

  QccDatasetSetMaxMinValues(dataset1);

  return(0);
}


int QccDatasetReadWholefile(QccDataset *dataset)
{
  if (dataset == NULL)
    return(0);

  dataset->access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  
  if (QccDatasetStartRead(dataset))
    {
      QccErrorAddMessage("(QccDatasetReadWholefile): Error calling QccDatasetStartRead()");
      return(1);
    }
  if (QccDatasetReadBlock(dataset))
    {
      QccErrorAddMessage("(QccDatasetReadWholefile): Error calling QccDatasetReadBlock()");
      return(1);
    }

  QccFileClose(dataset->fileptr);

  return(0);
}


int QccDatasetReadHeader(QccDataset *dataset)
{
  
  if (QccFileReadMagicNumber(dataset->fileptr,
                             dataset->magic_num,
                             &dataset->major_version,
                             &dataset->minor_version))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): Error reading magic number in dataset %s",
                         dataset->filename);
      return(1);
    }

  if (strcmp(dataset->magic_num, QCCDATASET_MAGICNUM))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): %s is not of dataset (%s) type",
                         dataset->filename, QCCDATASET_MAGICNUM);
      return(1);
    }
  
  fscanf(dataset->fileptr, "%d", &(dataset->num_vectors));
  if (ferror(dataset->fileptr) || feof(dataset->fileptr))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): Error reading number of vectors in dataset %s",
                         dataset->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(dataset->fileptr, 0))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): Error reading vector dimension in dataset %s",
                         dataset->filename);
      return(1);
    }
  fscanf(dataset->fileptr, "%d", &(dataset->vector_dimension));
  if (ferror(dataset->fileptr) || feof(dataset->fileptr))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): Error reading vector dimension in dataset %s",
                         dataset->filename);
      return(1);
    }

  if (QccFileSkipWhiteSpace(dataset->fileptr, 0))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): Error reading min/max values in dataset %s",
                         dataset->filename);
      return(1);
    }
  fscanf(dataset->fileptr, "%lf", &(dataset->min_val));
  if (ferror(dataset->fileptr) || feof(dataset->fileptr))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): Error reading min values in dataset %s",
                         dataset->filename);
      return(1);
    }
  if (QccFileSkipWhiteSpace(dataset->fileptr, 0))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): Error reading min/max values in dataset %s",
                         dataset->filename);
      return(1);
    }
  fscanf(dataset->fileptr, "%lf%*1[\n]", &(dataset->max_val));
  if (ferror(dataset->fileptr) || feof(dataset->fileptr))
    {
      QccErrorAddMessage("(QccDatasetReadHeader): Error reading max values in dataset %s",
                         dataset->filename);
      return(1);
    }

  return(0);
}


int QccDatasetStartRead(QccDataset *dataset)
{
  int block_size;

  if (dataset == NULL)
    return(0);

  if ((dataset->fileptr = QccFileOpen(dataset->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccDatasetRead): Error opening %s for reading",
                         dataset->filename);
      return(1);
    }

  if (QccDatasetReadHeader(dataset))
    {
      QccErrorAddMessage("(QccDatasetRead): Error reading header of %s",
                         dataset->filename);
      return(1);
    }

  block_size = QccDatasetGetBlockSize(dataset);
    
  if (block_size > dataset->num_vectors)
    {
      QccErrorAddMessage("(QccDatasetRead): Dataset %s contains fewer vectors (%d) than requested block size (%d)",
                         dataset->filename, dataset->num_vectors, block_size);
      return(1);
    }

  if (QccDatasetAlloc(dataset))
    {
      QccErrorAddMessage("(QccDatasetRead): Error calling QccDatasetAlloc()");
      return(1);
    }
      
  dataset->num_blocks_accessed = 0;

  return(0);
}


int QccDatasetEndRead(QccDataset *dataset)
{
  if (dataset == NULL)
    return(0);

  QccFileClose(dataset->fileptr);

  QccDatasetFree(dataset);

  return(0);
}


int QccDatasetReadBlock(QccDataset *dataset)
{
  int vector, component;
  int num_vectors_to_read;

  if (dataset == NULL)
    return(0);
  if ((dataset->fileptr == NULL) || (!dataset->access_block_size))
    return(0);

  num_vectors_to_read = QccDatasetGetBlockSize(dataset);

  for (vector = 0; vector < num_vectors_to_read; vector++)
    for (component = 0; component < dataset->vector_dimension; 
         component++)
      {
        if (QccFileReadDouble(dataset->fileptr,
                              &(dataset->vectors[vector][component])))
          {
            QccErrorAddMessage("(QccDatasetReadBlock): Error calling QccFileReadDouble()");
            return(1);
          }
      }
  dataset->num_blocks_accessed++;

  return(0);
}


int QccDatasetReadSlidingBlock(QccDataset *dataset)
{
  int return_value;
  int vector, component;
  int num_vectors_left_to_read;

  if (dataset == NULL)
    return(0);
  if ((dataset->fileptr == NULL) || (!dataset->access_block_size))
    return(0);

  if (!dataset->num_blocks_accessed)
    {
      dataset->access_block_size = QccMathMin(dataset->access_block_size,
                                              dataset->num_vectors);
      if (QccDatasetReadBlock(dataset))
        {
          QccErrorAddMessage("(QccDatasetReadSlidingBlock): Error calling QccDatasetReadBlock()");
          goto Error;
        }
      dataset->num_blocks_accessed = 1;
    }
  else
    {
      num_vectors_left_to_read = 
        dataset->num_vectors - 
        (dataset->access_block_size + dataset->num_blocks_accessed) + 1;
      for (vector = 0; vector < dataset->access_block_size - 1 ; vector++)
        QccVectorCopy(dataset->vectors[vector],
                      dataset->vectors[vector + 1],
                      dataset->vector_dimension);
      if (num_vectors_left_to_read <= 0)
        {
          if (dataset->access_block_size > 0)
            {
              dataset->access_block_size--;
              QccVectorFree(dataset->vectors[dataset->access_block_size]);
              dataset->vectors[dataset->access_block_size] = NULL;
            }
        }
      else
        for (component = 0; component < dataset->vector_dimension; 
             component++)
          if (QccFileReadDouble(dataset->fileptr,
                                &(dataset->vectors
                                  [dataset->access_block_size - 1]
                                  [component])))
            {
              QccErrorAddMessage("(QccDatasetReadSlidingBlock): Error calling QccFileReadDouble()");
              goto Error;
            }
      dataset->num_blocks_accessed++;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccDatasetWriteWholefile(QccDataset *dataset)
{
  if (dataset == NULL)
    return(0);

  dataset->access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  
  if (QccDatasetStartWrite(dataset))
    {
      QccErrorAddMessage("(QccDatasetWriteWholefile): Error calling QccDatasetStartWrite()");
      return(1);
    }
  if (QccDatasetWriteBlock(dataset))
    {
      QccErrorAddMessage("(QccDatasetWriteWholefile): Error calling QccDatasetWriteBlock()");
      return(1);
    }

  QccFileClose(dataset->fileptr);

  return(0);
}


int QccDatasetWriteHeader(QccDataset *dataset)
{
  if (dataset == NULL)
    return(0);
  if (dataset->fileptr == NULL)
    return(0);

  if (QccFileWriteMagicNumber(dataset->fileptr, QCCDATASET_MAGICNUM))
    goto QccErr;

  fprintf(dataset->fileptr, "%d %d\n",
          dataset->num_vectors,
          dataset->vector_dimension);
  if (ferror(dataset->fileptr))
    goto QccErr;

  fprintf(dataset->fileptr, "% 16.9e % 16.9e\n",
          dataset->min_val, dataset->max_val);
  if (ferror(dataset->fileptr))
    goto QccErr;
              
  return(0);

 QccErr:
  QccErrorAddMessage("(QccDatasetWriteHeader): Error writing header to %s",
                     dataset->filename);
  return(1);

}


int QccDatasetStartWrite(QccDataset *dataset)
{
  if (dataset == NULL)
    return(0);

  if ((dataset->fileptr = QccFileOpen(dataset->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccDatasetWrite): Error opening %s for writing",
                         dataset->filename);
      return(1);
    }

  if (QccDatasetWriteHeader(dataset))
    {
      QccErrorAddMessage("(QccDatasetWrite): Error writing header to %s",
                         dataset->filename);
      return(1);
    }

  if (QccDatasetAlloc(dataset))
    {
      QccErrorAddMessage("(QccDatasetRead): Error calling QccDatasetAlloc()");
      return(1);
    }
      
  dataset->num_blocks_accessed = 0;

  return(0);
}


int QccDatasetEndWrite(QccDataset *dataset)
{
  if (dataset == NULL)
    return(0);

  QccFileClose(dataset->fileptr);

  QccDatasetFree(dataset);

  return(0);
}


int QccDatasetWriteBlock(QccDataset *dataset)
{
  int vector;
  int component;
  int num_vectors_to_write;

  if (dataset == NULL)
    return(0);
  if ((dataset->fileptr == NULL) || (!dataset->access_block_size))
    return(0);
  if (dataset->vectors == NULL)
    return(0);

  num_vectors_to_write = QccDatasetGetBlockSize(dataset);

  for (vector = 0; vector < num_vectors_to_write; vector++)
    {
      for (component = 0; component < dataset->vector_dimension; component++)
        if (QccFileWriteDouble(dataset->fileptr,
                               dataset->vectors[vector][component]))
          {
            QccErrorAddMessage("(QccDatasetWriteBlock): Error calling QccFileWriteDouble()");
            return(1);
          }
    }

  dataset->num_blocks_accessed++;

  return(0);
}


int QccDatasetSetMaxMinValues(QccDataset *dataset)
{
  int block_size;

  if (dataset == NULL)
    return(0);
  if (dataset->vectors == NULL)
    return(0);

  block_size = QccDatasetGetBlockSize(dataset);

  dataset->min_val =
    QccMatrixMinValue(dataset->vectors,
                           block_size,
                           dataset->vector_dimension);
  dataset->max_val =
    QccMatrixMaxValue(dataset->vectors,
                           block_size,
                           dataset->vector_dimension);

  return(0);
}


double QccDatasetMSE(const QccDataset *dataset1, const QccDataset *dataset2)
{
  double mse = 0.0;
  int vector;
  int block_size;
  int vector_dimension;

  if (dataset1 == NULL)
    return((double)0);
  if (dataset2 == NULL)
    return((double)0);
  if (dataset1->vectors == NULL)
    return((double)0);
  if (dataset2->vectors == NULL)
    return((double)0);

  vector_dimension = dataset1->vector_dimension;
  block_size = QccDatasetGetBlockSize(dataset1);

  if ((dataset2->vector_dimension != vector_dimension) ||
      (QccDatasetGetBlockSize(dataset2) != block_size))
    {
      QccErrorAddMessage("(QccDatasetMSE): %s and %s are not the same size datasets",
                         dataset1->filename, dataset2->filename);
      return((double)-1.0);
    }

  for (vector = 0; vector < block_size; vector++)
    mse += 
      QccVectorSquareDistance(dataset1->vectors[vector],
                              dataset2->vectors[vector],
                              vector_dimension) / vector_dimension;
  return(mse/block_size);
}


int QccDatasetMeanVector(const QccDataset *dataset, QccVector mean)
{
  int vector;
  int vector_dimension;
  int block_size;

  if (dataset == NULL)
    return(0);
  if (mean == NULL)
    return(0);

  if (dataset->vectors == NULL)
    return(0);

  if (dataset->num_vectors <= 0)
    return(0);

  vector_dimension = dataset->vector_dimension;
  block_size = QccDatasetGetBlockSize(dataset);

  if (QccVectorZero(mean, vector_dimension))
    {
      QccErrorAddMessage("(QccDatasetMeanVector): Error calling QccVectorZero()");
      return(1);
    }

  for (vector = 0; vector < block_size; vector++)
    {
      if (QccVectorAdd(mean, dataset->vectors[vector],
                       vector_dimension))
        {
          QccErrorAddMessage("(QccDatasetMeanVector): Error calling QccVectorAdd()");
          return(1);
        }
    }

  if (QccVectorScalarMult(mean, (double)1/block_size,
                          vector_dimension))
    {
      QccErrorAddMessage("(QccDatasetMeanVector): Error calling QccVectorScalarMult()");
      return(1);
    }

  return(0);
}


int QccDatasetCovarianceMatrix(const QccDataset *dataset, QccMatrix covariance)
{
  int return_value;
  QccVector mean = NULL;
  int row, col1, col2;
  int vector_dimension;
  int block_size;

  if (dataset == NULL)
    return(0);
  if (covariance == NULL)
    return(0);

  if (dataset->vectors == NULL)
    return(0);
  if (dataset->num_vectors <= 0)
    return(0);

  vector_dimension = dataset->vector_dimension;
  block_size = QccDatasetGetBlockSize(dataset);

  if ((mean = QccVectorAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccDatasetCovarianceMatrix): Error calling QccVectorAlloc()");
      goto Error;
    }

  if (QccDatasetMeanVector(dataset, mean))
    {
      QccErrorAddMessage("(QccDatasetCovarianceMatrix): Error calling QccDatasetMeanVector()");
      goto Error;
    }

  if (QccMatrixZero(covariance, vector_dimension, vector_dimension))
    {
      QccErrorAddMessage("(QccDatasetCovarianceMatrix): Error calling QccMatrixZero()");
      goto Error;
    }

  if (block_size > 1)
    {
      for (col1 = 0; col1 < vector_dimension; col1++)
        for (col2 = col1; col2 < vector_dimension; col2++)
          for (row = 0; row < block_size; row++)
            covariance[col2][col1] +=
              (dataset->vectors[row][col1] - mean[col1]) *
              (dataset->vectors[row][col2] - mean[col1]);
      
      for (col1 = 0; col1 < vector_dimension; col1++)
        for (col2 = col1; col2 < vector_dimension; col2++)
          {
            covariance[col2][col1] /= (block_size - 1);
            covariance[col1][col2] = covariance[col2][col1];
          }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(mean);
  return(return_value);
}


int QccDatasetCalcVectorPowers(const QccDataset *dataset,
                               QccVector vector_power)
{
  int vector_dimension;
  int block_size;
  int vector;
  double norm_value;

  if ((dataset == NULL) || (vector_power == NULL))
    return(0);

  if (dataset->vectors == NULL)
    return(0);

  vector_dimension = dataset->vector_dimension;
  block_size = QccDatasetGetBlockSize(dataset);

  for (vector = 0; vector < block_size; vector++)
    {
      norm_value = QccVectorNorm(dataset->vectors[vector],
                                 vector_dimension);
      vector_power[vector] = norm_value * norm_value;
    }

  return(0);
}


