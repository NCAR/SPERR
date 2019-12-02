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


#define QCCWAVKLTTCE3D_BOUNDARY_VALUE (1e-6)

//definition used in context information
#define QCCWAVKLTTCE3D_Z 0 //zero, AKA, insignificant
#define QCCWAVKLTTCE3D_NZN_NEW 2 //non-zero-neighbor
#define QCCWAVKLTTCE3D_S 3 //significant
#define QCCWAVKLTTCE3D_S_NEW 4
#define QCCWAVKLTTCE3D_NZN 5

// more refinement can be made if direction of Tarp filter is considered
// but the improvement is minor

//ugly fix, use 1-D IIR filter to provide PMF estimate
#define QCCWAVKLTTCE3D_ALPHA_1D 0.995 
#define QCCWAVKLTTCE3D_ALPHA_1D_O 0.005

// this should be 0.5, however, 0.3 seems to work a little better (minor)
// reason unknown
#define QCCWAVKLTTCE3D_REFINE_HOLDER 0.3 

// threshold for cross-scale prediction...
#define QCCWAVKLTTCE3D_PREDICT_THRESHOLD 0.05  
//weight factor for cross-scale and current scale
#define QCCWAVKLTTCE3D_CURRENT_SCALE 0.6 
#define QCCWAVKLTTCE3D_PARENT_SCALE 0.4

#define QCCWAVKLTTCE3D_MAXBITPLANES 128

                                      
static int QccWAVklttce3DEncodeBitPlaneInfo(QccBitBuffer *output_buffer,
                                            int num_subbands,
                                            int *max_coefficient_bits)
{

  int subband;
  int num_zeros = 0;

  for (subband = 1; subband < num_subbands; subband++)
    {
      num_zeros = max_coefficient_bits[subband-1] - max_coefficient_bits[subband];
      if (num_zeros>0)
           
        QccBitBufferPutBit(output_buffer, 0);
      else

        QccBitBufferPutBit(output_buffer, 1);
    }
  
  for (subband = 1; subband < num_subbands; subband++)
    {
      for (num_zeros = abs(max_coefficient_bits[subband-1] - max_coefficient_bits[subband]);
           num_zeros > 0;
           num_zeros--)
        if (QccBitBufferPutBit(output_buffer, 0))
          return(1);

      if (QccBitBufferPutBit(output_buffer, 1))
        return(1);
    }

  return(0);
}


