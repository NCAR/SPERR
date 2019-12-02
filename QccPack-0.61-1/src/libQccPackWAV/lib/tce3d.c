/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* 
 *
 * Written by
 *
 * Jing Zhang, at Mississippi State University, 2008 
 *
 */


#include "libQccPack.h"

#define QCCWAVTCE3D_BOUNDARY_VALUE (1e-6)

//definition used in context information
#define QCCWAVTCE3D_Z 0 //zero, AKA, insignificant
#define QCCWAVTCE3D_NZN_NEW 2 //non-zero-neighbor
#define QCCWAVTCE3D_S 3 //significant
#define QCCWAVTCE3D_S_NEW 4
#define QCCWAVTCE3D_NZN 5

//ugly fix, use 1-D IIR filter to provide PMF estimate
#define QCCWAVTCE3D_ALPHA_1D 0.995 
#define QCCWAVTCE3D_ALPHA_1D_O 0.005

#define QCCWAVTCE3D_REFINE_HOLDER 0.3 

// threshold for cross-scale prediction...
#define QCCWAVTCE3D_PREDICT_THRESHOLD 0.05  
//weight factor for cross-scale and current scale
#define QCCWAVTCE3D_CURRENT_SCALE 0.6 
#define QCCWAVTCE3D_PARENT_SCALE 0.4

#define QCCWAVTCE3D_MAXBITPLANES 128


