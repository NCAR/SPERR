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

/* 
 *
 * Written by
 *
 * Chao Tian, at Cornell University, 2003 
 *
 */


#include "libQccPack.h"


#define QCCTCE_BOUNDARY_VALUE (1e-6)

//definition used in context information
#define QCCTCE_Z 0 //zero, AKA, insignificant
#define QCCTCE_NZN_NEW 2 //non-zero-neighbor
#define QCCTCE_S 3 //significant
#define QCCTCE_S_NEW 4
#define QCCTCE_NZN 5

// more refinement can be made if direction of Tarp filter is considered
// but the improvement is minor
#define QCCWAVTCE_ALPHA 0.4 
#define QCCWAVTCE_ALPHA_HIGH 0.4
#define QCCWAVTCE_ALPHA_LOW 0.4

//ugly fix, use 1-D IIR filter to provide PMF estimate
#define QCCWAVTCE_ALPHA_1D 0.995 
#define QCCWAVTCE_ALPHA_1D_O 0.005

// this should be 0.5, however, 0.3 seems to work a little better (minor)
// reason unknown
#define QCCWAVTCE_REFINE_HOLDER 0.3 

// threshold for cross-scale prediction...
#define QCCWAVTCE_PREDICT_THRESHOLD 0.05  
//weight factor for cross-scale and current scale
#define QCCWAVTCE_CURRENT_SCALE 0.7 
#define QCCWAVTCE_PARENT_SCALE 0.3


static int QccWAVtceUpdateModel(QccENTArithmeticModel *model, double prob)
{
  double probabilities[2]; 
  probabilities[1] = prob;
  probabilities[0] = 1 - probabilities[1];
  
  if (QccENTArithmeticSetModelProbabilities(model,probabilities,0))
    {
      QccErrorAddMessage("(QccWAVtceUpdateModel): Error calling QccENTArithmeticSetModelProbabilities()");
      return(1);
    } 
  return(0);
}


static int QccWAVtceEncodeDWT(QccWAVSubbandPyramid *image_subband_pyramid,
                              char **sign_array,
                              const QccIMGImageComponent *image,
                              int num_levels,
                              double *image_mean,
                              int *max_coefficient_bits,
                              double stepsize,
                              const QccWAVWavelet *wavelet)
{
  double coefficient_magnitude;
  double max_coefficient;
  int num_subbands,subband;
  int row, col;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_rows;
  int subband_num_cols;
  
  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);
  
  if (QccMatrixCopy(image_subband_pyramid->matrix,
                    image->image,
                    image->num_rows,
                    image->num_cols))
    {
      QccErrorAddMessage("(QccWAVtceEncodeDWT): Error calling QccMatrixCopy()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidSubtractMean(image_subband_pyramid,
                                       image_mean,
                                       NULL))
    {
      QccErrorAddMessage("(QccWAVtceEncodeDWT): Error calling QccWAVSubbandPyramidSubtractMean()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidDWT(image_subband_pyramid,
                              num_levels,
                              wavelet))
    {
      QccErrorAddMessage("(QccWAVtceEncodeDWT): Error calling QccWAVSubbandPyramidDWT()");
      return(1);
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(image_subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVtceEncodeDWT): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }

      if (QccWAVSubbandPyramidSubbandOffsets(image_subband_pyramid,
                                             subband,
                                             &subband_origin_row,
                                             &subband_origin_col))
        {
          QccErrorAddMessage("(QccWAVtceEncodeDWT): Error calling QccWAVSubbandPyramidSubbandOffsets()");
          return(1);
        }
      max_coefficient = -MAXFLOAT;
      
      if (stepsize > 0)
        for (row = 0; row < subband_num_rows; row++)
          for (col = 0; col < subband_num_cols; col++)
            {
              coefficient_magnitude =
                floor(fabs(image_subband_pyramid->matrix
                           [subband_origin_row + row]
                           [subband_origin_col + col] / stepsize));
              
              if (image_subband_pyramid->matrix
                  [subband_origin_row + row]
                  [subband_origin_col + col] < 0)
                sign_array
                  [subband_origin_row + row]
                  [subband_origin_col + col] = 1;
              else
                sign_array
                  [subband_origin_row + row]
                  [subband_origin_col + col] = 0;
              
              image_subband_pyramid->matrix
                [subband_origin_row + row]
                [subband_origin_col + col] = coefficient_magnitude;     
              
              if (coefficient_magnitude > max_coefficient)
                max_coefficient = coefficient_magnitude;     
            }
      else
        for (row = 0; row < subband_num_rows; row++)
          for (col = 0; col < subband_num_cols; col++)
            {
              coefficient_magnitude =
                fabs(image_subband_pyramid->matrix
                     [subband_origin_row + row]
                     [subband_origin_col + col]);
              
              if (image_subband_pyramid->matrix
                  [subband_origin_row + row]
                  [subband_origin_col + col] < 0)
                sign_array
                  [subband_origin_row + row]
                  [subband_origin_col + col] = 1;
              else
                sign_array
                  [subband_origin_row + row]
                  [subband_origin_col + col] = 0;
              
              image_subband_pyramid->matrix
                [subband_origin_row + row]
                [subband_origin_col + col] = coefficient_magnitude;     

              if (coefficient_magnitude > max_coefficient)
                max_coefficient = coefficient_magnitude;
            }

      max_coefficient_bits[subband] = 
        (int)floor(QccMathMax(0, QccMathLog2(max_coefficient + 0.0001)));  

      if (max_coefficient_bits[0] < max_coefficient_bits[subband])
	max_coefficient_bits[0] = max_coefficient_bits[subband];
    }   

  return(0);
}


