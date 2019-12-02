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
 *  This code was written by Justin Rucker, based on
 *  the 2D WDR implementation by Yufei Yuan.
 */

#include "libQccPack.h"


#define QCCWAVWDR3D_ENCODE 0
#define QCCWAVWDR3D_DECODE 1
#define QCCWAVWDR3D_SYMBOLNUM 4
#define QCCWAVWDR3D_LONGSYMBOL 2

#define POSITIVE 2
#define NEGATIVE 3
#define ZERO 0
#define ONE 1

static const int QccWAVwdr3DArithmeticContexts[] = {4, 2};
#define QCCWAVWDR3D_RUN_CONTEXT 0
#define QCCWAVWDR3D_REFINEMENT_CONTEXT (QCCWAVWDR3D_RUN_CONTEXT + 1)
#define QCCWAVWDR3D_NUM_CONTEXTS (QCCWAVWDR3D_REFINEMENT_CONTEXT + 1)

#define QCCWAVWDR3D_SIGNIFICANCEMASK 0x03
#define QCCWAVWDR3D_SIGNMASK 0x04

#define QCCWAVWDR3D_MAXBITPLANES 128


static void QccWAVwdr3DPutSign(unsigned char *state, int sign_value)
{
  *state = sign_value;  
}


static int QccWAVwdr3DGetSign(unsigned char state)
{
  return(state);
}


static int QccWAVwdr3DInputOutput(QccBitBuffer *buffer,
                                  int *symbol,
                                  int method,
                                  QccENTArithmeticModel *model,
                                  int target_bit_cnt)
{
  int return_value;
  
  if (method == QCCWAVWDR3D_ENCODE)
    {
      if (model == NULL)
        {
          if (QccBitBufferPutBit(buffer, *symbol))
            {
              QccErrorAddMessage("(QccWAVwdr3DInputOutput): Error calling QccBitBufferPutBit()");
              goto Error;
            }
        }
      else
        {
          if (QccENTArithmeticEncode(symbol, 1, model, buffer))
            {
              QccErrorAddMessage("(QccWAVwdr3DInputOutput): Error calling QccENTArithmeticEncode()");
              goto Error;
            }
        }
      if (buffer->bit_cnt >= target_bit_cnt)
        {
          QccBitBufferFlush(buffer);
          return(2);
        }
    }
  else
    if (model == NULL)
      {
        if (QccBitBufferGetBit(buffer, symbol))
          {
            QccErrorAddMessage("(QccWAVwdr3DInputOutput): Error calling QccBitBufferGetBit()");
            return(2);
          }
      }
    else
      if (QccENTArithmeticDecode(buffer, model, symbol, 1))
        {
          QccErrorAddMessage("(QccWAVwdr3DInputOutput): Error calling QccENTArithmeticDecode()");
          return(2);
        }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);  
}


static int QccWAVwdr3DInputSymbol(QccBitBuffer *bit_buffer,
                                  unsigned int *run_length,
                                  int *sign, 
                                  QccENTArithmeticModel *model)
{
  int symbol, return_value;
  int target_bit_cnt = 0;
  
  *run_length = 1;
  if ((return_value = QccWAVwdr3DInputOutput(bit_buffer,
                                             &symbol,
                                             QCCWAVWDR3D_DECODE,
                                             model,
                                             target_bit_cnt)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdr3DInputSymbol): Error Calling QccWAVwdr3DInputOutput()");
          goto Error;
        }
    }
  
  while ((symbol != POSITIVE) && (symbol != NEGATIVE))
    {
      if (symbol == ZERO)
        *run_length <<= 1;
      else
        *run_length = (*run_length << 1) + 1;
      
      if ((return_value = QccWAVwdr3DInputOutput(bit_buffer,
                                                 &symbol,
                                                 QCCWAVWDR3D_DECODE,
                                                 model,
                                                 target_bit_cnt)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdr3DInputSymbol): Error Calling QccWAVwdr3DInputOutput()");
              goto Error;
            }
        }
    }
  
  if (symbol == POSITIVE)
    *sign = POSITIVE;
  else
    *sign = NEGATIVE;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


