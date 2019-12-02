/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2007  James E. Fowler
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

#define QCCTCE3D_BOUNDARY_VALUE (1e-6)

//definition used in context information
#define QCCTCE3D_Z 0 //zero, AKA, insignificant
#define QCCTCE3D_NZN_NEW 2 //non-zero-neighbor
#define QCCTCE3D_S 3 //significant
#define QCCTCE3D_S_NEW 4
#define QCCTCE3D_NZN 5

// more refinement can be made if direction of Tarp filter is considered
// but the improvement is minor


//ugly fix, use 1-D IIR filter to provide PMF estimate
#define QCCWAVTCE3D_ALPHA_1D 0.995 
#define QCCWAVTCE3D_ALPHA_1D_O 0.005

// this should be 0.5, however, 0.3 seems to work a little better (minor)
// reason unknown
#define QCCWAVTCE3D_REFINE_HOLDER 0.3 

// threshold for cross-scale prediction...
#define QCCWAVTCE3D_PREDICT_THRESHOLD 0.05  
//weight factor for cross-scale and current scale
#define QCCWAVTCE3D_CURRENT_SCALE 0.6 
#define QCCWAVTCE3D_PARENT_SCALE 0.4



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


static int QccWAVTce3DUpdateModel(QccENTArithmeticModel *model, double prob)
{
  double p[2];

  p[0] = 1 - prob;
  p[1] = prob;

  if (QccENTArithmeticSetModelProbabilities(model,
                                            p,
                                            0))
    {
       
      QccErrorAddMessage("(QccWAVTarp3DUpdateModel): Error calling QccENTArithmeticSetModelProbabilities()");
      return(1);
    }

  return(0);
}


static int QccWAVtce3DLosslessEncodeDWT(QccWAVSubbandPyramid3DInt
                                        *image_subband_pyramid,
                                        char ***sign_array,
                                        const QccIMGImageCube *image,
                                        int transform_type,
                                        int temporal_num_levels,
                                        int spatial_num_levels,
                                        int *image_mean,
                                        int *max_coefficient_bits,
                                        const QccWAVWavelet *wavelet)
{
  int coefficient_magnitude;
  int max_coefficient = -MAXINT;
  int frame, row, col;
  int num_subbands,subband;
  int subband_origin_frame;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;

  num_subbands =
    QccWAVSubbandPyramid3DIntNumLevelsToNumSubbandsPacket(temporal_num_levels,
                                                          spatial_num_levels);
 
  for (frame = 0; frame <image->num_frames; frame++)
    for (row = 0; row <image->num_rows; row++)
      for (col = 0; col <image->num_cols; col++)
        image_subband_pyramid->volume[frame][row][col] =
          (int)image->volume[frame][row][col];

  if (QccWAVSubbandPyramid3DIntSubtractMean(image_subband_pyramid,
                                            image_mean ))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeDWT): Error calling QccWAVSubbandPyramid3DIntSubtractMean()");
      return(1);
    }

  if (QccWAVSubbandPyramid3DIntDWT(image_subband_pyramid,
                                   transform_type,
                                   temporal_num_levels,
                                   spatial_num_levels,
                                   wavelet))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeDWT): Error calling QccWAVSubbandPyramid3DIntDWT()");
      return(1);
    }
 
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramid3DIntSubbandSize(image_subband_pyramid,
                                               subband,
                                               &subband_num_frames,
                                               &subband_num_rows,
                                               &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessEncodeDWT): Error calling QccWAVSubbandPyramid3DIntSubbandSize(()");
          return(1);
        }
      if (QccWAVSubbandPyramid3DIntSubbandOffsets(image_subband_pyramid,
                                                  subband,
                                                  &subband_origin_frame,
                                                  &subband_origin_row,
                                                  &subband_origin_col))
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessEncodeDWT): Error calling QQccWAVSubbandPyramid3DIntSubbandOffsets()");
          return(1);
        }

      max_coefficient = -MAXINT;
  
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
                  sign_array
                    [subband_origin_frame+frame]
                    [subband_origin_row + row]
                    [subband_origin_col + col] = 1;
              
                }
              else
                sign_array
                  [subband_origin_frame+frame]
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


