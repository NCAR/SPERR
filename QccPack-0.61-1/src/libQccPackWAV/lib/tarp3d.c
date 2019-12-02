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


#include "libQccPack.h"


#define QCCTARP3D_BOUNDARY_VALUE 0.0

#define QCCTARP3D_INSIGNIFICANT 0
#define QCCTARP3D_PREVIOUSLY_SIGNIFICANT 1
#define QCCTARP3D_NEWLY_SIGNIFICANT 2

#define QCCTARP3D_SIGNIFICANCEMASK 0x03
#define QCCTARP3D_SIGNMASK 0x04

#define QCCTARP3D_MAXBITPLANES 128


static void QccWAVTarp3DSetSignificance(unsigned char *state,
                                        int significance)
{
  *state &= (0xff ^ QCCTARP3D_SIGNIFICANCEMASK); 
  *state |= significance;
}


static int QccWAVTarp3DGetSignificance(unsigned char state)
{
  return(state & QCCTARP3D_SIGNIFICANCEMASK);
}


static void QccWAVTarp3DSetSign(unsigned char *state, int sign_value)
{
  if (sign_value)
    *state |= QCCTARP3D_SIGNMASK;
  else
    *state &= (0xff ^ QCCTARP3D_SIGNMASK); 
}


static int QccWAVTarp3DGetSign(unsigned char state)
{
  return((state & QCCTARP3D_SIGNMASK) == QCCTARP3D_SIGNMASK);
}


static int QccWAVTarp3DUpdateModel(QccENTArithmeticModel *model, double prob)
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


