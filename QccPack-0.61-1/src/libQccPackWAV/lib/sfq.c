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


int QccWAVsfqWaveletAnalysis(const QccIMGImageComponent *input_image,
                             QccWAVSubbandPyramid *subband_pyramid,
                             QccWAVZerotree *zerotree,
                             const QccSQScalarQuantizer *mean_quantizer,
                             const QccWAVWavelet *wavelet,
                             const QccWAVPerceptualWeights *perceptual_weights,
                             double lambda)
{
  int return_value;
  int num_levels;

  if ((subband_pyramid == NULL) ||
      (mean_quantizer == NULL) ||
      (wavelet == NULL))
    return(0);

  num_levels = zerotree->num_levels;

  if (QccMatrixCopy(subband_pyramid->matrix,
                    input_image->image,
                    input_image->num_rows,
                    input_image->num_cols))
    {
      QccErrorAddMessage("(QccWAVsfqWaveletAnalysis): Error calling QccMatrixCopy()");
      goto Error;
    }
  
  if (QccWAVSubbandPyramidSubtractMean(subband_pyramid,
                                       &(zerotree->image_mean),
                                       mean_quantizer))
    {
      QccErrorAddMessage("(QccWAVsfqWaveletAnalysis): Error calling QccWAVSubbandPyramidSubtractMean()");
      goto Error;
    }
  
  if (num_levels)
    if (QccWAVSubbandPyramidDWT(subband_pyramid,
                                num_levels,
                                wavelet))
      {
        QccErrorAddMessage("(QccWAVsfqWaveletAnalysis): Error calling QccWAVSubbandPyramidDWT()");
        goto Error;
      }

  if (perceptual_weights != NULL)
    if (QccWAVPerceptualWeightsApply(subband_pyramid,
                                     perceptual_weights))
      {
        QccErrorAddMessage("(QccWAVsfqWaveletAnalysis): Error calling QccWAVPerceptualWeightsApply()");
        QccErrorExit();
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVsfqBasebandEncodeProcess(QccIMGImageComponent *baseband_image,
                                          QccSQScalarQuantizer 
                                          *baseband_quantizer,
                                          QccChannel *channels,
                                          QccIMGImageComponent 
                                          *reconstructed_baseband,
                                          double *distortion, 
                                          double *rate)
{
  int return_value;
  QccVector distortion_vector = NULL;

  if ((baseband_image == NULL) ||
      (baseband_quantizer == NULL) ||
      (channels == NULL))
    return(0);

  if ((distortion_vector = QccVectorAlloc(baseband_image->num_rows *
                                          baseband_image->num_cols)) == 
      NULL)
    {
      QccErrorAddMessage("(QccWAVsfqBasebandEncodeProcess): Error calling QccVectorAlloc()");
      goto Error;
    }

  if (QccIMGImageComponentScalarQuantize(baseband_image,
                                         baseband_quantizer,
                                         distortion_vector,
                                         &(channels[0])))
    {
      QccErrorAddMessage("(QccWAVsfqBasebandEncodeProcess): Error calling QccIMGImageComponentScalarQuantize()");
      goto Error;
    }

  if (QccIMGImageComponentInverseScalarQuantize(&(channels[0]),
                                                baseband_quantizer,
                                                reconstructed_baseband))
    {
      QccErrorAddMessage("(QccWAVsfqBasebandEncodeProcess): Error calling QccIMGImageComponentInverseScalarQuantize()");
      goto Error;
    }

  if (QccChannelNormalize(&(channels[0])))
    {
      QccErrorAddMessage("(QccWAVsfqBasebandEncodeProcess): Error calling QccChannelNormalize()");
      goto Error;
    }

  if (distortion != NULL)
    *distortion = 
      QccVectorMean(distortion_vector,
                    baseband_image->num_rows *
                    baseband_image->num_cols);
  if (rate != NULL)
    *rate = QccChannelEntropy(&(channels[0]), 1);

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(distortion_vector);
  return(return_value);
}


int QccWAVsfqBasebandEncode(const QccWAVSubbandPyramid *subband_pyramid,
                            QccWAVZerotree *zerotree,
                            QccSQScalarQuantizer *baseband_quantizer,
                            QccChannel *channels,
                            double lambda,
                            QccIMGImageComponent *reconstructed_baseband)
{
  int return_value;
  QccIMGImageComponent baseband_image;
  double distortion = 0;
  double rate = 0;
  double baseband_quantizer_start_stepsize;
  double baseband_quantizer_end_stepsize;
  int done = 0;
  int last_time = 0;
  QccVector J = NULL;
  int pass;
  int num_passes;
  double min_J = MAXDOUBLE;
  double winner = 0;
  double max_coefficient = -MAXDOUBLE;
  int row, col;

  if ((subband_pyramid == NULL) ||
      (zerotree == NULL) ||
      (baseband_quantizer == NULL) ||
      (channels == NULL))
    return(0);

  if (baseband_quantizer->stepsize <= 0.0)
    {
      baseband_quantizer_start_stepsize =
        QCCWAVSFQ_BASEBANDQUANTIZER_START_STEPSIZE;
      baseband_quantizer_end_stepsize =
        QCCWAVSFQ_BASEBANDQUANTIZER_END_STEPSIZE;
    }
  else
    {
      baseband_quantizer_start_stepsize =
        baseband_quantizer_end_stepsize =
        baseband_quantizer->stepsize;
      last_time = 1;
    }
  
  num_passes =
    ceil((baseband_quantizer_end_stepsize -
          baseband_quantizer_start_stepsize) /
         QCCWAVSFQ_BASEBANDQUANTIZER_INCREMENT) + 1;

  baseband_image.num_rows = zerotree->num_rows[0];
  baseband_image.num_cols = zerotree->num_cols[0];
  baseband_image.image = subband_pyramid->matrix;
  for (row = 0; row < baseband_image.num_rows; row++)
    for (col = 0; col < baseband_image.num_cols; col++)
      if (fabs(baseband_image.image[row][col]) > max_coefficient)
        max_coefficient = fabs(baseband_image.image[row][col]);

  if ((J = QccVectorAlloc(num_passes)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqBasebandEncode): Error calling QccVectorAlloc()");
      goto Error;
    }
  for (pass = 0; pass < num_passes; pass++)
    J[pass] = MAXDOUBLE;

  baseband_quantizer->stepsize = 
    baseband_quantizer_start_stepsize;
  
  pass = 0;
  do
    {
      if (last_time)
        done = 1;
      
      if (QccSQScalarQuantization(max_coefficient,
                                  baseband_quantizer,
                                  NULL,
                                  &(channels[0].alphabet_size)))
        {
          QccErrorAddMessage("(QccWAVsfqBasebandEncode): Error calling QccSQScalarQuantization()");
          goto Error;
        }
      channels[0].alphabet_size =
        channels[0].alphabet_size * 2 + 1;
      baseband_quantizer->num_levels = channels[0].alphabet_size;

      if (QccWAVsfqBasebandEncodeProcess(&baseband_image,
                                         baseband_quantizer,
                                         channels,
                                         reconstructed_baseband,
                                         &distortion, &rate))
        {
          QccErrorAddMessage("(QccWAVsfqBasebandEncode): Error calling QccWAVsfqBasebandEncodeProcess()");
          goto Error;
        }

      if (!done)
        {
          J[pass] =
            distortion + lambda * rate;

          /****/
          /*
            printf("    step: %f\n", baseband_quantizer->stepsize);
            printf("     MSE: %f\n", distortion);
            printf("    rate: %f\n", rate);
            printf("       J: %f\n\n", J[pass]);
          */

          if (pass == num_passes - 1)
            {
              for (pass = 0;
                   pass < num_passes;
                   pass++)
                if (J[pass] < min_J)
                  {
                    min_J = J[pass];
                    winner = baseband_quantizer_start_stepsize +
                      QCCWAVSFQ_BASEBANDQUANTIZER_INCREMENT * pass;
                  }
              
              baseband_quantizer->stepsize = winner;
              last_time = 1;
            }
          else
            {
              baseband_quantizer->stepsize +=
                QCCWAVSFQ_BASEBANDQUANTIZER_INCREMENT;
              pass++;
            }
        }
    }
  while (!done);

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(J);
  return(return_value);
}


static int QccWAVsfqCalcCodewordLengths(QccVector codeword_lengths,
                                        int num_symbols,
                                        QccChannel *channels,
                                        QccWAVZerotree *zerotree)
{
  int subband, row, col;
  int symbol;
  int channel_index;
  int num_nonnull_symbols = 0;
  
  for (symbol = 0; symbol < num_symbols; symbol++)
    codeword_lengths[symbol] = 0;
  
  for (subband = 1;
       subband < zerotree->num_subbands; subband++)
    for (row = 0, channel_index = 0; 
         row < zerotree->num_rows[subband]; row++)
      for (col = 0; col < zerotree->num_cols[subband]; 
           col++, channel_index++)
        if (!QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
          {
            codeword_lengths[channels[subband].channel_symbols
                            [channel_index]]++;
            num_nonnull_symbols++;
          }
  
  for (symbol = 0; symbol < num_symbols; symbol++)
    codeword_lengths[symbol] =
      -QccMathLog2(codeword_lengths[symbol] / num_nonnull_symbols);
  
  return(0);
}


static double QccWAVsfqCalcChildrenCost(QccVector *costs,
                                        int child_subband,
                                        int parent_row,
                                        int parent_col,
                                        int num_child_cols)
{
  int child_row, child_col;
  int num_children;
  double cost = 0;
  
  num_children = (child_subband < 4) ? 1 : 2;
  
  for (child_row = num_children*parent_row;
       child_row < num_children*(parent_row + 1);
       child_row++)
    for (child_col = num_children*parent_col;
         child_col < num_children*(parent_col + 1);
         child_col++)
      cost +=
        costs[child_subband][child_row*num_child_cols + child_col];
  
  return(cost);
  
}

static double QccWAVsfqSelectMinimumCost(QccWAVZerotree *zerotree, 
                                         int subband, 
                                         int row, int col, int cost_index,
                                         QccVector *J, 
                                         QccVector *residue_tree_J,
                                         QccVector *delta_J,
                                         QccVector *squared_subband_images,
                                         int *zerotree_pruned)
{
  int level;
  double cost1, cost2;
  double minimum_cost;
  int child_subband;
  int child_subband_start, child_subband_end;
  
  level = QccWAVSubbandPyramidCalcLevelFromSubband(subband, zerotree->num_levels);
  
  if (subband)
    child_subband_start = child_subband_end =
      subband + 3;
  else
    {
      child_subband_start = 1;
      child_subband_end = 3;
    }
  
  cost1 = cost2 = 0.0;
  
  if (level > 1)
    {
      for (child_subband = child_subband_start;
           child_subband <= child_subband_end;
           child_subband++)
        {
          cost1 += 
            QccWAVsfqCalcChildrenCost(squared_subband_images, 
                                      child_subband,
                                      row, col,
                                      zerotree->num_cols
                                      [child_subband]);
          cost2 +=
            QccWAVsfqCalcChildrenCost(J, child_subband,
                                      row, col,
                                      zerotree->num_cols
                                      [child_subband]) +
            QccWAVsfqCalcChildrenCost(residue_tree_J, child_subband,
                                      row, col,
                                      zerotree->num_cols
                                      [child_subband]);
        }
      
      if (cost1 <= cost2)
        {
          zerotree->zerotree[subband][row][col] =
            QCCWAVZEROTREE_SYMBOLZTROOT;
          QccWAVZerotreeCarveOutZerotree(zerotree,
                                         subband,
                                         row, col);
          minimum_cost = cost1;
          *zerotree_pruned = 1;
        }
      else
        minimum_cost = cost2;
      
      squared_subband_images[subband][cost_index] += cost1;
      delta_J[subband][cost_index] = fabs(cost1 - cost2);
    }
  else
    minimum_cost = 0.0;
  
  return(minimum_cost);
}


static int QccWAVsfqZerotreeFindThresholdHigh(QccVector sorted_variances,
                                              int *zerotree_symbols,
                                              QccVector delta_J,
                                              int *addresses,
                                              int num_variances,
                                              double lambda,
                                              int *first_zero_position)
{
  int return_value;
  int *blist = NULL;
  int variance_index;
  int bindex;
  int blength;
  int previous_zero_position = 0;
  int hindex = 0;
  int bsum;
  double J_sum;
  int num_reversals = 0;
  int bits_saved = 0;

  if ((blist = (int *)malloc(sizeof(int) * num_variances)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqZerotreeFindThresholdHigh): Error allocating memory");
      goto Error;
    }

  do
    {
      for (variance_index = 0, bindex = -1; 
           variance_index < num_variances; variance_index++)
        {
          if (zerotree_symbols[addresses[variance_index]] ==
              QCCWAVZEROTREE_SYMBOLZTROOT)
            {
              if (bindex >= 0)
                blist[bindex] = variance_index - previous_zero_position;
              bindex++;
              previous_zero_position = variance_index;
            }
        }
      blength = bindex;
      
      for (hindex = 0; hindex < blength; hindex++)
        {
          for (variance_index = 0, bindex = 0, J_sum = 0.0, bsum = 0; 
               bindex <= hindex; variance_index++)
            if (zerotree_symbols[addresses[variance_index]] ==
                QCCWAVZEROTREE_SYMBOLZTROOT)
              {
                bsum += blist[bindex];
                J_sum += delta_J[addresses[variance_index]];
                bindex++;
              }
          
          if (bsum*lambda > J_sum)
            {
              for (variance_index--; variance_index >= 0; variance_index--)
                {
                  if (zerotree_symbols[addresses[variance_index]] ==
                      QCCWAVZEROTREE_SYMBOLZTROOT)
                    {
                      zerotree_symbols[addresses[variance_index]] = 
                        QCCWAVZEROTREE_SYMBOLSIGNIFICANT;
                      num_reversals++;
                    }
                }
              bits_saved += bsum;
              break;
            }
        }
    }
  while (hindex < blength);

  *first_zero_position = -1;
  for (variance_index = 0; variance_index < num_variances; 
       variance_index++)
    if (zerotree_symbols[addresses[variance_index]] ==
        QCCWAVZEROTREE_SYMBOLZTROOT)
      {
        *first_zero_position = variance_index;
        break;
      }

  printf("High: bits saved = %d, position %d\n", 
         bits_saved, variance_index);
         
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVsfqZerotreeFindThresholdLow(QccVector sorted_variances,
                                             int *zerotree_symbols,
                                             QccVector delta_J,
                                             int *addresses,
                                             int num_variances,
                                             double lambda,
                                             int first_zero_position,
                                             int *last_one_position)
{
  int return_value;
  int *blist = NULL;
  int variance_index;
  int bindex;
  int blength;
  int previous_one_position = 0;
  int hindex = 0;
  int bsum;
  double J_sum;
  int num_reversals = 0;
  int bits_saved = 0;

  if ((blist = (int *)malloc(sizeof(int) * num_variances)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqZerotreeFindThresholdLow): Error allocating memory");
      goto Error;
    }

  do
    {
      for (variance_index = num_variances - 1, bindex = -1; 
           (variance_index >= 0) && (variance_index > first_zero_position); 
           variance_index--)
        {
          if (zerotree_symbols[addresses[variance_index]] ==
              QCCWAVZEROTREE_SYMBOLSIGNIFICANT)
            {
              if (bindex >= 0)
                blist[bindex] = previous_one_position - variance_index;
              bindex++;
              previous_one_position = variance_index;
            }
        }
      blength = bindex;
      
      for (hindex = 0; hindex < blength; hindex++)
        {
          for (variance_index = num_variances - 1, bindex = 0, J_sum = 0.0,
                 bsum = 0; 
               bindex <= hindex; variance_index--)
            if (zerotree_symbols[addresses[variance_index]] ==
                QCCWAVZEROTREE_SYMBOLSIGNIFICANT)
              {
                bsum += blist[bindex];
                J_sum += delta_J[addresses[variance_index]];
                bindex++;
              }
          
          if (bsum*lambda > J_sum)
            {
              for (variance_index++; variance_index < num_variances;
                   variance_index++)
                {
                  if (zerotree_symbols[addresses[variance_index]] ==
                      QCCWAVZEROTREE_SYMBOLSIGNIFICANT)
                    {
                      zerotree_symbols[addresses[variance_index]] = 
                        QCCWAVZEROTREE_SYMBOLZTROOT;
                      num_reversals++;
                    }
                }
              bits_saved += bsum;
              break;
            }
        }
    }
  while (hindex < blength);

  *last_one_position = -1;
  for (variance_index = num_variances - 1;
       variance_index >= 0; variance_index--)
    if (zerotree_symbols[addresses[variance_index]] ==
        QCCWAVZEROTREE_SYMBOLSIGNIFICANT)
      {
        *last_one_position = variance_index;
        break;
      }

  printf("Low: bits saved = %d, position %d\n", 
         bits_saved, variance_index);
         
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVsfqZerotreePrediction(QccIMGImageComponent 
                                       *quantized_variances,
                                       QccWAVZerotree *zerotree,
                                       QccVector *delta_J,
                                       int subband,
                                       double lambda,
                                       int *high_tree_thresholds,
                                       int *low_tree_thresholds)
{
  int return_value;
  int row, col;
  QccIMGImageComponent filtered_variances;
  double filter_coefficients[] = { 0.333333, 0.333333, 0.333333 };
  QccFilter filter =
    {
      QCCFILTER_SYMMETRICWHOLE,
      3,
      NULL
    };
  int child_subband;
  int child_subband_start, child_subband_end;
  QccVector parent_variances = NULL;
  QccVector sorted_variances = NULL;
  QccVector delta_J_vector = NULL;
  int num_variances;
  int variance_index;
  int *zerotree_symbols = NULL;
  int *addresses = NULL;
  int first_zero_position, last_one_position;
  double high_variance, low_variance;

  filter.coefficients = filter_coefficients;

  QccIMGImageComponentInitialize(&filtered_variances);
  filtered_variances = *quantized_variances;
  filtered_variances.image = NULL;
  if (QccIMGImageComponentAlloc(&filtered_variances))
    {
      QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  for (row = 0; row < zerotree->num_rows[subband]; row++)
    for (col = 0; col < zerotree->num_cols[subband]; col++)
      {
        filtered_variances.image[row][col] = 0.0;
        if (QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
          quantized_variances->image[row][col] = 0.0;
      }
  
  if (QccIMGImageComponentFilterSeparable(quantized_variances,
                                          &filtered_variances,
                                          &filter, &filter,
                                          QCCFILTER_SYMMETRIC_EXTENSION))
    {
      QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error calling QccIMGImageComponentFilterSeparable()");
      goto Error;
    }

  if (!subband)
    {
      child_subband_start = 1;
      child_subband_end = 3;
    }
  else
    child_subband_start = child_subband_end = subband + 3;

  if ((parent_variances = 
       QccVectorAlloc(zerotree->num_rows[child_subband_start] *
                      zerotree->num_cols[child_subband_start])) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((sorted_variances = 
       QccVectorAlloc(zerotree->num_rows[child_subband_start] *
                      zerotree->num_cols[child_subband_start])) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((delta_J_vector = 
       QccVectorAlloc(zerotree->num_rows[child_subband_start] *
                      zerotree->num_cols[child_subband_start])) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((zerotree_symbols =
       (int *)malloc(sizeof(int) * zerotree->num_rows[child_subband_start] *
                     zerotree->num_cols[child_subband_start])) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error allocating memory");
      goto Error;
    }
  if ((addresses =
       (int *)malloc(sizeof(int) * zerotree->num_rows[child_subband_start] *
                     zerotree->num_cols[child_subband_start])) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error allocating memory");
      goto Error;
    }

  for (child_subband = child_subband_start;
       child_subband <= child_subband_end;
       child_subband++)
    {
      for (variance_index = 0, row = 0; 
           row < zerotree->num_rows[child_subband]; row++)
        for (col = 0; col < zerotree->num_cols[child_subband]; col++)
          if (!QccWAVZerotreeNullSymbol(zerotree->zerotree
                                        [child_subband][row][col]))
            {
              parent_variances[variance_index] =
                filtered_variances.image[row/2][col/2];
              zerotree_symbols[variance_index] =
                zerotree->zerotree[child_subband][row][col];
              addresses[variance_index] = variance_index;
              delta_J_vector[variance_index++] =
                delta_J[child_subband][row * 
                                      zerotree->num_cols[child_subband] + col];
            }
      num_variances = variance_index;

      if (QccVectorSortComponents(parent_variances, sorted_variances,
                                  num_variances,
                                  QCCVECTOR_SORTDESCENDING,
                                  addresses))
        {
          QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error calling QccVectorSortComponents()");
          goto Error;
        }

      printf("Subband %d, num nonull %d\n", 
             child_subband,
             num_variances);

      {
        int i;
        printf("subband %d\n", child_subband);
        for (i = 0; i < num_variances; i++)
          printf("%d %f %f\n", 
                 zerotree_symbols[addresses[i]],
                 sorted_variances[i], 
                 delta_J_vector[addresses[i]]);
      }

      if (QccWAVsfqZerotreeFindThresholdHigh(sorted_variances,
                                             zerotree_symbols,
                                             delta_J_vector,
                                             addresses,
                                             num_variances,
                                             lambda,
                                             &first_zero_position))
        {
          QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error calling QccWAVsfqZerotreeFindThresholdHigh()");
          goto Error;
        }
      high_tree_thresholds[child_subband] = addresses[first_zero_position];

      /*
        printf("Subband %d\n", child_subband);
        {
        int i;
        printf("subband %d\n", child_subband);
        for (i = 0; i < num_variances; i++)
        printf("%f %d\n", sorted_variances[i], 
        zerotree_symbols[addresses[i]]);
        }
      */

      if (QccWAVsfqZerotreeFindThresholdLow(sorted_variances,
                                            zerotree_symbols,
                                            delta_J_vector,
                                            addresses,
                                            num_variances,
                                            lambda,
                                            first_zero_position,
                                            &last_one_position))
        {
          QccErrorAddMessage("(QccWAVsfqZerotreePrediction): Error calling QccWAVsfqZerotreeFindThresholdLow()");
          goto Error;
        }
      low_tree_thresholds[child_subband] = addresses[last_one_position];

      /*
        printf("Subband %d\n", child_subband);
        {
        int i;
        printf("subband %d\n", child_subband);
        for (i = 0; i < num_variances; i++)
        printf("%f %d\n", sorted_variances[i], 
        zerotree_symbols[addresses[i]]);
        }
      */

      for (variance_index = 0, row = 0; 
           row < zerotree->num_rows[child_subband]; row++)
        for (col = 0; col < zerotree->num_cols[child_subband]; col++)
          if (!QccWAVZerotreeNullSymbol(zerotree->zerotree
                                        [child_subband][row][col]))
            {
              zerotree->zerotree[child_subband][row][col] =
                zerotree_symbols[variance_index];
              if (zerotree->zerotree[child_subband][row][col] ==
                  QCCWAVZEROTREE_SYMBOLZTROOT)
                QccWAVZerotreeCarveOutZerotree(zerotree,
                                               child_subband, row, col);
              else
                QccWAVZerotreeUndoZerotree(zerotree,
                                           child_subband, row, col);
              high_variance =
                (high_tree_thresholds[child_subband] >= 0) ?
                parent_variances[high_tree_thresholds[child_subband]] :
                -MAXDOUBLE;
              low_variance =
                (low_tree_thresholds[child_subband] >= 0) ?
                parent_variances[low_tree_thresholds[child_subband]] :
                MAXDOUBLE;
              /*****/
              if ((parent_variances[variance_index] >
                   high_variance) ||
                  (parent_variances[variance_index] <
                   low_variance))
                zerotree->zerotree[child_subband][row][col] =
                  QCCWAVZEROTREE_SYMBOLTEMP;

              variance_index++;
            }
      /****/
      printf("Subband: %d %f %f %d %d\n", 
             child_subband,
             parent_variances[high_tree_thresholds[child_subband]],
             parent_variances[low_tree_thresholds[child_subband]],
             high_tree_thresholds[child_subband],
             low_tree_thresholds[child_subband]);

      /*
        if (parent_variances[high_tree_thresholds[child_subband]] <=
        parent_variances[low_tree_thresholds[child_subband]])
        printf("*** ERROR ***\n");
      */
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&filtered_variances);
  QccVectorFree(parent_variances);
  QccVectorFree(sorted_variances);
  QccVectorFree(delta_J_vector);
  if (zerotree_symbols != NULL)
    QccFree(zerotree_symbols);
  return(return_value);
}


static int QccWAVsfqPruneZerotree(QccVector *squared_subband_images,
                                  QccIMGImageComponent *quantized_variances,
                                  QccVector *distortion,
                                  QccChannel *channels,
                                  QccWAVZerotree *zerotree,
                                  QccSQScalarQuantizer *quantizer,
                                  double lambda,
                                  int *high_tree_thresholds,
                                  int *low_tree_thresholds)
{
  int return_value;
  int num_subbands;
  int subband;
  QccVector *J = NULL;
  QccVector *residue_tree_J = NULL;
  QccVector *delta_J = NULL;
  QccVector codeword_lengths = NULL;
  int zerotree_changed;
  int row, col;
  int cost_index;
  
  num_subbands = zerotree->num_subbands;
  
  if ((codeword_lengths = 
       QccVectorAlloc(quantizer->num_levels)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqPruneZerotree): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  if ((J = (QccVector *)malloc(sizeof(QccVector)*num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqPruneZerotree): Error allocating memory");
      goto Error;
    }
  if ((residue_tree_J = 
       (QccVector *)malloc(sizeof(QccVector)*num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqPruneZerotree): Error allocating memory");
      goto Error;
    }
  if ((delta_J = 
       (QccVector *)malloc(sizeof(QccVector)*num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqPruneZerotree): Error allocating memory");
      goto Error;
    }
  
  for (subband = QCCWAVSFQ_ZEROTREE_STARTSUBBAND; 
       subband < num_subbands; subband++)
    {

      if ((J[subband] =
           QccVectorAlloc(channels[subband].channel_length)) == NULL)
        {
          QccErrorAddMessage("(QccWAVsfqPruneZerotree): Error calling QccVectorAlloc()");
          goto Error;
        }
      if ((residue_tree_J[subband] =
           QccVectorAlloc(channels[subband].channel_length)) == NULL)
        {
          QccErrorAddMessage("(QccWAVsfqPruneZerotree): Error calling QccVectorAlloc()");
          goto Error;
        }
      if ((delta_J[subband] =
           QccVectorAlloc(channels[subband].channel_length)) == NULL)
        {
          QccErrorAddMessage("(QccWAVsfqPruneZerotree): Error calling QccVectorAlloc()");
          goto Error;
        }
    }
  

  do
    {
      /*
        printf("Pruning iteration %d\n", iteration);
        iteration++;
      */
      zerotree_changed = 0;
      QccWAVsfqCalcCodewordLengths(codeword_lengths, quantizer->num_levels,
                                   channels, zerotree);
      for (subband = num_subbands - 1; 
           subband >= QCCWAVSFQ_ZEROTREE_STARTSUBBAND; subband--)
        {
          for (row = 0, cost_index = 0;
               row < zerotree->num_rows[subband];
               row++)
            for (col = 0;
                 col < zerotree->num_cols[subband];
                 col++, cost_index++)
              {
                if (!QccWAVZerotreeNullSymbol(zerotree->zerotree
                                              [subband][row][col]))
                  {
                    J[subband][cost_index] =
                      (subband) ?
                      distortion[subband][cost_index] +
                      lambda * 
                      codeword_lengths[channels[subband].channel_symbols
                                      [cost_index]] : 0.0;
                    if (zerotree->zerotree[subband][row][col] ==
                        QCCWAVZEROTREE_SYMBOLSIGNIFICANT)
                      residue_tree_J[subband][cost_index] =
                        QccWAVsfqSelectMinimumCost(zerotree, subband, 
                                                   row, col, cost_index,
                                                   J, residue_tree_J,
                                                   delta_J,
                                                   squared_subband_images,
                                                   &zerotree_changed);
                  }
                else
                  J[subband][cost_index] = 0.0;
              }
        }
    }
  while (zerotree_changed);
  
  if ((high_tree_thresholds != NULL) && (low_tree_thresholds != NULL))
    for (subband = 0; subband < zerotree->num_subbands - 6; subband++)
      if (QccWAVsfqZerotreePrediction(&quantized_variances[subband],
                                      zerotree,
                                      delta_J,
                                      subband, lambda,
                                      high_tree_thresholds,
                                      low_tree_thresholds))
        {
          QccErrorAddMessage("(QccWAVsfqPruneZerotree): Error calling QccWAVsfqZerotreePrediction()");
          goto Error;
        }

  return_value = 0;
  goto Return;
  
 Error:
  return_value = 1;
 Return:
  if (codeword_lengths != NULL)
    QccVectorFree(codeword_lengths);
  if (J != NULL)
    {
      for (subband = QCCWAVSFQ_ZEROTREE_STARTSUBBAND; 
           subband < num_subbands; subband++)
        QccVectorFree(J[subband]);
      QccFree(J);
    }
  if (residue_tree_J != NULL)
    {
      for (subband = QCCWAVSFQ_ZEROTREE_STARTSUBBAND; 
           subband < num_subbands; subband++)
        QccVectorFree(residue_tree_J[subband]);
      QccFree(residue_tree_J);
    }
  if (delta_J != NULL)
    {
      for (subband = QCCWAVSFQ_ZEROTREE_STARTSUBBAND; 
           subband < num_subbands; subband++)
        QccVectorFree(delta_J[subband]);
      QccFree(delta_J);
    }
  
  return(return_value);
}


static int QccWAVsfqHighpassEncodeProcess(QccIMGImageComponent *subband_images,
                                          QccIMGImageComponent 
                                          *quantized_subband_images,
                                          QccVector *squared_subband_images,
                                          QccWAVZerotree *zerotree,
                                          QccSQScalarQuantizer 
                                          *highpass_quantizer,
                                          QccChannel *channels,
                                          double lambda,
                                          int *high_tree_thresholds,
                                          int *low_tree_thresholds,
                                          double *distortion, double *rate,
                                          QccIMGImageComponent
                                          *reconstructed_baseband)
{
  int return_value;
  int num_subbands;
  int subband;
  QccVector *distortion_vector = NULL;
  int row, col;
  int image_index, total_num_symbols;
  int num_nonnull_symbols = 0;
  int num_predicted_symbols = 0;

  num_subbands = zerotree->num_subbands;
  
  if ((distortion_vector =
       (QccVector *)malloc(sizeof(QccVector)*num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqHighpassEncodeProcess): Error allocating memory");
      goto Error;
    }

  for (subband = 0; subband < num_subbands; subband++)
    if ((distortion_vector[subband] = 
         QccVectorAlloc(channels[subband].channel_length)) == NULL)
      {
        QccErrorAddMessage("(QccWAVsfqHighpassEncodeProcess): Error calling QccVectorAlloc()");
        goto Error;
      }
  
  for (subband = 1; subband < num_subbands; subband++)
    {
      if (QccIMGImageComponentScalarQuantize(&(subband_images[subband]),
                                             highpass_quantizer,
                                             distortion_vector[subband],
                                             &(channels[subband])))
        {
          QccErrorAddMessage("(QccWAVsfqHighpassEncodeProcess): Error calling QccIMGImageComponentScalarQuantize()");
          goto Error;
        }
      if (QccIMGImageComponentInverseScalarQuantize(&(channels[subband]),
                                                    highpass_quantizer,
                                                    &(quantized_subband_images[subband])))
        {
          QccErrorAddMessage("(QccWAVsfqHighpassEncodeProcess): Error calling QccIMGImageComponentInverseScalarQuantize()");
          goto Error;
        }

      if (QccChannelNormalize(&(channels[subband])))
        {
          QccErrorAddMessage("(QccWAVsfqHighpassEncodeProcess): Error calling QccChannelNormalize()");
          goto Error;
        }
    }
  
  for (row = 0; row < reconstructed_baseband->num_rows; row++)
    for (col = 0; col < reconstructed_baseband->num_cols; col++)
      quantized_subband_images[0].image[row][col] =
        reconstructed_baseband->image[row][col];

  for (subband = 0; subband < num_subbands; subband++)
    for (row = 0, image_index = 0; 
         row < subband_images[subband].num_rows; row++)
      for (col = 0; col < subband_images[subband].num_cols; 
           col++, image_index++)
        {
          squared_subband_images[subband][image_index] =
            subband_images[subband].image[row][col] *
            subband_images[subband].image[row][col];
          quantized_subband_images[subband].image[row][col] =
            quantized_subband_images[subband].image[row][col] *
            quantized_subband_images[subband].image[row][col];
        }
  
  QccWAVZerotreeMakeFullTree(zerotree);
  
  if (QccWAVsfqPruneZerotree(squared_subband_images,
                             quantized_subband_images,
                             distortion_vector,
                             channels,
                             zerotree,
                             highpass_quantizer,
                             lambda,
                             high_tree_thresholds,
                             low_tree_thresholds))
    {
      QccErrorAddMessage("(QccWAVsfqHighpassEncodeProcess): Error calling QccWAVsfqPruneZerotree()");
      goto Error;
    }
  
  *distortion = 0.0;
  *rate = 0.0;
  total_num_symbols = 0;
  for (subband = 1; subband < num_subbands; subband++)
    {
      num_nonnull_symbols = 0;
      num_predicted_symbols = 0;
      for (row = 0, image_index = 0; 
           row < subband_images[subband].num_rows; row++)
        for (col = 0; col < subband_images[subband].num_cols; 
             col++, image_index++, total_num_symbols++)
          if (QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
            {
              channels[subband].channel_symbols[image_index] =
                QCCCHANNEL_NULLSYMBOL;
              
              *distortion +=
                subband_images[subband].image[row][col] *
                subband_images[subband].image[row][col];
            }
          else
            if (zerotree->zerotree[subband][row][col] ==
                QCCWAVZEROTREE_SYMBOLTEMP)
              {
                num_predicted_symbols++;
                *distortion +=
                  distortion_vector[subband][image_index];
                
                QccWAVZerotreeMakeSymbolNull(&(zerotree->zerotree
                                               [subband][row][col]));
              }
            else
              {
                num_nonnull_symbols++;
                *distortion +=
                  distortion_vector[subband][image_index];
              }

      *rate += QccChannelEntropy(&(channels[subband]), 1) *
        (num_predicted_symbols + num_nonnull_symbols) +
        num_nonnull_symbols;

    }
  
  *distortion /= total_num_symbols;
  *rate /= total_num_symbols;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (distortion_vector != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccVectorFree(distortion_vector[subband]);
      QccFree(distortion_vector);
    }
  return(return_value);
}


int QccWAVsfqHighpassEncode(const QccWAVSubbandPyramid *subband_pyramid,
                            QccWAVZerotree *zerotree,
                            QccSQScalarQuantizer *highpass_quantizer,
                            QccChannel *channels,
                            double lambda,
                            int *high_tree_thresholds,
                            int *low_tree_thresholds,
                            QccIMGImageComponent *reconstructed_baseband)
{
  int return_value;
  double distortion = 0;
  double rate = 0;
  double highpass_quantizer_start_stepsize;
  double highpass_quantizer_end_stepsize;
  int done = 0;
  int last_time = 0;
  QccVector J = NULL;
  int pass;
  int num_passes;
  double min_J = MAXDOUBLE;
  double winner = 0;
  int subband;
  int row, col;
  QccIMGImageComponent *subband_images = NULL;
  QccIMGImageComponent *quantized_subband_images = NULL;
  QccVector *squared_subband_images = NULL;
  int num_subbands;
  double max_coefficient = -MAXDOUBLE;

  if ((subband_pyramid == NULL) ||
      (zerotree == NULL) ||
      (highpass_quantizer == NULL) ||
      (channels == NULL))
    return(0);

  num_subbands = zerotree->num_subbands;

  if ((subband_images =
       (QccIMGImageComponent *)malloc(sizeof(QccIMGImageComponent) * 
                                      num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error allocating memory");
      goto Error;
    }
  if ((quantized_subband_images =
       (QccIMGImageComponent *)malloc(sizeof(QccIMGImageComponent) * 
                                      num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error allocating memory");
      goto Error;
    }
  if ((squared_subband_images =
       (QccVector *)malloc(sizeof(QccVector)*num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error allocating memory");
      goto Error;
    }

  for (subband = 0; subband < num_subbands; subband++)
    {
      QccIMGImageComponentInitialize(&subband_images[subband]);
      subband_images[subband].num_rows = zerotree->num_rows[subband];
      subband_images[subband].num_cols = zerotree->num_cols[subband];
      
      QccIMGImageComponentInitialize(&quantized_subband_images[subband]);
      quantized_subband_images[subband].num_rows = 
        zerotree->num_rows[subband];
      quantized_subband_images[subband].num_cols = 
        zerotree->num_cols[subband];
      
      if (QccIMGImageComponentAlloc(&(subband_images[subband])))
        {
          QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
      if (QccIMGImageComponentAlloc(&(quantized_subband_images[subband])))
        {
          QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
      if ((squared_subband_images[subband] = 
           QccVectorAlloc(channels[subband].channel_length)) == NULL)
        {
          QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error calling QccVectorAlloc()");
          goto Error;
        }
    }

  if (QccWAVSubbandPyramidSplitToImageComponent(subband_pyramid,
                                                subband_images))
    {
      QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error calling QccWAVSubbandPyramidSplitToImageComponent()");
      goto Error;
    }
  
  for (subband = 1; subband < num_subbands; subband++)
    for (row = 0; row < subband_images[subband].num_rows; row++)
      for (col = 0; col < subband_images[subband].num_cols; col++)
        if (fabs(subband_images[subband].image[row][col]) > max_coefficient)
          max_coefficient = fabs(subband_images[subband].image[row][col]);

  if (highpass_quantizer->stepsize <= 0.0)
    {
      highpass_quantizer_start_stepsize =
        QCCWAVSFQ_HIGHPASSQUANTIZER_START_STEPSIZE;
      highpass_quantizer_end_stepsize =
        QCCWAVSFQ_HIGHPASSQUANTIZER_END_STEPSIZE;
    }
  else
    {
      highpass_quantizer_start_stepsize =
        highpass_quantizer_end_stepsize =
        highpass_quantizer->stepsize;
      last_time = 1;
    }  
  num_passes = 
    (highpass_quantizer_end_stepsize -
     highpass_quantizer_start_stepsize) /
    QCCWAVSFQ_HIGHPASSQUANTIZER_INCREMENT + 1;

  if ((J = QccVectorAlloc(num_passes)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error calling QccVectorAlloc()");
      goto Error;
    }
  for (pass = 0; pass < num_passes; pass++)
    J[pass] = MAXDOUBLE;

  highpass_quantizer->stepsize = highpass_quantizer_start_stepsize;
  
  pass = 0;
  do
    {
      if (last_time)
        done = 1;
      
      if (QccSQScalarQuantization(max_coefficient,
                                  highpass_quantizer,
                                  NULL,
                                  &(channels[1].alphabet_size)))
        {
          QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error calling QccSQScalarQuantization()");
          goto Error;
        }
      channels[1].alphabet_size =
        channels[1].alphabet_size * 2 + 1;
      highpass_quantizer->num_levels = channels[1].alphabet_size;

      for (subband = 2; subband < num_subbands; subband++)
        channels[subband].alphabet_size = 
          channels[1].alphabet_size;
      
      if (QccWAVsfqHighpassEncodeProcess(subband_images,
                                         quantized_subband_images,
                                         squared_subband_images,
                                         zerotree,
                                         highpass_quantizer,
                                         channels,
                                         lambda,
                                         high_tree_thresholds,
                                         low_tree_thresholds,
                                         &distortion, &rate,
                                         reconstructed_baseband))
        {
          QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error calling QccWAVsfqEncode()");
          goto Error;
        }

      if (!done)
        {
          J[pass] =
            distortion + lambda * rate;
          
          /****/
          /*
            printf("    step: %f\n", highpass_quantizer->stepsize);
            printf("     MSE: %f\n", distortion);
            printf("    rate: %f\n", rate);
            printf("       J: %f\n\n", J[pass]);
          */

          if (pass == num_passes - 1)
            {
              for (pass = 0; 
                   pass < num_passes; 
                   pass++)
                if (J[pass] < min_J)
                  {
                    min_J = J[pass];
                    winner = highpass_quantizer_start_stepsize +
                      QCCWAVSFQ_HIGHPASSQUANTIZER_INCREMENT * pass;
                  }
              highpass_quantizer->stepsize = winner;
              last_time = 1;
            }
          else
            {
              highpass_quantizer->stepsize +=
                QCCWAVSFQ_HIGHPASSQUANTIZER_INCREMENT;
              pass++;
            }
        }
    }
  while (!done);

  for (subband = 1; subband < num_subbands; subband++)
    if (QccChannelRemoveNullSymbols(&(channels[subband])))
      {
        QccErrorAddMessage("(QccWAVsfqHighpassEncode): Error calling QccChannelRemoveNullSymbols()");
        goto Error;
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(J);
  if (subband_images != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccIMGImageComponentFree(&(subband_images[subband]));
      QccFree(subband_images);
    }
  if (squared_subband_images != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccVectorFree(squared_subband_images[subband]);
      QccFree(squared_subband_images);
    }
  if (quantized_subband_images != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccIMGImageComponentFree(&(quantized_subband_images[subband]));
      QccFree(quantized_subband_images);
    }
  return(return_value);
}


static int QccWAVsfqDecodeReconstructZerotree(QccWAVZerotree *zerotree,
                                              int *zerotree_symbols,
                                              int *zerotree_index,
                                              int *high_tree_thresholds,
                                              int *low_tree_thresholds,
                                              int subband,
                                              QccIMGImageComponent
                                              *subband_images)
{
  int return_value;
  int row, col;
  QccIMGImageComponent variances;
  QccIMGImageComponent filtered_variances;
  double filter_coefficients[] = { 0.333333, 0.333333, 0.333333 };
  QccFilter filter =
    {
      QCCFILTER_SYMMETRICWHOLE,
      3,
      NULL
    };
  int parent_subband;
  int variance_index;
  double high_variance, low_variance, parent_variance;

  filter.coefficients = filter_coefficients;

  if ((subband <= 0) || 
      (subband >= zerotree->num_subbands - 3))
    return(0);

  if ((high_tree_thresholds == NULL) ||
      (low_tree_thresholds == NULL))
    {
      for (row = 0; row < zerotree->num_rows[subband]; row++)
        for (col = 0; col < zerotree->num_cols[subband]; col++)
          if (!QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
            {
              zerotree->zerotree[subband][row][col] =
                zerotree_symbols[(*zerotree_index)++];
              if (zerotree->zerotree[subband][row][col] ==
                  QCCWAVZEROTREE_SYMBOLZTROOT)
                QccWAVZerotreeCarveOutZerotree(zerotree,
                                               subband, row, col);
            }
      return(0);
    }

  parent_subband = (subband < 4) ? 0 : subband - 3;

  QccIMGImageComponentInitialize(&variances);
  QccIMGImageComponentInitialize(&filtered_variances);
  variances = subband_images[parent_subband];
  variances.image = NULL;
  filtered_variances = subband_images[parent_subband];
  filtered_variances.image = NULL;
  if (QccIMGImageComponentAlloc(&variances))
    {
      QccErrorAddMessage("(QccWAVsfqDecodeReconstructZerotree): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  if (QccIMGImageComponentAlloc(&filtered_variances))
    {
      QccErrorAddMessage("(QccWAVsfqDecodeReconstructZerotree): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }

  for (row = 0; row < zerotree->num_rows[parent_subband]; row++)
    for (col = 0; col < zerotree->num_cols[parent_subband]; col++)
      {
        filtered_variances.image[row][col] = 0.0;
        if (QccWAVZerotreeNullSymbol(zerotree->zerotree
                                     [parent_subband][row][col]))
          variances.image[row][col] = 0.0;
        else
          variances.image[row][col] =
            subband_images[parent_subband].image[row][col] *
            subband_images[parent_subband].image[row][col];
      }

  if (QccIMGImageComponentFilterSeparable(&variances,
                                          &filtered_variances,
                                          &filter, &filter,
                                          QCCFILTER_SYMMETRIC_EXTENSION))
    {
      QccErrorAddMessage("(QccWAVsfqDecodeReconstructZerotree): Error calling QccIMGImageComponentFilterSeparable()");
      goto Error;
    }

  high_variance = -MAXDOUBLE;
  low_variance = MAXDOUBLE;
  for (row = 0, variance_index = 0; row < zerotree->num_rows[subband]; row++)
    for (col = 0; col < zerotree->num_cols[subband]; col++)
      if (!QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
        {
          parent_variance = filtered_variances.image[row / 2][col / 2];
          if (variance_index == high_tree_thresholds[subband])
            high_variance = parent_variance;
          if (variance_index == low_tree_thresholds[subband])
            low_variance = parent_variance;
          variance_index++;
        }

  /****/
  printf("Subband: %d %f %f %d %d\n", 
         subband,
         high_variance, low_variance,
         high_tree_thresholds[subband],
         low_tree_thresholds[subband]);

  for (row = 0; row < zerotree->num_rows[subband]; row++)
    for (col = 0; col < zerotree->num_cols[subband]; col++)
      if (!QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
        {
          parent_variance = filtered_variances.image[row / 2][col / 2];
          if (parent_variance > high_variance)
            zerotree->zerotree[subband][row][col] =
              QCCWAVZEROTREE_SYMBOLSIGNIFICANT;
          else
            /*****/
            if (parent_variance < low_variance)
              zerotree->zerotree[subband][row][col] =
                QCCWAVZEROTREE_SYMBOLZTROOT;
            else
              zerotree->zerotree[subband][row][col] =
                zerotree_symbols[(*zerotree_index)++];

          if (zerotree->zerotree[subband][row][col] ==
              QCCWAVZEROTREE_SYMBOLZTROOT)
            QccWAVZerotreeCarveOutZerotree(zerotree,
                                           subband, row, col);
        }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&variances);
  QccIMGImageComponentFree(&filtered_variances);
  return(return_value);
}


static int QccWAVsfqDecodeReconstructSubband(QccWAVZerotree *zerotree,
                                             QccChannel *channels,
                                             int subband,
                                             const QccSQScalarQuantizer 
                                             *highpass_quantizer,
                                             const QccSQScalarQuantizer 
                                             *baseband_quantizer,
                                             QccIMGImageComponent 
                                             *subband_images)
{
  int return_value;
  int row, col, channel_index;

  if (QccChannelDenormalize(&(channels[subband])))
    {
      QccErrorAddMessage("(QccWAVsfqDecodeReconstructSubband): Error calling QccChannelDenormalize()");
      goto Error;
    }

  for (row = 0, channel_index = 0; 
       row < zerotree->num_rows[subband]; 
       row++)
    for (col = 0; 
         col < zerotree->num_cols[subband]; 
         col++)
      if (!QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
        {
          if (QccSQInverseScalarQuantization(channels
                                             [subband].channel_symbols
                                             [channel_index++],
                                             (subband) ? highpass_quantizer :
                                             baseband_quantizer,
                                             &(subband_images
                                               [subband].image[row][col])))
            {
              QccErrorAddMessage("(QccWAVsfqDecodeReconstructSubband): Error calling QccSQInverseScalarQuantization()");
              goto Error;
            }
        }
      else
        subband_images[subband].image[row][col] = 0.0;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVsfqDecode(QccBitBuffer *input_buffer,
                    QccIMGImageComponent *output_image,
                    QccWAVZerotree *zerotree,
                    const QccSQScalarQuantizer *baseband_quantizer,
                    const QccSQScalarQuantizer *highpass_quantizer,
                    int *high_tree_thresholds,
                    int *low_tree_thresholds,
                    QccChannel *channels,
                    const QccWAVWavelet *wavelet,
                    const QccWAVPerceptualWeights *perceptual_weights)
{
  int return_value;
  int num_subbands;
  int subband;
  QccIMGImageComponent *subband_images = NULL;
  QccWAVSubbandPyramid subband_pyramid;
  int *zerotree_symbols = NULL;
  QccENTArithmeticModel *model = NULL;
  int num_contexts;
  int *num_symbols = NULL;
  int zerotree_index;

  if ((output_image == NULL) ||
      (zerotree == NULL) ||
      (baseband_quantizer == NULL) ||
      (highpass_quantizer == NULL) ||
      (channels == NULL) ||
      (wavelet == NULL))
    return(0);
  
  num_subbands = zerotree->num_subbands;
  
  if ((zerotree_symbols = 
       (int *)malloc(sizeof(int) * 
                     zerotree->image_num_rows * zerotree->image_num_cols)) 
      == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error allocating memory");
      goto Error;
    }
    
  if (QccWAVsfqDisassembleBitstreamZerotree(num_subbands,
                                            high_tree_thresholds,
                                            low_tree_thresholds,
                                            zerotree_symbols,
                                            input_buffer))
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVsfqDisassembleBitstreamZerotree()");
      goto Error;
    }

  if ((subband_images =
       (QccIMGImageComponent *)malloc(sizeof(QccIMGImageComponent) * 
                                      num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error allocating memory");
      goto Error;
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      QccIMGImageComponentInitialize(&subband_images[subband]);
      subband_images[subband].num_rows = zerotree->num_rows[subband];
      subband_images[subband].num_cols = zerotree->num_cols[subband];
      if (QccIMGImageComponentAlloc(&(subband_images[subband])))
        {
          QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
    }
  
  num_contexts = zerotree->num_subbands;
  if ((num_symbols = (int *)malloc(sizeof(int) * num_contexts)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error allocating memory");
      goto Error;
    }
  for (subband = 0; subband < zerotree->num_subbands; subband++)
    num_symbols[subband] = channels[subband].alphabet_size;
  
  if ((model = 
       QccENTArithmeticDecodeStart(input_buffer,
                                   num_symbols,
                                   num_contexts,
                                   NULL,
                                   QCCENT_ANYNUMBITS)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  
  for (subband = 0, zerotree_index = 0; subband < num_subbands; subband++)
    {
      if (QccWAVsfqDecodeReconstructZerotree(zerotree,
                                             zerotree_symbols,
                                             &zerotree_index,
                                             high_tree_thresholds,
                                             low_tree_thresholds,
                                             subband,
                                             subband_images))
        {
          QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVsfqDecodeReconstructZerotree()");
          goto Error;
        }
      if (QccWAVsfqDisassembleBitstreamChannel(model,
                                               zerotree,
                                               channels,
                                               subband,
                                               input_buffer))
        {
          QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVsfqDisassembleBitstreamChannel()");
          goto Error;
        }

      if (QccWAVsfqDecodeReconstructSubband(zerotree,
                                            channels,
                                            subband,
                                            highpass_quantizer,
                                            baseband_quantizer,
                                            subband_images))
        {
          QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVsfqDecodeReconstructSubband()");
          goto Error;
        }
    }


  QccWAVSubbandPyramidInitialize(&subband_pyramid);
  subband_pyramid.num_rows = zerotree->image_num_rows;
  subband_pyramid.num_cols = zerotree->image_num_cols;
  subband_pyramid.num_levels = zerotree->num_levels;
  if (QccWAVSubbandPyramidAlloc(&subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (QccWAVSubbandPyramidAssembleFromImageComponent(subband_images,
                                                     &subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVSubbandPyramidAssembleFromImageComponent()");
      goto Error;
    }
  
  if (perceptual_weights != NULL)
    if (QccWAVPerceptualWeightsRemove(&subband_pyramid,
                                      perceptual_weights))
      {
        QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVPerceptualWeightsRemove()");
        QccErrorExit();
      }

  if (zerotree->num_levels)
    if (QccWAVSubbandPyramidInverseDWT(&subband_pyramid,
                                       wavelet))
      {
        QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVWaveletSynthesisSubbandPyramid()");
        goto Error;
      }
  
  if (QccWAVSubbandPyramidAddMean(&subband_pyramid,
                                  zerotree->image_mean))
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccWAVSubbandPyramidAddMean()");
      goto Error;
    }
  
  if (QccMatrixCopy(output_image->image,
                    subband_pyramid.matrix,
                    subband_pyramid.num_rows,
                    subband_pyramid.num_cols))
    {
      QccErrorAddMessage("(QccWAVsfqDecode): Error calling QccMatrixCopy()");
      goto Error;
    }
  
  QccIMGImageComponentSetMin(output_image);
  QccIMGImageComponentSetMax(output_image);

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (zerotree_symbols != NULL)
    QccFree(zerotree_symbols);
  if (subband_images != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccIMGImageComponentFree(&(subband_images[subband]));
      QccFree(subband_images);
    }
  QccWAVSubbandPyramidFree(&subband_pyramid);
  QccENTArithmeticFreeModel(model);
  if (num_symbols != NULL)
    QccFree(num_symbols);
  return(return_value);
}


int QccWAVsfqAssembleBitstreamHeader(const QccWAVZerotree *zerotree,
                                     const QccSQScalarQuantizer
                                     *mean_quantizer,
                                     const QccSQScalarQuantizer
                                     *baseband_quantizer,
                                     const QccSQScalarQuantizer
                                     *highpass_quantizer,
                                     const int *high_tree_thresholds,
                                     const int *low_tree_thresholds,
                                     QccBitBuffer *output_buffer,
                                     int *num_output_bits)
{
  int return_value;
  int mean_partition;
  int no_tree_prediction;
  int subband, row, col;
  int num_nonnull_symbols;

  if ((zerotree == NULL) || (output_buffer == NULL))
    return(0);
  
  no_tree_prediction =
    ((high_tree_thresholds == NULL) && (low_tree_thresholds == NULL));
  
  if (QccBitBufferPutChar(output_buffer,
                          (char)no_tree_prediction))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutChar()");
      goto Error;
    }
  if (QccBitBufferPutChar(output_buffer,
                          (char)zerotree->num_levels))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutChar()");
      goto Error;
    }
  if (QccBitBufferPutDouble(output_buffer,
                            baseband_quantizer->stepsize))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  if (QccBitBufferPutInt(output_buffer,
                         baseband_quantizer->num_levels))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  if (QccBitBufferPutDouble(output_buffer,
                            highpass_quantizer->stepsize))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  if (QccBitBufferPutInt(output_buffer,
                         highpass_quantizer->num_levels))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer,
                         zerotree->image_num_cols))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  if (QccBitBufferPutInt(output_buffer,
                         zerotree->image_num_rows))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccSQScalarQuantization(zerotree->image_mean,
                              mean_quantizer,
                              NULL,
                              &mean_partition))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstream): Error calling QccSQScalarQuantization()");
      goto Error;
    }
  if (QccBitBufferPutInt(output_buffer,
                         mean_partition))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  
  if (!no_tree_prediction)
    for (subband = 1;
         subband < zerotree->num_subbands - 3;
         subband++)
      {
        if (QccBitBufferPutInt(output_buffer,
                               high_tree_thresholds[subband]))
          {
            QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutInt()");
            goto Error;
          }
        if (QccBitBufferPutInt(output_buffer,
                               low_tree_thresholds[subband]))
          {
            QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutInt()");
            goto Error;
          }
      }
  
  for (num_nonnull_symbols = 0, subband = QCCWAVSFQ_ZEROTREE_STARTSUBBAND; 
       subband < zerotree->num_subbands - 3; subband++)
    for (row = 0; row < zerotree->num_rows[subband]; row++)
      for (col = 0; col < zerotree->num_cols[subband]; col++)
        if (!QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
          num_nonnull_symbols++;
  
  if (QccBitBufferPutInt(output_buffer,
                         num_nonnull_symbols))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (num_output_bits != NULL)
    *num_output_bits = output_buffer->bit_cnt;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVsfqDisassembleBitstreamHeader(QccWAVZerotree *zerotree,
                                        QccSQScalarQuantizer *mean_quantizer,
                                        QccSQScalarQuantizer 
                                        *baseband_quantizer,
                                        QccSQScalarQuantizer
                                        *highpass_quantizer,
                                        int *no_tree_prediction,
                                        QccBitBuffer *input_buffer)
{
  int return_value;
  unsigned char ch;
  int mean_partition;

  if ((zerotree == NULL) || (input_buffer == NULL))
    return(0);
  
  if (QccBitBufferGetChar(input_buffer,
                          &ch))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  *no_tree_prediction = (int)ch;
  
  if (QccBitBufferGetChar(input_buffer,
                          &ch))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  zerotree->num_levels = ch;
  zerotree->num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(zerotree->num_levels);
  
  if (QccBitBufferGetDouble(input_buffer,
                            &(baseband_quantizer->stepsize)))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  if (QccBitBufferGetInt(input_buffer,
                         &(baseband_quantizer->num_levels)))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(input_buffer,
                            &(highpass_quantizer->stepsize)))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  if (QccBitBufferGetInt(input_buffer,
                         &(highpass_quantizer->num_levels)))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer,
                         &(zerotree->image_num_cols)))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  if (QccBitBufferGetInt(input_buffer,
                         &(zerotree->image_num_rows)))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer,
                         &mean_partition))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccSQInverseScalarQuantization(mean_partition,
                                     mean_quantizer,
                                     &(zerotree->image_mean)))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccSQInverseScalarQuantization()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVsfqAssembleBitstreamZerotree(const QccWAVZerotree *zerotree, 
                                              QccBitBuffer *output_buffer)
{
  int return_value;
  int subband, row, col;
  
  if ((zerotree == NULL) || (output_buffer == NULL))
    return(0);
  
  for (subband = QCCWAVSFQ_ZEROTREE_STARTSUBBAND; 
       subband < zerotree->num_subbands - 3; subband++)
    for (row = 0; row < zerotree->num_rows[subband]; row++)
      for (col = 0; col < zerotree->num_cols[subband]; col++)
        if (!QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
          if (QccBitBufferPutBit(output_buffer,
                                 (zerotree->zerotree[subband][row][col] ==
                                  QCCWAVZEROTREE_SYMBOLSIGNIFICANT)))
            {
              QccErrorAddMessage("(QccWAVsfqAssembleBitstreamZerotree): Error calling QccBitBufferPutBit()");
              goto Error;
            }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVsfqDisassembleBitstreamZerotree(int num_subbands,
                                          int *high_tree_thresholds,
                                          int *low_tree_thresholds,
                                          int *zerotree_symbols,
                                          QccBitBuffer *input_buffer)
{
  int return_value;
  int subband;
  int bit_value;
  int symbol, num_symbols;
  
  if ((high_tree_thresholds != NULL) && (low_tree_thresholds != NULL))
    {
      for (subband = 1;
           subband < num_subbands - 3;
           subband++)
        {
          if (QccBitBufferGetInt(input_buffer,
                                 &(high_tree_thresholds[subband])))
            {
              QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetInt()");
              goto Error;
            }
          if (QccBitBufferGetInt(input_buffer,
                                 &(low_tree_thresholds[subband])))
            {
              QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetInt()");
              goto Error;
            }
        }
      
      for (subband = 1;
           subband < num_subbands - 3;
           subband++)
        printf("Subband:%d %d %d\n",
               subband,
               high_tree_thresholds[subband], low_tree_thresholds[subband]);
    }
  
  if (QccBitBufferGetInt(input_buffer,
                         &num_symbols))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  for (symbol = 0; symbol < num_symbols; symbol++)
    {
      if (QccBitBufferGetBit(input_buffer, &bit_value))
        {
          QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamZerotree): Error calling QccBitBufferGetBit()");
          goto Error;
        }
      switch (bit_value)
        {
        case 0:
          zerotree_symbols[symbol] =
            QCCWAVZEROTREE_SYMBOLZTROOT;
          break;
        case 1:
          zerotree_symbols[symbol] =
            QCCWAVZEROTREE_SYMBOLSIGNIFICANT;
          break;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVsfqAssembleBitstreamChannels(const QccWAVZerotree *zerotree, 
                                              const QccChannel *channels, 
                                              QccBitBuffer *output_buffer)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  int num_contexts;
  int *num_symbols = NULL;
  int subband;
  
  num_contexts = zerotree->num_subbands;
  if ((num_symbols = (int *)malloc(sizeof(int) * num_contexts)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamChannels): Error allocating memory");
      goto Error;
    }
  for (subband = 0; subband < zerotree->num_subbands; subband++)
    num_symbols[subband] = channels[subband].alphabet_size;
  
  if ((model = 
       QccENTArithmeticEncodeStart(num_symbols,
                                   num_contexts,
                                   NULL,
                                   QCCENT_ANYNUMBITS)) == NULL)
    {
      QccErrorAddMessage("QccWAVsfqAssembleBitstreamChannels): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  
  for (subband = 0; subband < zerotree->num_subbands; subband++)
    {
      if (QccENTArithmeticSetModelContext(model, subband))
        {
          QccErrorAddMessage("(QccWAVsfqAssembleBitstreamChannels): Error calling QccENTArithmeticSetModelContext()");
          goto Error;
        }
      
      if (QccENTArithmeticEncode(channels[subband].channel_symbols, 
                                 channels[subband].channel_length,
                                 model,
                                 output_buffer))
        {
          QccErrorAddMessage("(QccWAVsfqAssembleBitstreamChannels): Error calling QccENTArithmeticEncode()");
          goto Error;
        }
    }
  
  if (QccENTArithmeticEncodeEnd(model,
                                model->current_context,
                                output_buffer))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstreamChannels): Error calling QccENTArithmeticEncodeEnd()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (model != NULL)
    QccENTArithmeticFreeModel(model);
  if (num_symbols != NULL)
    QccFree(num_symbols);
  return(return_value);
}


int QccWAVsfqDisassembleBitstreamChannel(QccENTArithmeticModel *model,
                                         QccWAVZerotree *zerotree, 
                                         QccChannel *channels, 
                                         int subband,
                                         QccBitBuffer *input_buffer)
{
  int return_value;
  int row, col;

  if (QccENTArithmeticSetModelContext(model, subband))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamChannel): Error calling QccArithmeticSetModelContext()");
      goto Error;
    }

  channels[subband].channel_length = 0;

  for (row = 0; row < zerotree->num_rows[subband]; row++)
    for (col = 0; col < zerotree->num_cols[subband]; col++)
      if (!QccWAVZerotreeNullSymbol(zerotree->zerotree[subband][row][col]))
        channels[subband].channel_length++;

  channels[subband].access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
  if (QccChannelAlloc(&channels[subband]))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamChannel): Error calling QccChannelAlloc()");
      goto Error;
    }

  if (QccENTArithmeticDecode(input_buffer,
                             model,
                             channels[subband].channel_symbols,
                             channels[subband].channel_length))
    {
      QccErrorAddMessage("(QccWAVsfqDisassembleBitstreamChannel): Error calling QccENTArithmeticDecode()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVsfqAssembleBitstream(const QccWAVZerotree *zerotree,
                               const QccSQScalarQuantizer *mean_quantizer,
                               const QccSQScalarQuantizer *baseband_quantizer,
                               const QccSQScalarQuantizer *highpass_quantizer,
                               const QccChannel *channels,
                               const int *high_tree_thresholds,
                               const int *low_tree_thresholds,
                               QccBitBuffer *output_buffer,
                               double *rate_header,
                               double *rate_zerotree,
                               double *rate_channels)
{
  int return_value;
  int bits_header, bits_zerotree, bits_channels;
  int num_pixels;
  
  if ((zerotree == NULL) ||
      (channels == NULL) ||
      (output_buffer == NULL))
    return(0);
  
  if (QccWAVsfqAssembleBitstreamHeader(zerotree, 
                                       mean_quantizer, 
                                       baseband_quantizer,
                                       highpass_quantizer,
                                       high_tree_thresholds,
                                       low_tree_thresholds,
                                       output_buffer,
                                       &bits_header))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstream): Error calling QccWAVsfqAssembleBitstreamHeader()");
      goto Error;
    }
  
  if (QccWAVsfqAssembleBitstreamZerotree(zerotree, output_buffer))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstream): Error calling QccWAVsfqAssembleBitstreamZerotree()");
      goto Error;
    }
  
  bits_zerotree = output_buffer->bit_cnt - bits_header;
  
  if (QccWAVsfqAssembleBitstreamChannels(zerotree, channels, output_buffer))
    {
      QccErrorAddMessage("(QccWAVsfqAssembleBitstream): Error calling QccWAVsfqAssembleBitstreamChannels()");
      goto Error;
    }
  
  bits_channels = output_buffer->bit_cnt - bits_zerotree;
  
  num_pixels = zerotree->image_num_rows * zerotree->image_num_cols;
  
  if (rate_header != NULL)
    *rate_header = (double)bits_header / num_pixels;
  if (rate_zerotree != NULL)
    *rate_zerotree = (double)bits_zerotree / num_pixels;
  if (rate_channels != NULL)
    *rate_channels = (double)bits_channels / num_pixels;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}