static int QccWAVtceEncodeBitPlaneInfo(QccBitBuffer *output_buffer,
                                       int num_subbands,
                                       int *max_coefficient_bits)
{
  int subband;
  int num_zeros = 0;
  
  for (subband = 1; subband < num_subbands; subband++)
    {
      for (num_zeros = max_coefficient_bits[0] - max_coefficient_bits[subband];
           num_zeros > 0;
           num_zeros--)
        if (QccBitBufferPutBit(output_buffer, 0))
          return(1);

      if (QccBitBufferPutBit(output_buffer, 1))
        return(1);
    }

  return(0);
}


static int QccWAVtceDecodeBitPlaneInfo(QccBitBuffer *input_buffer,
                                       int num_subbands,
                                       int *max_bits,
                                       int max_coefficient_bits)
{
  int subband;
  int bit_value;
  
  max_bits[0] = max_coefficient_bits;
  
  for (subband = 1; subband < num_subbands; subband++)
    {
      max_bits[subband] = max_coefficient_bits;
      do
        {
          if (QccBitBufferGetBit(input_buffer,&bit_value))
            return(1);
          max_bits[subband]--;
        }
      while (bit_value == 0);
      max_bits[subband]++;  
    }
  
  return(0);
}


int QccWAVtceEncodeHeader(QccBitBuffer *output_buffer, 
                          int num_levels, 
                          int num_rows, int num_cols,
                          double image_mean,
                          double stepsize,
                          int max_coefficient_bits)
{
  int return_value;
  
  if (QccBitBufferPutChar(output_buffer, (unsigned char)num_levels))
    {
      QccErrorAddMessage("(QccWAVtceEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVtceEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVtceEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(output_buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVtceEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(output_buffer, stepsize))
    {
      QccErrorAddMessage("(QccWAVtceEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtceEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVtceRevEst(QccWAVSubbandPyramid *coefficients,
                           char **significance_map,
                           int subband,
                           double *subband_significance,
                           double **p)
{
  
  int return_value;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_rows;
  int subband_num_cols;
  int row, col;
  int current_row, current_col;
  double p1, p3;
  QccVector p2 = NULL;
  char *p_char;
  int v;  
  double filter_coef;
  double alpha_v, alpha_h;
  
  if (subband % 3==1)
    { 
      //LH horizental more important?
      alpha_h = QCCWAVTCE_ALPHA_HIGH;
      alpha_v = QCCWAVTCE_ALPHA_LOW;
    }
  else
    {
      if (subband % 3==2)
        {
          alpha_h = QCCWAVTCE_ALPHA_LOW;
          alpha_v = QCCWAVTCE_ALPHA_HIGH;   
        }
      else
        {
          alpha_h = QCCWAVTCE_ALPHA;
          alpha_v = QCCWAVTCE_ALPHA;
        }
    }
  
  filter_coef = (1 - alpha_h) * (1 - alpha_v) / (2*alpha_h + 2*alpha_v);
  
  subband_significance[subband] = 0.0;
  
  if (QccWAVSubbandPyramidSubbandSize(coefficients,
                                      subband,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVtceRevEst): Error calling QccWAVSubbandPyramidSubbandSize()");
      goto Error;
    }
  if (QccWAVSubbandPyramidSubbandOffsets(coefficients,
                                         subband,
                                         &subband_origin_row,
                                         &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVtceRevEst): Error calling QccWAVSubbandPyramidSubbandOffsets()");
      goto Error;
    }
  
  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceRevEst): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  p1 = QccMathMax(QCCTCE_BOUNDARY_VALUE,
                  ((double)subband_significance[subband]) / 
                  (subband_num_rows*subband_num_cols)) *
    (1 + alpha_h) / (2 * alpha_v + 2 * alpha_h);

  for (col = 0; col < subband_num_cols; col++)
    p2[col] = p1;
  
  for (row = subband_num_rows - 1; row >= 0; row--)
    {
      p1 = p2[0]*(1 - alpha_v)/(1 + alpha_h);
      current_row = subband_origin_row + row;
      for (col = subband_num_cols - 1; col >= 0; col--)
        {       
          current_col = subband_origin_col + col;
          p_char = &(significance_map[current_row][current_col]);
          p[current_row][current_col] = alpha_h * p1 + alpha_v*p2[col];
          if (*p_char == QCCTCE_S)
            {
              p1 = alpha_h * p1 + filter_coef * QCCWAVTCE_REFINE_HOLDER;
              subband_significance[subband] += QCCWAVTCE_REFINE_HOLDER;
            }
          else
            {
              v = (*p_char == QCCTCE_S_NEW);
              p1 = alpha_h * p1 + filter_coef * v;
              subband_significance[subband] += v;
            }   
          p2[col] = p1 + alpha_v * p2[col];   
        }
      
      p3 = p2[0]*(1 - alpha_v)/(1 + alpha_h);
      for (col = 0; col < subband_num_cols; col++)
        {
          p_char = &(significance_map[current_row][subband_origin_col + col]);
          p2[col] = p2[col] + alpha_h * p3;
          if (*p_char == QCCTCE_S)
            p3 = alpha_h * p3 + filter_coef * QCCWAVTCE_REFINE_HOLDER;
          else
            {
              v = (*p_char == QCCTCE_S_NEW);
              p3 = alpha_h * p3 + filter_coef * v;
            }        
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(p2);
  return(return_value);
}


static int QccWAVUpdateNZNStatus(int subband_origin_row,
                                 int subband_origin_col,
                                 int subband_num_row,
                                 int subband_num_col,
                                 int row,
                                 int col,
                                 char** significance_map,
                                 int subband)
{
  //Updata neighbors
  int current_row, current_col;
  int shifted_row, shifted_col;
  current_row = subband_origin_row + row;
  current_col = subband_origin_col + col;
  
  if (row > 0)
    {  
      shifted_row = current_row - 1;
      shifted_col = current_col;
      if (significance_map[shifted_row][shifted_col] < QCCTCE_S)
        significance_map[shifted_row][shifted_col] = QCCTCE_NZN_NEW; 
      
      if (col > 0)
        {
          shifted_row = current_row - 1;
          shifted_col = current_col - 1;
          if (significance_map[shifted_row][shifted_col] < QCCTCE_S)
            significance_map[shifted_row][shifted_col] = QCCTCE_NZN_NEW;   
        }
      if (col < subband_num_col - 1)
        {
          shifted_row = current_row - 1;
          shifted_col = current_col + 1;
          if (significance_map[shifted_row][shifted_col] < QCCTCE_S)
            significance_map[shifted_row][shifted_col] = QCCTCE_NZN_NEW;   
        }
    }
  if (row < subband_num_row - 1)
    {
      shifted_row = current_row + 1;
      shifted_col = current_col;
      if (significance_map[shifted_row][shifted_col] < QCCTCE_S)
        significance_map[shifted_row][shifted_col] = QCCTCE_NZN_NEW;
      
      if (col > 0)
        {
          shifted_row = current_row + 1;
          shifted_col = current_col - 1;
          if (significance_map[shifted_row][shifted_col] < QCCTCE_S)
            significance_map[shifted_row][shifted_col] = QCCTCE_NZN_NEW;   
        }
      if (col < subband_num_col - 1)
        {
          shifted_row = current_row + 1;
          shifted_col = current_col + 1;
          if (significance_map[shifted_row][shifted_col] < QCCTCE_S)
            significance_map[shifted_row][shifted_col] = QCCTCE_NZN_NEW;   
        }
    }
  if (col > 0)
    {
      shifted_row = current_row;
      shifted_col = current_col - 1;  
      if (significance_map[shifted_row][shifted_col] < QCCTCE_S)
        significance_map[shifted_row][shifted_col] = QCCTCE_NZN_NEW;
      
    }
  if (col < subband_num_col - 1)
    {
      shifted_row = current_row;
      shifted_col = current_col + 1;
      if (significance_map[shifted_row][shifted_col] < QCCTCE_S)
        significance_map[shifted_row][shifted_col] = QCCTCE_NZN_NEW;
    }

  return(0);
}


static int QccWAVtceIPBand(QccWAVSubbandPyramid *coefficients,
                           char **significance_map,
                           char **sign_array,
                           double **p_estimation,
                           double *subband_significance,
                           int subband,
                           double threshold,
                           QccENTArithmeticModel *model,
                           QccBitBuffer *buffer)
{
  
  int return_value;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_rows;
  int subband_num_cols;
  int row, col;
  int current_row, current_col;
  double p1;
  double p3;
  double p;
  double p_forward;
  double p_parent;
  QccVector p2 = NULL;
  int parent_subband;   
  double scale;
  double parent_density;
  double child_density; 
  int v;
  int symbol;
  double p_lowerbound = 0;
  double weight[2];
  double filter_coef;
  double increment;
  char *p_char;
  double alpha_v, alpha_h;
  
  if (subband % 3==1)
    { 
      //LH horizental more important
      alpha_h = QCCWAVTCE_ALPHA_HIGH;
      alpha_v = QCCWAVTCE_ALPHA_LOW;
    }
  else
    {
      if (subband % 3==2)
        {
          alpha_h = QCCWAVTCE_ALPHA_LOW;
          alpha_v = QCCWAVTCE_ALPHA_HIGH;   
        }
      else
        {
          alpha_h = QCCWAVTCE_ALPHA;
          alpha_v = QCCWAVTCE_ALPHA;
        }
    }
  
  // initialization all the constants
  filter_coef = (1 - alpha_h)*(1 - alpha_v) / (2*alpha_h + 2*alpha_v);
  weight[0] = (1 - alpha_h) * (1 - alpha_v)/((1 + alpha_h)*(1 + alpha_v));
  weight[1] = (2*alpha_h + 2*alpha_v) / ((1 + alpha_h)*(1 + alpha_v));
  
  if (QccWAVSubbandPyramidSubbandSize(coefficients,
                                      subband,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVtceIPBand): Error calling QccWAVSubbandPyramidSubbandSize()");
      goto Error;
    }
  if (QccWAVSubbandPyramidSubbandOffsets(coefficients,
                                         subband,
                                         &subband_origin_row,
                                         &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVtceIPBand): Error calling QccWAVSubbandPyramidSubbandOffsets()");
      goto Error;
    }
  
  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceIPBand): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  child_density =
    subband_significance[subband] / (subband_num_rows * subband_num_cols);
  p_lowerbound = 1.0 / (subband_num_rows * subband_num_cols);
  
  if (subband <= 3)
    {
      parent_subband = 0;
      parent_density =
        subband_significance[0] / (subband_num_rows * subband_num_cols);
    }
  else
    {
      parent_subband = subband - 3;
      parent_density =
        subband_significance[parent_subband] /
        (subband_num_rows * subband_num_cols) * 4;
    }
  
  for (col = 0; col < subband_num_cols; col++)
    p2[col] = p_estimation[subband_origin_row][subband_origin_col + col] *
      2 * (1 + alpha_h) / (2*alpha_v + 2*alpha_h);
  
  scale =
    QccMathMax(child_density, p_lowerbound) /
    QccMathMax(parent_density, p_lowerbound); 
  increment =
    1.0 / (subband_num_rows * subband_num_cols) /
    QccMathMax(parent_density, p_lowerbound);
  
  for (row = 0; row < subband_num_rows; row++)
    {    
      p1 = p2[0]*(1 - alpha_v)/(1 + alpha_h);
      current_row = subband_origin_row + row;
      for (col = 0; col < subband_num_cols; col++)
        {   
          current_col = subband_origin_col + col;
          p_char = &(significance_map[current_row][current_col]);
          p_forward = alpha_h*p1 + alpha_v*p2[col];
          
          if (*p_char < QCCTCE_S)
            {      
              p = p_forward + p_estimation[current_row][current_col];
              if (subband != 0)
                { 
                  if (subband <= 3)
                    p_parent = p_estimation[row][col];
                  else
                    p_parent = p_estimation[current_row/2][current_col/2];
                  if (p < QCCWAVTCE_PREDICT_THRESHOLD)
                    {          
                      p_parent = QccMathMin(p_parent*scale, 0.8);
                      p =
                        p * QCCWAVTCE_CURRENT_SCALE +
                        QCCWAVTCE_PARENT_SCALE * p_parent;
                    }
                }
              
              if (QccWAVtceUpdateModel(model, p))
                {
                  QccErrorAddMessage("(QccWAVtceIPBand): Error calling QccWAVtceUpdateModel()");
                  return_value = 1;
                  goto Error;
                }
              if (buffer->type == QCCBITBUFFER_OUTPUT)
                {
                  if (coefficients->matrix[current_row][current_col] >=
                      threshold)
                    {
                      symbol = 1;
                      coefficients->matrix[current_row][current_col] -=
                        threshold;
                    }
                  else
                    symbol = 0;
                  return_value =
                    QccENTArithmeticEncode(&symbol, 1,model, buffer);
                }
              else
                {
                  if (QccENTArithmeticDecode(buffer,model,&symbol, 1))
                    {
                      return_value = 2;
                      goto Return;
                    }
                  else
                    return_value = 0;
                  
                  if (symbol)
                    coefficients->matrix[current_row][current_col] =
                      1.5 * threshold;
                }
              v = symbol;
              
              if (symbol)
                {
                  subband_significance[subband]++;
                  scale += increment;
                  *p_char = QCCTCE_S_NEW;
                  
                  if (QccWAVtceUpdateModel(model, 0.5))
                    {
                      QccErrorAddMessage("(QccWAVtceIPBand): Error calling QccWAVtceUpdateModel()");
                      goto Error;
                    }
                  
                  if (buffer->type == QCCBITBUFFER_OUTPUT)
                    {
                      symbol = (int)(sign_array[current_row][current_col]);
                      return_value =
                        QccENTArithmeticEncode(&symbol, 1,model, buffer);      
                    }
                  else
                    {
                      if (QccENTArithmeticDecode(buffer,model,&symbol, 1))
                        return_value = 2;      
                      sign_array[current_row][current_col] = (char)symbol;
                    }
                  QccWAVUpdateNZNStatus(subband_origin_row,
                                        subband_origin_col,
                                        subband_num_rows,
                                        subband_num_cols,
                                        row,
                                        col,
                                        significance_map,
                                        subband);
                }    
              
              if (return_value == 2)
                goto Return;
              else
                {
                  if (return_value)
                    {
                      QccErrorAddMessage("(QccWAVtceIPBand): QccWAVtceIPBand()");
                      goto Error;
                    }
                }    
              p1 = alpha_h * p1 + filter_coef * v;
              p2[col] = p1 + alpha_v * p2[col];
            }
          else
            {    
              p1 = alpha_h * p1 + filter_coef * QCCWAVTCE_REFINE_HOLDER;
              p2[col] = p1 + alpha_v * p2[col];
            }
          p_estimation[subband_origin_row + row][subband_origin_col + col] =
            p_forward;
        }
      
      p3 = p2[subband_num_cols-1]*(1 - alpha_v)/(1 + alpha_h);  
      for (col = subband_num_cols - 1; col >= 0; col--)
        {   
          p2[col] = p2[col] + alpha_h * p3;
          p_char = &(significance_map[current_row][subband_origin_col + col]);
          if (*p_char == QCCTCE_S)
            p3 = alpha_h * p3 + filter_coef * QCCWAVTCE_REFINE_HOLDER;  
          else
            {
              v = (*p_char == QCCTCE_S_NEW);
              p3 = alpha_h * p3 + filter_coef * v;
            }
        }
    }
  
  for (row = subband_num_rows - 1; row >= 0; row--)
    {
      p1 = p2[subband_num_cols-1]*(1 - alpha_v)/(1 + alpha_h); 
      current_row = subband_origin_row + row;
      for (col = subband_num_cols-1; col >=0; col--)
        {   
          current_col = subband_origin_col + col; 
          p_char = &(significance_map[current_row][current_col]);
          p_estimation[current_row][current_col] +=
            alpha_h*p1 + alpha_v*p2[col];
          if (*p_char == QCCTCE_S)
            {
              p_estimation[current_row][current_col]=
                QCCWAVTCE_REFINE_HOLDER * weight[0] +
                p_estimation[current_row][current_col] * weight[1];
              p1 = alpha_h * p1 + filter_coef * QCCWAVTCE_REFINE_HOLDER;
            }
          else
            {
              v = (*p_char == QCCTCE_S_NEW);
              p_estimation[current_row][current_col] =
                v * weight[0] +
                p_estimation[current_row][current_col] * weight[1];
              p1 = alpha_h * p1 + filter_coef * v;
            }    
          p2[col] = p1 + alpha_v * p2[col];   
        }
      
      p3 = p2[0]*(1 - alpha_v)/(1 + alpha_h);
      for (col = 0; col < subband_num_cols; col++)
        {
          p2[col] = p2[col] + alpha_h * p3;
          current_col = subband_origin_col + col; 
          p_char = &(significance_map[current_row][current_col]);
          if (*p_char == QCCTCE_S)
            p3 = alpha_h * p3 + filter_coef * QCCWAVTCE_REFINE_HOLDER;
          else
            {
              v = (*p_char == QCCTCE_S_NEW);
              p3 = alpha_h * p3 + filter_coef * v;
            }        
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return: 
  QccVectorFree(p2); 
  return(return_value); 
}


static int QccWAVtceIPPass(QccWAVSubbandPyramid *coefficients,    
                           char **significance_map,
                           char **sign_array,
                           double threshold,
                           double **p_estimation,
                           double *subband_significance,
                           QccENTArithmeticModel *model,
                           QccBitBuffer *buffer,
                           int *max_coefficient_bits)
{
  int subband,num_subbands; 
  int return_value;
  
  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(coefficients->num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)max_coefficient_bits[subband]))
        {
          return_value =
            QccWAVtceRevEst(coefficients,
                            significance_map,
                            subband,
                            subband_significance,
                            p_estimation);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVtceIPPass): Error calling QccWAVtceRevEst()");
              return(1);
            }
          else
            {
              if (return_value == 2)
                return(2);
            }
          
          return_value = QccWAVtceIPBand(coefficients,
                                         significance_map,
                                         sign_array,
                                         p_estimation,
                                         subband_significance,
                                         subband,
                                         threshold,
                                         model,
                                         buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVtceIPPass): Error calling QccWAVtceIPBand()");
              return(1);
            }
          else
            {
              if (return_value == 2)
                return(2);
            }
        }
    } 

  return(0);
}