static int QccWAVtce3DEncodeBitPlaneInfo(QccBitBuffer *output_buffer,
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


static int QccWAVtce3DDecodeBitPlaneInfo(QccBitBuffer *input_buffer,
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


static int QccWAVtce3DUpdateModel(QccENTArithmeticModel *model, double prob)
{
  double p[2];
  
  p[0] = 1 - prob;
  p[1] = prob;
  
  if (QccENTArithmeticSetModelProbabilities(model,
                                            p,
                                            0))
    {
      
      QccErrorAddMessage("(QccWAVtce3DUpdateModel): Error calling QccENTArithmeticSetModelProbabilities()");
      return(1);
    }
  
  return(0);
}


static int QccWAVtce3DEncodeDWT(QccWAVSubbandPyramid3D *image_subband_pyramid,
                                char ***sign_array,
                                const QccIMGImageCube *image,
                                int transform_type,
                                int temporal_num_levels,
                                int spatial_num_levels,
                                double *image_mean,
                                int *max_coefficient_bits,
                                const QccWAVWavelet *wavelet)
{
  double coefficient_magnitude;
  double max_coefficient = -MAXFLOAT;
  int frame, row, col;
  int num_subbands,subband;
  int subband_origin_frame;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  
  if (transform_type ==
      QCCWAVSUBBANDPYRAMID3D_DYADIC)
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(spatial_num_levels);
  else
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(temporal_num_levels,
                                                         spatial_num_levels);
  
  if (QccVolumeCopy(image_subband_pyramid->volume,
                    image->volume,
                    image->num_frames,
                    image->num_rows,
                    image->num_cols))
    {
      QccErrorAddMessage("(QccWAVtce3DEncodeDWT): Error calling QccVolumeCopy()");
      return(1);
    }
  
  if (QccWAVSubbandPyramid3DSubtractMean(image_subband_pyramid,
                                         image_mean,
                                         NULL))
    {
      QccErrorAddMessage("(QccWAVtce3DEncodeDWT): Error calling QccWAVSubbandPyramid3DSubtractMean()");
      return(1);
    }
  
  if (QccWAVSubbandPyramid3DDWT(image_subband_pyramid,
                                transform_type,
                                temporal_num_levels,
                                spatial_num_levels,
                                wavelet))
    {
      QccErrorAddMessage("(QccWAVtce3DEncodeDWT): Error calling QccWAVSubbandPyramid3DDWT()");
      return(1);
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramid3DSubbandSize(image_subband_pyramid,
                                            subband,
                                            &subband_num_frames,
                                            &subband_num_rows,
                                            &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVtce3DEncodeDWT): Error calling QccWAVSubbandPyramid3DSubbandSize()");
          return(1);
        }
      if (QccWAVSubbandPyramid3DSubbandOffsets(image_subband_pyramid,
                                               subband,
                                               &subband_origin_frame,
                                               &subband_origin_row,
                                               &subband_origin_col))
        {
          QccErrorAddMessage("(QccWAVtce3DEncodeDWT): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
          return(1);
        }
      
      max_coefficient = -MAXFLOAT;
      
      for (frame = 0; frame < subband_num_frames; frame++)
        for (row = 0; row < subband_num_rows; row++)
          for (col = 0; col < subband_num_cols; col++)
            {
              coefficient_magnitude =
                fabs(image_subband_pyramid->volume
                     [subband_origin_frame+frame]
                     [subband_origin_row+row]
                     [subband_origin_col+col]);
              if (image_subband_pyramid->volume
                  [subband_origin_frame+frame]
                  [subband_origin_row+row]
                  [subband_origin_col+col] !=
                  coefficient_magnitude)
                {
                  image_subband_pyramid->volume
                    [subband_origin_frame+frame]
                    [subband_origin_row+row]
                    [subband_origin_col+col] =
                    coefficient_magnitude;
                  sign_array[subband_origin_frame+frame]
                    [subband_origin_row + row]
                    [subband_origin_col + col] = 1;
                }
              else
                sign_array[subband_origin_frame+frame]
                  [subband_origin_row + row]
                  [subband_origin_col + col] = 0;
              
              if (coefficient_magnitude > max_coefficient)
                max_coefficient = coefficient_magnitude;
            }
      
      max_coefficient_bits[subband] = 
        (int)floor(QccMathMax(0, QccMathLog2(max_coefficient + 0.0001)));
    }
  
  return(0);
}


int QccWAVtce3DEncodeHeader(QccBitBuffer *buffer, 
                            int transform_type,
                            int temporal_num_levels,
                            int spatial_num_levels,
                            int num_frames,
                            int num_rows,
                            int num_cols,
                            double image_mean,
                            int *max_coefficient_bits,
                            double alpha)
{
  int return_value;
  
  if (QccBitBufferPutBit(buffer, transform_type))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPutBit()");
      goto Error;
    }
  
  if (transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      if (QccBitBufferPutChar(buffer, (unsigned char)temporal_num_levels))
        {
          QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPuChar()");
          goto Error;
        }
    }
  
  if (QccBitBufferPutChar(buffer, (unsigned char)spatial_num_levels))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_frames))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, max_coefficient_bits[0]))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(buffer, alpha))
    {
      QccErrorAddMessage("(QccWAVTarpEncodeHeader3D): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVtce3DRevEst(QccWAVSubbandPyramid3D *coefficients,
                             char ***significance_map,
                             int subband,
                             double *subband_significance,
                             double ***p,
                             double alpha)
{
  int return_value;
  int subband_origin_frame;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  int frame,row, col;
  int current_frame, current_row, current_col;
  QccMatrix p1=NULL;
  QccVector p2 = NULL;
  QccMatrix p3=NULL;
  QccMatrix p4=NULL;
  QccVector p5 = NULL;
  char *p_char;
  int v;  
  double filter_coef;
  
  filter_coef =
    (1 - alpha) * (1 - alpha)*(1 - alpha) / (3*alpha + alpha*alpha*alpha) / 3;
  
  subband_significance[subband] = 0.0;
  if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                        subband,
                                        &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVtce3DRevEst): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      goto Error;
    }

  if ((subband_num_frames == 0) ||
      (subband_num_cols == 0) ||
      (subband_num_rows == 0))
    return(0);
  
  if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                           subband,
                                           &subband_origin_frame,
                                           &subband_origin_row,
                                           &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVtce3DRevEst): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      goto Error;
    }
  
  if ((p1 = QccMatrixAlloc(subband_num_rows, subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DRevEst): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((p3 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p4 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p5 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DRevEst): Error calling QccVectorAlloc()");
      goto Error;
    }

  for (row = 0; row < subband_num_rows; row++)
    for (col = 0; col < subband_num_cols; col++)
      p3[row][col]=QCCWAVTCE3D_BOUNDARY_VALUE;  
  
  for (frame = subband_num_frames - 1; frame >= 0; frame--)
    {//bbb
      current_frame=subband_origin_frame+frame;
      
      for (col = 0; col < subband_num_cols; col++)
        p2[col] = QCCWAVTCE3D_BOUNDARY_VALUE; 
      
      for (row = subband_num_rows - 1; row >= 0; row--)
        {//bb
          current_row = subband_origin_row + row;
          for (col = subband_num_cols - 1; col >= 0; col--)
            {       
              current_col = subband_origin_col + col;
              p_char = &(significance_map
                         [current_frame][current_row][current_col]);
              if (col == (subband_num_cols-1) )
                p[current_frame][current_row][current_col] =
                  alpha * (QCCWAVTCE3D_BOUNDARY_VALUE + p2[col] + p3[row][col]);
              else
                p[current_frame][current_row][current_col] =
                  alpha *(p1[row][col+1]+p2[col]+p3[row][col]);
              
              if (*p_char == QCCWAVTCE3D_S)
                {
                  if (col == subband_num_cols-1 )
                    p1[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else
                    p1[row][col] =
                      alpha * p1[row][col+1] +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  
                  subband_significance[subband] += QCCWAVTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCWAVTCE3D_S_NEW);
                  if (col == (subband_num_cols-1) )
                    p1[row][col] =
                      alpha *  QCCWAVTCE3D_BOUNDARY_VALUE  + filter_coef * v;
                  else
                    p1[row][col] = alpha * p1[row][col+1] + filter_coef * v;
                  
                  subband_significance[subband] += v;
                }   
              p2[col] = p1[row][col] + alpha * p2[col];   
            }
          
          for (col = 0; col < subband_num_cols; col++)
            {
              p_char =
                &(significance_map
                  [current_frame][current_row][subband_origin_col + col]);
              if(col==0)
                p2[col] = p2[col] + alpha * QCCWAVTCE3D_BOUNDARY_VALUE;
              else
                p2[col] = p2[col] + alpha * p4[row][col-1];
              p3[row][col]=p2[col]+alpha*p3[row][col];
              
              if (*p_char == QCCWAVTCE3D_S)
                {
                  if(col==0)
                    p4[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else 
                    p4[row][col] =
                      alpha * p4[row][col-1] +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCWAVTCE3D_S_NEW);
                  if(col==0)
                    p4[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE + filter_coef * v;
                  else 
                    p4[row][col] = alpha* p4[row][col-1] + filter_coef *v;
                }        
            }
        }//bb
      for (col = 0; col < subband_num_cols; col++)
        p5[col] = QCCWAVTCE3D_BOUNDARY_VALUE;
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          {
            p3[row][col] = p3[row][col] + alpha*p5[col];
            if (col == 0)
              p5[col] = p1[row][col]+alpha*p5[col] +
                alpha*QCCWAVTCE3D_BOUNDARY_VALUE;
            else
              p5[col] = p1[row][col] + alpha*p5[col] + alpha*p4[row][col-1];
          }
    }//bbb
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(p2);
  QccVectorFree(p5);
  QccMatrixFree(p1, subband_num_rows); 
  QccMatrixFree(p3, subband_num_rows); 
  QccMatrixFree(p4, subband_num_rows); 
  return(return_value);
}


static int QccWAV3DUpdateNZNStatus(int subband_origin_frame,
				   int subband_origin_row,
				   int subband_origin_col,
				   int subband_num_frame,
				   int subband_num_row,
				   int subband_num_col,
				   int frame,
				   int row,
				   int col,
				   char ***significance_map,
				   int subband)
{
  //Updata neighbors
  int current_frame, current_row, current_col;
  int shifted_frame, shifted_row, shifted_col;

  current_frame = subband_origin_frame + frame;
  current_row = subband_origin_row + row;
  current_col = subband_origin_col + col;

  //previous frame
  if (frame > 0)
    {
      if (row > 0)
        { 
          shifted_frame = current_frame - 1; 
          shifted_row = current_row - 1;
          shifted_col = current_col;
          if (significance_map
              [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
            significance_map
              [shifted_frame][shifted_row][shifted_col] = QCCWAVTCE3D_NZN_NEW;
          
          if (col > 0)
            {
              shifted_frame = current_frame - 1;
              shifted_row = current_row - 1;
              shifted_col = current_col - 1;
              if (significance_map
                  [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCWAVTCE3D_NZN_NEW;   
            }
          if (col < subband_num_col - 1)
            {
              shifted_frame = current_frame - 1;
              shifted_row = current_row - 1;
              shifted_col = current_col + 1;
              if (significance_map
                  [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCWAVTCE3D_NZN_NEW;   
            }
        }
      if (row < subband_num_row - 1)
        {
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row + 1;
	  shifted_col = current_col;
	  if (significance_map
              [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCWAVTCE3D_NZN_NEW;
          
	  if (col > 0)
	    {
	      shifted_frame = current_frame - 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col - 1;
	      if (significance_map
                  [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCWAVTCE3D_NZN_NEW;  
	    }
	  if (col < subband_num_col - 1)
	    {
	      shifted_frame = current_frame - 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col + 1;
	      if (significance_map
                  [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCWAVTCE3D_NZN_NEW;   
	    }
	}
      if (col > 0)
	{
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row;
	  shifted_col = current_col - 1;  
	  if (significance_map
              [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCWAVTCE3D_NZN_NEW;
	}
      if (col < subband_num_col - 1)
	{
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row;
	  shifted_col = current_col + 1;
	  if (significance_map
              [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCWAVTCE3D_NZN_NEW;
	}
      {
	shifted_frame = current_frame - 1;
	shifted_row = current_row;
	shifted_col = current_col;
	if (significance_map
            [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
	  significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCWAVTCE3D_NZN_NEW;
      }
    }
  //next frame
  
  if (frame < subband_num_frame - 1)
    {
      if (row > 0)
        { 
          shifted_frame = current_frame + 1; 
          shifted_row = current_row - 1;
          shifted_col = current_col;
          if (significance_map
              [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
            significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCWAVTCE3D_NZN_NEW;
          
          if (col > 0)
            {
              shifted_frame = current_frame + 1;
              shifted_row = current_row - 1;
              shifted_col = current_col - 1;
              if (significance_map
                  [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] = 
                  QCCWAVTCE3D_NZN_NEW;   
            }
          if (col < subband_num_col - 1)
            {
              shifted_frame = current_frame + 1;
              shifted_row = current_row - 1;
              shifted_col = current_col + 1;
              if (significance_map
                  [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCWAVTCE3D_NZN_NEW;   
            }
        }
      if (row < subband_num_row - 1)
	{
	  shifted_frame = current_frame+ 1;
	  shifted_row = current_row + 1;
	  shifted_col = current_col;
	  if (significance_map
              [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCWAVTCE3D_NZN_NEW;
          
	  if (col > 0)
	    {
	      shifted_frame = current_frame + 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col - 1;
	      if (significance_map
                  [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] = 
                  QCCWAVTCE3D_NZN_NEW;  
	    }
	  if (col < subband_num_col - 1)
	    {
	      shifted_frame = current_frame + 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col + 1;
	      if (significance_map
                  [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCWAVTCE3D_NZN_NEW;   
	    }
	}
      if (col > 0)
	{
	  shifted_frame = current_frame + 1;
	  shifted_row = current_row;
	  shifted_col = current_col - 1;  
	  if (significance_map
              [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCWAVTCE3D_NZN_NEW;
	}
      if (col < subband_num_col - 1)
	{
	  shifted_frame = current_frame + 1;
	  shifted_row = current_row;
	  shifted_col = current_col + 1;
	  if (significance_map
              [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCWAVTCE3D_NZN_NEW;
	}
      {
	shifted_frame = current_frame + 1;
	shifted_row = current_row;
	shifted_col = current_col;
	if (significance_map
            [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
	  significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCWAVTCE3D_NZN_NEW;
      }
    }
  
  //current frame
  {
    if (row > 0)
      { 
        shifted_frame = current_frame ; 
        shifted_row = current_row - 1;
        shifted_col = current_col;
        if (significance_map
            [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCWAVTCE3D_NZN_NEW;
        
        if (col > 0)
          {
            shifted_frame = current_frame;
            shifted_row = current_row - 1;
            shifted_col = current_col - 1;
            if (significance_map
                [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] =
                QCCWAVTCE3D_NZN_NEW;   
          }
        if (col < subband_num_col - 1)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row - 1;
            shifted_col = current_col + 1;
            if (significance_map
                [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] =
                QCCWAVTCE3D_NZN_NEW;   
          }
      }
    if (row < subband_num_row - 1)
      {
        shifted_frame = current_frame;
        shifted_row = current_row + 1;
        shifted_col = current_col;
        if (significance_map
            [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCWAVTCE3D_NZN_NEW;
        
        if (col > 0)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row + 1;
            shifted_col = current_col - 1;
            if (significance_map
                [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] =
                QCCWAVTCE3D_NZN_NEW;  
          }
        if (col < subband_num_col - 1)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row + 1;
            shifted_col = current_col + 1;
            if (significance_map
                [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] =
                QCCWAVTCE3D_NZN_NEW;   
          }
      }
    if (col > 0)
      {
        shifted_frame = current_frame ;
        shifted_row = current_row;
        shifted_col = current_col - 1;  
        if (significance_map
            [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCWAVTCE3D_NZN_NEW;
      }
    if (col < subband_num_col - 1)
      {
        shifted_frame = current_frame ;
        shifted_row = current_row;
        shifted_col = current_col + 1;
        if (significance_map
            [shifted_frame][shifted_row][shifted_col] < QCCWAVTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCWAVTCE3D_NZN_NEW;
      }
  }
  
  return(0);
}


static int QccWAVtce3DNZNPass(QccWAVSubbandPyramid3D *coefficients,    
                              char ***significance_map,
                              char ***sign_array,
                              double threshold,                           
                              double *subband_significance,
                              QccENTArithmeticModel *model,
                              QccBitBuffer *buffer,
                              int *max_coefficient_bits)
{ 
  int subband,num_subbands;
  int return_value;
  int subband_origin_frame;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  int frame,row, col;
  int current_frame, current_row, current_col;
  int symbol;
  double p = 0.5;
  
  if (coefficients->transform_type ==
      QCCWAVSUBBANDPYRAMID3D_DYADIC)
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(coefficients->spatial_num_levels);
  else
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                         coefficients->spatial_num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)(max_coefficient_bits[subband])))
        { 
          if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                                subband,
                                                &subband_num_frames,
                                                &subband_num_rows,
                                                &subband_num_cols))
            {
              QccErrorAddMessage("(QccWAVtce3DNZNPass): Error calling QccWAVSubbandPyramid3DSubbandSize()");
              return(1);
            }
          if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                                   subband,
                                                   &subband_origin_frame,
                                                   &subband_origin_row,
                                                   &subband_origin_col))
            {
              QccErrorAddMessage("(QccWAVtce3DNZNPass): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
              return(1);
            }  
          
          for (frame = 0; frame < subband_num_frames; frame++) 
            {
              current_frame = subband_origin_frame + frame;
              for (row = 0; row < subband_num_rows; row++)
                {
                  current_row = subband_origin_row + row;
                  for (col = 0; col < subband_num_cols; col++)
                    { 
                      current_col = subband_origin_col + col;
                      if (significance_map
                          [current_frame][current_row][current_col] ==
                          QCCWAVTCE3D_NZN)
                        {
                          if (QccWAVtce3DUpdateModel(model, p))
                            {
                              QccErrorAddMessage("(QccWAVtceNZNPass): Error calling QccWAVtceUpdateModel()");
                              return_value = 1;
                              return(1);
                            }
                          if (buffer->type == QCCBITBUFFER_OUTPUT)
                            {
                              if (coefficients->volume
                                  [current_frame][current_row][current_col] >=
                                  threshold)
                                {
                                  symbol = 1;
                                  coefficients->volume
                                    [current_frame][current_row][current_col] -=
                                    threshold;
                                }
                              else
                                symbol = 0;
                              
                              return_value =
                                QccENTArithmeticEncode(&symbol,
                                                       1, model, buffer);
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
                                coefficients->volume
                                  [current_frame][current_row][current_col] =
                                  1.5 * threshold;
                            }
                          
                          p =
                            p * QCCWAVTCE3D_ALPHA_1D +
                            symbol * QCCWAVTCE3D_ALPHA_1D_O;
                          
                          if (symbol)
                            {
                              subband_significance[subband]++;
                              significance_map
                                [current_frame][current_row][current_col] =
                                QCCWAVTCE3D_S_NEW;
                              
                              if (QccWAVtce3DUpdateModel(model, 0.5))
                                {
                                  QccErrorAddMessage("(QccWAVtce3DNZNPass): Error calling QccWAVtce3DUpdateModel()");
                                  return(1);
                                }
                              
                              if (buffer->type == QCCBITBUFFER_OUTPUT)
                                {
                                  symbol =
                                    (int)(sign_array
                                          [current_frame]
                                          [current_row]
                                          [current_col]);
                                  
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
                                  
                                  sign_array
                                    [current_frame][current_row][current_col] =
                                    (char)symbol;
                                }
                              
                              QccWAV3DUpdateNZNStatus(subband_origin_frame,
                                                      subband_origin_row,
                                                      subband_origin_col,
                                                      subband_num_frames,
                                                      subband_num_rows,
                                                      subband_num_cols,
                                                      frame,
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
    }
  
  return(0);
}


static int QccWAVtce3DIPBand(QccWAVSubbandPyramid3D *coefficients,
                             char ***significance_map,
                             char ***sign_array,
                             double ***p_estimation,
                             double *subband_significance,
                             int subband,
                             double threshold,
                             QccENTArithmeticModel *model,
                             QccBitBuffer *buffer,
                             double alpha)
{
  
  int return_value;
  int subband_origin_frame;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  int frame,row, col;
  int current_frame, current_row, current_col;
  QccMatrix p1 = NULL;
  QccVector p2 = NULL;
  QccMatrix p3 = NULL;
  QccMatrix p4 = NULL;
  QccVector p5 = NULL;
  double p;
  double p_forward;
  double p_parent = 0;
  int parent_subband;   
  double scale;
  double parent_density = 0;
  double child_density; 
  int v;
  int symbol;
  double p_lowerbound = 0;
  double weight[2];
  double filter_coef;
  double increment;
  char *p_char;
  
  // initialization all the constants
  filter_coef =
    (1 - alpha) * (1 - alpha)*(1 - alpha) / (3*alpha + alpha*alpha*alpha) / 3;
  
  weight[0] =
    (1 - alpha) * (1 - alpha) * (1 - alpha) /
    ((1 + alpha) * (1 + alpha) * (1 + alpha));
  weight[1] =
    (2*3*alpha + 2*alpha*alpha*alpha) / ((1 + alpha)*(1 + alpha)*(1 + alpha));
  
  if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                        subband,
                                        &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVtce3DIPBand): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      goto Error;
    }

  if ((subband_num_frames == 0) ||
      (subband_num_cols == 0) ||
      (subband_num_rows == 0))
    return(0);

  if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                           subband,
                                           &subband_origin_frame,
                                           &subband_origin_row,
                                           &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVtce3DIPBand): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      goto Error;
    }
  
  if ((p1 = QccMatrixAlloc(subband_num_rows, subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIPBand): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((p3 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p4 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p5 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIPBand): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  child_density =
    subband_significance[subband] /
    (subband_num_frames*subband_num_rows * subband_num_cols);
  p_lowerbound = 1.0 / (subband_num_frames*subband_num_rows * subband_num_cols);
  
  if (coefficients->transform_type ==
      QCCWAVSUBBANDPYRAMID3D_DYADIC)
    {
      if (subband <= 7)
	{
	  parent_subband = 0;
          parent_density =
            subband_significance[parent_subband] /
	    (subband_num_frames*subband_num_rows * subband_num_cols) ;
	}
      else
	{
	  parent_subband = subband-7;
          parent_density =
            subband_significance[parent_subband] /
	    (subband_num_frames*subband_num_rows * subband_num_cols)*8;
	}
    }
  else
    {
      if ((subband % (3*coefficients->spatial_num_levels+1)) <= 3)
	{
          
          if ((subband % (3*coefficients->spatial_num_levels + 1)) == 0)
            {
              if ((subband / (3*coefficients->spatial_num_levels+1)) <= 1)
                {
                  parent_subband = 0;
                  parent_density =
                    subband_significance[parent_subband] /
                    (subband_num_frames*subband_num_rows * subband_num_cols) ;
                }
              else
                {
                  parent_subband =
                    subband - (3*coefficients->spatial_num_levels+1);
                  parent_density =
                    subband_significance[parent_subband] /
                    (subband_num_frames*subband_num_rows * subband_num_cols)*2 ;
                }
            }
          
          if ((subband%(3*coefficients->spatial_num_levels+1)) == 1)
            {
              parent_subband = subband - 1;
              parent_density =
                subband_significance[parent_subband] /
                (subband_num_frames*subband_num_rows * subband_num_cols) ;
            }
          if ((subband%(3*coefficients->spatial_num_levels+1))==2)
            {
              parent_subband = subband - 2;
              parent_density =
                subband_significance[parent_subband] /
                (subband_num_frames*subband_num_rows * subband_num_cols) ;
            }
          if ((subband%(3*coefficients->spatial_num_levels+1))==3)
            {
              parent_subband = subband - 3;
              parent_density =
                subband_significance[parent_subband] /
                (subband_num_frames*subband_num_rows * subband_num_cols) ;
            }
	}
      else
	{
	  parent_subband = subband - 3;
          parent_density =
            subband_significance[parent_subband] /
	    (subband_num_frames*subband_num_rows * subband_num_cols) * 4;
	}
    }
  
  for (row = 0; row < subband_num_rows; row++)
    for (col = 0; col < subband_num_cols; col++)
      p3[row][col]=QCCWAVTCE3D_BOUNDARY_VALUE;  
  
  scale =
    QccMathMax(child_density, p_lowerbound) /
    QccMathMax(parent_density, p_lowerbound); 
  increment =
    1.0 / (subband_num_frames*subband_num_rows * subband_num_cols) /
    QccMathMax(parent_density, p_lowerbound);
  // printf("IPband=%d,sunnamd_num_rows=%d,%d\n",subband,subband_num_rows,subband_num_cols );
  //printf("%d,%d,%d\n",subband_origin_frame,subband_origin_row,subband_origin_col );
  
  for (frame = 0; frame < subband_num_frames; frame++)
    {//aaaa
      current_frame = subband_origin_frame + frame;
      for (col = 0; col < subband_num_cols; col++)
        p2[col] =QCCWAVTCE3D_BOUNDARY_VALUE;  ;
      
      for (row = 0; row < subband_num_rows; row++)
        {  //aaa  
           //printf("row=%d\n",row);
          current_row = subband_origin_row + row;
          for (col = 0; col < subband_num_cols; col++)
            { //aa  
              current_col = subband_origin_col + col;
              p_char = &(significance_map
                         [current_frame][current_row][current_col]);
              if (col == 0)
                p_forward =alpha * (QCCWAVTCE3D_BOUNDARY_VALUE + p2[col] +
                                    p3[row][col]);
              else
                p_forward = alpha * (p1[row][col-1] + p2[col] + p3[row][col]);
              
              if (*p_char < QCCWAVTCE3D_S)
                { //a     
                  p = p_forward +
                    p_estimation[current_frame][current_row][current_col];
                  
                  if (subband != 0)
                    { //b
                      if (coefficients->transform_type ==
                          QCCWAVSUBBANDPYRAMID3D_DYADIC)
                        {
                          if(subband==1)			       
                            p_parent =
                              p_estimation
                              [0][current_row-subband_num_rows][current_col];
                          if(subband==2)			       
                            p_parent =
                              p_estimation
                              [0][current_row][current_col-subband_num_cols];
                          if(subband==3)			       
                            p_parent =
                              p_estimation[0][current_row][current_col];
                          if(subband==4)			       
                            p_parent =
                              p_estimation
                              [0][current_row-subband_num_rows]
                              [current_col-subband_num_cols];	       	    
                          if(subband==5)		       
                            p_parent =
                              p_estimation
                              [0][current_row-subband_num_rows][current_col];
                          if(subband==6)			       
                            p_parent =
                              p_estimation
                              [0][current_row][current_col-subband_num_cols];
                          if(subband==7)			       
                            p_parent =
                              p_estimation
                              [0][current_row-subband_num_rows]
                              [current_col-subband_num_cols];
                          if(subband>7)
                            p_parent =
                              p_estimation
                              [current_frame/2][current_row/2][current_col/2];
                        }
                      else
                        {
                          if ((subband %
                               (3*coefficients->spatial_num_levels+1)) <= 3)
                            {
		              if ((subband %
                                   (3*coefficients->spatial_num_levels+1)) == 0)
                                {
                                  if ((subband /
                                       (3*coefficients->spatial_num_levels+1))
                                      <=1 )
                                    p_parent =
                                      p_estimation[0][current_row][current_col];
			          else
                                    p_parent =
                                      p_estimation
                                      [current_frame/2][current_row]
                                      [current_col];
                                } 
                              
			      if ((subband %
                                   (3*coefficients->spatial_num_levels+1))==1)
			        p_parent =
                                  p_estimation[current_frame]
                                  [current_row-subband_num_rows][current_col];
			      if ((subband %
                                   (3*coefficients->spatial_num_levels+1)) == 2)
			        p_parent =
                                  p_estimation
                                  [current_frame][current_row]
                                  [current_col-subband_num_cols];
			      if ((subband %
                                   (3*coefficients->spatial_num_levels+1)) == 3)
			        p_parent =
                                  p_estimation
                                  [current_frame]
                                  [current_row-subband_num_rows]
                                  [current_col-subband_num_cols];
			    }
                          else
                            p_parent =
                              p_estimation
                              [current_frame][current_row/2][current_col/2];
                        }
                      
                      if (p < QCCWAVTCE3D_PREDICT_THRESHOLD)
                        {          
                          p_parent = QccMathMin(p_parent*scale, 0.8);
                          p =
                            p * QCCWAVTCE3D_CURRENT_SCALE +
                            QCCWAVTCE3D_PARENT_SCALE * p_parent;
                        }
                    }//b
                  if (p > 1)
                    {
                      // printf("p=%f      ",p);
                      p = 1;
                    }
                  if (QccWAVtce3DUpdateModel(model, p))
                    {
                      QccErrorAddMessage("(QccWAVtceIPBand): Error calling QccWAVtceUpdateModel()");
                      return_value = 1;
                      goto Error;
                    }
                  if (buffer->type == QCCBITBUFFER_OUTPUT)
                    {
                      if (coefficients->volume
                          [current_frame][current_row][current_col] >=
                          threshold)
                        {
                          symbol = 1;
                          coefficients->volume
                            [current_frame][current_row][current_col] -=
                            threshold;
                        }
                      else
                        symbol = 0;
                      return_value =
                        QccENTArithmeticEncode(&symbol, 1, model, buffer);
                    }
                  else
                    {
                      if (QccENTArithmeticDecode(buffer, model, &symbol, 1))
                        {
                          return_value = 2;
                          goto Return;
                        }
                      else
                        return_value = 0;
                      
                      if (symbol)
                        coefficients->volume
                          [current_frame][current_row][current_col] =
                          1.5 * threshold;
                    }
                  v = symbol;
                  
                  if (symbol)
                    {//c
                      subband_significance[subband]++;
                      scale += increment;
                      *p_char = QCCWAVTCE3D_S_NEW;
                      
                      if (QccWAVtce3DUpdateModel(model, 0.5))
                        {
                          QccErrorAddMessage("(QccWAVtce3DIPBand): Error calling QccWAVtce3DUpdateModel()");
                          goto Error;
                        }
                      
                      if (buffer->type == QCCBITBUFFER_OUTPUT)
                        {
                          symbol = (int)(sign_array
                                         [current_frame]
                                         [current_row][current_col]);
                          return_value =
                            QccENTArithmeticEncode(&symbol, 1,model, buffer);
                        }
                      else
                        {
                          if (QccENTArithmeticDecode(buffer,model,&symbol, 1))
                            return_value = 2;      
                          sign_array
                            [current_frame][current_row][current_col] =
                            (char)symbol;
                        }
                      QccWAV3DUpdateNZNStatus(subband_origin_frame,
                                              subband_origin_row,
                                              subband_origin_col,
                                              subband_num_frames,
                                              subband_num_rows,
                                              subband_num_cols,
                                              frame,
                                              row,
                                              col,
                                              significance_map,
                                              subband);
                    }//c    
                  
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
                  
                  if (col == 0)
                    p1[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE + filter_coef * v;
                  else
                    p1[row][col] = alpha * p1[row][col-1] + filter_coef * v;
                  p2[col] = p1[row][col] + alpha * p2[col];
                  
                }//a
              else
                { 
                  if (col == 0)
                    p1[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE  +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else
                    p1[row][col] =
                      alpha* p1[row][col-1] +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  p2[col] = p1[row][col] + alpha * p2[col];
                  
                }
              p_estimation
                [subband_origin_frame+frame]
                [subband_origin_row + row]
                [subband_origin_col + col] =
                p_forward;
            }//aa
          
          
          for (col = subband_num_cols - 1; col >= 0; col--)
            {  
              if (col ==( subband_num_cols - 1)) 
                p2[col] = p2[col] + alpha * QCCWAVTCE3D_BOUNDARY_VALUE;
              else
                p2[col] = p2[col] + alpha * p4[row][col+1];
              p3[row][col]=p2[col]+alpha*p3[row][col];
              p_char = &(significance_map
                         [current_frame]
                         [current_row]
                         [subband_origin_col + col]);
              if (*p_char == QCCWAVTCE3D_S)
                {
                  if (col == (subband_num_cols - 1))
                    p4[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER; 
                  else 
                    p4[row][col] =
                      alpha * p4[row][col+1] +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;;
                } 
              else
                {
                  v = (*p_char == QCCWAVTCE3D_S_NEW);
                  if (col == subband_num_cols - 1)
                    p4[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE + filter_coef * v;
                  else
                    p4[row][col] = alpha * p4[row][col+1] + filter_coef * v;
                }
            }
        }//aaa
      //	printf("frame=%d\n",frame);
      for (col = 0; col < subband_num_cols; col++)
        p5[col]=0;
      for (row = subband_num_rows - 1; row >= 0; row--)
        {
          for (col = 0; col < subband_num_cols; col++)
            {
              p3[row][col]=p3[row][col]+alpha*p5[col];
              if (col ==( subband_num_cols - 1))
                p5[col] =
                  p1[row][col] + alpha*p5[col]+alpha*QCCWAVTCE3D_BOUNDARY_VALUE ;
              else 
                p5[col] = p1[row][col] + alpha*p5[col] + alpha*p4[row][col+1];
            }
        }
    }//aaaa
  
  for (frame = subband_num_frames-1;frame >= 0; frame--)
    {//bbb
      current_frame=subband_origin_frame+frame;

      for (row = subband_num_rows - 1; row >= 0; row--)
        {//bb
          
          current_row = subband_origin_row + row;
          for (col = subband_num_cols-1; col >= 0; col--)
            {   
              current_col = subband_origin_col + col; 
              p_char = &(significance_map
                         [current_frame][current_row][current_col]);
              if (col ==( subband_num_cols-1) )
                p_estimation[current_frame][current_row][current_col] +=
                  alpha * (QCCWAVTCE3D_BOUNDARY_VALUE + p2[col] + p3[row][col]);
              else
                p_estimation[current_frame][current_row][current_col] +=
                  alpha *(p1[row][col+1]+p2[col]+p3[row][col]);
              
              if (*p_char == QCCWAVTCE3D_S)
                {
                  p_estimation[current_frame][current_row][current_col]=
                    QCCWAVTCE3D_REFINE_HOLDER * weight[0] +
                    p_estimation[current_frame][current_row][current_col] *
                    weight[1];
                  if (col == (subband_num_cols - 1))
                    p1[row][col] = alpha * QCCWAVTCE3D_BOUNDARY_VALUE +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else
                    p1[row][col] = alpha * p1[row][col+1] +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCWAVTCE3D_S_NEW);
                  p_estimation[current_frame][current_row][current_col] =
                    v * weight[0] +
                    p_estimation[current_frame][current_row][current_col] *
                    weight[1];
                  if (col == (subband_num_cols-1) )
                    p1[row][col] = alpha *QCCWAVTCE3D_BOUNDARY_VALUE +
                      filter_coef * v;
                  else
                    p1[row][col] = alpha * p1[row][col+1] + filter_coef * v;
                }    
              p2[col] = p1[row][col] + alpha * p2[col];   
            }
          
          for (col = 0; col < subband_num_cols; col++)
            {
	      if (col==0)
                p2[col] = p2[col] + alpha * QCCWAVTCE3D_BOUNDARY_VALUE;
	      else
                p2[col] = p2[col] + alpha * p4[row][col-1];
	      p3[row][col]=p2[col]+alpha*p3[row][col];
              current_col = subband_origin_col + col; 
              p_char = &(significance_map
                         [current_frame][current_row][current_col]);
              if (*p_char == QCCWAVTCE3D_S)
                {
	          if(col==0)
                    p4[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else 
                    p4[row][col] =
                      alpha * p4[row][col-1] +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCWAVTCE3D_S_NEW);
                  if(col==0)
                    p4[row][col] =
                      alpha * QCCWAVTCE3D_BOUNDARY_VALUE + filter_coef * v;
                  else 
                    p4[row][col] = alpha * p4[row][col - 1] + filter_coef *v;
                }        
            }
        }//bb
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          {
            p3[row][col] = p3[row][col] + alpha*p5[col];
            if (col==0)
              p5[col] = p1[row][col] + alpha*p5[col] +
                alpha* QCCWAVTCE3D_BOUNDARY_VALUE; 
            else
              p5[col] = p1[row][col] + alpha*p5[col] + alpha*p4[row][col-1];
          }
    }//bbb
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return: 
  QccVectorFree(p2); 
  QccVectorFree(p5);
  QccMatrixFree(p1, subband_num_rows); 
  QccMatrixFree(p3, subband_num_rows); 
  QccMatrixFree(p4, subband_num_rows); 
  return(return_value); 
}


static int QccWAVtce3DIPPass(QccWAVSubbandPyramid3D *coefficients,    
                             char ***significance_map,
                             char ***sign_array,
                             double threshold,
                             double ***p_estimation,
                             double *subband_significance,
                             QccENTArithmeticModel *model,
                             QccBitBuffer *buffer,
                             int *max_coefficient_bits,
                             double alpha)
{
  int subband,num_subbands; 
  int return_value;
  
  if (coefficients->transform_type ==
      QCCWAVSUBBANDPYRAMID3D_DYADIC)
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(coefficients->spatial_num_levels);
  else
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                         coefficients->spatial_num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)max_coefficient_bits[subband]))
        {
	  // printf("IP=%d\n",subband);
          return_value =
            QccWAVtce3DRevEst(coefficients,
                              significance_map,
                              subband,
                              subband_significance,
                              p_estimation,
                              alpha);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVtce3DIPPass): Error calling QccWAVtce3DRevEst()");
              return(1);
            }
          else
            {
              if (return_value == 2)
                return(2);
            }
          
          return_value = QccWAVtce3DIPBand(coefficients,
                                           significance_map,
                                           sign_array,
                                           p_estimation,
                                           subband_significance,
                                           subband,
                                           threshold,
                                           model,
                                           buffer,
                                           alpha);
          
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVtce3DIPPass): Error calling QccWAVtce3DIPBand()");
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


int static QccWAVtce3DSPass(QccWAVSubbandPyramid3D *coefficients,    
                            char ***significance_map,
                            double threshold,
                            QccENTArithmeticModel *model,
                            QccBitBuffer *buffer,
                            int *max_coefficient_bits)
{
  int subband,num_subbands; 
  int return_value;
  int subband_origin_frame;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  int frame,row, col;
  int current_frame, current_row, current_col;
  char *p_char;
  int symbol;
  double p = 0.4;
  
  if (coefficients->transform_type ==
      QCCWAVSUBBANDPYRAMID3D_DYADIC)
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(coefficients->spatial_num_levels);
  else
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                         coefficients->spatial_num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)(max_coefficient_bits[subband])))
        { 
          if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                                subband,
                                                &subband_num_frames,
                                                &subband_num_rows,
                                                &subband_num_cols))
            {
              QccErrorAddMessage("(QccWAVtce3DSPass): Error calling QccWAVSubbandPyramid3DSubbandSize()");
              return(1);
            }
          if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                                   subband,
                                                   &subband_origin_frame,
                                                   &subband_origin_row,
                                                   &subband_origin_col))
            {
              QccErrorAddMessage("(QccWAVtce3DSPass): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
              return(1);
            }  
          
          for (frame = 0; frame < subband_num_frames; frame++) 
            {
              current_frame = subband_origin_frame + frame;
              for (row = 0; row < subband_num_rows; row++)
                {
                  current_row = subband_origin_row + row;
                  for (col = 0; col < subband_num_cols; col++)
                    { 
                      current_col = subband_origin_col + col;
                      p_char = &(significance_map
                                 [current_frame][current_row][current_col]);
                      
                      if (*p_char == QCCWAVTCE3D_S)
                        { 
                          
                          if (QccWAVtce3DUpdateModel(model, p))
                            {
                              QccErrorAddMessage("(QccWAVtce3DSPass): Error calling QccWAVTce3DUpdateModel()");
                              return(1);
                            }
                          
                          if (buffer->type == QCCBITBUFFER_OUTPUT)
                            {
                              if (coefficients->volume
                                  [current_frame][current_row][current_col] >=
                                  threshold)
                                {
                                  symbol = 1;
                                  coefficients->volume
                                    [current_frame][current_row][current_col] -=
                                    threshold;
                                }
                              else
                                symbol = 0;
                              
                              return_value =
                                QccENTArithmeticEncode(&symbol,
                                                       1, model, buffer);
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
                                                         model, &symbol, 1))
                                return(2);
                              coefficients->volume
                                [current_frame][current_row][current_col] +=
                                (symbol == 1) ? threshold / 2 : -threshold / 2;
                            }
                          p =
                            p * QCCWAVTCE3D_ALPHA_1D +
                            symbol * QCCWAVTCE3D_ALPHA_1D_O;
                        }
                      else
                        {
                          if (*p_char == QCCWAVTCE3D_S_NEW)
                            *p_char = QCCWAVTCE3D_S;
                          if (*p_char == QCCWAVTCE3D_NZN_NEW)
                            *p_char = QCCWAVTCE3D_NZN;     
                        }
                    }
                }
            }
        }
    } 
  
  return(0);
}


int QccWAVtce3DEncode(const QccIMGImageCube *image,
                      QccBitBuffer *buffer,
                      int transform_type,
                      int temporal_num_levels,
                      int spatial_num_levels,
                      double alpha,
                      const QccWAVWavelet *wavelet,
                      int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid3D image_subband_pyramid;
  char ***sign_array = NULL;
  char ***significance_map = NULL;
  double ***p_estimation = NULL;
  double *subband_significance = NULL;
  double image_mean;
  int *max_coefficient_bits = NULL;
  double threshold;
  int num_symbols[1] = { 2 };
  int frame, row, col;
  int num_subbands;
  int subband;
  int bitplane = 0;
  int max_max_coefficient_bits = 0;

  if (image == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  
  image_subband_pyramid.spatial_num_levels = 0;
  image_subband_pyramid.temporal_num_levels = 0;
  image_subband_pyramid.num_frames = image->num_frames;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  
  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if ((sign_array =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((sign_array[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((sign_array[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
            goto Error;
          }
    }
  
  if ((significance_map =
       (char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((significance_map[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((significance_map[frame][row] =
             (char *)malloc(sizeof( char) *
                            image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        significance_map[frame][row][col] = QCCWAVTCE3D_Z;
  
  if ((p_estimation =
       (double ***)malloc(sizeof(double **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((p_estimation[frame] =
           (double **)malloc(sizeof(double *) *
                             image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((p_estimation[frame][row] =
             (double *)malloc(sizeof(double) *
                              image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        p_estimation[frame][row][col] = 0.0000001;
  
  if (transform_type ==
      QCCWAVSUBBANDPYRAMID3D_DYADIC)
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(spatial_num_levels);
  else
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(temporal_num_levels,
                                                         spatial_num_levels);
  
  if ((max_coefficient_bits =
       (int *)malloc(sizeof(int) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
      goto Error;
    }
  if ((subband_significance =
       (double*)malloc(num_subbands* sizeof(double))) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error allocating memory");
      goto Error;
    } 
  
  if (QccWAVtce3DEncodeDWT(&image_subband_pyramid,
                           sign_array,
                           image,
                           transform_type,
                           temporal_num_levels,
                           spatial_num_levels,
                           &image_mean,
                           max_coefficient_bits,                        
                           wavelet))
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error calling QccWAVtce3DEncodeDWT()");
      goto Error;
    }
  
  if (QccWAVtce3DEncodeHeader(buffer,
                              transform_type,
                              temporal_num_levels,
                              spatial_num_levels,
                              image->num_frames,
                              image->num_rows,
                              image->num_cols, 
                              image_mean,
                              max_coefficient_bits,
                              alpha))
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error calling QccWAVtce3DEncodeHeader()");
      goto Error;
    }
  
  if (QccWAVtce3DEncodeBitPlaneInfo(buffer,
                                    num_subbands,
                                    max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error calling QccWAVtce3DEncodeBitPlaneInfo()");
      goto Error;
    }
  
  if ((model = QccENTArithmeticEncodeStart(num_symbols,
					   1,
					   NULL,
					   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);
  
  for (subband = 0; subband < num_subbands; subband++)
    if (max_max_coefficient_bits < max_coefficient_bits[subband])
      max_max_coefficient_bits = max_coefficient_bits[subband];
  
  threshold = pow((double)2, (double)max_max_coefficient_bits);
  
  while (bitplane < QCCWAVTCE3D_MAXBITPLANES)  
    {
      return_value = QccWAVtce3DNZNPass(&image_subband_pyramid,
				        significance_map,
                                        sign_array,
                                        threshold,
                                        subband_significance,
                                        model,
                                        buffer,
				        max_coefficient_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DEncode): Error calling QccWAVtce3DNZNPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
      
      return_value = QccWAVtce3DIPPass(&image_subband_pyramid,
                                       significance_map,
                                       sign_array,
                                       threshold,
                                       p_estimation,
                                       subband_significance,
                                       model,
                                       buffer,
                                       max_coefficient_bits,
                                       alpha);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DEncode): Error calling QccWAVtce3DIPPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
      
      return_value = QccWAVtce3DSPass(&image_subband_pyramid, 
                                      significance_map,
                                      threshold,
                                      model,
                                      buffer,
                                      max_coefficient_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DEncode): Error calling QccWAVtce3DSPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }  
      
      threshold =floor( threshold/ 2.0);
      
      bitplane++;
    }
  
  if (QccENTArithmeticEncodeEnd(model,
                                0,
                                buffer))
    {
      QccErrorAddMessage("(QccENTArithmeticEncodeChannel): Error calling QccENTArithmeticEncodeEnd()");
      goto Error;
    }  
  
 Finished:
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  if (significance_map != NULL)
    {
      for (frame = 0; frame < image->num_frames; frame++)
        {
          if (significance_map[frame] != NULL)
            {
              for (row = 0; row < image->num_rows; row++)
                if (significance_map[frame][row] != NULL)
                  QccFree(significance_map[frame][row]);
              QccFree(significance_map[frame]);
            }
        }
      QccFree(significance_map);
    }
  if (sign_array != NULL)
    {
      for (frame = 0; frame < image->num_frames; frame++)
        {
          if (sign_array[frame] != NULL)
            {
              for (row = 0; row < image->num_rows; row++)
                if (sign_array[frame][row] != NULL)
                  QccFree(sign_array[frame][row]);
              QccFree(sign_array[frame]);
            }
        }
      QccFree(sign_array);
    }
  if (p_estimation != NULL)
    {
      for (frame = 0; frame < image->num_frames; frame++)
        {
          if (p_estimation[frame] != NULL)
            {
              for (row = 0; row < image->num_rows; row++)
                if (p_estimation[frame][row] != NULL)
                  QccFree(p_estimation[frame][row]);
              QccFree(p_estimation[frame]);
            }
        }
      QccFree(p_estimation);
    }
  if (subband_significance != NULL)
    QccFree(subband_significance);
  if (max_coefficient_bits != NULL)
    QccFree(max_coefficient_bits);
  QccENTArithmeticFreeModel(model);
  
  return(return_value);
}


static int QccWAVtce3DDecodeInverseDWT(QccWAVSubbandPyramid3D
				       *image_subband_pyramid, 
				       char ***sign_array,
                                       QccIMGImageCube *image,
                                       double image_mean,
                                       const QccWAVWavelet *wavelet)
{
  int frame, row, col;
  
  for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (sign_array[frame][row][col])
          image_subband_pyramid->volume[frame][row][col] *= -1;
  
  if (QccWAVSubbandPyramid3DInverseDWT(image_subband_pyramid,
                                       wavelet))
    {
      QccErrorAddMessage("(QccWAVtce3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DInverseDWT()");
      return(1);
    }
  
  if (QccWAVSubbandPyramid3DAddMean(image_subband_pyramid,
                                    image_mean))
    {
      QccErrorAddMessage("(QccWAVtce3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DAddMean()");
      return(1);
    }
  
  if (QccVolumeCopy(image->volume,
                    image_subband_pyramid->volume,
                    image->num_frames,
                    image->num_rows,
                    image->num_cols))
    {
      QccErrorAddMessage("(QccWAVtce3DDecodeInverseDWT): Error calling QccVolumeCopy()");
      return(1);
    }
  
  if (QccIMGImageCubeSetMaxMin(image))
    {
      QccErrorAddMessage("(QccWAVtce3DDecodeInverseDWT): QccError calling QccIMGImageCubeSetMaxMin()");
      return(1);
    }
  
  return(0);
}


int QccWAVtce3DDecodeHeader(QccBitBuffer *buffer, 
                            int *transform_type,
                            int *temporal_num_levels, 
                            int *spatial_num_levels, 
                            int *num_frames,
                            int *num_rows,
                            int *num_cols,
                            double *image_mean,
                            int *max_coefficient_bits,
                            double *alpha)
{
  int return_value;
  unsigned char ch;
  
  if (QccBitBufferGetBit(buffer, transform_type))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetBit()");
      goto Error;
    }
  
  if (*transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      if (QccBitBufferGetChar(buffer, &ch))
        {
          QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetChar()");
          goto Error;
        }
      *temporal_num_levels = (int)ch;
    }
  
  if (QccBitBufferGetChar(buffer, &ch))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  *spatial_num_levels = (int)ch;
  
  if (*transform_type == QCCWAVSUBBANDPYRAMID3D_DYADIC)
    *temporal_num_levels = *spatial_num_levels;
  
  if (QccBitBufferGetInt(buffer, num_frames))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(buffer, alpha))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVtce3DDecode(QccBitBuffer *buffer,
                      QccIMGImageCube *image,                     
                      int transform_type,
                      int temporal_num_levels,
                      int spatial_num_levels,
                      double alpha,
                      const QccWAVWavelet *wavelet,
                      double image_mean,
                      int max_coefficient_bits,
                      int target_bit_cnt)
  
{
  
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid3D image_subband_pyramid;
  char ***sign_array = NULL;
  char ***significance_map = NULL;
  double ***p_estimation = NULL;
  double *subband_significance = NULL;
  int *max_bits = NULL;
  double threshold;
  int frame, row, col;
  int num_symbols[1] = { 2 };
  int num_subbands;
  int subband;
  int max_max_coefficient_bits = 0;
  
  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  
  if (image == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  image_subband_pyramid.transform_type = transform_type;
  image_subband_pyramid.temporal_num_levels = temporal_num_levels;
  image_subband_pyramid.spatial_num_levels = spatial_num_levels;
  image_subband_pyramid.num_frames = image->num_frames;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  
  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    }
  
  if ((sign_array =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVTce3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((sign_array[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVTce3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((sign_array[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVTce3DDecode): Error allocating memory");
            goto Error;
          }
    }
  
  if ((significance_map =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVTce3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((significance_map[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVTce3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((significance_map[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVTce3DDecode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        significance_map[frame][row][col] = QCCWAVTCE3D_Z;
  
  if (transform_type ==
      QCCWAVSUBBANDPYRAMID3D_DYADIC)
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(spatial_num_levels);
  else
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(temporal_num_levels,
                                                         spatial_num_levels);
  
  if ((max_bits = (int *)malloc(sizeof(int) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DDecode): Error allocating memory");
      goto Error;
    }
  
  if (QccWAVtce3DDecodeBitPlaneInfo(buffer,
                                    num_subbands,
                                    max_bits,
                                    max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtce3DDecode): Error calling QccWAVtceDecodeBitPlaneInfo()");
      goto Error;
    }
  
  if ((p_estimation =
       (double ***)malloc(sizeof(double **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((p_estimation[frame] =
           (double **)malloc(sizeof(double *) *
                             image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((p_estimation[frame][row] =
             (double *)malloc(sizeof(double) *
                              image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DDecode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        p_estimation[frame][row][col] = 0.0000001;
  
  if ((subband_significance =
       (double*)calloc(num_subbands, sizeof(double))) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DDecode): Error allocating memory");
      goto Error;
    }  
  
  if ((model =
       QccENTArithmeticDecodeStart(buffer,
				   num_symbols,
				   1,
				   NULL,
                                   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);

  for (subband = 0; subband <num_subbands; subband++)
    if (max_max_coefficient_bits <= max_bits[subband])
      max_max_coefficient_bits = max_bits[subband];
  
  threshold = pow((double)2, (double)max_max_coefficient_bits);
  
  while (1)
    {
      return_value = QccWAVtce3DNZNPass(&image_subband_pyramid,
                                        significance_map,
                                        sign_array,
                                        threshold,
                                        subband_significance,
                                        model,
                                        buffer,
                                        max_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DDecode): Error calling QccWAVtce3DNZNPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
      
      return_value = QccWAVtce3DIPPass(&image_subband_pyramid,
                                       significance_map,
                                       sign_array,
                                       threshold,
                                       p_estimation,
                                       subband_significance,
                                       model,
                                       buffer,
                                       max_bits,
                                       alpha);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DDecode): Error calling QccWAVtce3DIPPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
      
      return_value = QccWAVtce3DSPass(&image_subband_pyramid, 
                                      significance_map,
                                      threshold,
                                      model,
                                      buffer,
                                      max_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DDecode): Error calling QccWAVtce3DSPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }  
      
      threshold /= 2.0;
    }
  
 Finished:
  if (QccWAVtce3DDecodeInverseDWT(&image_subband_pyramid,
                                  sign_array,
                                  image,
                                  image_mean,
                                  wavelet))
    {
      QccErrorAddMessage("(QccWAVtce3DDecode): Error calling QccWAVtce3DDecodeInverseDWT()");
      goto Error;
    }
  
  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  
  if (significance_map!= NULL)
    {
      for (frame = 0; frame < image->num_frames; frame++)
        {
          if (significance_map[frame] != NULL)
            {
              for (row = 0; row < image->num_rows; row++)
                if (significance_map[frame][row] != NULL)
                  QccFree(significance_map[frame][row]);
              QccFree(significance_map[frame]);
            }
        }
      QccFree(significance_map);
    }
  if (sign_array!= NULL)
    {
      for (frame = 0; frame < image->num_frames; frame++)
        {
          if (sign_array[frame] != NULL)
            {
              for (row = 0; row < image->num_rows; row++)
                if (sign_array[frame][row] != NULL)
                  QccFree(sign_array[frame][row]);
              QccFree(sign_array[frame]);
            }
        }
      QccFree(sign_array);
    }
  if (p_estimation != NULL)
    {
      for (frame = 0; frame < image->num_frames; frame++)
        {
          if (p_estimation[frame] != NULL)
            {
              for (row = 0; row < image->num_rows; row++)
                if (p_estimation[frame][row] != NULL)
                  QccFree(p_estimation[frame][row]);
              QccFree(p_estimation[frame]);
            }
        }
      QccFree(p_estimation);
    }
  if (subband_significance != NULL)
    QccFree(subband_significance);
  if (max_bits != NULL)
    QccFree(max_bits);
  QccENTArithmeticFreeModel(model);

  return(return_value);
}