static int QccWAVwdr3DOutputSymbol(QccBitBuffer *bit_buffer,
                                   unsigned int run_length,
                                   int reduced_bits,
                                   int target_bit_cnt,
                                   QccENTArithmeticModel *model)
{
  int i, symbol, return_value;
  unsigned int mask;
  
  for (i = reduced_bits - 1; i >= 0; i--)
    {
      mask = 1 << i;
      if (run_length & mask)
        symbol = ONE;
      else
        symbol = ZERO;
      if ((return_value = QccWAVwdr3DInputOutput(bit_buffer,
                                                 &symbol,
                                                 QCCWAVWDR3D_ENCODE,
                                                 model,
                                                 target_bit_cnt)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdr3DOutputSymbol): Error calling QccWAVwdr3DBitBufferPutBits()");
              goto Error;
            }
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


static QccWAVwdr3DCoefficientBlock
*QccWAVwdr3DGetCoefficientBlockFromNode(QccListNode *node)
{
  return((QccWAVwdr3DCoefficientBlock *)(node->value));
}


int QccWAVwdr3DEncodeHeader(QccBitBuffer *output_buffer,
                            int transform_type,
                            int temporal_num_levels,
                            int spatial_num_levels,
                            int num_frames,
                            int num_rows,
                            int num_cols,
                            double image_mean,
                            int max_coefficient_bits)
{
  int return_value;
  
  if (QccBitBufferPutBit(output_buffer, transform_type))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncodeHeader): Error calling QccBitBufferPutBit()");
      goto Error;
    }
  
  if (transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      if (QccBitBufferPutChar(output_buffer, (unsigned char)temporal_num_levels))
        {
          QccErrorAddMessage("(QccWAVwdr3DEncodeHeader): Error calling QccBitBufferPuChar()");
          goto Error;
        }
    }
  
  if (QccBitBufferPutChar(output_buffer, (unsigned char)spatial_num_levels))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_frames))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(output_buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVwdr3DEncodeDWT(QccWAVSubbandPyramid3D *image_subband_pyramid,
                                unsigned char ***sign_array,
                                const QccIMGImageCube *image_cube,
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
                        image_cube->volume,
                        image_cube->num_frames,
                        image_cube->num_rows,
                        image_cube->num_cols))
        {
          QccErrorAddMessage("(QccWAVwdr3DEncodeDWT): Error calling QccVolumeCopy()");
          return(1);
        }
      
      if (QccWAVSubbandPyramid3DSubtractMean(image_subband_pyramid,
                                             image_mean,
                                             NULL))
        {
          QccErrorAddMessage("(QccWAVwdr3DEncodeDWT): Error calling QccWAVSubbandPyramid3DSubtractMean()");
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
          QccErrorAddMessage("(QccWAVwdr3DEncodeDWT): Error calling QccVolumeCopy()");
          return(1);
        }
      *image_mean = QccIMGImageCubeShapeAdaptiveMean(image_cube, mask);
      
      for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
        for (row = 0; row < image_subband_pyramid->num_rows; row++)
          for (col = 0; col < image_subband_pyramid->num_cols; col++)
            if (QccAlphaTransparent(mask_subband_pyramid->volume
                                    [frame][row][col]))
              image_subband_pyramid->volume[frame][row][col] = 0;
            else
              image_subband_pyramid->volume[frame][row][col] =
                image_cube->volume[frame][row][col] - *image_mean;
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
          QccErrorAddMessage("(QccWAVwdr3DEncodeDWT): Error calling QccWAVSubbandPyramid3DShapeAdaptiveDWT()");
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
        QccErrorAddMessage("(QccWAVwdr3DEncodeDWT): Error calling QccWAVSubbandPyramid3DDWT()");
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
              QccWAVwdr3DPutSign(&sign_array[frame][row][col],
                                 NEGATIVE);
            }
          else
            QccWAVwdr3DPutSign(&sign_array[frame][row][col],
                               POSITIVE);
          if (coefficient_magnitude > max_coefficient)
            max_coefficient = coefficient_magnitude;
        }
  
  *max_coefficient_bits = (int)floor(QccMathLog2(max_coefficient));
  
  return(0);
}


static int QccWAVwdr3DDecodeInverseDWT(QccWAVSubbandPyramid3D
                                       *image_subband_pyramid,
                                       QccWAVSubbandPyramid3D
                                       *mask_subband_pyramid,
                                       unsigned char ***sign_array,
                                       QccIMGImageCube *image_cube,
                                       double image_mean,
                                       const QccWAVWavelet *wavelet)
{
  int frame, row, col;
  
  for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (QccWAVwdr3DGetSign(sign_array[frame][row][col]) == NEGATIVE)
          image_subband_pyramid->volume[frame][row][col] *= -1;
  
  if (mask_subband_pyramid != NULL)
    {
      if (QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT(image_subband_pyramid,
                                                        mask_subband_pyramid,
                                                        wavelet))
        {
          QccErrorAddMessage("(QccWAVwdr3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramid3DInverseDWT(image_subband_pyramid,
                                         wavelet))
      {
        QccErrorAddMessage("(QccWAVwdr3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DInverseDWT()");
        return(1);
      }
  
  if (QccWAVSubbandPyramid3DAddMean(image_subband_pyramid,
                                    image_mean))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DAddMean()");
      return(1);
    }
  
  if (mask_subband_pyramid != NULL)
    for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        for (col = 0; col < image_subband_pyramid->num_cols; col++)
          if (QccAlphaTransparent(mask_subband_pyramid->volume
                                  [frame][row][col]))
            image_subband_pyramid->volume[frame][row][col] = 0;
  
  if (QccVolumeCopy(image_cube->volume,
                    image_subband_pyramid->volume,
                    image_cube->num_frames,
                    image_cube->num_rows,
                    image_cube->num_cols))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeInverseDWT): Error calling QccVolumeCopy()");
      return(1);
    }
  
  if (QccIMGImageCubeSetMaxMin(image_cube))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeInverseDWT): Error calling QccIMGImageCubeSetMaxMin()");
      return(1);
    }
  
  return(0);
}