int QccWAVtce3DLosslessEncodeHeader(QccBitBuffer *buffer, 
                                    int transform_type,
                                    int temporal_num_levels,
                                    int spatial_num_levels,
                                    int num_frames,
                                    int num_rows,
                                    int num_cols,
                                    int image_mean,
                                    int *max_coefficient_bits,
                                    double alpha)
{
  int return_value;
  
  if (QccBitBufferPutBit(buffer, transform_type))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeHeader): Error calling QccBitBufferPutBit()");
      goto Error;
    }
  
  if (transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      if (QccBitBufferPutChar(buffer, (unsigned char)temporal_num_levels))
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessEncodeHeader): Error calling QccBitBufferPuChar()");
          goto Error;
        }
    }
  
  if (QccBitBufferPutChar(buffer, (unsigned char)spatial_num_levels))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_frames))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    } 
 
  if (QccBitBufferPutInt(buffer, max_coefficient_bits[0]))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }

  if (QccBitBufferPutDouble(buffer, alpha))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVtce3DIntRevEst(QccWAVSubbandPyramid3DInt *coefficients,
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
  int current_frame,current_row, current_col;
  QccMatrix p1=NULL;
  QccVector p2 = NULL;
  QccMatrix p3=NULL;
  QccMatrix p4=NULL;
  QccVector p5 = NULL;
  char *p_char;
  int v;  
  double filter_coef;
  
  filter_coef = (1 - alpha) * (1 - alpha)*(1 - alpha) /
    (3*alpha + alpha*alpha*alpha)/3;
  
  subband_significance[subband] = 0.0;
  if (QccWAVSubbandPyramid3DIntSubbandSize(coefficients,
                                           subband,
                                           &subband_num_frames,
                                           &subband_num_rows,
                                           &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVtce3DIntRevEst): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      goto Error;
    }
  if (QccWAVSubbandPyramid3DIntSubbandOffsets(coefficients,
                                              subband,
                                              &subband_origin_frame,
                                              &subband_origin_row,
                                              &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVtce3DIntRevEst): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      goto Error;
    }
  
  if ((p1 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntRevEst): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((p3 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p4 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p5 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntRevEst): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < subband_num_rows; row++)
    for (col = 0; col < subband_num_cols; col++)
      p3[row][col]=QCCTCE3D_BOUNDARY_VALUE;  
	
  for (frame = subband_num_frames-1;frame >= 0; frame--)
    {//bbb
      current_frame=subband_origin_frame+frame;

      for (col = 0; col < subband_num_cols; col++)
        p2[col] = QCCTCE3D_BOUNDARY_VALUE; 
       
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
                  alpha * (QCCTCE3D_BOUNDARY_VALUE + p2[col] + p3[row][col]);
              else
                p[current_frame][current_row][current_col] =
                  alpha *(p1[row][col+1]+p2[col]+p3[row][col]);
              
              if (*p_char == QCCTCE3D_S)
                {
                  if (col == subband_num_cols-1 )
                    p1[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else

                    p1[row][col] = alpha * p1[row][col+1] +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                   
                  subband_significance[subband] += QCCWAVTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCTCE3D_S_NEW);
                  if (col == (subband_num_cols-1) )
                    p1[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE +
                      filter_coef * v;
                  else
                    p1[row][col] = alpha * p1[row][col+1] + filter_coef * v;
                   
                  subband_significance[subband] += v;
                }   
              p2[col] = p1[row][col] + alpha * p2[col];   
            }
      
          for (col = 0; col < subband_num_cols; col++)
            {
              p_char = &(significance_map
                         [current_frame][current_row]
                         [subband_origin_col + col]);
              if(col==0)
                p2[col] = p2[col] + alpha * QCCTCE3D_BOUNDARY_VALUE;
              else
                p2[col] = p2[col] + alpha * p4[row][col-1];
                
              p3[row][col]=p2[col]+alpha*p3[row][col];
              if (*p_char == QCCTCE3D_S)
                {
                  if(col==0)
                    p4[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else 
                    p4[row][col] = alpha * p4[row][col-1] +
                      filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCTCE3D_S_NEW);
                  if(col==0)
                    p4[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE +
                      filter_coef * v;
                  else 
                    p4[row][col] = alpha* p4[row][col-1] + filter_coef *v;
                }        
            }
        }//bb
      for (col = 0; col < subband_num_cols; col++)
        p5[col] = QCCTCE3D_BOUNDARY_VALUE;
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          {
            p3[row][col]=p3[row][col]+alpha*p5[col];
            if(col==0)
              p5[col]=p1[row][col]+alpha*p5[col]+alpha*QCCTCE3D_BOUNDARY_VALUE;
            else
              p5[col]=p1[row][col]+alpha*p5[col]+alpha*p4[row][col-1];
          }
    }//bbb
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
 
  QccVectorFree(p2);
  QccVectorFree(p5);
  QccMatrixFree(p1,subband_num_rows); 
  QccMatrixFree(p3,subband_num_rows); 
  QccMatrixFree(p4,subband_num_rows); 
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
				   char *** significance_map,
				   int subband)
{
  //Update neighbors
  int current_frame,current_row, current_col;
  int shifted_frame,shifted_row, shifted_col;
  current_frame=subband_origin_frame+frame;
  current_row = subband_origin_row + row;
  current_col = subband_origin_col + col;
  //previous frame
  if(frame>0)
    {
      if (row > 0)
        { 
          shifted_frame = current_frame - 1; 
          shifted_row = current_row - 1;
          shifted_col = current_col;
          if (significance_map[shifted_frame][shifted_row][shifted_col] <
              QCCTCE3D_S)
            significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCTCE3D_NZN_NEW;
      
          if (col > 0)
            {
              shifted_frame = current_frame - 1;
              shifted_row = current_row - 1;
              shifted_col = current_col - 1;
              if (significance_map[shifted_frame][shifted_row][shifted_col] <
                  QCCTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCTCE3D_NZN_NEW;   
            }
          if (col < subband_num_col - 1)
            {
              shifted_frame = current_frame - 1;
              shifted_row = current_row - 1;
              shifted_col = current_col + 1;
              if (significance_map[shifted_frame][shifted_row][shifted_col] <
                  QCCTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCTCE3D_NZN_NEW;   
            }
        }
      if (row < subband_num_row - 1)
        {
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row + 1;
	  shifted_col = current_col;
	  if (significance_map[shifted_frame][shifted_row][shifted_col] <
              QCCTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCTCE3D_NZN_NEW;
      
	  if (col > 0)
	    {
	      shifted_frame = current_frame - 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col - 1;
	      if (significance_map[shifted_frame][shifted_row][shifted_col] <
                  QCCTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCTCE3D_NZN_NEW;  
	    }
	  if (col < subband_num_col - 1)
	    {
	      shifted_frame = current_frame - 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col + 1;
	      if (significance_map[shifted_frame][shifted_row][shifted_col] <
                  QCCTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCTCE3D_NZN_NEW;   
	    }
	}
      if (col > 0)
	{
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row;
	  shifted_col = current_col - 1;  
	  if (significance_map[shifted_frame][shifted_row][shifted_col] <
              QCCTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCTCE3D_NZN_NEW;
	}
      if (col < subband_num_col - 1)
	{
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row;
	  shifted_col = current_col + 1;
	  if (significance_map[shifted_frame][shifted_row][shifted_col] <
              QCCTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCTCE3D_NZN_NEW;
	}
      {
	shifted_frame = current_frame - 1;
	shifted_row = current_row;
	shifted_col = current_col;
	if (significance_map[shifted_frame][shifted_row][shifted_col] <
            QCCTCE3D_S)
	  significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCTCE3D_NZN_NEW;
      }

    }
  //next frame

  if (frame<subband_num_frame - 1)
    {
      if (row > 0)
        { 
          shifted_frame = current_frame + 1; 
          shifted_row = current_row - 1;
          shifted_col = current_col;
          if (significance_map[shifted_frame][shifted_row][shifted_col] <
              QCCTCE3D_S)
            significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCTCE3D_NZN_NEW;
      
          if (col > 0)
            {
              shifted_frame = current_frame + 1;
              shifted_row = current_row - 1;
              shifted_col = current_col - 1;
              if (significance_map[shifted_frame][shifted_row][shifted_col] <
                  QCCTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCTCE3D_NZN_NEW;   
            }
          if (col < subband_num_col - 1)
            {
              shifted_frame = current_frame + 1;
              shifted_row = current_row - 1;
              shifted_col = current_col + 1;
              if (significance_map[shifted_frame][shifted_row][shifted_col] <
                  QCCTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCTCE3D_NZN_NEW;   
            }
        }
      if (row < subband_num_row - 1)
	{
	  shifted_frame = current_frame+ 1;
	  shifted_row = current_row + 1;
	  shifted_col = current_col;
	  if (significance_map[shifted_frame][shifted_row][shifted_col] <
              QCCTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCTCE3D_NZN_NEW;
      
	  if (col > 0)
	    {
	      shifted_frame = current_frame + 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col - 1;
	      if (significance_map[shifted_frame][shifted_row][shifted_col] <
                  QCCTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCTCE3D_NZN_NEW;  
	    }
	  if (col < subband_num_col - 1)
	    {
	      shifted_frame = current_frame + 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col + 1;
	      if (significance_map[shifted_frame][shifted_row][shifted_col] <
                  QCCTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] =
                  QCCTCE3D_NZN_NEW;   
	    }
	}
      if (col > 0)
	{
	  shifted_frame = current_frame + 1;
	  shifted_row = current_row;
	  shifted_col = current_col - 1;  
	  if (significance_map[shifted_frame][shifted_row][shifted_col] <
              QCCTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCTCE3D_NZN_NEW;
	}
      if (col < subband_num_col - 1)
	{
	  shifted_frame = current_frame + 1;
	  shifted_row = current_row;
	  shifted_col = current_col + 1;
	  if (significance_map[shifted_frame][shifted_row][shifted_col] <
              QCCTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] =
              QCCTCE3D_NZN_NEW;
	}
      {
	shifted_frame = current_frame + 1;
	shifted_row = current_row;
	shifted_col = current_col;
	if (significance_map[shifted_frame][shifted_row][shifted_col] <
            QCCTCE3D_S)
	  significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCTCE3D_NZN_NEW;
      }

    }


  //current frame
  {
    if (row > 0)
      { 
        shifted_frame = current_frame ; 
        shifted_row = current_row - 1;
        shifted_col = current_col;
        if (significance_map[shifted_frame][shifted_row][shifted_col] <
            QCCTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCTCE3D_NZN_NEW;
      
        if (col > 0)
          {
            shifted_frame = current_frame;
            shifted_row = current_row - 1;
            shifted_col = current_col - 1;
            if (significance_map[shifted_frame][shifted_row][shifted_col] <
                QCCTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] =
                QCCTCE3D_NZN_NEW;   
          }
        if (col < subband_num_col - 1)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row - 1;
            shifted_col = current_col + 1;
            if (significance_map[shifted_frame][shifted_row][shifted_col] <
                QCCTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] =
                QCCTCE3D_NZN_NEW;   
          }
      }
    if (row < subband_num_row - 1)
      {
        shifted_frame = current_frame;
        shifted_row = current_row + 1;
        shifted_col = current_col;
        if (significance_map[shifted_frame][shifted_row][shifted_col] <
            QCCTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCTCE3D_NZN_NEW;
      
        if (col > 0)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row + 1;
            shifted_col = current_col - 1;
            if (significance_map[shifted_frame][shifted_row][shifted_col] <
                QCCTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] =
                QCCTCE3D_NZN_NEW;  
          }
        if (col < subband_num_col - 1)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row + 1;
            shifted_col = current_col + 1;
            if (significance_map[shifted_frame][shifted_row][shifted_col] <
                QCCTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] =
                QCCTCE3D_NZN_NEW;   
          }
      }
    if (col > 0)
      {
        shifted_frame = current_frame ;
        shifted_row = current_row;
        shifted_col = current_col - 1;  
        if (significance_map[shifted_frame][shifted_row][shifted_col] <
            QCCTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCTCE3D_NZN_NEW;
      }
    if (col < subband_num_col - 1)
      {
        shifted_frame = current_frame ;
        shifted_row = current_row;
        shifted_col = current_col + 1;
        if (significance_map[shifted_frame][shifted_row][shifted_col] <
            QCCTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] =
            QCCTCE3D_NZN_NEW;
      }
  }
  
  return(0);
}



static int QccWAVtce3DIntNZNPass(QccWAVSubbandPyramid3DInt *coefficients,    
                                 char ***significance_map,
                                 char ***sign_array,
                                 int threshold,
                                 double *subband_significance,
                                 QccENTArithmeticModel *model,
                                 QccBitBuffer *buffer,
                                 int *max_coefficient_bits)
{ 
  int subband, num_subbands;
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

  num_subbands =
    QccWAVSubbandPyramid3DIntNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                          coefficients->spatial_num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)(max_coefficient_bits[subband])))
        { 
          if (QccWAVSubbandPyramid3DIntSubbandSize(coefficients,
                                                   subband,
                                                   &subband_num_frames,
                                                   &subband_num_rows,
                                                   &subband_num_cols))
            {
              QccErrorAddMessage("(QccWAVtce3DIntNZNPass): Error calling QccWAVSubbandPyramid3DIntSubbandSize()");
              return(1);
            }
          if (QccWAVSubbandPyramid3DIntSubbandOffsets(coefficients,
                                                      subband,
                                                      &subband_origin_frame,
                                                      &subband_origin_row,
                                                      &subband_origin_col))
            {
              QccErrorAddMessage("(QccWAVtce3DIntNZNPass): Error calling QccWAVSubbandPyramid3DIntSubbandOffsets()");
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
                      if (significance_map[current_frame][current_row][current_col] == QCCTCE3D_NZN)
                        {
                          if (QccWAVTce3DUpdateModel(model, p))
                            {
                              QccErrorAddMessage("(QccWAVtce3DIntNZNPass): Error calling QccWAVTce3DUpdateModel()");
                              return_value = 1;
                              return(1);
                            }
                          if (buffer->type == QCCBITBUFFER_OUTPUT)
                            {
                              if (coefficients->volume[current_frame][current_row][current_col] >=
                                  threshold)
                                {
                                  symbol = 1;
                                  coefficients->volume[current_frame][current_row][current_col] -=
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
                                coefficients->volume[current_frame][current_row][current_col] =
                                  
                                  threshold + (threshold >> 1);

                            }

                          p =
                            p * QCCWAVTCE3D_ALPHA_1D + symbol * QCCWAVTCE3D_ALPHA_1D_O;
			   
                          if (symbol)
                            {
                              subband_significance[subband]++;
                              significance_map[current_frame][current_row][current_col] =
                                QCCTCE3D_S_NEW;
                          
                              if (QccWAVTce3DUpdateModel(model, 0.5))
                                {
                                  QccErrorAddMessage("(QccWAVtce3DIntNZNPass): Error calling QccWAVTce3DUpdateModel()");
                                  return(1);
                                }
                          
                              if (buffer->type == QCCBITBUFFER_OUTPUT)
                                {
                                  symbol =
                                    (int)(sign_array[current_frame][current_row][current_col]);
                              
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
                              
                                  sign_array[current_frame][current_row][current_col] =
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

static int QccWAVtce3DIntIPBand(QccWAVSubbandPyramid3DInt *coefficients,
                                char ***significance_map,
                                char ***sign_array,
                                double ***p_estimation,
                                double *subband_significance,
                                int subband,
                                int threshold,
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
  int current_frame,current_row, current_col;
  QccMatrix p1=NULL;
  QccVector p2 = NULL;
  QccMatrix p3=NULL;
  QccMatrix p4=NULL;
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
  filter_coef = (1 - alpha) * (1 - alpha)*(1 - alpha) / (3*alpha + alpha*alpha*alpha)/3;
 
  weight[0] = (1 - alpha) * (1 - alpha)* (1 - alpha)/((1 + alpha)*(1 + alpha)*(1 + alpha));
  weight[1] = (2*3*alpha + 2*alpha*alpha*alpha) / ((1 + alpha)*(1 + alpha)*(1 + alpha));

  if (QccWAVSubbandPyramid3DIntSubbandSize(coefficients,
                                           subband,
                                           &subband_num_frames,
                                           &subband_num_rows,
                                           &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccWAVSubbandPyramid3DIntSubbandSize()");
      goto Error;
    }
  if (QccWAVSubbandPyramid3DIntSubbandOffsets(coefficients,
                                              subband,
                                              &subband_origin_frame,
                                              &subband_origin_row,
                                              &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccWAVSubbandPyramid3DIntSubbandOffsets()");
      goto Error;
    }
  
  if ((p1 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((p3 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p4 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p5 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccVectorAlloc()");
      goto Error;
    }

  child_density =
    subband_significance[subband] / (subband_num_frames*subband_num_rows * subband_num_cols);
  p_lowerbound = 1.0 / (subband_num_frames*subband_num_rows * subband_num_cols);
  if ((subband%(3*coefficients->spatial_num_levels+1))<=3)
    {
      if ((subband%(3*coefficients->spatial_num_levels+1))==0)
	{
	  if ((subband/(3*coefficients->spatial_num_levels+1))<=1)
	    {
	      parent_subband = 0;
              parent_density =
                subband_significance[parent_subband] /
		(subband_num_frames*subband_num_rows * subband_num_cols) ;
	    }
	  else
	    {
	      parent_subband = subband-(3*coefficients->spatial_num_levels+1);
              parent_density =
                subband_significance[parent_subband] /
		(subband_num_frames*subband_num_rows * subband_num_cols)*2 ;
	    }
	}

      if ((subband%(3*coefficients->spatial_num_levels+1))==1)
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
  
  for (row = 0; row < subband_num_rows; row++)
    for (col = 0; col < subband_num_cols; col++)
      p3[row][col]=QCCTCE3D_BOUNDARY_VALUE;  

  scale =
    QccMathMax(child_density, p_lowerbound) /
    QccMathMax(parent_density, p_lowerbound); 
  increment =
    1.0 / (subband_num_frames*subband_num_rows * subband_num_cols) /
    QccMathMax(parent_density, p_lowerbound);
   
  for (frame = 0; frame < subband_num_frames; frame++)
    {//aaaa
      current_frame=subband_origin_frame+frame;
      for (col = 0; col < subband_num_cols; col++)
        p2[col] =QCCTCE3D_BOUNDARY_VALUE;  ;

      for (row = 0; row < subband_num_rows; row++)
        {  //aaa  
          current_row = subband_origin_row + row;
          for (col = 0; col < subband_num_cols; col++)
            { //aa  
              current_col = subband_origin_col + col;
              p_char = &(significance_map[current_frame][current_row][current_col]);
              if (col == 0)
                p_forward =alpha * (QCCTCE3D_BOUNDARY_VALUE + p2[col] +
                                    p3[row][col]);
              else
                p_forward = alpha * (p1[row][col-1] + p2[col] + p3[row][col]);
          
              if (*p_char < QCCTCE3D_S)
                { //a     
                  p = p_forward + p_estimation[current_frame][current_row][current_col];

                  if (subband != 0)
                    { //b
                      if ((subband%(3*coefficients->spatial_num_levels+1))<=3)
                        {
                          if ((subband%(3*coefficients->spatial_num_levels+1))==0)
                            {
                              if((subband/(3*coefficients->spatial_num_levels+1))<=1)
                                p_parent = p_estimation[0][current_row][current_col];
	   
                              else
                                p_parent = p_estimation[current_frame/2][current_row][current_col];
                            } 

                          if ((subband%(3*coefficients->spatial_num_levels+1))==1)
                            p_parent = p_estimation[current_frame][current_row-subband_num_rows][current_col];

                          if ((subband%(3*coefficients->spatial_num_levels+1))==2)
                            p_parent = p_estimation[current_frame][current_row][current_col-subband_num_cols];

                          if ((subband%(3*coefficients->spatial_num_levels+1))==3)
                            p_parent = p_estimation[current_frame][current_row-subband_num_rows][current_col-subband_num_cols];
			       

                        }
    
                      else
              
                        p_parent = p_estimation[current_frame][current_row/2][current_col/2];

                      if (p < QCCWAVTCE3D_PREDICT_THRESHOLD)
                        {          
                          p_parent = QccMathMin(p_parent*scale, 0.8);
                          p =
                            p * QCCWAVTCE3D_CURRENT_SCALE +
                            QCCWAVTCE3D_PARENT_SCALE * p_parent;
                        }
                    }//b
                  if(p>1)
                    {
			      
                      p=1;
                    }
                  if (QccWAVTce3DUpdateModel(model, p))
                    {
                      QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccWAVtceUpdateModel()");
                      return_value = 1;
                      goto Error;
                    }
                  if (buffer->type == QCCBITBUFFER_OUTPUT)
                    {
                      if (coefficients->volume[current_frame][current_row][current_col] >=
                          threshold)
                        {
                          symbol = 1;
                          coefficients->volume[current_frame][current_row][current_col] -=
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
                        coefficients->volume[current_frame][current_row][current_col] =
                           
                          threshold + (threshold >> 1);

                    }
                  v = symbol;
	                
                  if (symbol)
                    {//c
                      subband_significance[subband]++;
                      scale += increment;
                      *p_char = QCCTCE3D_S_NEW;
                  
                      if (QccWAVTce3DUpdateModel(model, 0.5))
                        {
                          QccErrorAddMessage("(QccWAVtce3DIntIPBand): Error calling QccWAVtceUpdateModel()");
                          goto Error;
                        }
                  
                      if (buffer->type == QCCBITBUFFER_OUTPUT)
                        {
                          symbol = (int)(sign_array[current_frame][current_row][current_col]);
                          return_value =
                            QccENTArithmeticEncode(&symbol, 1,model, buffer);      
                        }
                      else
                        {
                          if (QccENTArithmeticDecode(buffer,model,&symbol, 1))
                            return_value = 2;      
                          sign_array[current_frame][current_row][current_col] = (char)symbol;
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
                          QccErrorAddMessage("(QccWAVtce3DIntIPBand): QccWAVtceIPBand()");
                          goto Error;
                        }
                    } 

                  if (col == 0)
                    p1[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE + filter_coef * v;
                  else
                    p1[row][col] = alpha * p1[row][col-1] + filter_coef * v;
                  p2[col] = p1[row][col] + alpha * p2[col];
 
                }//a
              else
                { 
                  if (col == 0)
                    p1[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE  + filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else
                    p1[row][col] = alpha* p1[row][col-1] + filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  p2[col] = p1[row][col] + alpha * p2[col];
                }
              p_estimation[subband_origin_frame+frame][subband_origin_row + row][subband_origin_col + col] =
                p_forward;


            }//aa
     
           
          for (col = subband_num_cols - 1; col >= 0; col--)
            {  
              if (col ==( subband_num_cols - 1)) 
                p2[col] = p2[col] + alpha * QCCTCE3D_BOUNDARY_VALUE;
              else
                p2[col] = p2[col] + alpha * p4[row][col+1];
              p3[row][col]=p2[col]+alpha*p3[row][col];
              p_char = &(significance_map[current_frame][current_row][subband_origin_col + col]);
              if (*p_char == QCCTCE3D_S)
                {
                  if (col == (subband_num_cols - 1))
                    p4[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE + filter_coef * QCCWAVTCE3D_REFINE_HOLDER; 
                  else 
                    p4[row][col] = alpha * p4[row][col+1] + filter_coef * QCCWAVTCE3D_REFINE_HOLDER;;
                } 
              else
                {
                  v = (*p_char == QCCTCE3D_S_NEW);
                  if (col == subband_num_cols - 1)
                    p4[row][col]  = alpha * QCCTCE3D_BOUNDARY_VALUE + filter_coef * v;
                  else
                    p4[row][col] = alpha * p4[row][col+1] + filter_coef * v;
                }
            }
        }//aaa
	 
      for (col = 0; col < subband_num_cols; col++)
        p5[col]=0;
      for (row = subband_num_rows - 1; row >= 0; row--)
        {
          for (col = 0; col < subband_num_cols; col++)
            {
              p3[row][col]=p3[row][col]+alpha*p5[col];
              if (col ==( subband_num_cols - 1))
                p5[col]=p1[row][col]+alpha*p5[col]+alpha*QCCTCE3D_BOUNDARY_VALUE ;
              else 
                p5[col]=p1[row][col]+alpha*p5[col]+alpha*p4[row][col+1];

            }

        }
    }//aaaa
     
  for (frame = subband_num_frames-1;frame >= 0; frame--)
    {//bbb
      current_frame=subband_origin_frame+frame;
     
      for (row = subband_num_rows - 1; row >= 0; row--)
        {//bb
          current_row = subband_origin_row + row;
          for (col = subband_num_cols-1; col >=0; col--)
            {   
              current_col = subband_origin_col + col; 
              p_char = &(significance_map[current_frame][current_row][current_col]);
              if (col ==( subband_num_cols-1) )
                p_estimation[current_frame][current_row][current_col]+=alpha * (QCCTCE3D_BOUNDARY_VALUE + p2[col] + p3[row][col]);
              else
                p_estimation[current_frame][current_row][current_col] +=alpha *(p1[row][col+1]+p2[col]+p3[row][col]);
                 
              if (*p_char == QCCTCE3D_S)
                {
                  p_estimation[current_frame][current_row][current_col]=
                    QCCWAVTCE3D_REFINE_HOLDER * weight[0] +
                    p_estimation[current_frame][current_row][current_col] * weight[1];
                  if (col == (subband_num_cols-1) )
                    p1[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE + filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else
  
                    p1[row][col] = alpha * p1[row][col+1] + filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCTCE3D_S_NEW);
                  p_estimation[current_frame][current_row][current_col] =
                    v * weight[0] +p_estimation[current_frame][current_row][current_col] * weight[1];
                  if (col == (subband_num_cols-1) )
                    p1[row][col] = alpha *  QCCTCE3D_BOUNDARY_VALUE  + filter_coef * v;
                  else

                    p1[row][col] = alpha * p1[row][col+1] + filter_coef * v;
                }    
              p2[col] = p1[row][col] + alpha * p2[col];   
            }
          
          for (col = 0; col < subband_num_cols; col++)
            {
	      if(col==0)
                p2[col] = p2[col] + alpha * QCCTCE3D_BOUNDARY_VALUE;
	      else
                p2[col] = p2[col] + alpha * p4[row][col-1];
              p3[row][col]=p2[col]+alpha*p3[row][col];
              current_col = subband_origin_col + col; 
              p_char = &(significance_map[current_frame][current_row][current_col]);
              if (*p_char == QCCTCE3D_S)
                {
	          if(col==0)
                    p4[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE + filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                  else 
                    p4[row][col] = alpha * p4[row][col-1] + filter_coef * QCCWAVTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCTCE3D_S_NEW);
                  if(col==0)
                    p4[row][col] = alpha * QCCTCE3D_BOUNDARY_VALUE + filter_coef * v;
                  else 
                    p4[row][col] = alpha * p4[row][col-1] + filter_coef *v;
                 
                }        
            }
        }//bb
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          {
            p3[row][col]=p3[row][col]+alpha*p5[col];
            if(col==0)
              p5[col]=p1[row][col]+alpha*p5[col]+alpha* QCCTCE3D_BOUNDARY_VALUE; 
            else
              p5[col]=p1[row][col]+alpha*p5[col]+alpha*p4[row][col-1];

          }
    }//bbb
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return: 
  QccVectorFree(p2); 
  QccVectorFree(p5);
  QccMatrixFree(p1,subband_num_rows); 
  QccMatrixFree(p3,subband_num_rows); 
  QccMatrixFree(p4,subband_num_rows); 
  return(return_value); 
}


static int QccWAVtce3DIntIPPass(QccWAVSubbandPyramid3DInt *coefficients,    
                                char ***significance_map,
                                char ***sign_array,
                                int threshold,
                                double ***p_estimation,
                                double *subband_significance,
                                QccENTArithmeticModel *model,
                                QccBitBuffer *buffer,
                                int *max_coefficient_bits,
                                double alpha)
{
  int subband,num_subbands; 
  int return_value;

  num_subbands =
    QccWAVSubbandPyramid3DIntNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                          coefficients->spatial_num_levels);
  
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)max_coefficient_bits[subband]))
        {
          return_value =
            QccWAVtce3DIntRevEst(coefficients,
                                 significance_map,
                                 subband,
                                 subband_significance,
                                 p_estimation,
                                 alpha);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVtce3DIntIPPass): Error calling QccWAVtceRevEst()");
              return(1);
            }
          else
            {
              if (return_value == 2)
                return(2);
            }
          
          return_value = QccWAVtce3DIntIPBand(coefficients,
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
              QccErrorAddMessage("(QccWAVtce3DIntIPPass): Error calling QccWAVtceIPBand()");
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


int static QccWAVtce3DIntSPass(QccWAVSubbandPyramid3DInt *coefficients,    
                               char ***significance_map,
                               int threshold,
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
  int current_frame,current_row, current_col;
  char *p_char;
  int symbol;
  double p = 0.4;

  num_subbands =
    QccWAVSubbandPyramid3DIntNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                          coefficients->spatial_num_levels);
 
  for (subband = 0; subband < num_subbands; subband++)
    {
      if ((threshold - 0.000001) <
          pow((double)2, (double)(max_coefficient_bits[subband])))
        { 
          if (QccWAVSubbandPyramid3DIntSubbandSize(coefficients,
                                                   subband,
                                                   &subband_num_frames,
                                                   &subband_num_rows,
                                                   &subband_num_cols))
            {
              QccErrorAddMessage("(QccWAVtce3DIntSPass): Error calling QccWAVSubbandPyramid3DIntSubbandSize()");
              return(1);
            }
          if (QccWAVSubbandPyramid3DIntSubbandOffsets(coefficients,
                                                      subband,
                                                      &subband_origin_frame,
                                                      &subband_origin_row,
                                                      &subband_origin_col))
            {
              QccErrorAddMessage("(QccWAVtce3DIntSPass): Error calling QccWAVSubbandPyramid3DIntSubbandOffsets()");
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
                      p_char = &(significance_map[current_frame][current_row][current_col]);

                      if (*p_char == QCCTCE3D_S)
                        { 
                      
                          if (QccWAVTce3DUpdateModel(model, p))
                            {
                              QccErrorAddMessage("(QccWAVtce3DIntSPass): Error calling QccWAVtceUpdateModel()");
                              return(1);
                            }
                      
                          if (buffer->type == QCCBITBUFFER_OUTPUT)
                            {
                              if (coefficients->volume[current_frame][current_row][current_col] >=
                                  threshold)
                                {
                                  symbol = 1;
                                  coefficients->volume[current_frame][current_row][current_col] -=
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
                              coefficients->volume[current_frame][current_row][current_col] -=
                                threshold;
                              coefficients->volume[current_frame][current_row][current_col] +=
                                (symbol == 1) ? threshold + (threshold >> 1) : (threshold >> 1) ;
                            }
                          p =
                            p * QCCWAVTCE3D_ALPHA_1D + symbol * QCCWAVTCE3D_ALPHA_1D_O;
                        }
                      else
                        {
                          if (*p_char == QCCTCE3D_S_NEW)
                            *p_char = QCCTCE3D_S;
                          if (*p_char == QCCTCE3D_NZN_NEW)
                            *p_char = QCCTCE3D_NZN;     
                        }
                    }
                }
            }
        }
    } 

  return(0);
}


int QccWAVtce3DLosslessEncode(const QccIMGImageCube *image,
                              QccBitBuffer *buffer,
                              int transform_type,
                              int temporal_num_levels,
                              int spatial_num_levels,
                              double alpha,
                              const QccWAVWavelet *wavelet)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid3DInt image_subband_pyramid;
  char ***sign_array = NULL;
  char ***significance_map = NULL;
  double ***p_estimation = NULL;
  double *subband_significance = NULL;
  int image_mean;
  int *max_coefficient_bits=NULL;
  int threshold;
  int num_symbols[1] = { 2 };
  int frame, row, col;
  int bitplane = 0;
  int num_subbands;
  int max_max_coefficient_bits = 0;
  int subband;

  if (image == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramid3DIntInitialize(&image_subband_pyramid);
  
  image_subband_pyramid.spatial_num_levels = 0;
  image_subband_pyramid.temporal_num_levels = 0;
  image_subband_pyramid.num_frames = image->num_frames;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramid3DIntAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccWAVSubbandPyramid3DIntAlloc()");
      goto Error;
    }
  
  if ((sign_array =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory1");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((sign_array[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory2");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((sign_array[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory3");
            goto Error;
          }
    }
  
  if ((significance_map =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory4");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((significance_map[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory5");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((significance_map[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory6");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        significance_map[frame][row][col] = QCCTCE3D_Z;

 
  
  if ((p_estimation =
       (double ***)malloc(sizeof(double **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory7");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((p_estimation[frame] =
           (double **)malloc(sizeof(double *) *
                             image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory8");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((p_estimation[frame][row] =
             (double *)malloc(sizeof(double) *
                              image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory9");
            goto Error;
          }
    }
  

  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        p_estimation[frame][row][col] = 0.0000001;
  
  num_subbands =
    QccWAVSubbandPyramid3DIntNumLevelsToNumSubbandsPacket(temporal_num_levels,
                                                          spatial_num_levels);

  
  if ((max_coefficient_bits =
       (int *)malloc(sizeof(int) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory10");
      goto Error;
    }
  if ((subband_significance =
       (double*)malloc(num_subbands* sizeof(double))) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error allocating memory11");
      goto Error;
    } 
   
   
  if (QccWAVtce3DLosslessEncodeDWT(&image_subband_pyramid,
                                   sign_array,
                                   image,
                                   transform_type,
                                   temporal_num_levels,
                                   spatial_num_levels,
                                   &image_mean,
                                   max_coefficient_bits,
                          
                                   wavelet))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccWAVtce3DLosslessEncodeDWT()");
      goto Error;
    }
    
  if (QccWAVtce3DLosslessEncodeHeader(buffer,
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
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccWAVtce3DLosslessEncodeHeader()");
      goto Error;
    }
   
  if (QccWAVtce3DEncodeBitPlaneInfo(buffer,
                                    num_subbands,
                                    max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccWAVtceEncodeBitPlaneInfo()");
      goto Error;
    }
   
 
  if ((model = QccENTArithmeticEncodeStart(num_symbols,
					   1,
					   NULL,
					   QCCENT_ANYNUMBITS)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);
  
  for (subband = 0; subband < num_subbands; subband++)
    if (max_max_coefficient_bits < max_coefficient_bits[subband])
      max_max_coefficient_bits = max_coefficient_bits[subband];
  
  threshold = pow((double)2, (double)max_max_coefficient_bits);
 
  bitplane = max_max_coefficient_bits + 1; 
  while (bitplane > 0)  
    {
      return_value = QccWAVtce3DIntNZNPass(&image_subband_pyramid,
                                           significance_map,
                                           sign_array,
                                           threshold,
                                           subband_significance,
                                           model,
                                           buffer,
                                           max_coefficient_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccWAVtce3DIntNZNPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
 
      return_value = QccWAVtce3DIntIPPass(&image_subband_pyramid,
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
          QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccWAVtce3DIntIPPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
         
      return_value = QccWAVtce3DIntSPass(&image_subband_pyramid, 
                                         significance_map,
                                         threshold,
                                         model,
                                         buffer,
                                         max_coefficient_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccWAVtce3DIntSPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }  
        
      threshold = floor(threshold/2.0);

      bitplane--;
    }
  
  if (QccENTArithmeticEncodeEnd(model,
                                0,
                                buffer))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessEncode): Error calling QccENTArithmeticEncodeEnd()");
      goto Error;
    }  

 Finished:
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DIntFree(&image_subband_pyramid);
 
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
  if (max_coefficient_bits != NULL)
    QccFree(max_coefficient_bits);
  QccENTArithmeticFreeModel(model);
  
  return(return_value);
}


static int QccWAVtce3DLosslessDecodeInverseDWT(QccWAVSubbandPyramid3DInt
                                               *image_subband_pyramid,
                                               char ***sign_array,
                                               QccIMGImageCube *image,
                                               int image_mean,
                                               const QccWAVWavelet *wavelet)
{
  int frame, row, col;

  for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (sign_array[frame][row][col])
          image_subband_pyramid->volume[frame][row][col] *= -1;
 
  if (QccWAVSubbandPyramid3DIntInverseDWT(image_subband_pyramid,
                                          wavelet))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DIntInverseDWT()");
      return(1);
    }
  
  if (QccWAVSubbandPyramid3DIntAddMean(image_subband_pyramid,
                                       image_mean))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DIntAddMean()");
      return(1);
    }
  
  for (frame = 0; frame <image->num_frames; frame++)
    for (row = 0; row <image->num_rows; row++)
      for (col = 0; col <image->num_cols; col++)
	image->volume[frame][row][col] =
          (double)image_subband_pyramid->volume[frame][row][col];

  if (QccIMGImageCubeSetMaxMin(image))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeInverseDWT): QccError calling QccIMGImageCubeSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccWAVtce3DLosslessDecodeHeader(QccBitBuffer *buffer, 
                                    int *transform_type,
                                    int *temporal_num_levels, 
                                    int *spatial_num_levels, 
                                    int *num_frames,
                                    int *num_rows,
                                    int *num_cols,
                                    int *image_mean,
                                    int *max_coefficient_bits,
                                    double *alpha)
{
  int return_value;
  unsigned char ch;
  
  if (QccBitBufferGetBit(buffer, transform_type))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetBit()");
      goto Error;
    }

  if (*transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      if (QccBitBufferGetChar(buffer, &ch))
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetChar()");
          goto Error;
        }
      *temporal_num_levels = (int)ch;
    }
  
  if (QccBitBufferGetChar(buffer, &ch))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  *spatial_num_levels = (int)ch;

  if (*transform_type == QCCWAVSUBBANDPYRAMID3D_DYADIC)
    *temporal_num_levels = *spatial_num_levels;

  if (QccBitBufferGetInt(buffer, num_frames))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }

  if (QccBitBufferGetInt(buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
   
  if (QccBitBufferGetDouble(buffer, alpha))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVtce3DLosslessDecode(QccBitBuffer *buffer,
                              QccIMGImageCube *image,
                              int transform_type,
                              int temporal_num_levels,
                              int spatial_num_levels,
                              double alpha,
                              const QccWAVWavelet *wavelet,
                              int image_mean,
                              int max_coefficient_bits,
                              int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid3DInt image_subband_pyramid;
  char ***sign_array = NULL;
  char ***significance_map = NULL;
  double ***p_estimation = NULL;
  double *subband_significance = NULL;
  int *max_bits = NULL;
  int threshold;
  int frame, row, col;
  int num_symbols[1] = { 2 };
  QccWAVWavelet lazy_wavelet_transform;
  int num_subbands;
  int max_max_coefficient_bits = 0;
  int subband;
  int bitplane;

  QccWAVSubbandPyramid3DIntInitialize(&image_subband_pyramid);
 
  QccWAVWaveletInitialize(&lazy_wavelet_transform);

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

  if (QccWAVSubbandPyramid3DIntAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    }
  
  if ((sign_array =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((sign_array[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((sign_array[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
            goto Error;
          }
    }
  
  if ((significance_map =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((significance_map[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((significance_map[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        significance_map[frame][row][col] = QCCTCE3D_Z;

  num_subbands =
    QccWAVSubbandPyramid3DIntNumLevelsToNumSubbandsPacket(image_subband_pyramid.temporal_num_levels,
                                                          image_subband_pyramid.spatial_num_levels);

  if ((max_bits = (int *)malloc(sizeof(int) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
      goto Error;
    }
  
  if (QccWAVtce3DDecodeBitPlaneInfo(buffer,
                                    num_subbands,
                                    max_bits,
                                    max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error calling QccWAVtceEncodeBitPlaneInfo()");
      goto Error;
    }
     
  if ((p_estimation =
       (double ***)malloc(sizeof(double **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((p_estimation[frame] =
           (double **)malloc(sizeof(double *) *
                             image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((p_estimation[frame][row] =
             (double *)malloc(sizeof(double) *
                              image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
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
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error allocating memory");
      goto Error;
    }  
 
  if ((model =
       QccENTArithmeticDecodeStart(buffer,
				   num_symbols,
				   1,
				   NULL,
                                   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);

  for (subband = 0; subband < num_subbands; subband++)
    if (max_max_coefficient_bits < max_bits[subband])
      max_max_coefficient_bits = max_bits[subband];
  
  threshold = pow((double)2, (double)max_max_coefficient_bits);
  
  bitplane = max_max_coefficient_bits + 1; 
  while (bitplane > 0)  
    {
      return_value = QccWAVtce3DIntNZNPass(&image_subband_pyramid,
				           significance_map,
                                           sign_array,
                                           threshold,
                                           subband_significance,
                                           model,
                                           buffer,
                                           max_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error calling QccWAVtce3DIntNZNPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
        
      return_value = QccWAVtce3DIntIPPass(&image_subband_pyramid,
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
          QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error calling QccWAVtce3DIntIPPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
        
      return_value = QccWAVtce3DIntSPass(&image_subband_pyramid, 
                                         significance_map,
                                         threshold,
                                         model,
                                         buffer,
                                         max_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error calling QccWAVtce3DIntSPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }  
        
      threshold = floor(threshold/ 2.0);

      bitplane--;
    }
  

 Finished:
  if (QccWAVtce3DLosslessDecodeInverseDWT(&image_subband_pyramid,
				          sign_array,
                                          image,
				          image_mean,
				          wavelet))
    {
      QccErrorAddMessage("(QccWAVtce3DLosslessDecode): Error calling QccWAVtce3DLosslessDecodeInverseDWT()");
      goto Error;
    }
   
  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DIntFree(&image_subband_pyramid);
  
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
  QccENTArithmeticFreeModel(model);
  return(return_value);
}