static int QccWAVklttce3DDecodeBitPlaneInfo(QccBitBuffer *input_buffer,
                                            int num_subbands,
                                            int *max_bits,
                                            int max_coefficient_bits)
{
  int subband;
  int bit_value;
  int *max_bits_sign;
  if ((max_bits_sign =
       (int *)malloc(sizeof(int) * num_subbands )) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DDecodeBitPlaneInfo): Error allocating memory");
      
    }
  
  max_bits[0] = max_coefficient_bits;

  for (subband = 1; subband < num_subbands; subband++)
    {
      
      QccBitBufferGetBit(input_buffer,&bit_value);
            
      if (bit_value == 0)
        max_bits_sign[subband] = 1;  
      else
        max_bits_sign[subband] = -1;
    }
  
  for (subband = 1; subband < num_subbands; subband++)
    {
      max_bits[subband] = 0;
      do
        {
          if (QccBitBufferGetBit(input_buffer,&bit_value))
            return(1);
          max_bits[subband]++;
        }
      while (bit_value == 0);
      max_bits[subband]--;  
      max_bits[subband] = max_bits[subband - 1] - 
        max_bits[subband] * max_bits_sign[subband];
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


static int QccWAVklttceForwardKLTDWT(QccWAVSubbandPyramid3D
                                     *image_subband_pyramid,
                                     int num_levels,
                                     const QccWAVWavelet *wavelet,
                                     QccHYPklt *klt)
{
  int return_value;
  QccIMGImageCube image_cube;

  image_subband_pyramid->temporal_num_levels = 0;
  image_subband_pyramid->spatial_num_levels = 0;
 
  image_cube.num_frames = image_subband_pyramid->num_frames;
  image_cube.num_rows = image_subband_pyramid->num_rows;
  image_cube.num_cols = image_subband_pyramid->num_cols;
  image_cube.volume = image_subband_pyramid->volume;

  if (QccHYPkltTrain(&image_cube,
                     klt))
    {
      QccErrorAddMessage("(QccWAVklttceForwardKLTDWT): Error calling QccHYPkltTrain()");
      goto Error;
    }

  if (QccHYPkltTransform(&image_cube,
                         klt,
                         klt->num_bands))
    {
      QccErrorAddMessage("(QccWAVklttceForwardKLTDWT): Error calling QccHYPkltTransform()");
      goto Error;
    }
  
  if (QccWAVSubbandPyramid3DDWT(image_subband_pyramid,
                                QCCWAVSUBBANDPYRAMID3D_PACKET,
                                0,
                                num_levels,
                                wavelet))
    {
      QccErrorAddMessage("(QccWAVklttceForwardKLTDWT): Error calling QccWAVSubbandPyramid3DDWT()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVklttceInverseKLTDWT(QccWAVSubbandPyramid3D
                                     *image_subband_pyramid,
                                     const QccWAVWavelet *wavelet,
                                     int num_levels,
                                     const QccHYPklt *klt)
{
  int return_value;
  QccIMGImageCube image_cube;

  if (QccWAVSubbandPyramid3DInverseDWT(image_subband_pyramid,
                                       wavelet))
    {
      QccErrorAddMessage("(QccWAVklttceInverseKLTDWT): Error calling QccWAVSubbandPyramid3DInverseDWT()");
      goto Error;
    }

  image_cube.num_frames = image_subband_pyramid->num_frames;
  image_cube.num_rows = image_subband_pyramid->num_rows;
  image_cube.num_cols = image_subband_pyramid->num_cols;
  image_cube.volume = image_subband_pyramid->volume;

  if (QccHYPkltInverseTransform(&image_cube,
                                klt))
    {
      QccErrorAddMessage("(QccWAVklttceInverseKLTDWT): Error calling QccHYPkltInverseTransform(()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVklttce3DForwardTransform(QccWAVSubbandPyramid3D
                                          *image_subband_pyramid,
                                          char ***sign_array,
                                          const QccIMGImageCube *image,
                                          int num_levels,
                                          int **max_coefficient_bits,
                                          const QccWAVWavelet *wavelet,
                                          QccHYPklt *klt)
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

  num_subbands =
    QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(0, num_levels);
  
  if (QccVolumeCopy(image_subband_pyramid->volume,
                    image->volume,
                    image->num_frames,
                    image->num_rows,
                    image->num_cols))
    {
      QccErrorAddMessage("(QccWAVklttce3DForwardTransform): Error calling QccVolumeCopy()");
      return(1);
    }

  if (QccWAVklttceForwardKLTDWT(image_subband_pyramid,
                                num_levels,
                                wavelet,
                                klt))
    {
      QccErrorAddMessage("(QccWAVklttce3DForwardTransform): Error calling QccWAVklttceForwardKLTDWT()");
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
          QccErrorAddMessage("(QccWAVklttce3DForwardTransform): Error calling QccWAVSubbandPyramid3DSubbandSize()");
          return(1);
        }
      if (QccWAVSubbandPyramid3DSubbandOffsets(image_subband_pyramid,
                                               subband,
                                               &subband_origin_frame,
                                               &subband_origin_row,
                                               &subband_origin_col))
        {
          QccErrorAddMessage("(QccWAVklttce3DForwardTransform): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
          return(1);
        }
         
      for (frame = 0; frame < subband_num_frames; frame++)
        {
          max_coefficient = -MAXFLOAT;

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
 
          max_coefficient_bits[subband][frame] = 
            (int)floor(QccMathMax(0, QccMathLog2(max_coefficient + 0.0001)));
        }
    }
       
  return(0);
}


static int QccWAVklttce3DInverseTransform(QccWAVSubbandPyramid3D
                                          *image_subband_pyramid,
                                          char ***sign_array,
                                          QccIMGImageCube *image,
                                          const QccWAVWavelet *wavelet,
                                          const QccHYPklt *klt)
{
  int frame, row, col;

  for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (sign_array[frame][row][col])
          image_subband_pyramid->volume[frame][row][col] *= -1;
  
  if (QccWAVklttceInverseKLTDWT(image_subband_pyramid,
                                wavelet, 
                                image_subband_pyramid->spatial_num_levels,
                                klt))
    {
      QccErrorAddMessage("(QccWAVklttce3DInverseTransform): Error calling QccWAVklttceInverseKLTDWT()");
      return(1);
    }
  
  if (QccVolumeCopy(image->volume,
                    image_subband_pyramid->volume,
                    image->num_frames,
                    image->num_rows,
                    image->num_cols))
    {
      QccErrorAddMessage("(QccWAVklttce3DInverseTransform): Error calling QccVolumeCopy()");
      return(1);
    }
  
  if (QccIMGImageCubeSetMaxMin(image))
    {
      QccErrorAddMessage("(QccWAVklttce3DInverseTransform): QccError calling QccIMGImageCubeSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccWAVklttce3DEncodeHeader(QccBitBuffer *buffer, 
                               int num_levels,
                               int num_frames,
                               int num_rows,
                               int num_cols,
                               int *max_coefficient_bits,
                               double alpha)
{
  int return_value;
  
  if (QccBitBufferPutChar(buffer, (unsigned char)num_levels))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_frames))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncodeHeader): Error calling QccBitBufferPut()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncodeHeader): Error calling QccBitBufferPut()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }

  if (QccBitBufferPutInt(buffer, max_coefficient_bits[0]))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }

  if (QccBitBufferPutDouble(buffer, alpha))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVklttce3DEncodeKLT(QccBitBuffer *buffer,
                                   const QccHYPklt *klt)
{
  int band1, band2;

  for (band1 = 0; band1 < klt->num_bands; band1++)
    if (QccBitBufferPutDouble(buffer, klt->mean[band1]))
      {
        QccErrorAddMessage("(QccWAVklttce3DEncodeKLT): Error calling QccBitBufferPutInt()");
        return(1);
      }

  for (band1 = 0; band1 < klt->num_bands; band1++)
    for (band2 = 0; band2 < klt->num_bands; band2++)
      if (QccBitBufferPutDouble(buffer, klt->matrix[band1][band2]))
        {
          QccErrorAddMessage("(QccWAVklttce3DEncodeKLT): Error calling QccBitBufferPutDouble()");
          return(1);
        }

  return(0);
}


static int QccWAVklttce3DDecodeKLT(QccBitBuffer *buffer,
                                   QccHYPklt *klt)
{
  int band1, band2;

  for (band1 = 0; band1 < klt->num_bands; band1++)
    if (QccBitBufferGetDouble(buffer, &klt->mean[band1]))
      {
        QccErrorAddMessage("(QccWAVklttce3DEncodeKLT): Error calling QccBitBufferGetInt()");
        return(1);
      }

  for (band1 = 0; band1 < klt->num_bands; band1++)
    for (band2 = 0; band2 < klt->num_bands; band2++)
      if (QccBitBufferGetDouble(buffer, &klt->matrix[band1][band2]))
        {
          QccErrorAddMessage("(QccWAVklttce3DEncodeKLT): Error calling QccBitBufferGetDouble()");
          return(1);
        }

  return(0);
}


static int QccWAVklttce3DRevEst(QccWAVSubbandPyramid3D *coefficients,
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
  
  filter_coef = (1 - alpha) * (1 - alpha)*(1 - alpha) / (3*alpha + alpha*alpha*alpha)/3;
  
  subband_significance[subband] = 0.0;
  if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                        subband,
                                        &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVklttce3DRevEst): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      goto Error;
    }
  if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                           subband,
                                           &subband_origin_frame,
                                           &subband_origin_row,
                                           &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVklttce3DRevEst): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      goto Error;
    }
  
  if ((p1 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DRevEst): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((p3 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p4 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DRevEst): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p5 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DRevEst): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < subband_num_rows; row++)
    for (col = 0; col < subband_num_cols; col++)
      p3[row][col]=QCCWAVKLTTCE3D_BOUNDARY_VALUE;  

  for (frame = subband_num_frames-1;frame >= 0; frame--)
    {//bbb
      current_frame=subband_origin_frame+frame;

      for (col = 0; col < subband_num_cols; col++)
        p2[col] = QCCWAVKLTTCE3D_BOUNDARY_VALUE; 
       
      for (row = subband_num_rows - 1; row >= 0; row--)
        {//bb
           
          current_row = subband_origin_row + row;
          for (col = subband_num_cols - 1; col >= 0; col--)
            {       
              current_col = subband_origin_col + col;
              p_char = &(significance_map[current_frame][current_row][current_col]);
              if (col == (subband_num_cols-1) )
                p[current_frame][current_row][current_col] =alpha * (QCCWAVKLTTCE3D_BOUNDARY_VALUE + p2[col] + p3[row][col]);
              else
                p[current_frame][current_row][current_col] =alpha *(p1[row][col+1]+p2[col]+p3[row][col]);
                 
              
              if (*p_char == QCCWAVKLTTCE3D_S)
                {
                  if (col == subband_num_cols-1 )
                    p1[row][col] = alpha *  p[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                  else

                    p1[row][col] = alpha * p1[row][col+1] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                   
                  subband_significance[subband] += QCCWAVKLTTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCWAVKLTTCE3D_S_NEW);
                  if (col == (subband_num_cols-1) )
                    p1[row][col] = alpha *   p[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col]   + filter_coef * v;
                  else

                    p1[row][col] = alpha * p1[row][col+1] + filter_coef * v;
                   
                  subband_significance[subband] += v;
                }   
              p2[col] = p1[row][col] + alpha * p2[col];   
            }
      
          for (col = 0; col < subband_num_cols; col++)
            {
              p_char = &(significance_map[current_frame][current_row][subband_origin_col + col]);
              if (col==0)
                p2[col] = p2[col] + alpha *  p[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] ;
              else
                p2[col] = p2[col] + alpha * p4[row][col-1];
               
              if (*p_char == QCCWAVKLTTCE3D_S)
                {
                  if (col==0)
                    p4[row][col] = alpha * p[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col]  + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                  else 
                    p4[row][col] = alpha * p4[row][col-1] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                }
              else
                {
                  v = (*p_char == QCCWAVKLTTCE3D_S_NEW);
                  if (col==0)
                    p4[row][col] = alpha *  p[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col]  + filter_coef * v;
                  else 
                    p4[row][col] = alpha* p4[row][col-1] + filter_coef *v;
                }        
            }
        }//bb
      for (col = 0; col < subband_num_cols; col++)
        p5[col] = p[subband_origin_frame+frame][subband_origin_row][subband_origin_col + col];
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          {
            p3[row][col]=p3[row][col]+alpha*p5[col];
            if (col==0)
              p5[col]=p1[row][col]+alpha*p5[col]+alpha* p[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] ;
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
  if (frame>0)
    {
      if (row > 0)
        { 
          shifted_frame = current_frame - 1; 
          shifted_row = current_row - 1;
          shifted_col = current_col;
          if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
            significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
          if (col > 0)
            {
              shifted_frame = current_frame - 1;
              shifted_row = current_row - 1;
              shifted_col = current_col - 1;
              if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
            }
          if (col < subband_num_col - 1)
            {
              shifted_frame = current_frame - 1;
              shifted_row = current_row - 1;
              shifted_col = current_col + 1;
              if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
            }
        }
      if (row < subband_num_row - 1)
        {
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row + 1;
	  shifted_col = current_col;
	  if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
	  if (col > 0)
	    {
	      shifted_frame = current_frame - 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col - 1;
	      if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;  
	    }
	  if (col < subband_num_col - 1)
	    {
	      shifted_frame = current_frame - 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col + 1;
	      if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
	    }
	}
      if (col > 0)
	{
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row;
	  shifted_col = current_col - 1;  
	  if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
	}
      if (col < subband_num_col - 1)
	{
	  shifted_frame = current_frame - 1;
	  shifted_row = current_row;
	  shifted_col = current_col + 1;
	  if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
	}
      {
	shifted_frame = current_frame - 1;
	shifted_row = current_row;
	shifted_col = current_col;
	if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
	  significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
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
          if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
            significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
          if (col > 0)
            {
              shifted_frame = current_frame + 1;
              shifted_row = current_row - 1;
              shifted_col = current_col - 1;
              if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
            }
          if (col < subband_num_col - 1)
            {
              shifted_frame = current_frame + 1;
              shifted_row = current_row - 1;
              shifted_col = current_col + 1;
              if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
                significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
            }
        }
      if (row < subband_num_row - 1)
	{
	  shifted_frame = current_frame+ 1;
	  shifted_row = current_row + 1;
	  shifted_col = current_col;
	  if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
	  if (col > 0)
	    {
	      shifted_frame = current_frame + 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col - 1;
	      if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;  
	    }
	  if (col < subband_num_col - 1)
	    {
	      shifted_frame = current_frame + 1;
	      shifted_row = current_row + 1;
	      shifted_col = current_col + 1;
	      if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
		significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
	    }
	}
      if (col > 0)
	{
	  shifted_frame = current_frame + 1;
	  shifted_row = current_row;
	  shifted_col = current_col - 1;  
	  if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
	}
      if (col < subband_num_col - 1)
	{
	  shifted_frame = current_frame + 1;
	  shifted_row = current_row;
	  shifted_col = current_col + 1;
	  if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
	    significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
	}
      {
	shifted_frame = current_frame + 1;
	shifted_row = current_row;
	shifted_col = current_col;
	if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
	  significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      }

    }

  //current frame
  {
    if (row > 0)
      { 
        shifted_frame = current_frame ; 
        shifted_row = current_row - 1;
        shifted_col = current_col;
        if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
        if (col > 0)
          {
            shifted_frame = current_frame;
            shifted_row = current_row - 1;
            shifted_col = current_col - 1;
            if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
          }
        if (col < subband_num_col - 1)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row - 1;
            shifted_col = current_col + 1;
            if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
          }
      }
    if (row < subband_num_row - 1)
      {
        shifted_frame = current_frame;
        shifted_row = current_row + 1;
        shifted_col = current_col;
        if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
        if (col > 0)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row + 1;
            shifted_col = current_col - 1;
            if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;  
          }
        if (col < subband_num_col - 1)
          {
            shifted_frame = current_frame ;
            shifted_row = current_row + 1;
            shifted_col = current_col + 1;
            if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
              significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;   
          }
      }
    if (col > 0)
      {
        shifted_frame = current_frame ;
        shifted_row = current_row;
        shifted_col = current_col - 1;  
        if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      
      }
    if (col < subband_num_col - 1)
      {
        shifted_frame = current_frame ;
        shifted_row = current_row;
        shifted_col = current_col + 1;
        if (significance_map[shifted_frame][shifted_row][shifted_col] < QCCWAVKLTTCE3D_S)
          significance_map[shifted_frame][shifted_row][shifted_col] = QCCWAVKLTTCE3D_NZN_NEW;
      }
  }
  
  return(0);
}