static int QccWAVwdr3DAlgorithmInitialize(QccWAVSubbandPyramid3D *coefficients,
                                          const QccWAVSubbandPyramid3D *mask,
                                          QccList *ICS,
                                          unsigned int *virtual_end)
{
  int return_value;
  int subband_num_frames, subband_num_rows, subband_num_cols;
  int frame_offset, row_offset, col_offset;
  int frame, row, col, num_subbands, i;
  unsigned int current_index = 1;
  QccWAVwdr3DCoefficientBlock coefficient_block;
  QccListNode *new_node;
  int transparent;
  
  *virtual_end = 0;
  
  if (coefficients->transform_type == QCCWAVSUBBANDPYRAMID3D_DYADIC)
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(coefficients->spatial_num_levels);
  else
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                         coefficients->spatial_num_levels);
  
  for (i = 0; i < num_subbands; i++)
    {
      if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                            i,
                                            &subband_num_frames,
                                            &subband_num_rows,
                                            &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVwdr3DAlgorithmInitialize): Error calling QccWAVSubbandPyramid3DSubbandSize()");
          goto Error;
        }
      
      if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                               i,
                                               &frame_offset,
                                               &row_offset,
                                               &col_offset))
        {
          QccErrorAddMessage("(QccWAVwdr3DAlgorithmInitialize): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
        }
      
      for (frame = 0; frame < subband_num_frames; frame++)
        for (col = 0; col < subband_num_cols; col++)
          for (row = 0; row < subband_num_rows; row++)
            {
              if (mask == NULL)
                transparent = 0;
              else
                transparent =
                  QccAlphaTransparent(mask->volume
                                      [frame + frame_offset]
                                      [row + row_offset]
                                      [col + col_offset]);
              if (!transparent)
                {
                  (*virtual_end)++;
                  coefficient_block.frame = frame + frame_offset;
                  coefficient_block.row = row + row_offset;
                  coefficient_block.col = col + col_offset;
                  coefficient_block.index = current_index;
                  current_index++;
                  if ((new_node =
                       QccListCreateNode(sizeof(QccWAVwdr3DCoefficientBlock),
                                         (void *)(&coefficient_block))) ==
                      NULL)
                    {
                      QccErrorAddMessage("(QccWAVwdr3DAlgorithmInitialize): Error calling QccListCreateNode()");
                      goto Error;
                    }
                  
                  if (QccListAppendNode(ICS, new_node))
                    {
                      QccErrorAddMessage("(QccWAVwdr3DAlgorithmInitialize): Error calling QccListAppendNode()");
                      goto Error;
                    }
                }
            }
    }
  
  (*virtual_end)++;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVwdr3DInputOutputRefinementBit(QccWAVSubbandPyramid3D
                                               *subband_pyramid,
                                               QccBitBuffer *buffer, 
                                               double threshold,
                                               QccWAVwdr3DCoefficientBlock
                                               *coefficient,
                                               int method,
                                               QccENTArithmeticModel *model, 
                                               int target_bit_cnt) 
{
  int bit, return_value;
  
  if (method == QCCWAVWDR3D_ENCODE)
    {
      bit =
        (subband_pyramid->volume
         [coefficient->frame][coefficient->row][coefficient->col] >=
         threshold);
      
      if (bit)
        subband_pyramid->volume
          [coefficient->frame][coefficient->row][coefficient->col] -=
          threshold;
    }
  
  if (model != NULL)
    if (QccENTArithmeticSetModelContext(model, QCCWAVWDR3D_REFINEMENT_CONTEXT))
      {
        QccErrorAddMessage("(QccWAVwdr3DInputOutputRefinementBit): Error calling QccENTArithmeticSetModelContext()");
        goto Error;
      }
  
  if ((return_value = QccWAVwdr3DInputOutput(buffer,
                                             &bit,
                                             method,
                                             model,
                                             target_bit_cnt)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdr3DInputOutputRefinementBit): Error calling QccWAVwdr3DInputOutput()");
          goto Error;
        }
    }
  
  if (method == QCCWAVWDR3D_DECODE)
    subband_pyramid->volume
      [coefficient->frame][coefficient->row][coefficient->col] += 
      (bit) ? (threshold / 2) : (-threshold / 2);
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVwdr3DOutputRunLength(QccWAVSubbandPyramid3D *subband_pyramid,
                                      QccBitBuffer *buffer,
                                      QccWAVwdr3DCoefficientBlock
                                      *coefficient_block,
                                      double threshold,
                                      int target_bit_cnt,
                                      unsigned int *relative_origin,
                                      unsigned char *significant,
                                      QccENTArithmeticModel *model)
{
  int return_value;
  int frame, row, col, reduced_bits;
  unsigned int run_length, temp;
  
  frame = coefficient_block->frame;
  row = coefficient_block->row;
  col = coefficient_block->col;
  
  if (subband_pyramid->volume[frame][row][col] >= threshold)
    {
      *significant = TRUE;
      run_length = coefficient_block->index - *relative_origin;
      *relative_origin = coefficient_block->index;
      temp = run_length;
      reduced_bits = 0;
      
      while (temp != 0)
        {
          temp >>= 1;
          reduced_bits++;
        }
      
      reduced_bits--;
      
      if (model != NULL)
        if (QccENTArithmeticSetModelContext(model, QCCWAVWDR3D_RUN_CONTEXT))
          {
            QccErrorAddMessage("(QccWAVwdr3DOutputRunLength): Error calling QccENTArithmeticSetModelContext()");
            goto Error;
          }
      
      if ((return_value = QccWAVwdr3DOutputSymbol(buffer,
                                                  run_length,
                                                  reduced_bits,
                                                  target_bit_cnt,
                                                  model)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdr3DOutputRunLength): Error calling QccWAVwdr3DOutputSymbol()");
              goto Error;
            }
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


static int QccWAVwdr3DInputRunLength(QccWAVSubbandPyramid3D *subband_pyramid,
                                     QccBitBuffer *buffer,
                                     unsigned int *relative_origin,
                                     int *sign,
                                     QccENTArithmeticModel *model)
{
  int return_value;
  unsigned int run_length;
  
  if (model != NULL)
    if (QccENTArithmeticSetModelContext(model, QCCWAVWDR3D_RUN_CONTEXT))
      {
        QccErrorAddMessage("(QccWAVwdr3DInputRunLength): Error calling QccENTArithmeticSetModelContext()");
        goto Error;
      }
  
  if ((return_value =
       QccWAVwdr3DInputSymbol(buffer, &run_length, sign, model)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdr3DInputRunLength): Error calling QccWAVwdr3DInputSymbol()");
          goto Error;
        }
    }
  *relative_origin += run_length;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


static int QccWAVwdr3DAddNodeToTPS(QccList *TPS,
                                   int frame,
                                   int row,
                                   int col,
                                   QccWAVSubbandPyramid3D *subband_pyramid,
                                   double threshold,
                                   unsigned char ***sign_array,
                                   QccBitBuffer *buffer,
                                   int method,
                                   int target_bit_cnt,
                                   QccENTArithmeticModel *model)
{
  int return_value, symbol;
  QccWAVwdr3DCoefficientBlock coefficient_block;
  QccListNode *new_node = NULL;
  
  coefficient_block.frame = frame;
  coefficient_block.row = row;
  coefficient_block.col = col;
  coefficient_block.index = 0;
  
  if ((new_node = QccListCreateNode(sizeof(QccWAVwdr3DCoefficientBlock),
                                    (void *)(&coefficient_block))) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdr3DAddNodeToTPS): Error calling QccListCreateNode()");
      goto Error;
    }
  
  if (QccListAppendNode(TPS, new_node))
    {
      QccErrorAddMessage("(QccWAVwdr3DAddNodeToTPS): Error calling QccListAppendNode()");
      goto Error;
    }
  
  if (method == QCCWAVWDR3D_ENCODE)
    {
      symbol = QccWAVwdr3DGetSign(sign_array[frame][row][col]);
      
      if ((return_value = QccWAVwdr3DInputOutput(buffer,
                                                 &symbol,
                                                 QCCWAVWDR3D_ENCODE,
                                                 model,
                                                 target_bit_cnt)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdr3DAddNodeToTPS): Error calling QccWAVwdr3DInputOutput()");
              goto Error;
            }
        }
      
      subband_pyramid->volume[frame][row][col] -= threshold;
    }
  else
    subband_pyramid->volume[frame][row][col] = 1.5 * threshold;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


static int QccWAVwdr3DSortingPass(QccWAVSubbandPyramid3D *subband_pyramid,
                                  unsigned char ***sign_array,
                                  QccBitBuffer *buffer,
                                  double threshold,
                                  QccList *ICS,
                                  QccList *TPS,
                                  int method, 
                                  int target_bit_cnt,
                                  unsigned int *relative_origin,
                                  unsigned int virtual_end,
                                  QccENTArithmeticModel *model)
{
  int sign = 0;
  int return_value;
  unsigned char significant;
  QccWAVwdr3DCoefficientBlock *current_coefficient_block;
  QccListNode *current_list_node;
  QccListNode *next_list_node;
  
  current_list_node = ICS->start;
  
  if (method == QCCWAVWDR3D_ENCODE)
    while (current_list_node != NULL)
      {
        next_list_node = current_list_node->next;
        current_coefficient_block =
          QccWAVwdr3DGetCoefficientBlockFromNode(current_list_node);
        significant = FALSE;
        
        if ((return_value =
             QccWAVwdr3DOutputRunLength(subband_pyramid,
                                        buffer,
                                        current_coefficient_block,
                                        threshold,
                                        target_bit_cnt,
                                        relative_origin,
                                        &significant,
                                        model)))
          {
            if (return_value == 2)
              return(2);
            else
              {
                QccErrorAddMessage("(QccWAVwdr3DSortingPass): Error calling QccWAVwdr3DOutputRunLength()");
                goto Error;
              }
          }
        
        if (significant == TRUE)
          {
            if ((return_value =
                 QccWAVwdr3DAddNodeToTPS(TPS,
                                         current_coefficient_block->frame,
                                         current_coefficient_block->row,
                                         current_coefficient_block->col,
                                         subband_pyramid,
                                         threshold,
                                         sign_array,
                                         buffer,
                                         method,
                                         target_bit_cnt,
                                         model)))
              {
                if (return_value == 2)
                  return(2);
                else
                  {
                    QccErrorAddMessage("(QccWAVwdr3DSortingPass): Error calling QccWAVwdr3DAddNodeToTPS()");
                    goto Error;
                  }
              }
            
            if (QccListDeleteNode(ICS, current_list_node))
              {
                QccErrorAddMessage("(QccWAVwdr3DSortingPass): Error calling QccListDeleteNode()");
                goto Error;
              }
          }
        
        current_list_node = next_list_node;
      }
  else
    {
      while (*relative_origin != virtual_end)
        {
          if ((return_value = QccWAVwdr3DInputRunLength(subband_pyramid,
                                                        buffer,
                                                        relative_origin,
                                                        &sign,
                                                        model)))
            {
              if (return_value == 2)
                return(2);
              else
                {
                  QccErrorAddMessage("(QccWAVwdr3DSortingPass): Error calling QccWAVwdr3DInputRunLength()");
                  goto Error;
                }
            }
          
          if (*relative_origin == virtual_end)
            break;
          
          if (*relative_origin > virtual_end)
            {
              QccErrorAddMessage("(QccWAVwdr3DSortingPass): Synchronization Error In Decoding");
              goto Error;
            }
          
          current_coefficient_block =
            QccWAVwdr3DGetCoefficientBlockFromNode(current_list_node);
          
          while (current_coefficient_block->index != *relative_origin)
            {
              current_list_node = current_list_node->next;
              current_coefficient_block =
                QccWAVwdr3DGetCoefficientBlockFromNode(current_list_node);
            }
          
          if (QccWAVwdr3DAddNodeToTPS(TPS,
                                      current_coefficient_block->frame,
                                      current_coefficient_block->row,
                                      current_coefficient_block->col,
                                      subband_pyramid,
                                      threshold,
                                      sign_array,
                                      buffer,
                                      method,
                                      target_bit_cnt,
                                      model))
            {
              QccErrorAddMessage("(QccWAVwdr3DSortingPass): Error calling QccWAVwdr3DAddNodeToTPS()");
              goto Error;
            }
          
          if (sign == POSITIVE)
            QccWAVwdr3DPutSign(&sign_array[current_coefficient_block->frame]
                               [current_coefficient_block->row]
                               [current_coefficient_block->col],
                               POSITIVE);
          else
            QccWAVwdr3DPutSign(&sign_array[current_coefficient_block->frame]
                               [current_coefficient_block->row]
                               [current_coefficient_block->col],
                               NEGATIVE);
          
          next_list_node = current_list_node->next;
          if (QccListDeleteNode(ICS, current_list_node))
            {
              QccErrorAddMessage("(QccWAVwdr3DSortingPass): Error calling QccListDeleteNode()");
              goto Error;
            }
          current_list_node = next_list_node;
        }
      
      if (sign == NEGATIVE)
        {
          QccErrorAddMessage("(QccWAVwdr3DSortingPass): Synchronization Error In Decoding");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVwdr3DOutputVirtualEnd(QccBitBuffer *buffer,
                                       unsigned int relative_origin,
                                       unsigned int virtual_end,
                                       int target_bit_cnt, 
                                       QccENTArithmeticModel *model)
{
  unsigned int run_length, temp;
  int symbol = POSITIVE, reduced_bits, return_value;
  
  run_length = virtual_end - relative_origin;
  temp = run_length;
  reduced_bits = 0;

  while (temp != 0)
    {
      temp >>= 1;
      reduced_bits++;
    }
  reduced_bits--;
  
  if (model != NULL)
    if (QccENTArithmeticSetModelContext(model, QCCWAVWDR3D_RUN_CONTEXT))
      {
        QccErrorAddMessage("(QccWAVwdr3DOutputVirtualEnd): Error calling QccENTArithmeticSetModelContext()");
        goto Error;
      }
  
  if ((return_value = QccWAVwdr3DOutputSymbol(buffer,
                                              run_length,
                                              reduced_bits,
                                              target_bit_cnt,
                                              model)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdr3DOutputVirtualEnd): Error calling QccWAVwdr3DOutputSymbol()");
          goto Error;
        }
    }
  
  if ((return_value = QccWAVwdr3DInputOutput(buffer,
                                             &symbol,
                                             QCCWAVWDR3D_ENCODE,
                                             model,
                                             target_bit_cnt)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdr3DOutputVirtualEnd): Error calling QccWAVwdr3DInputOutput()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


static int QccWAVwdr3DUpdateIndex(QccList *ICS)
{
  QccListNode *current_node, *stop;
  QccWAVwdr3DCoefficientBlock *current_coefficient;
  unsigned int adjust_index = 1;
  
  current_node = ICS->start;
  stop = ICS->end;
  if ((current_node == NULL) || (stop == NULL))
    return(0);
  stop = stop->next;
  
  while (current_node != stop)
    {
      current_coefficient =
        QccWAVwdr3DGetCoefficientBlockFromNode(current_node);
      current_coefficient->index = adjust_index;
      adjust_index++;
      current_node = current_node->next;
    }
  
  return(0);
}


static int QccWAVwdr3DRefinementPass(QccWAVSubbandPyramid3D *subband_pyramid,
                                     QccBitBuffer *buffer,
                                     double threshold, 
                                     QccList *SCS,
                                     QccListNode *stop,
                                     int method,
                                     int target_bit_cnt,
                                     QccENTArithmeticModel *model)
{
  QccListNode *current_node;
  QccWAVwdr3DCoefficientBlock *current_coefficient;
  int return_value;
  
  current_node = SCS->start;
  if ((current_node == NULL) || (stop == NULL))
    return(0);
  stop = stop->next;
  
  while (current_node != stop)
    {
      current_coefficient = QccWAVwdr3DGetCoefficientBlockFromNode(current_node);
      
      if ((return_value =
           QccWAVwdr3DInputOutputRefinementBit(subband_pyramid,
                                               buffer,
                                               threshold,
                                               current_coefficient,
                                               method,
                                               model,
                                               target_bit_cnt)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdr3DRefinementPass): Error calling QccWAVwdr3DInputOutputRefinementBit()");
              goto Error;
            }
        }
      
      current_node = current_node->next;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


int QccWAVwdr3DEncode(QccIMGImageCube *image_cube,
                      QccIMGImageCube *mask,
                      int transform_type,
                      int temporal_num_levels,
                      int spatial_num_levels,
                      const QccWAVWavelet *wavelet,
                      QccBitBuffer *output_buffer,
                      int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid3D image_subband_pyramid;
  QccWAVSubbandPyramid3D mask_subband_pyramid;
  unsigned char ***sign_array = NULL;
  double image_mean=0;
  int max_coefficient_bits;
  double threshold;
  int frame;
  int row, col;
  
  QccList ICS;
  QccList SCS;
  QccList TPS;
  QccListNode *stop;
  unsigned int relative_origin;
  unsigned int virtual_end;
  int bitplane = 0;
  
  if (image_cube == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccListInitialize(&ICS);
  QccListInitialize(&SCS);
  QccListInitialize(&TPS);
  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramid3DInitialize(&mask_subband_pyramid);
  
  image_subband_pyramid.temporal_num_levels = 0;
  image_subband_pyramid.spatial_num_levels = 0;
  image_subband_pyramid.num_frames = image_cube->num_frames;
  image_subband_pyramid.num_rows = image_cube->num_rows;
  image_subband_pyramid.num_cols = image_cube->num_cols;
  image_subband_pyramid.transform_type = transform_type;
  
  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    } 
  
  if (mask != NULL)
    {
      if ((mask->num_frames != image_cube->num_frames) ||
          (mask->num_rows != image_cube->num_rows) ||
          (mask->num_cols != image_cube->num_cols))
        {
          QccErrorAddMessage("(QccWAVwdr3DEncode): Mask and image must be same size");
          goto Error;
        }
      
      mask_subband_pyramid.temporal_num_levels = 0;
      mask_subband_pyramid.spatial_num_levels = 0;
      mask_subband_pyramid.num_frames = mask->num_frames;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      
      if (QccWAVSubbandPyramid3DAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccWAVSubbandPyramid3DAlloc()");
          goto Error;
        }
    }
  
  if ((sign_array =
       (unsigned char ***)malloc(sizeof(unsigned char **) *
                                 (image_cube->num_frames))) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdr3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image_cube->num_frames; frame++)
    {
      if ((sign_array[frame] =
           (unsigned char **)malloc(sizeof(unsigned char *) *
                                    (image_cube->num_rows))) == NULL)
        {
          QccErrorAddMessage("(QccWAVwdr3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image_cube->num_rows; row++)
        if ((sign_array[frame][row] =
             (unsigned char *)malloc(sizeof(unsigned char) *
                                     (image_cube->num_cols))) == NULL)
          {
            QccErrorAddMessage("(QccWAVwdr3DEncode): Error allocating memory");
            goto Error;
          }
    } 
  
  for (frame = 0; frame < (image_cube->num_frames); frame++)
    for (row = 0; row < (image_cube->num_rows); row++)
      for (col = 0; col < (image_cube->num_cols); col++)
        sign_array[frame][row][col] = ZERO;
  
  if (QccWAVwdr3DEncodeDWT(&image_subband_pyramid,
                           sign_array,
                           image_cube,
                           transform_type,
                           temporal_num_levels,
                           spatial_num_levels,
                           &image_mean,
                           &max_coefficient_bits,
                           ((mask != NULL) ? &mask_subband_pyramid : NULL),
                           mask,
                           wavelet))                          
    {
      QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccWAVwdr3DEncodeDWT()");
      goto Error;
    }
  
  
  if (QccWAVwdr3DEncodeHeader(output_buffer,
                              transform_type,
                              temporal_num_levels,
                              spatial_num_levels,
                              image_cube->num_frames,
                              image_cube->num_rows,
                              image_cube->num_cols,
                              image_mean,
                              max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccWAVwdr3DEncodeHeader()");
      goto Error;
    }
  
  if ((model = QccENTArithmeticEncodeStart(QccWAVwdr3DArithmeticContexts,
                                           QCCWAVWDR3D_NUM_CONTEXTS,
                                           NULL,
                                           QCCENT_ANYNUMBITS)) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  
  if (QccWAVwdr3DAlgorithmInitialize(&image_subband_pyramid,
                                     ((mask != NULL) ?
                                      &mask_subband_pyramid : NULL),
                                     &ICS,
                                     &virtual_end))
    {
      QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccWAVwdr3DAlgorithmInitialize()");
      goto Error;
    }
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  while (bitplane < QCCWAVWDR3D_MAXBITPLANES)
    {
      stop = SCS.end; 
      relative_origin = 0;
      return_value =
        QccWAVwdr3DSortingPass(&image_subband_pyramid,
                               sign_array,
                               output_buffer,
                               threshold,
                               &ICS,
                               &TPS,
                               QCCWAVWDR3D_ENCODE,
                               target_bit_cnt,
                               &relative_origin,
                               virtual_end,
                               model);
      
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccWAVwdr3DSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      return_value = QccWAVwdr3DOutputVirtualEnd(output_buffer,
                                                 relative_origin,
                                                 virtual_end,
                                                 target_bit_cnt,
                                                 model);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccWAVwdr3DOutputVirtualEnd()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      QccWAVwdr3DUpdateIndex(&ICS);
      return_value =
        QccWAVwdr3DRefinementPass(&image_subband_pyramid,
                                  output_buffer,
                                  threshold,
                                  &SCS,
                                  stop,
                                  QCCWAVWDR3D_ENCODE,
                                  target_bit_cnt,
                                  model);
      
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccWAVwdr3DRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      if (QccListConcatenate(&SCS, &TPS))
        {
          QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccListConcatenate()");
          return(1);
        }
      
      threshold /= 2.0;
      bitplane++;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  QccWAVSubbandPyramid3DFree(&mask_subband_pyramid);
  if (sign_array != NULL)
    {
      for (frame = 0; frame < image_cube->num_frames; frame++)
        if (sign_array[frame] != NULL)
          {
            for (row = 0; row < image_cube->num_rows; row++)
              if (sign_array[frame][row] != NULL)
                free(sign_array[frame][row]);
            free(sign_array[frame]);
          }
      free(sign_array);
    }
  QccListFree(&ICS);
  QccListFree(&SCS);
  QccListFree(&TPS);
  QccENTArithmeticFreeModel(model);    
  return(return_value);
}


int QccWAVwdr3DDecodeHeader(QccBitBuffer *input_buffer,
                            int *transform_type,
                            int *temporal_num_levels,
                            int *spatial_num_levels,
                            int *num_frames,
                            int *num_rows,
                            int *num_cols,
                            double *image_mean,
                            int *max_coefficient_bits)
{
  int return_value;
  unsigned char ch;
  
  if (QccBitBufferGetBit(input_buffer, transform_type))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeHeader): Error calling QccBitBufferGetBit()");
      goto Error;
    }
  
  if (*transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      if (QccBitBufferGetChar(input_buffer, &ch))
        {
          QccErrorAddMessage("(QccWAVwdr3DDecodeHeader): Error calling QccBitBufferGetChar()");
          goto Error;
        }
      *temporal_num_levels = (int)ch;
    }
  
  if (QccBitBufferGetChar(input_buffer, &ch))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  *spatial_num_levels = (int)ch;
  
  if (*transform_type == QCCWAVSUBBANDPYRAMID3D_DYADIC)
    *temporal_num_levels = *spatial_num_levels;
  
  if (QccBitBufferGetInt(input_buffer, num_frames))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(input_buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVwdr3DDecode(QccBitBuffer *input_buffer,
                      QccIMGImageCube *image_cube,
                      QccIMGImageCube *mask,
                      int transform_type,
                      int temporal_num_levels,
                      int spatial_num_levels,
                      const QccWAVWavelet *wavelet,
                      double image_mean,
                      int max_coefficient_bits,
                      int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid3D image_subband_pyramid;
  QccWAVSubbandPyramid3D mask_subband_pyramid;
  unsigned char ***sign_array = NULL;
  double threshold;
  int frame;
  int row, col;
  QccWAVWavelet lazy_wavelet_transform;
  QccList ICS;
  QccList SCS;
  QccList TPS;
  QccListNode *stop;
  unsigned int relative_origin;
  unsigned int virtual_end;
  
  if (image_cube == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramid3DInitialize(&mask_subband_pyramid);
  QccListInitialize(&ICS);
  QccListInitialize(&SCS);
  QccListInitialize(&TPS);
  QccWAVWaveletInitialize(&lazy_wavelet_transform);
  
  image_subband_pyramid.transform_type = transform_type;
  image_subband_pyramid.temporal_num_levels = temporal_num_levels;
  image_subband_pyramid.spatial_num_levels = spatial_num_levels;
  image_subband_pyramid.num_frames = image_cube->num_frames;
  image_subband_pyramid.num_rows = image_cube->num_rows;
  image_subband_pyramid.num_cols = image_cube->num_cols;
  
  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_frames != image_cube->num_frames) ||
          (mask->num_rows != image_cube->num_rows) ||
          (mask->num_cols != image_cube->num_cols))
        {
          QccErrorAddMessage("(QccWAVwdr3DDecode): Mask and image cube must be same size");
          goto Error;
        }
      
      if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
        {
          QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccWAVWaveletCreate()");
          goto Error;
        }
      
      mask_subband_pyramid.temporal_num_levels = 0;
      mask_subband_pyramid.spatial_num_levels = 0;
      mask_subband_pyramid.num_frames = mask->num_frames;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramid3DAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
          goto Error;
        }
      
      if (QccVolumeCopy(mask_subband_pyramid.volume,
                        mask->volume,
                        mask->num_frames,
                        mask->num_rows,
                        mask->num_cols))
        {
          QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccVolumeCopy()");
          goto Error;
        }
      
      if (QccWAVSubbandPyramid3DDWT(&mask_subband_pyramid,
                                    transform_type,
                                    temporal_num_levels,
                                    spatial_num_levels,
                                    &lazy_wavelet_transform))
        {
          QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccWAVSubbandPyramid3DDWT()");
          goto Error;
        }
    }
  
  if ((sign_array =
       (unsigned char ***)malloc(sizeof(unsigned char **) *
                                 (image_cube->num_frames))) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdr3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < (image_cube->num_frames); frame++)
    {
      if ((sign_array[frame] =
           (unsigned char **)malloc(sizeof(unsigned char *) *
                                    (image_cube->num_rows))) == NULL)
        {
          QccErrorAddMessage("(QccWAVwdr3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < (image_cube->num_rows); row++)
        if ((sign_array[frame][row] =
             (unsigned char *)malloc(sizeof(unsigned char) *
                                     (image_cube->num_cols))) == NULL)
          {
            QccErrorAddMessage("(QccWAVwdr3DDecode): Error allocating memory");
            goto Error;
          }
    }
  for (frame = 0; frame < image_subband_pyramid.num_frames; frame++)
    for (row = 0; row < image_subband_pyramid.num_rows; row++)
      for (col = 0; col < image_subband_pyramid.num_cols; col++)
        {
          image_subband_pyramid.volume[frame][row][col] = 0.0;
          sign_array[frame][row][col] = 0;
        }
  
  if ((model = QccENTArithmeticDecodeStart(input_buffer,
                                           QccWAVwdr3DArithmeticContexts,
                                           QCCWAVWDR3D_NUM_CONTEXTS,
                                           NULL,
                                           target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  
  if (QccWAVwdr3DAlgorithmInitialize(&image_subband_pyramid,
                                     ((mask != NULL) ?
                                      &mask_subband_pyramid : NULL),
                                     &ICS,
                                     &virtual_end))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccWAVwdr3DAlgorithmInitialize()");
      goto Error;
    }
  
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  while (1)
    {
      stop = SCS.end;
      relative_origin = 0;
      return_value =
        QccWAVwdr3DSortingPass(&image_subband_pyramid,
                               sign_array,
                               input_buffer,
                               threshold,
                               &ICS,
                               &TPS,
                               QCCWAVWDR3D_DECODE,
                               0,
                               &relative_origin,
                               virtual_end,
                               model);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccWAVwdr3DSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      QccWAVwdr3DUpdateIndex(&ICS);
      return_value =
        QccWAVwdr3DRefinementPass(&image_subband_pyramid,
                                  input_buffer,
                                  threshold,
                                  &SCS,
                                  stop,
                                  QCCWAVWDR3D_DECODE,
                                  0,
                                  model);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccWAVwdr3DRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      if (QccListConcatenate(&SCS, &TPS))
        {
          QccErrorAddMessage("(QccWAVwdr3DEncode): Error calling QccListConcatenate()");
          return(1);
        }
      
      threshold /= 2.0;
    }
  
  if (QccWAVwdr3DDecodeInverseDWT(&image_subband_pyramid,
                                  ((mask != NULL) ?
                                   &mask_subband_pyramid : NULL),
                                  sign_array,
                                  image_cube,
                                  image_mean,
                                  wavelet))
    {
      QccErrorAddMessage("(QccWAVwdr3DDecode): Error calling QccWAVDecodeInverseDWT()");
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
  if (sign_array != NULL)
    {
      for (frame = 0; frame < (image_cube->num_frames); frame++)
        if (sign_array[frame] != NULL)
          {
            for (row = 0; row < (image_cube->num_rows); row++)
              if (sign_array[frame][row] != NULL)
                free(sign_array[frame][row]);
            free(sign_array[frame]);
          }
      free(sign_array);
    }
  QccENTArithmeticFreeModel(model);
  QccListFree(&ICS);
  QccListFree(&SCS);
  QccListFree(&TPS);
  QccWAVWaveletFree(&lazy_wavelet_transform);
  return(return_value);  
}