static int QccWAVTarp3DEncodeDWT(QccWAVSubbandPyramid3D *image_subband_pyramid,
                                 unsigned char ***state_array,
                                 const QccIMGImageCube *image,
                                 int transform_type,
                                 int temporal_num_levels,
                                 int spatial_num_levels,
                                 double *image_mean,
                                 int *max_coefficient_bits,
                                 QccWAVSubbandPyramid3D *mask_subband_pyramid,
                                 const QccIMGImageCube *mask,
                                 const QccWAVWavelet *wavelet)
{
  double coefficient_magnitude;
  double max_coefficient = -MAXFLOAT;
  int frame, row, col;
  
  if (mask == NULL)
    {
      if (QccVolumeCopy(image_subband_pyramid->volume,
                        image->volume,
                        image->num_frames,
                        image->num_rows,
                        image->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarp3DEncodeDWT): Error calling QccVolumeCopy()");
          return(1);
        }
      
      if (QccWAVSubbandPyramid3DSubtractMean(image_subband_pyramid,
                                             image_mean,
                                             NULL))
        {
          QccErrorAddMessage("(QccWAVTarp3DEncodeDWT): Error calling QccWAVSubbandPyramid3DSubtractMean()");
          return(1);
        }
    }
  else
    {
      if (QccVolumeCopy(mask_subband_pyramid->volume,
                        mask->volume,
                        mask->num_frames,
                        mask->num_rows,
                        mask->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarp3DEncodeDWT): Error calling QccVolumeCopy()");
          return(1);
        }
      
      *image_mean = QccIMGImageCubeShapeAdaptiveMean(image, mask);
      
      for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
        for (row = 0; row < image_subband_pyramid->num_rows; row++)
          for (col = 0; col < image_subband_pyramid->num_cols; col++)
            if (QccAlphaTransparent(mask_subband_pyramid->volume
                                    [frame][row][col]))
              image_subband_pyramid->volume[frame][row][col] = 0;
            else
              image_subband_pyramid->volume[frame][row][col] =
                image->volume[frame][row][col] - *image_mean;
    }
  
  if (mask != NULL)
    {
      if (QccWAVSubbandPyramid3DShapeAdaptiveDWT(image_subband_pyramid,
                                                 mask_subband_pyramid,
                                                 transform_type,
                                                 temporal_num_levels,
                                                 spatial_num_levels,
                                                 wavelet))
        {
          QccErrorAddMessage("(QccWAVTarp3DEncodeDWT): Error calling QccWAVSubbandPyramid3DShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramid3DDWT(image_subband_pyramid,
                                  transform_type,
                                  temporal_num_levels,
                                  spatial_num_levels,
                                  wavelet))
      {
        QccErrorAddMessage("(QccWAVTarp3DEncodeDWT): Error calling QccWAVSubbandPyramid3DDWT()");
        return(1);
      }
  
  for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        {
          coefficient_magnitude =
            fabs(image_subband_pyramid->volume[frame][row][col]);
          if (image_subband_pyramid->volume[frame][row][col] !=
              coefficient_magnitude)
            {
              image_subband_pyramid->volume[frame][row][col] =
                coefficient_magnitude;
              QccWAVTarp3DSetSign(&state_array[frame][row][col], 1);
            }
          else
            QccWAVTarp3DSetSign(&state_array[frame][row][col], 0);
          if (coefficient_magnitude > max_coefficient)
            max_coefficient = coefficient_magnitude;
        }
  
  *max_coefficient_bits = (int)floor(QccMathLog2(max_coefficient));
  
  return(0);
}


int QccWAVTarp3DEncodeHeader(QccBitBuffer *buffer, 
                             int transform_type,
                             int temporal_num_levels,
                             int spatial_num_levels,
                             int num_frames,
                             int num_rows,
                             int num_cols,
                             double image_mean,
                             int max_coefficient_bits,
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
  
  if (QccBitBufferPutInt(buffer, max_coefficient_bits))
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


static int QccWAVTarp3DSignificanceInputOutput(double *coefficient,
                                               double p,
                                               double threshold,
                                               unsigned char *state,
                                               QccENTArithmeticModel *model,
                                               QccBitBuffer *buffer)
{
  int symbol;
  int return_value;
  
  if (QccWAVTarp3DGetSignificance(*state))
    return(0);
  
  if (QccWAVTarp3DUpdateModel(model, p))
    {
      QccErrorAddMessage("(QccWAVTarp3DSignificanceInputOutput): Error calling QccWAVTarp3DUpdateModel()");
      return(1);
    }
  
  if (buffer->type == QCCBITBUFFER_OUTPUT)
    {
      if (*coefficient >= threshold)
	{
	  symbol = 1;
	  *coefficient -= threshold;
	}
      else
	symbol = 0;
      
      return_value =
	QccENTArithmeticEncode(&symbol, 1,
			       model, buffer);
      
      if (return_value == 2)
	return(2);
      else
	if (return_value)
	  {
	    QccErrorAddMessage("(QccWAVTarp3DSignificanceInputOutput): Error calling QccENTArithmeticEncode()");
	    return(1);
	  }
    }
  else
    {
      if (QccENTArithmeticDecode(buffer,
				 model,
				 &symbol, 1))
	return(2);
      
      if (symbol)
	*coefficient  = 1.5 * threshold;
    }
  
  if (symbol)
    {
      QccWAVTarp3DSetSignificance(state, QCCTARP3D_NEWLY_SIGNIFICANT);
      
      if (QccWAVTarp3DUpdateModel(model, 0.5))
	{
	  QccErrorAddMessage("(QccWAVTarp3DSignificanceInputOutput): Error calling QccWAVTarp3DUpdateModel()");
	  return(1);
	}
      
      if (buffer->type == QCCBITBUFFER_OUTPUT)
	{
	  symbol = QccWAVTarp3DGetSign(*state);
          
	  return_value =
	    QccENTArithmeticEncode(&symbol, 1,
				   model, buffer);
	  if (return_value == 2)
	    return(2);
	  else
	    if (return_value)
	      {
		QccErrorAddMessage("(QccWAVTarp3DSignificanceInputOutput): Error calling QccENTArithmeticEncode()");
		return(1);
	      }
	}
      else
	{
	  if (QccENTArithmeticDecode(buffer,
				     model,
				     &symbol, 1))
	    return(2);
          
	  QccWAVTarp3DSetSign(state, symbol);
	}
    }
  
  return(0);
}


static int QccWAVTarp3DSignificancePassSubband(QccWAVSubbandPyramid3D
                                               *coefficients,
                                               QccWAVSubbandPyramid3D
                                               *mask,
                                               unsigned char ***state_array,
                                               double alpha,
                                               int subband,
                                               double threshold,
                                               QccENTArithmeticModel *model,
                                               QccBitBuffer *buffer)
{
  int return_value;
  int subband_origin_frame;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  int frame, row, col;
  QccVector p2 = NULL;
  QccVector p5 = NULL;
  QccMatrix p1 = NULL;
  QccMatrix p4 = NULL;
  QccMatrix p3 = NULL;
  double beta;
  double p;
  int v;
  int transparent;

  beta = (1.0-alpha)*(1.0-alpha)*(1.0-alpha)/(3.0*alpha+alpha*alpha*alpha);
  
  if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                        subband,
                                        &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVTarp3DSignificancePassSubband): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      goto Error;
    }
  if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                           subband,
                                           &subband_origin_frame,
                                           &subband_origin_row,
                                           &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVTarp3DSignificancePassSubband): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      goto Error;
    }
  
  if ((!subband_num_frames) || (!subband_num_rows) || (!subband_num_cols))
    return(0);

  if ((p2 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DSignificancePassSubband): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  if ((p5 = QccVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DSignificancePassSubband): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  if ((p1 = QccMatrixAlloc(subband_num_rows, subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DSignificancePassSubband): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((p4 = QccMatrixAlloc(subband_num_rows, subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DSignificancePassSubband): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  if ((p3 = QccMatrixAlloc(subband_num_rows, subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DSignificancePassSubband): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  for (row = 0; row < subband_num_rows; row++)
    for (col = 0; col < subband_num_cols; col++)
      p3[row][col] = QCCTARP3D_BOUNDARY_VALUE;   
  
  for (frame = 0; frame < subband_num_frames; frame++)
    {
      for (col = 0; col < subband_num_cols; col++)
        p2[col] = QCCTARP3D_BOUNDARY_VALUE;
      for (row = 0; row < subband_num_rows; row++)
        {
          for (col = 0; col < subband_num_cols; col++)
            {
              transparent =
                (mask != NULL) ? 
                QccAlphaTransparent(mask->volume
                                    [subband_origin_frame + frame]
                                    [subband_origin_row + row]
                                    [subband_origin_col + col]) : 0;

              if (!transparent)
                {
                  if (col == 0)
                    p =
                      alpha * (QCCTARP3D_BOUNDARY_VALUE + p2[col] +
                               p3[row][col]);
                  else
                    p = alpha * (p1[row][col-1] + p2[col] + p3[row][col]);
                  
                  return_value =
                    QccWAVTarp3DSignificanceInputOutput(&coefficients->volume
                                                        [subband_origin_frame +
                                                         frame]
                                                        [subband_origin_row +
                                                         row]
                                                        [subband_origin_col +
                                                         col],
                                                        p,
                                                        threshold,
                                                        &state_array
                                                        [subband_origin_frame +
                                                         frame]
                                                        [subband_origin_row +
                                                         row]
                                                        [subband_origin_col +
                                                         col],
                                                        model,
                                                        buffer);
                  if (return_value == 2)
                    goto Return;
                  else
                    if (return_value)
                      {
                        QccErrorAddMessage("(QccWAVTarp3DSignificancePassSubband): Error calling QccWAVTarp3DSignificanceInputOutput()");
                        goto Error;
                      }
                  
                  v =
                    QccWAVTarp3DGetSignificance(state_array
                                                [subband_origin_frame + frame]
                                                [subband_origin_row + row]
                                                [subband_origin_col + col]) !=
                    QCCTARP3D_INSIGNIFICANT;
                  if (col == 0)
                    p1[row][col] = alpha * QCCTARP3D_BOUNDARY_VALUE + beta * v;
                  else
                    p1[row][col] = alpha * p1[row][col-1] + beta * v;
                  p2[col] = p1[row][col] + alpha * p2[col];
                }
            }
          
          for (col = subband_num_cols - 1; col >= 0; col--)
            {
              transparent =
                (mask != NULL) ? 
                QccAlphaTransparent(mask->volume
                                    [subband_origin_frame + frame]
                                    [subband_origin_row + row]
                                    [subband_origin_col + col]) : 0;

              if (!transparent)
                {
                  if (col == subband_num_cols - 1)
                    p2[col] += (alpha * QCCTARP3D_BOUNDARY_VALUE);
                  else
                    p2[col] += (alpha * p4[row][col+1]);
                  p3[row][col] = p2[col] + alpha * p3[row][col];
                  v =
                    QccWAVTarp3DGetSignificance(state_array
                                                [subband_origin_frame + frame]
                                                [subband_origin_row + row]
                                                [subband_origin_col + col]) !=
                    QCCTARP3D_INSIGNIFICANT;
                  if (col == subband_num_cols - 1)
                    p4[row][col] = alpha * QCCTARP3D_BOUNDARY_VALUE + beta * v;
                  else
                    p4[row][col] = alpha * p4[row][col+1] + beta * v;
                }
            }
        }
      
      for (col = 0; col < subband_num_cols; col++)
        p5[col] = QCCTARP3D_BOUNDARY_VALUE;
      for (row = subband_num_rows - 1; row >= 0 ; row--)
        {
          for (col = 0; col < subband_num_cols; col++)
            {
              transparent =
                (mask != NULL) ? 
                QccAlphaTransparent(mask->volume
                                    [subband_origin_frame + frame]
                                    [subband_origin_row + row]
                                    [subband_origin_col + col]) : 0;

              if (!transparent)
                {
                  p3[row][col] += (alpha * p5[col]);
                  if (col == subband_num_cols - 1)
                    p5[col] = p1[row][col] + alpha * p5[col] +
                      alpha * QCCTARP3D_BOUNDARY_VALUE;
                  else
                    p5[col] = p1[row][col] + alpha * p5[col] +
                      alpha * p4[row][col+1];
                }
            }
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(p2);
  QccVectorFree(p5);
  QccMatrixFree(p1, subband_num_rows);
  QccMatrixFree(p4, subband_num_rows);
  QccMatrixFree(p3, subband_num_rows);
  return(return_value);
}


static int QccWAVTarp3DSignificancePass(QccWAVSubbandPyramid3D *coefficients,
                                        QccWAVSubbandPyramid3D *mask,
                                        unsigned char ***state_array,
                                        double alpha,
                                        double threshold,
                                        QccENTArithmeticModel *model,
                                        QccBitBuffer *buffer)
{
  int subband;
  int num_subbands;
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
      return_value =
	QccWAVTarp3DSignificancePassSubband(coefficients,
                                            mask,
                                            state_array,
                                            alpha,
                                            subband,
                                            threshold,
                                            model,
                                            buffer);
      if (return_value == 1)
	{
	  QccErrorAddMessage("(QccWAVTarp3DSignificancePass): Error calling QccWAVTarp3DSignificancePassSubband()");
	  return(1);
	}
      else
	if (return_value == 2)
	  return(2);
    }
  
  return(0);
}


static int QccWAVTarp3DRefinementInputOutput(double *coefficient,
                                             double threshold,
                                             QccENTArithmeticModel *model,
                                             QccBitBuffer *buffer)
{
  int return_value;
  int symbol;
  
  if (QccWAVTarp3DUpdateModel(model, 0.5))
    {
      QccErrorAddMessage("(QccWAVTarp3DRefinementInputOutput): Error calling QccWAVTarp3DUpdateModel()");
      return(1);
    }
  
  if (buffer->type == QCCBITBUFFER_OUTPUT)
    {
      if (*coefficient >= threshold)
	{
	  symbol = 1;
	  *coefficient -= threshold;
	}
      else
	symbol = 0;
      
      return_value =
	QccENTArithmeticEncode(&symbol, 1,
			       model, buffer);
      if (return_value == 2)
	return(2);
      else
	if (return_value)
	  {
	    QccErrorAddMessage("(QccWAVTarp3DRefinementInputOutput): Error calling QccENTArithmeticEncode()");
	    return(1);
	  }
    }
  else
    {
      if (QccENTArithmeticDecode(buffer,
				 model,
				 &symbol, 1))
	return(2);
      
      *coefficient +=
	(symbol == 1) ? threshold / 2 : -threshold / 2;
    }
  
  return(0);
}


static int QccWAVTarp3DRefinementPassSubband(QccWAVSubbandPyramid3D
                                             *coefficients,
                                             QccWAVSubbandPyramid3D
                                             *mask,
                                             unsigned char ***state_array,
                                             double threshold,
                                             int subband,
                                             QccENTArithmeticModel *model,
                                             QccBitBuffer *buffer)
{
  int return_value;
  int subband_origin_frame;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  int frame, row, col;
  
  if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                        subband,
                                        &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVTarp3DRefinementPassSubband): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      return(1);
    }
  if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                           subband,
                                           &subband_origin_frame,
                                           &subband_origin_row,
                                           &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVTarp3DRefinementPassSubband): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      return(1);
    }
  
  for (frame = 0; frame < subband_num_frames; frame++)
    for (row = 0; row < subband_num_rows; row++)
      for (col = 0; col < subband_num_cols; col++)
        if (QccWAVTarp3DGetSignificance(state_array
                                        [subband_origin_frame + frame]
                                        [subband_origin_row + row]
                                        [subband_origin_col + col]) ==
            QCCTARP3D_PREVIOUSLY_SIGNIFICANT)
          {
            return_value =
              QccWAVTarp3DRefinementInputOutput(&coefficients->volume
                                                [subband_origin_frame + frame]
                                                [subband_origin_row + row]
                                                [subband_origin_col + col],
                                                threshold,
                                                model,
                                                buffer);
            if (return_value == 1)
              {
                QccErrorAddMessage("(QccWAVTarp3DRefinementPassSubband): Error calling QccWAVTarp3DRefinementPassInputOutput()");
                return(1);
              }
            else
              if (return_value == 2)
                return(2);
          }
        else
          if (QccWAVTarp3DGetSignificance(state_array
                                          [subband_origin_frame + frame]
                                          [subband_origin_row + row]
                                          [subband_origin_col + col]) ==
              QCCTARP3D_NEWLY_SIGNIFICANT)
            QccWAVTarp3DSetSignificance(&state_array
                                        [subband_origin_frame + frame]
                                        [subband_origin_row + row]
                                        [subband_origin_col + col],
                                        QCCTARP3D_PREVIOUSLY_SIGNIFICANT);
  
  return(0);
}


static int QccWAVTarp3DRefinementPass(QccWAVSubbandPyramid3D *coefficients,
                                      QccWAVSubbandPyramid3D *mask,
                                      unsigned char ***state_array,
                                      double threshold,
                                      QccENTArithmeticModel *model,
                                      QccBitBuffer *buffer)
{
  int subband;
  int num_subbands;
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
      return_value =
	QccWAVTarp3DRefinementPassSubband(coefficients,
                                          mask,
                                          state_array,
                                          threshold,
                                          subband,
                                          model,
                                          buffer);
      if (return_value == 1)
	{
	  QccErrorAddMessage("(QccWAVTarp3DRefinementPass): Error calling QccWAVTarp3DRefinementPassSubband()");
	  return(1);
	}
      else
	if (return_value == 2)
	  return(2);
    }
  
  return(0);
}


int QccWAVTarp3DEncode(const QccIMGImageCube *image,
                       const QccIMGImageCube *mask,
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
  QccWAVSubbandPyramid3D mask_subband_pyramid;
  unsigned char ***state_array = NULL;
  double image_mean;
  int max_coefficient_bits;
  double threshold;
  int num_symbols[1] = { 2 };
  int frame, row, col;
  int bitplane = 0;

  if (image == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramid3DInitialize(&mask_subband_pyramid);
  
  image_subband_pyramid.spatial_num_levels = 0;
  image_subband_pyramid.temporal_num_levels = 0;
  image_subband_pyramid.num_frames = image->num_frames;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_frames != image->num_frames) ||
          (mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarp3DEncode): Mask and image cube must be same size");
          goto Error;
        }
      
      mask_subband_pyramid.temporal_num_levels = 0;
      mask_subband_pyramid.spatial_num_levels = 0;
      mask_subband_pyramid.num_frames = mask->num_frames;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramid3DAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVTarp3DEncode): Error calling QccWAVSubbandPyramid3DAlloc()");
          goto Error;
        }
    }
  
  if ((state_array =
       (unsigned char ***)malloc(sizeof(unsigned char **) * image->num_frames))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((state_array[frame] =
           (unsigned char **)malloc(sizeof(unsigned char *) *
                                    image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVTarp3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((state_array[frame][row] =
             (unsigned char *)malloc(sizeof(unsigned char) *
                                     image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVTarp3DEncode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        state_array[frame][row][col] = 0;
  
  if (QccWAVTarp3DEncodeDWT(&image_subband_pyramid,
                            state_array,
                            image,
                            transform_type,
                            temporal_num_levels,
                            spatial_num_levels,
                            &image_mean,
                            &max_coefficient_bits,
                            ((mask != NULL) ? &mask_subband_pyramid : NULL),
                            mask,
                            wavelet))
    {
      QccErrorAddMessage("(QccWAVTarp3DEncode): Error calling QccWAVTarp3DEncodeDWT()");
      goto Error;
    }
  
  alpha = (double)((float)alpha);

  if (QccWAVTarp3DEncodeHeader(buffer,
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
      QccErrorAddMessage("(QccWAVTarp3DEncode): Error calling QccWAVTarp3DEncodeHeader()");
      goto Error;
    }
  
  if ((model = QccENTArithmeticEncodeStart(num_symbols,
					   1,
					   NULL,
					   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);
  
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  while (bitplane < QCCTARP3D_MAXBITPLANES)
    {
      return_value =
        QccWAVTarp3DSignificancePass(&image_subband_pyramid,
                                     ((mask != NULL) ?
                                      &mask_subband_pyramid : NULL),
                                     state_array,
                                     alpha,
                                     threshold,
                                     model,
                                     buffer);
      if (return_value == 1)
	{
	  QccErrorAddMessage("(QccWAVTarp3DEncode): Error calling QccWAVTarpSignificancePass()");
	  goto Error;
	}
      else
	if (return_value == 2)
	  goto Finished;
      
      return_value =
        QccWAVTarp3DRefinementPass(&image_subband_pyramid,
                                   ((mask != NULL) ?
                                    &mask_subband_pyramid : NULL),
                                   state_array,
                                   threshold,
                                   model,
                                   buffer);
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccWAVTarp3DEncode): Error calling QccWAVTarp3DRefinementPass()");
            goto Error;
          }
        else
          if (return_value == 2)
            goto Finished;
      
      threshold /= 2.0;
      bitplane++;
    }
  
 Finished:
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  QccWAVSubbandPyramid3DFree(&mask_subband_pyramid);
  if (state_array != NULL)
    {
      for (frame = 0; frame < image->num_frames; frame++)
        {
          if (state_array[frame] != NULL)
            {
              for (row = 0; row < image->num_rows; row++)
                if (state_array[frame][row] != NULL)
                  QccFree(state_array[frame][row]);
              QccFree(state_array[frame]);
            }
        }
      QccFree(state_array);
    }
  QccENTArithmeticFreeModel(model);
  return(return_value);
}


static int QccWAVTarp3DDecodeInverseDWT(QccWAVSubbandPyramid3D
                                        *image_subband_pyramid,
                                        QccWAVSubbandPyramid3D
                                        *mask_subband_pyramid,
                                        unsigned char ***state_array,
                                        QccIMGImageCube *image,
                                        double image_mean,
                                        const QccWAVWavelet *wavelet)
{
  int frame, row, col;

  for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (QccWAVTarp3DGetSign(state_array[frame][row][col]))
          image_subband_pyramid->volume[frame][row][col] *= -1;
  
  if (mask_subband_pyramid != NULL)
    {
      if (QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT(image_subband_pyramid,
                                                        mask_subband_pyramid,
                                                        wavelet))
        {
          QccErrorAddMessage("(QccWAVTarp3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramid3DInverseDWT(image_subband_pyramid,
                                         wavelet))
      {
        QccErrorAddMessage("(QccWAVTarp3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DInverseDWT()");
        return(1);
      }
  
  if (QccWAVSubbandPyramid3DAddMean(image_subband_pyramid,
                                    image_mean))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DAddMean()");
      return(1);
    }
  
  if (mask_subband_pyramid != NULL)
    for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        for (col = 0; col < image_subband_pyramid->num_cols; col++)
          if (QccAlphaTransparent(mask_subband_pyramid->volume
                                  [frame][row][col]))
            image_subband_pyramid->volume[frame][row][col] = 0;
  
  if (QccVolumeCopy(image->volume,
                    image_subband_pyramid->volume,
                    image->num_frames,
                    image->num_rows,
                    image->num_cols))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeInverseDWT): Error calling QccVolumeCopy()");
      return(1);
    }
  
  if (QccIMGImageCubeSetMaxMin(image))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecodeInverseDWT): QccError calling QccIMGImageCubeSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccWAVTarp3DDecodeHeader(QccBitBuffer *buffer, 
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


int QccWAVTarp3DDecode(QccBitBuffer *buffer,
                       QccIMGImageCube *image,
                       const QccIMGImageCube *mask,
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
  QccWAVSubbandPyramid3D mask_subband_pyramid;
  unsigned char ***state_array = NULL;
  double threshold;
  int frame, row, col;
  int num_symbols[1] = { 2 };
  QccWAVWavelet lazy_wavelet_transform;
  
  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramid3DInitialize(&mask_subband_pyramid);
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
  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_frames != image->num_frames) ||
          (mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarp3DDecode): Mask and image cube must be same size");
          goto Error;
        }
      
      if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
        {
          QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccWAVWaveletCreate()");
          goto Error;
        }
      
      mask_subband_pyramid.temporal_num_levels = 0;
      mask_subband_pyramid.spatial_num_levels = 0;
      mask_subband_pyramid.num_frames = mask->num_frames;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramid3DAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
          goto Error;
        }
      
      if (QccVolumeCopy(mask_subband_pyramid.volume,
                        mask->volume,
                        mask->num_frames,
                        mask->num_rows,
                        mask->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccVolumeCopy()");
          goto Error;
        }
      
      if (QccWAVSubbandPyramid3DDWT(&mask_subband_pyramid,
                                    transform_type,
                                    temporal_num_levels,
                                    spatial_num_levels,
                                    &lazy_wavelet_transform))
        {
          QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccWAVSubbandPyramid3DDWT()");
          goto Error;
        }
    }
  
  if ((state_array =
       (unsigned char ***)malloc(sizeof(unsigned char **) *
                                 image->num_frames)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image->num_frames; frame++)
    {
      if ((state_array[frame] =
           (unsigned char **)malloc(sizeof(unsigned char *) *
                                    image->num_rows)) == NULL)
        {
          QccErrorAddMessage("(QccWAVTarp3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image->num_rows; row++)
        if ((state_array[frame][row] =
             (unsigned char *)malloc(sizeof(unsigned char) *
                                     image->num_cols)) == NULL)
          {
            QccErrorAddMessage("(QccWAVTarp3DDecode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < image->num_frames; frame++)
    for (row = 0; row < image->num_rows; row++)
      for (col = 0; col < image->num_cols; col++)
        {
          image_subband_pyramid.volume[frame][row][col] = 0.0;
          state_array[frame][row][col] = 0;
	}
  
  if ((model =
       QccENTArithmeticDecodeStart(buffer,
				   num_symbols,
				   1,
				   NULL,
                                   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);
  
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  while (1)
    {
      return_value = QccWAVTarp3DSignificancePass(&image_subband_pyramid,
                                                  ((mask != NULL) ?
                                                   &mask_subband_pyramid :
                                                   NULL),
                                                  state_array,
                                                  alpha,
                                                  threshold,
                                                  model,
                                                  buffer);
      if (return_value == 1)
	{
	  QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccWAVTarp3DSignificancePass()");
	  goto Error;
	}
      else
	if (return_value == 2)
	  goto Finished;
      
      return_value = QccWAVTarp3DRefinementPass(&image_subband_pyramid,
                                                ((mask != NULL) ?
                                                 &mask_subband_pyramid : NULL),
                                                state_array,
                                                threshold,
                                                model,
                                                buffer);
      if (return_value == 1)
	{
	  QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccWAVTarp3DRefinementPass()");
	  goto Error;
	}
      else
	if (return_value == 2)
	  goto Finished;
      
      threshold /= 2.0;
    }
  
 Finished:
  if (QccWAVTarp3DDecodeInverseDWT(&image_subband_pyramid,
                                   ((mask != NULL) ?
                                    &mask_subband_pyramid : NULL),
				   state_array,
                                   image,
				   image_mean,
				   wavelet))
    {
      QccErrorAddMessage("(QccWAVTarp3DDecode): Error calling QccWAVTarp3DDecodeInverseDWT()");
      goto Error;
    }
  
  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  QccWAVSubbandPyramid3DFree(&mask_subband_pyramid);
  if (state_array != NULL)
    {
      for (frame = 0; frame < image->num_frames; frame++)
        {
          if (state_array[frame] != NULL)
            {
              for (row = 0; row < image->num_rows; row++)
                if (state_array[frame][row] != NULL)
                  QccFree(state_array[frame][row]);
              QccFree(state_array[frame]);
            }
        }
      QccFree(state_array);
    }
  QccENTArithmeticFreeModel(model);
  return(return_value);
}