static int QccWAVklttce3DNZNPass(QccWAVSubbandPyramid3D *coefficients,    
                                 char ***significance_map,
                                 char ***sign_array,
                                 double threshold,
                                 double *subband_significance,
                                 QccENTArithmeticModel *model,
                                 QccBitBuffer *buffer,
                                 int **max_coefficient_bits)
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
  int symbol;
  double p = 0.5;
  num_subbands =
    QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                       coefficients->spatial_num_levels);
 
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      { 
        if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                              subband,
                                              &subband_num_frames,
                                              &subband_num_rows,
                                              &subband_num_cols))
          {
            QccErrorAddMessage("(QccWAVklttce3DNZNPass): Error calling QccWAVSubbandPyramid3DSubbandSize()");
            return(1);
          }
        if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                                 subband,
                                                 &subband_origin_frame,
                                                 &subband_origin_row,
                                                 &subband_origin_col))
          {
            QccErrorAddMessage("(QccWAVklttce3DNZNPass): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
            return(1);
          }  
          
        for (frame = 0; frame < subband_num_frames; frame++) 
          {
            current_frame = subband_origin_frame + frame;
            if ((threshold - 0.000001) <
                pow((double)2, (double)(max_coefficient_bits[subband][frame])))
              {
                for (row = 0; row < subband_num_rows; row++)
                  {
                    current_row = subband_origin_row + row;
                    for (col = 0; col < subband_num_cols; col++)
                      { 
                        current_col = subband_origin_col + col;
                        if (significance_map[current_frame][current_row][current_col] == QCCWAVKLTTCE3D_NZN)
                          {
                            if (QccWAVTce3DUpdateModel(model, p))
                              {
                                QccErrorAddMessage("(QccWAVklttce3DNZNPass): Error calling QccWAVklttceUpdateModel()");
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
                                    1.5 * threshold;
                              }

                            p =
                              p * QCCWAVKLTTCE3D_ALPHA_1D + symbol * QCCWAVKLTTCE3D_ALPHA_1D_O;
			   
                            if (symbol)
                              {
                                subband_significance[subband]++;
                                significance_map[current_frame][current_row][current_col] =
                                  QCCWAVKLTTCE3D_S_NEW;
                          
                                if (QccWAVTce3DUpdateModel(model, 0.5))
                                  {
                                    QccErrorAddMessage("(QccWAVklttce3DNZNPass): Error calling QccWAVklttceUpdateModel()");
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
    }

  return(0);
}


static int QccWAVklttce3DIPBand(QccWAVSubbandPyramid3D *coefficients,
                                char ***significance_map,
                                char ***sign_array,
                                double ***p_estimation,
                                double *subband_significance,
                                int subband,
                                double threshold,
                                QccENTArithmeticModel *model,
                                QccBitBuffer *buffer,
                                double alpha,int **max_coefficient_bits)
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

  if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                        subband,
                                        &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      goto Error;
    }
  if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                           subband,
                                           &subband_origin_frame,
                                           &subband_origin_row,
                                           &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      goto Error;
    }
  
  if ((p1 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((p3 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p4 = QccMatrixAlloc(subband_num_rows,subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((p5 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccVectorAlloc()");
      goto Error;
    }

  child_density =
    subband_significance[subband] / (subband_num_frames*subband_num_rows * subband_num_cols);
  p_lowerbound = 1.0 / (subband_num_frames*subband_num_rows * subband_num_cols);
  
  if ((subband%(3*coefficients->spatial_num_levels+1))<=3)
    {
      if ((subband%(3*coefficients->spatial_num_levels+1))==0)
	{
	  
          parent_subband = 0;
          parent_density =
            subband_significance[parent_subband] /
            (subband_num_frames*subband_num_rows * subband_num_cols) ;
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
      p3[row][col]= p_estimation[0][subband_origin_row+row][subband_origin_col+col];

  scale =
    QccMathMax(child_density, p_lowerbound) /
    QccMathMax(parent_density, p_lowerbound); 
  increment =
    1.0 / (subband_num_frames*subband_num_rows * subband_num_cols) /
    QccMathMax(parent_density, p_lowerbound);

  for (frame = 0; frame < subband_num_frames; frame++)
    {//aaaa
      current_frame=subband_origin_frame+frame;

      if ((threshold - 0.000001) <
          pow((double)2, (double)max_coefficient_bits[subband][frame]))
        {
          for (col = 0; col < subband_num_cols; col++)
            p2[col] =p_estimation[current_frame][subband_origin_row][subband_origin_col+col]; 

          for (row = 0; row < subband_num_rows; row++)
            {  //aaa  
              current_row = subband_origin_row + row;
              for (col = 0; col < subband_num_cols; col++)
                { //aa  
                  current_col = subband_origin_col + col;
                  p_char = &(significance_map[current_frame][current_row][current_col]);
                  if (col == 0)
                    p_forward =alpha * (QCCWAVKLTTCE3D_BOUNDARY_VALUE + p2[col] +
                                        p3[row][col]);
                  else
                    p_forward = alpha * (p1[row][col-1] + p2[col] + p3[row][col]);
          
                  if (*p_char < QCCWAVKLTTCE3D_S)
                    { //a     
                      p = p_forward + p_estimation[current_frame][current_row][current_col];

                      if (current_frame != 0)
                        { //b
                          if ((subband%(3*coefficients->spatial_num_levels+1))<=3)
                            {
		              if ((subband%(3*coefficients->spatial_num_levels+1))==0)
                                {
                                  if ((subband/(3*coefficients->spatial_num_levels+1))<=1)
                                    p_parent = p_estimation[current_frame-1][current_row][current_col];
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
                          {          
                            p_parent = QccMathMin(p_parent*scale, 0.8);
                            p =
                              p * QCCWAVKLTTCE3D_CURRENT_SCALE +
                              QCCWAVKLTTCE3D_PARENT_SCALE * p_parent;
                          }
                        }//b
                      if (p>1)
                        {
                          p=1;
                        }
                      if (QccWAVTce3DUpdateModel(model, p))
                        {
                          QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccWAVklttceUpdateModel()");
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
                              1.5 * threshold;
                        }
                      v = symbol;
	                
                      if (symbol)
                        {//c
                          subband_significance[subband]++;
                          scale += increment;
                          *p_char = QCCWAVKLTTCE3D_S_NEW;
                  
                          if (QccWAVTce3DUpdateModel(model, 0.5))
                            {
                              QccErrorAddMessage("(QccWAVklttce3DIPBand): Error calling QccWAVklttceUpdateModel()");
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
                              QccErrorAddMessage("(QccWAVklttce3DIPBand): QccWAVklttceIPBand()");
                              goto Error;
                            }
                        } 

                      if (col == 0)
                        p1[row][col] = alpha * p_forward  + filter_coef * v;
                      else
                        p1[row][col] = alpha * p1[row][col-1] + filter_coef * v;
                      p2[col] = p1[row][col] + alpha * p2[col];
 
                    }//a
                  else
                    { 
                      if (col == 0)
                        p1[row][col] = alpha * p_forward + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                      else
                        p1[row][col] = alpha* p1[row][col-1] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                      p2[col] = p1[row][col] + alpha * p2[col];
                    }
                  p_estimation[subband_origin_frame+frame][subband_origin_row + row][subband_origin_col + col] =
                    p_forward;
                }//aa
           
              for (col = subband_num_cols - 1; col >= 0; col--)
                {  
                  if (col ==( subband_num_cols - 1)) 
                    p2[col] = p2[col] + alpha * p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col];
                  else
                    p2[col] = p2[col] + alpha * p4[row][col+1];
                  p3[row][col]=p2[col]+alpha*p3[row][col];
                  p_char = &(significance_map[current_frame][current_row][subband_origin_col + col]);
                  if (*p_char == QCCWAVKLTTCE3D_S)
                    {
                      if (col == (subband_num_cols - 1))
                        p4[row][col] = alpha * p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER; 
                      else 
                        p4[row][col] = alpha * p4[row][col+1] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;;
                    } 
                  else
                    {
                      v = (*p_char == QCCWAVKLTTCE3D_S_NEW);
                      if (col == subband_num_cols - 1)
                        p4[row][col]  = alpha * p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] + filter_coef * v;
                      else
                        p4[row][col] = alpha * p4[row][col+1] + filter_coef * v;
                    }
                }
            }//aaa
	 
          for (col = 0; col < subband_num_cols; col++)
	    p5[col]=p_estimation[current_frame][subband_origin_row +subband_num_rows - 1][subband_origin_col+col]; 
          for (row = subband_num_rows - 1; row >= 0; row--)
            {
              for (col = 0; col < subband_num_cols; col++)
                {
		  p3[row][col]=p3[row][col]+alpha*p5[col];
                  if (col ==( subband_num_cols - 1))
		    p5[col]=p1[row][col]+alpha*p5[col]+alpha*p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] ;
                  else 
                    p5[col]=p1[row][col]+alpha*p5[col]+alpha*p4[row][col+1];

                }
            }
        }
    }//aaaa
	  
  for (frame = subband_num_frames-1;frame >= 0; frame--)
    {//bbb
      current_frame=subband_origin_frame+frame;

      if ((threshold - 0.000001) <
          pow((double)2, (double)max_coefficient_bits[subband][frame]))
        {
          for (row = subband_num_rows - 1; row >= 0; row--)
            {//bb
       
              current_row = subband_origin_row + row;
              for (col = subband_num_cols-1; col >=0; col--)
                {   
                  current_col = subband_origin_col + col; 
                  p_char = &(significance_map[current_frame][current_row][current_col]);
                  if (col ==( subband_num_cols-1) )
                    p_estimation[current_frame][current_row][current_col]+=alpha * (p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] + p2[col] + p3[row][col]);
                  else
                    p_estimation[current_frame][current_row][current_col] +=alpha *(p1[row][col+1]+p2[col]+p3[row][col]);
                 
                  if (*p_char == QCCWAVKLTTCE3D_S)
                    {
                      p_estimation[current_frame][current_row][current_col]=
                        QCCWAVKLTTCE3D_REFINE_HOLDER * weight[0] +
                        p_estimation[current_frame][current_row][current_col] * weight[1];
                      if (col == (subband_num_cols-1) )
                        p1[row][col] = alpha * p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                      else
                        p1[row][col] = alpha * p1[row][col+1] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                    }
                  else
                    {
                      v = (*p_char == QCCWAVKLTTCE3D_S_NEW);
                      p_estimation[current_frame][current_row][current_col] =
                        v * weight[0] +p_estimation[current_frame][current_row][current_col] * weight[1];
                      if (col == (subband_num_cols-1) )
                        p1[row][col] = alpha *  p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col]  + filter_coef * v;
                      else

                        p1[row][col] = alpha * p1[row][col+1] + filter_coef * v;
                    }    
                  p2[col] = p1[row][col] + alpha * p2[col];   
                }
          
              for (col = 0; col < subband_num_cols; col++)
                {
                  if (col==0)
                    p2[col] = p2[col] + alpha * p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col];
                  else
                    p2[col] = p2[col] + alpha * p4[row][col-1];
             
                  current_col = subband_origin_col + col; 
                  p_char = &(significance_map[current_frame][current_row][current_col]);
                  if (*p_char == QCCWAVKLTTCE3D_S)
                    {
                      if (col==0)
                        p4[row][col] = alpha * p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                      else 
                        p4[row][col] = alpha * p4[row][col-1] + filter_coef * QCCWAVKLTTCE3D_REFINE_HOLDER;
                    }
                  else
                    {
                      v = (*p_char == QCCWAVKLTTCE3D_S_NEW);
		      if (col==0)
		        p4[row][col] = alpha * p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col] + filter_coef * v;
	 	      else 
		        p4[row][col] = alpha * p4[row][col-1] + filter_coef *v;
                 
                    }        
                }
            }//bb
        
          for (row = 0; row < subband_num_rows; row++)
            for (col = 0; col < subband_num_cols; col++)
	      {
		p3[row][col]=p3[row][col]+alpha*p5[col];
                if (col==0)
		  p5[col]=p1[row][col]+alpha*p5[col]+alpha* p_estimation[subband_origin_frame+frame][subband_origin_row +row][subband_origin_col + col]; 
                else
                  p5[col]=p1[row][col]+alpha*p5[col]+alpha*p4[row][col-1];

	      }
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


static int QccWAVklttce3DIPPass(QccWAVSubbandPyramid3D *coefficients,    
                                char ***significance_map,
                                char ***sign_array,
                                double threshold,
                                double ***p_estimation,
                                double *subband_significance,
                                QccENTArithmeticModel *model,
                                QccBitBuffer *buffer,
                                int **max_coefficient_bits,
                                double alpha)
{
  int subband,num_subbands; 
  int return_value;

  num_subbands =
    QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                       coefficients->spatial_num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      {
        return_value =
          QccWAVklttce3DRevEst(coefficients,
                               significance_map,
                               subband,
                               subband_significance,
                               p_estimation,
                               alpha);
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVklttce3DIPPass): Error calling QccWAVklttce3DRevEst()");
            return(1);
          }
        else
          {
            if (return_value == 2)
              return(2);
          }
          
        return_value = QccWAVklttce3DIPBand(coefficients,
                                            significance_map,
                                            sign_array,
                                            p_estimation,
                                            subband_significance,
                                            subband,
                                            threshold,
                                            model,
                                            buffer,
                                            alpha,max_coefficient_bits);

        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVklttce3DIPPass): Error calling QccWAVklttce3DIPBand()");
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


int static QccWAVklttce3DSPass(QccWAVSubbandPyramid3D *coefficients,    
                               char ***significance_map,
                               double threshold,
                               QccENTArithmeticModel *model,
                               QccBitBuffer *buffer,
                               int **max_coefficient_bits)
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
    QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                       coefficients->spatial_num_levels);
 
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                            subband,
                                            &subband_num_frames,
                                            &subband_num_rows,
                                            &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVklttce3DSPass): Error calling QccWAVSubbandPyramid3DSubbandSize()");
          return(1);
        }
      if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                               subband,
                                               &subband_origin_frame,
                                               &subband_origin_row,
                                               &subband_origin_col))
        {
          QccErrorAddMessage("(QccWAVklttce3DSPass): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
          return(1);
        }  
          
      for (frame = 0; frame < subband_num_frames; frame++) 
        {
          current_frame = subband_origin_frame + frame;

          if ((threshold - 0.000001) <
              pow((double)2, (double)(max_coefficient_bits[subband][frame])))
            {            
              for (row = 0; row < subband_num_rows; row++)
                {
                  current_row = subband_origin_row + row;
                  for (col = 0; col < subband_num_cols; col++)
                    { 
                      current_col = subband_origin_col + col;
                      p_char = &(significance_map[current_frame][current_row][current_col]);

                      if (*p_char == QCCWAVKLTTCE3D_S)
                        { 
                      
                          if (QccWAVTce3DUpdateModel(model, p))
                            {
                              QccErrorAddMessage("(QccWAVklttce3DSPass): Error calling QccWAVklttceUpdateModel()");
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
                              coefficients->volume[current_frame][current_row][current_col] +=
                                (symbol == 1) ? threshold / 2 : -threshold / 2;
                            }
                          p =
                            p * QCCWAVKLTTCE3D_ALPHA_1D + symbol * QCCWAVKLTTCE3D_ALPHA_1D_O;
                        }
                      else
                        {
                          if (*p_char == QCCWAVKLTTCE3D_S_NEW)
                            *p_char = QCCWAVKLTTCE3D_S;
                          if (*p_char == QCCWAVKLTTCE3D_NZN_NEW)
                            *p_char = QCCWAVKLTTCE3D_NZN;     
                        }
                    }
                }
            }
        }
    } 

  return(0);
}


int QccWAVklttce3DEncode(const QccIMGImageCube *image,
                         QccBitBuffer *buffer,
                         int num_levels,
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
  int **max_coefficient_bits = NULL;
  int *max_coefficient_bits_1D = NULL;
  int threshold;
  int num_symbols[1] = { 2 };
  int frame, row, col;
  int bitplane = 0;
  int num_subbands;
  int max_max_coefficient_bits = 0;
  int subband;
  QccHYPklt klt;

  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  QccHYPkltInitialize(&klt);
  
  if (image == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  image_subband_pyramid.spatial_num_levels = 0;
  image_subband_pyramid.temporal_num_levels = 0;
  image_subband_pyramid.num_frames = image->num_frames;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;

  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    }
  
  klt.num_bands = image->num_frames;
  if (QccHYPkltAlloc(&klt))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccHYPkltAlloc()");
      goto Error;
    }

  if ((sign_array =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((sign_array[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((sign_array[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
            goto Error;
          }
    }
  
  if ((significance_map =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((significance_map[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((significance_map[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        significance_map[frame][row][col] = QCCWAVKLTTCE3D_Z;
  
  if ((p_estimation =
       (double ***)malloc(sizeof(double **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((p_estimation[frame] =
           (double **)malloc(sizeof(double *) *
                             image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((p_estimation[frame][row] =
             (double *)malloc(sizeof(double) *
                              image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
            goto Error;
          }
    }

  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        p_estimation[frame][row][col] = 0.0000001;
  
  num_subbands =
    QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(0, num_levels);

  if ((max_coefficient_bits =
       (int **)malloc(sizeof(int*) * num_subbands )) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
      goto Error;
    }

  for (subband = 0; subband < num_subbands; subband++)
    if ((max_coefficient_bits[subband] =
         (int *)malloc(sizeof(int) * image->num_frames)) == NULL)
      {
        QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
        goto Error;
      }

  if ((max_coefficient_bits_1D =
       (int *)malloc(sizeof(int) * image->num_frames*num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
      goto Error;
    }
 
  if ((subband_significance =
       (double*)malloc(num_subbands* sizeof(double))) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error allocating memory");
      goto Error;
    } 

  if (QccWAVklttce3DForwardTransform(&image_subband_pyramid,
                                     sign_array,
                                     image,
                                     num_levels,
                                     max_coefficient_bits,
                                     wavelet,
                                     &klt))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccWAVklttce3DForwardTransform()");
      goto Error;
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    for (frame = 0; frame < image->num_frames; frame++)
      max_coefficient_bits_1D[frame*num_subbands + subband] =
        max_coefficient_bits[subband][frame];
 
  if (QccWAVklttce3DEncodeHeader(buffer,
                                 num_levels,
                                 image->num_frames,
                                 image->num_rows,
                                 image->num_cols, 
                                 max_coefficient_bits_1D,
                                 alpha))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccWAVklttce3DEncodeHeader()");
      goto Error;
    }
  if (QccWAVklttce3DEncodeBitPlaneInfo(buffer,
                                       image->num_frames*num_subbands,
                                       max_coefficient_bits_1D))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccWAVklttce3DEncodeBitPlaneInfo()");
      goto Error;
    }
  
  if (QccWAVklttce3DEncodeKLT(buffer,
                              &klt))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccWAVklttce3DEncodeKLT()");
      goto Error;
    }

  if ((model = QccENTArithmeticEncodeStart(num_symbols,
					   1,
					   NULL,
					   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);

  for (subband = 0; subband < image->num_frames * num_subbands; subband++)
    if (max_max_coefficient_bits < max_coefficient_bits_1D[subband])
      max_max_coefficient_bits = max_coefficient_bits_1D[subband];
  
  threshold = pow((double)2, (double)max_max_coefficient_bits);
 
  while (bitplane < QCCWAVKLTTCE3D_MAXBITPLANES)  
    {
      return_value = QccWAVklttce3DNZNPass(&image_subband_pyramid,
                                           significance_map,
                                           sign_array,
                                           threshold,
                                           subband_significance,
                                           model, 
                                           buffer,
                                           max_coefficient_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccWAVklttce3DNZNPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }

      return_value = QccWAVklttce3DIPPass(&image_subband_pyramid,
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
          QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccWAVklttce3DIPPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
        
      return_value = QccWAVklttce3DSPass(&image_subband_pyramid, 
                                         significance_map,
                                         threshold,
                                         model,
                                         buffer,
                                         max_coefficient_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccWAVklttceSPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }  
        
      threshold /= 2.0;

      bitplane++;
    }
  
  if (QccENTArithmeticEncodeEnd(model,
                                0,
                                buffer))
    {
      QccErrorAddMessage("(QccWAVklttce3DEncode): Error calling QccENTArithmeticEncodeEnd()");
      goto Error;
    }  

 Finished:
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  QccHYPkltFree(&klt);

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


int QccWAVklttce3DDecodeHeader(QccBitBuffer *buffer, 
                               int *num_levels, 
                               int *num_frames,
                               int *num_rows,
                               int *num_cols,
                               int *max_coefficient_bits,
                               double *alpha)
{
  int return_value;
  unsigned char ch;
  
  if (QccBitBufferGetChar(buffer, &ch))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecodeHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  *num_levels = (int)ch;

  if (QccBitBufferGetInt(buffer, num_frames))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }

  if (QccBitBufferGetInt(buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }

  if (QccBitBufferGetDouble(buffer, alpha))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVklttce3DDecode(QccBitBuffer *buffer,
                         QccIMGImageCube *image,                     
                         int num_levels,
                         double alpha,
                         const QccWAVWavelet *wavelet,
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
  int **max_bits = NULL;
  int *max_bits_1D = NULL;
  int threshold;
  int frame, row, col;
  int num_symbols[1] = { 2 };
  int num_subbands;
  int subband;
  int max_max_coefficient_bits = 0;
  QccHYPklt klt;

  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  QccHYPkltInitialize(&klt);

  if (image == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  image_subband_pyramid.transform_type = QCCWAVSUBBANDPYRAMID3D_PACKET;
  image_subband_pyramid.temporal_num_levels = 0;
  image_subband_pyramid.spatial_num_levels = num_levels;
  image_subband_pyramid.num_frames = image->num_frames;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;

  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    }
  
  klt.num_bands = image->num_frames;
  if (QccHYPkltAlloc(&klt))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccHYPkltAlloc()");
      goto Error;
    }

  if ((sign_array =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((sign_array[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((sign_array[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
            goto Error;
          }
    }
  
  if ((significance_map =
       ( char ***)malloc(sizeof( char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((significance_map[frame] =
           (char **)malloc(sizeof( char *) *
                           image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((significance_map[frame][row] =
             ( char *)malloc(sizeof( char) *
                             image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        significance_map[frame][row][col] = QCCWAVKLTTCE3D_Z;

  num_subbands =
    QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(image_subband_pyramid.temporal_num_levels,
                                                       image_subband_pyramid.spatial_num_levels);

  if ((max_bits =
       (int **)malloc(sizeof(int*) * num_subbands )) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
      goto Error;
    }

  for (subband = 0; subband < num_subbands; subband++)
    if ((max_bits[subband] =
         (int *)malloc(sizeof(int) * image->num_frames)) == NULL)
      {
        QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
        goto Error;
      }
  if ((max_bits_1D =
       (int *)malloc(sizeof(int) * image->num_frames * num_subbands )) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
      goto Error;
    }

  if (QccWAVklttce3DDecodeBitPlaneInfo(buffer,
                                       image->num_frames*num_subbands,
                                       max_bits_1D,
                                       max_coefficient_bits))

    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccWAVklttce3DDecodeBitPlaneInfo()");
      goto Error;
    }

  for (subband = 0; subband < num_subbands; subband++)
    for (frame = 0; frame < image->num_frames; frame++)
      max_bits[subband][frame] = max_bits_1D[frame*num_subbands + subband];
     
  if ((p_estimation =
       (double ***)malloc(sizeof(double **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((p_estimation[frame] =
           (double **)malloc(sizeof(double *) *
                             image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((p_estimation[frame][row] =
             (double *)malloc(sizeof(double) *
                              image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
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
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error allocating memory");
      goto Error;
    }  
  
  if (QccWAVklttce3DDecodeKLT(buffer,
                              &klt))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccWAVklttce3DDecodeKLT()");
      goto Error;
    }

  if (target_bit_cnt != QCCENT_ANYNUMBITS)
    if (target_bit_cnt < buffer->bit_cnt)
      goto Finished;

  if ((model =
       QccENTArithmeticDecodeStart(buffer,
				   num_symbols,
				   1,
				   NULL,
                                   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);

  for (subband = 0; subband < image->num_frames*num_subbands; subband++)
    if (max_max_coefficient_bits <= max_bits_1D[subband])
      max_max_coefficient_bits = max_bits_1D[subband];

  threshold = pow((double)2, (double)max_max_coefficient_bits);
    
  while (1)
    {
      return_value = QccWAVklttce3DNZNPass(&image_subband_pyramid,
				           significance_map,
                                           sign_array,
                                           threshold,
                                           subband_significance,
                                           model,
                                           buffer,
                                           max_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccWAVklttceNZNPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
        
      return_value = QccWAVklttce3DIPPass(&image_subband_pyramid,
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
          QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccWAVklttceIPPass()");
          goto Error;
        }
      else
        {
          if (return_value == 2)
            goto Finished;
        }
        
      return_value = QccWAVklttce3DSPass(&image_subband_pyramid, 
                                         significance_map,
                                         threshold,
                                         model,
                                         buffer,
                                         max_bits);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccWAVklttceSPass()");
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
  if (QccWAVklttce3DInverseTransform(&image_subband_pyramid,
                                     sign_array,
                                     image,
                                     wavelet,
                                     &klt))
    {
      QccErrorAddMessage("(QccWAVklttce3DDecode): Error calling QccWAVklttce3DInverseTransform()");
      goto Error;
    }

  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  QccHYPkltFree(&klt);

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
  QccENTArithmeticFreeModel(model);
  return(return_value);
}