static int QccWAVtceNZNPass(QccWAVSubbandPyramid *coefficients,    
                            char **significance_map,
                            char **sign_array,
                            double threshold,
                            double** p_estimation,
                            double *subband_significance,
                            QccENTArithmeticModel *model,
                            QccBitBuffer *buffer,
                            int *max_coefficient_bits)
{ 
  int subband,num_subbands;
  int return_value;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_rows;
  int subband_num_cols;
  int row, col;
  int current_row, current_col;
  int symbol;
  double p = 0.5;
  
  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(coefficients->num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)(max_coefficient_bits[subband]-1)))
        {   
          if (QccWAVSubbandPyramidSubbandSize(coefficients,
                                              subband,
                                              &subband_num_rows,
                                              &subband_num_cols))
            {
              QccErrorAddMessage("(QccWAVtceNZNPass): Error calling QccWAVSubbandPyramidSubbandSize()");
              return(1);
            }
          if (QccWAVSubbandPyramidSubbandOffsets(coefficients,
                                                 subband,
                                                 &subband_origin_row,
                                                 &subband_origin_col))
            {
              QccErrorAddMessage("(QccWAVtceNZNPass): Error calling QccWAVSubbandPyramidSubbandOffsets()");
              return(1);
            }
          
          for (row = 0; row < subband_num_rows; row++)
            {
              current_row = subband_origin_row + row;
              for (col = 0; col < subband_num_cols; col++)
                { 
                  current_col = subband_origin_col + col;
                  if (significance_map[current_row][current_col] == QCCTCE_NZN)
                    {
                      if (QccWAVtceUpdateModel(model, p))
                        {
                          QccErrorAddMessage("(QccWAVtceNZNPass): Error calling QccWAVtceUpdateModel()");
                          return_value = 1;
                          return(1);
                        }
                      if (buffer->type == QCCBITBUFFER_OUTPUT)
                        {
                          if (coefficients->matrix[current_row][current_col] >=
                              threshold)
                            {
                              symbol = 1;
                              coefficients->matrix[current_row][current_col] -=
                                threshold;
                            }
                          else
                            symbol = 0;
                          
                          return_value =
                            QccENTArithmeticEncode(&symbol, 1, model, buffer);
                          if (return_value == 1)
                            return(1);
                          else
                            {
                              if (return_value == 2)
                              return(2);
                            }
                        }
                      else
                        {
                          if (QccENTArithmeticDecode(buffer,
                                                     model,
                                                     &symbol,
                                                     1))   
                            return(2);
                          
                          if (symbol)
                            coefficients->matrix[current_row][current_col] =
                              1.5 * threshold;
                        }

                      p =
                        p * QCCWAVTCE_ALPHA_1D + symbol * QCCWAVTCE_ALPHA_1D_O;
                      if (symbol)
                        {
                          subband_significance[subband]++;
                          significance_map[current_row][current_col] =
                            QCCTCE_S_NEW;
                          
                          if (QccWAVtceUpdateModel(model, 0.5))
                            {
                              QccErrorAddMessage("(QccWAVtceNZNPass): Error calling QccWAVtceUpdateModel()");
                              return(1);
                            }
                          
                          if (buffer->type == QCCBITBUFFER_OUTPUT)
                            {
                              symbol =
                                (int)(sign_array[current_row][current_col]);
                              
                              return_value =
                                QccENTArithmeticEncode(&symbol,
                                                       1,
                                                       model,
                                                       buffer);
                              if (return_value == 1)
                                return(1);
                              else
                                {
                                  if (return_value == 2)
                                    return(2);
                                }       
                            }
                          else
                            {
                              if (QccENTArithmeticDecode(buffer,
                                                         model,
                                                         &symbol,
                                                         1))
                                return(2);
                              
                              sign_array[current_row][current_col] =
                                (char)symbol;
                            }
                          
                          QccWAVUpdateNZNStatus(subband_origin_row,
                                                subband_origin_col,
                                                subband_num_rows,
                                                subband_num_cols,
                                                row,
                                                col,
                                                significance_map,
                                                subband);
                        }
                    }     
                }
            }
        }
    }

  return(0);
}


int static QccWAVtceSPass(QccWAVSubbandPyramid *coefficients,    
                          char **significance_map,
                          double threshold,
                          QccENTArithmeticModel *model,
                          QccBitBuffer *buffer,
                          int *max_coefficient_bits)
{
  
  int subband,num_subbands; 
  int return_value;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_rows;
  int subband_num_cols;
  int row, col;
  int current_row, current_col;
  char *p_char;
  int symbol;
  double p = 0.4;
  
  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(coefficients->num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)(max_coefficient_bits[subband])))
        {
          if (QccWAVSubbandPyramidSubbandSize(coefficients,
                                              subband,
                                              &subband_num_rows,
                                              &subband_num_cols))
            {
              QccErrorAddMessage("(QccWAVtceSPass): Error calling QccWAVSubbandPyramidSubbandSize()");
              return(1);
            }
          if (QccWAVSubbandPyramidSubbandOffsets(coefficients,
                                                 subband,
                                                 &subband_origin_row,
                                                 &subband_origin_col))
            {
              QccErrorAddMessage("(QccWAVtceSPass): Error calling QccWAVSubbandPyramidSubbandOffsets()");
              return(1);
            }
          
          for (row = 0; row < subband_num_rows; row++)
            {
              current_row = subband_origin_row + row;
              for (col = 0; col < subband_num_cols; col++)
                {     
                  current_col = subband_origin_col + col;
                  p_char = &(significance_map[current_row][current_col]);
                  if (*p_char == QCCTCE_S)
                    { 
                      
                      if (QccWAVtceUpdateModel(model, p))
                        {
                          QccErrorAddMessage("(QccWAVtceSPass): Error calling QccWAVtceUpdateModel()");
                          return(1);
                        }
                      
                      if (buffer->type == QCCBITBUFFER_OUTPUT)
                        {
                          if (coefficients->matrix[current_row][current_col] >=
                              threshold)
                            {
                              symbol = 1;
                              coefficients->matrix[current_row][current_col] -=
                                threshold;
                            }
                          else
                            symbol = 0;
                          
                          return_value =
                            QccENTArithmeticEncode(&symbol, 1, model, buffer);
                          if (return_value == 1)
                            return(1);
                          else
                            {
                              if (return_value == 2)
                                return(2);
                            }
                        }
                      else
                        {
                          if (QccENTArithmeticDecode(buffer,model,&symbol, 1))
                            return(2);
                          coefficients->matrix[current_row][current_col] +=
                            (symbol == 1) ? threshold / 2 : -threshold / 2;
                        }
                      p =
                        p * QCCWAVTCE_ALPHA_1D + symbol * QCCWAVTCE_ALPHA_1D_O;
                    }
                  else
                    {
                      if (*p_char == QCCTCE_S_NEW)
                        *p_char = QCCTCE_S;
                      if (*p_char == QCCTCE_NZN_NEW)
                        *p_char = QCCTCE_NZN;     
                    }
                }
            }
        }
    } 

  return(0);
}


int QccWAVtceEncode(const QccIMGImageComponent *image,
                    int num_levels,
                    int target_bit_cnt,
                    double stepsize,
                    const QccWAVWavelet *wavelet,
                    QccBitBuffer *output_buffer)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid image_subband_pyramid; 
  char **sign_array = NULL;
  char **significance_map = NULL;
  double **p_estimation = NULL;
  double *subband_significance = NULL;
  double image_mean;
  int *max_coefficient_bits = NULL;
  double threshold;
  int num_symbols[1] = { 2 };
  int row, col; 
  int num_subbands; 
  int bitplane_cnt = 1;
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);

  if (image == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if ((target_bit_cnt == QCCENT_ANYNUMBITS) &&
      (stepsize <= 0))
    {
      QccErrorAddMessage("(QccWAVtceEncode): Either a target bit cnt or a quantization stepsize must be specified");
      goto Error;
    } 
  if ((sign_array = (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((sign_array[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVtceEncode): Error allocating memory");
        goto Error;
      }
  
  if ((significance_map =
       (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((significance_map[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVtceEncode): Error allocating memory");
        goto Error;
      }

  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      significance_map[row][col] = QCCTCE_Z;
  
  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);
  
  if ((max_coefficient_bits =
       (int *)malloc(sizeof(int) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error allocating memory");
      goto Error;
    }
  
  if ((p_estimation =
       (double **)malloc(sizeof(double *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error allocating memory");
      goto Error;
    }
  
  for (row = 0; row < image->num_rows; row++)
    if ((p_estimation[row] =
         (double *)malloc(sizeof(double) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVtceEncode): Error allocating memory");
        goto Error;
      }
  
  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      p_estimation[row][col] = 0.0000001;
  
  if ((subband_significance =
       (double*)calloc(num_subbands, sizeof(double))) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error allocating memory");
      goto Error;
    }    
  
  image_subband_pyramid.num_levels = 0;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  //Wavelet transform and/or quantization
  if (QccWAVtceEncodeDWT(&image_subband_pyramid,
                         sign_array,
                         image,
                         num_levels,
                         &image_mean,
                         max_coefficient_bits,
                         stepsize,
                         wavelet))
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceEncodeDWT()");
      goto Error;
    }
  //encoder header information
  if (QccWAVtceEncodeHeader(output_buffer,
                            num_levels,
                            image->num_rows,
                            image->num_cols, 
                            image_mean,
                            stepsize,
                            max_coefficient_bits[0]))
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceEncodeHeader()");
      goto Error;
    }
  
  if (QccWAVtceEncodeBitPlaneInfo(output_buffer,
                                  num_subbands,
                                  max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceEncodeBitPlaneInfo()");
      goto Error;
    }
  
  if ((model = 
       QccENTArithmeticEncodeStart(num_symbols,
                                   1,
                                   NULL,
                                   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);
  
  threshold = pow((double)2, (double)max_coefficient_bits[0]);
  if (stepsize <= 0)
    // totally embedding without quantization
    while (1)
      {
        return_value = QccWAVtceNZNPass(&image_subband_pyramid,    
                                        significance_map,
                                        sign_array,
                                        threshold,
                                        p_estimation,
                                        subband_significance,
                                        model,
                                        output_buffer,
                                        max_coefficient_bits);
        
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceNZNPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        return_value = QccWAVtceIPPass(&image_subband_pyramid,    
                                       significance_map,
                                       sign_array,
                                       threshold,
                                       p_estimation,
                                       subband_significance,
                                       model,
                                       output_buffer,
                                       max_coefficient_bits);
        
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceIPPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        return_value = QccWAVtceSPass(&image_subband_pyramid,    
                                      significance_map,
                                      threshold,
                                      model,
                                      output_buffer,
                                      max_coefficient_bits);
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceSPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }  
        
        threshold /= 2.0;
      }
  else
    while (threshold > 0.75)
      {
        return_value = QccWAVtceNZNPass(&image_subband_pyramid,    
                                        significance_map,
                                        sign_array,
                                        threshold,
                                        p_estimation,
                                        subband_significance,
                                        model,
                                        output_buffer,
                                        max_coefficient_bits);
        
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceNZNPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        return_value = QccWAVtceIPPass(&image_subband_pyramid,    
                                       significance_map,
                                       sign_array,
                                       threshold,
                                       p_estimation,
                                       subband_significance,
                                       model,
                                       output_buffer,
                                       max_coefficient_bits);
        
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceIPPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        return_value = QccWAVtceSPass(&image_subband_pyramid,    
                                      significance_map,
                                      threshold,
                                      model,
                                      output_buffer,
                                      max_coefficient_bits);
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceEncode): Error calling QccWAVtceSPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }

        bitplane_cnt++;
        threshold /= 2.0;
      }
  
  QccENTArithmeticEncodeFlush(model, output_buffer);
  
 Finished:
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramidFree(&image_subband_pyramid);
  if (sign_array != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (sign_array[row] != NULL)
          QccFree(sign_array[row]);
      QccFree(sign_array);
    }
  if (significance_map != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (significance_map[row] != NULL)
          QccFree(significance_map[row]);
      QccFree(significance_map);
    }
  if (p_estimation != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (p_estimation[row] != NULL)
          QccFree(p_estimation[row]);
      QccFree(p_estimation);
    }
  if (subband_significance != NULL)
    QccFree(subband_significance);
  if (max_coefficient_bits != NULL)
    QccFree(max_coefficient_bits);
  QccENTArithmeticFreeModel(model);
  return(return_value);
}


static int QccWAVtceDecodeInverseDWT(QccWAVSubbandPyramid
                                     *image_subband_pyramid,
                                     char **sign_array,
                                     QccIMGImageComponent *image,
                                     double image_mean,
                                     double stepsize,
                                     const QccWAVWavelet *wavelet)
{
  int row, col;
  int num_subbands,subband;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_rows;
  int subband_num_cols;
  
  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(image_subband_pyramid->num_levels);
  
  if (stepsize > 0)
    for (subband = 0; subband < num_subbands; subband++)
      {
        if (QccWAVSubbandPyramidSubbandSize(image_subband_pyramid,
                                            subband,
                                            &subband_num_rows,
                                            &subband_num_cols))
            {
              QccErrorAddMessage("(QccWAVtceDecodeInverseDWT): Error calling QccWAVSubbandPyramidSubbandSize()");
              return(1);
            }
          if (QccWAVSubbandPyramidSubbandOffsets(image_subband_pyramid,
                                                 subband,
                                                 &subband_origin_row,
                                                 &subband_origin_col))
            {
              QccErrorAddMessage("(QccWAVtceDecodeInverseDWT): Error calling QccWAVSubbandPyramidSubbandOffsets()");
              return(1);         
            } 
          
          for (row = 0; row < subband_num_rows; row++)
            for (col = 0; col < subband_num_cols; col++)
              {
                if (sign_array[subband_origin_row + row]
                    [subband_origin_col + col])
                  image_subband_pyramid->matrix
                    [subband_origin_row + row][subband_origin_col + col] *= -1;
                image_subband_pyramid->matrix
                  [subband_origin_row + row][subband_origin_col + col] *=
                  stepsize;
              }
      }
  else
    {
      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        for (col = 0; col < image_subband_pyramid->num_cols; col++)
          if (sign_array[row][col])
            image_subband_pyramid->matrix[row][col] *= -1;  
    }
  
  if (QccWAVSubbandPyramidInverseDWT(image_subband_pyramid,wavelet))
    {
      QccErrorAddMessage("(QccWAVtceDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseDWT()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidAddMean(image_subband_pyramid, image_mean))
    {
      QccErrorAddMessage("(QccWAVtceDecodeInverseDWT): Error calling QccWAVSubbandPyramidAddMean()");
      return(1);
    }
  
  if (QccMatrixCopy(image->image,
                    image_subband_pyramid->matrix,
                    image->num_rows, image->num_cols))
    {
      QccErrorAddMessage("(QccWAVtceDecodeInverseDWT): Error calling QccMatrixCopy()");
      return(1);
    }

  if (QccIMGImageComponentSetMaxMin(image))
    {
      QccErrorAddMessage("(QccWAVtceDecodeInverseDWT): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccWAVtceDecodeHeader(QccBitBuffer *input_buffer, 
                          int *num_levels, 
                          int *num_rows,
                          int *num_cols,
                          double *image_mean,
                          double *stepsize,
                          int *max_coefficient_bits)
{
  int return_value;
  unsigned char ch;
  
  if (QccBitBufferGetChar(input_buffer, &ch))
    {
      QccErrorAddMessage("(QccWAVtceDecodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  *num_levels = (int)ch;
  
  if (QccBitBufferGetInt(input_buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVtceDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVtceDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(input_buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVtceDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  if (QccBitBufferGetDouble(input_buffer, stepsize))
    {
      QccErrorAddMessage("(QccWAVtceDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtceDecodeHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVtceDecode(QccBitBuffer *input_buffer,
                    QccIMGImageComponent *image,
                    int num_levels,
                    const QccWAVWavelet *wavelet,
                    double image_mean,
                    double stepsize,
                    int max_coefficient_bits,
                    int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid image_subband_pyramid;
  char **sign_array = NULL;
  char **significance_map = NULL;
  double **p_estimation = NULL;
  int *max_bits = NULL;
  double *subband_significance = NULL;
  double threshold;
  int row, col;
  int num_symbols[1] = { 2 };
  int num_subbands;
  int bitplane_cnt = 0;
  
  if (image == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);
  if ((max_bits = (int *)malloc(sizeof(int) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceDecode): Error allocating memory");
      goto Error;
    }
  
  if (QccWAVtceDecodeBitPlaneInfo(input_buffer,
                                  num_subbands,
                                  max_bits,
                                  max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVtceDecodeBitPlaneInfo()");
      goto Error;
    }
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  
  image_subband_pyramid.num_levels = num_levels;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if ((sign_array = (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceDecode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((sign_array[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVtceDecode): Error allocating memory");
        goto Error;
      }

  if ((significance_map =
       (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceDecode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((significance_map[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVtceDecode): Error allocating memory");
        goto Error;
      }

  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      {
        image_subband_pyramid.matrix[row][col] = 0.0;
        sign_array[row][col] = 0;
        significance_map[row][col] = QCCTCE_Z;
      }

  if ((p_estimation =
       (double **)malloc(sizeof(double *) * image->num_rows)) == NULL)
    {   
      QccErrorAddMessage("(QccWAVtceDecode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((p_estimation[row] =
         (double *)malloc(sizeof(double) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVtceDecode): Error allocating memory");
        goto Error;
      }
  
  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      p_estimation[row][col] = 0.0000001;
  
  if ((subband_significance =
       (double*)calloc(num_subbands,sizeof(double))) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceDecode): Error allocating memory");
      goto Error;
    }
  
  if ((model = QccENTArithmeticDecodeStart(input_buffer,
                                           num_symbols,
                                           1,
                                           NULL,
                                           target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtceDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);    
  threshold = pow((double)2, (double)max_coefficient_bits); 
  
  if (stepsize <= 0)
    while (1)
      {
        return_value = QccWAVtceNZNPass(&image_subband_pyramid,    
                                        significance_map,
                                        sign_array,
                                        threshold,
                                        p_estimation,
                                        subband_significance,
                                        model,
                                        input_buffer,
                                        max_bits);
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVtceNZNPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        return_value = QccWAVtceIPPass(&image_subband_pyramid,    
                                       significance_map,
                                       sign_array,
                                       threshold,
                                       p_estimation,
                                       subband_significance,
                                       model,
                                       input_buffer,
                                       max_bits);
        
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVtceIPPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        return_value = QccWAVtceSPass(&image_subband_pyramid,    
                                      significance_map,
                                      threshold,
                                      model,
                                      input_buffer,
                                      max_bits);
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVtceSPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        bitplane_cnt++;
        threshold /= 2.0;
      }
  else
    while (threshold > 0.75)
      {
        return_value = QccWAVtceNZNPass(&image_subband_pyramid,    
                                        significance_map,
                                        sign_array,
                                        threshold,
                                        p_estimation,
                                        subband_significance,
                                        model,
                                        input_buffer,
                                        max_bits);
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVtceNZNPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        return_value = QccWAVtceIPPass(&image_subband_pyramid,    
                                       significance_map,
                                       sign_array,
                                       threshold,
                                       p_estimation,
                                       subband_significance,
                                       model,
                                       input_buffer,
                                       max_bits);
        
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVtceIPPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }
        
        return_value = QccWAVtceSPass(&image_subband_pyramid,    
                                      significance_map,
                                      threshold,
                                      model,
                                      input_buffer,
                                      max_bits);
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVtceSPass()");
            goto Error;
          }
        else
          {
            if (return_value == 2)
              goto Finished;
          }

          bitplane_cnt++;
          threshold /= 2.0;
      }
  
 Finished:
  if (QccWAVtceDecodeInverseDWT(&image_subband_pyramid,
                                sign_array,
                                image,
                                image_mean,
                                stepsize,
                                wavelet))
    {
      QccErrorAddMessage("(QccWAVtceDecode): Error calling QccWAVtceDecodeInverseDWT()");
      goto Error;
    }
  
  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramidFree(&image_subband_pyramid);
  if (sign_array != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (sign_array[row] != NULL)
          QccFree(sign_array[row]);
      QccFree(sign_array);
    }
  if (p_estimation != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (p_estimation[row] != NULL)
          QccFree(p_estimation[row]);
      QccFree(p_estimation);
    }
  if (significance_map != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (significance_map[row] != NULL)
          QccFree(significance_map[row]);
      QccFree(significance_map);
    }
  if (subband_significance!=NULL)
    QccFree(subband_significance);
  if (max_bits != NULL)
    QccFree(max_bits);
  QccENTArithmeticFreeModel(model);

  return(return_value);
}
