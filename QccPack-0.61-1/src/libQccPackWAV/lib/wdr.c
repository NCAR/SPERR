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
 *  This code was written by Yufei Yuan <yuanyufei@hotmail.com>
 */

#include "libQccPack.h"


#define QCCWAVWDR_ENCODE 0
#define QCCWAVWDR_DECODE 1
#define QCCWAVWDR_SYMBOLNUM 4
#define QCCWAVWDR_LONGSYMBOL 2

#define POSITIVE 2
#define NEGATIVE 3
#define ZERO 0
#define ONE 1

static const int QccWAVwdrArithmeticContexts[] = {4, 2};
#define QCCWAVWDR_RUN_CONTEXT 0
#define QCCWAVWDR_REFINEMENT_CONTEXT (QCCWAVWDR_RUN_CONTEXT + 1)
#define QCCWAVWDR_NUM_CONTEXTS (QCCWAVWDR_REFINEMENT_CONTEXT + 1)

#define QCCWAVWDR_MAXBITPLANES 128


static int QccWAVwdrInputOutput(QccBitBuffer *buffer,
                                int *symbol,
                                int method,
                                QccENTArithmeticModel *model,
                                int target_bit_cnt)
{
  int return_value;
  
  if (method == QCCWAVWDR_ENCODE)
    {
      if (model == NULL)
        {
          if (QccBitBufferPutBit(buffer, *symbol))
            {
              QccErrorAddMessage("(QccWAVwdrInputOutput): Error calling QccBitBufferPutBit()");
              goto Error;
            }
        }
      else
        {
          if (QccENTArithmeticEncode(symbol, 1, model, buffer))
            {
              QccErrorAddMessage("(QccWAVwdrInputOutput): Error calling QccENTArithmeticEncode()");
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
            QccErrorAddMessage("(QccWAVwdrInputOutput): Error calling QccBitBufferGetBit()");
            return(2);
          }
      }
    else
      if (QccENTArithmeticDecode(buffer, model, symbol, 1))
        {
          QccErrorAddMessage("(QccWAVwdrInputOutput): Error calling QccENTArithmeticDecode()");
          return(2);
        }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);  
}


static int QccWAVwdrInputSymbol(QccBitBuffer *bit_buffer,
                                unsigned int *run_length,
                                int *sign, 
                                QccENTArithmeticModel *model)
{
  int symbol, return_value;
  int target_bit_cnt = 0;
  
  *run_length = 1;
  if ((return_value = QccWAVwdrInputOutput(bit_buffer,
                                           &symbol,
                                           QCCWAVWDR_DECODE,
                                           model,
                                           target_bit_cnt)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdrInputSymbol): Error Calling QccWAVwdrInputOutput()");
          goto Error;
        }
    }
  
  while ((symbol != POSITIVE) && (symbol != NEGATIVE))
    {
      if (symbol == ZERO)
        *run_length <<= 1;
      else
        *run_length = (*run_length << 1) + 1;
      
      if ((return_value = QccWAVwdrInputOutput(bit_buffer,
                                               &symbol,
                                               QCCWAVWDR_DECODE,
                                               model,
                                               target_bit_cnt)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdrInputSymbol): Error Calling QccWAVwdrInputOutput()");
              goto Error;
            }
        }
    }	
  
  if (symbol == POSITIVE)
    *sign = 0;
  else
    *sign = 1;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


static int QccWAVwdrOutputSymbol(QccBitBuffer *bit_buffer,
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
      if ((return_value = QccWAVwdrInputOutput(bit_buffer,
                                               &symbol,
                                               QCCWAVWDR_ENCODE,
                                               model,
                                               target_bit_cnt)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdrOutputSymbol): Error calling QccWAVwdrBitBufferPutBits()");
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


static QccWAVwdrCoefficientBlock
*QccWAVwdrGetCoefficientBlockFromNode(QccListNode *node)
{
  return((QccWAVwdrCoefficientBlock *)(node->value));
}


int QccWAVwdrEncodeHeader(QccBitBuffer *buffer,
                          int num_levels,
                          int num_rows,
                          int num_cols,
                          double image_mean,
                          int max_coefficient_bits)
{
  int return_value;
  
  if (QccBitBufferPutChar(buffer, (unsigned char)num_levels))
    {
      QccErrorAddMessage("(QccWAVwdrEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVwdrEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVwdrEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVwdrEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVwdrEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVwdrEncodeDWT(QccWAVSubbandPyramid *image_subband_pyramid,
                              int **sign_array,
                              const QccIMGImageComponent *image,
                              int num_levels, double *image_mean,
                              int *max_coefficient_bits,
                              QccWAVSubbandPyramid *mask_subband_pyramid,
                              const QccIMGImageComponent *mask,
                              const QccWAVWavelet *wavelet,
                              const QccWAVPerceptualWeights
                              *perceptual_weights)
{
  double max_coefficient = -MAXFLOAT;
  int row, col, return_value;
  
  if (mask == NULL)
    {
      if (QccMatrixCopy(image_subband_pyramid->matrix, image->image,
                        image->num_rows, image->num_cols))
        {
          QccErrorAddMessage("(QccWAVwdrEncodeDWT): Error calling QccMatrixCopy()");
          return(1);
        }
      
      if (QccWAVSubbandPyramidSubtractMean(image_subband_pyramid,
                                           image_mean,
                                           NULL))
        {
          QccErrorAddMessage("(QccWAVwdrEncodeDWT): Error calling QccWAVSubbandPyramidSubtractMean()");
          return(1);
        }
    }
  else
    {
      if (QccMatrixCopy(mask_subband_pyramid->matrix, mask->image,
                        mask->num_rows, mask->num_cols))
        {
          QccErrorAddMessage("(QccWAVwdrEncodeDWT): Error calling QccMatrixCopy()");
          return(1);
        }

      *image_mean = QccIMGImageComponentShapeAdaptiveMean(image, mask);

      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        for (col = 0; col < image_subband_pyramid->num_cols; col++)
          if (QccAlphaTransparent(mask_subband_pyramid->matrix[row][col]))
            image_subband_pyramid->matrix[row][col] = 0;
          else
            image_subband_pyramid->matrix[row][col] =
              image->image[row][col] - *image_mean;
    }
  
  if (mask != NULL)
    {
      if (QccWAVSubbandPyramidShapeAdaptiveDWT(image_subband_pyramid,
                                               mask_subband_pyramid,
                                               num_levels,
                                               wavelet))
        {
          QccErrorAddMessage("(QccWAVwdrEncodeDWT): Error calling QccWAVSubbandPyramidShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramidDWT(image_subband_pyramid,
                                num_levels,
                                wavelet))
      {
        QccErrorAddMessage("(QccWAVwdrEncodeDWT): Error calling QccWAVSubbandPyramidDWT()");
        return(1);
      }
  
  if (perceptual_weights != NULL)
    {
      if (QccWAVPerceptualWeightsApply(image_subband_pyramid,
                                       perceptual_weights))
        {
          QccErrorAddMessage("(QccWAVwdrEncodeDWT): Error calling QccWAVPerceptualWeightsApply()");
          goto Error;
        }
    }		
  
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    {
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        {
          if (image_subband_pyramid->matrix[row][col] >= 0)
            sign_array[row][col] = 0;
          else
            sign_array[row][col] = 1;
          image_subband_pyramid->matrix[row][col] =
            fabs(image_subband_pyramid->matrix[row][col]);
          if (image_subband_pyramid->matrix[row][col] > max_coefficient)
            max_coefficient = image_subband_pyramid->matrix[row][col];
        }
    }
  
  *max_coefficient_bits = (int)floor(QccMathLog2(max_coefficient));
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVwdrDecodeInverseDWT(QccWAVSubbandPyramid
                                     *image_subband_pyramid,
                                     QccWAVSubbandPyramid
                                     *mask_subband_pyramid,
                                     int **sign_array,
                                     QccIMGImageComponent *image,
                                     double image_mean,
                                     const QccWAVWavelet *wavelet, 
                                     const QccWAVPerceptualWeights
                                     *perceptual_weights) 
{
  int row, col;
  int return_value;
  
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    for (col = 0; col < image_subband_pyramid->num_cols; col++)
      if (sign_array[row][col])
        image_subband_pyramid->matrix[row][col] *= -1;
  
  if (perceptual_weights != NULL)
    if (QccWAVPerceptualWeightsRemove(image_subband_pyramid,
                                      perceptual_weights))
      {
        QccErrorAddMessage("(QccWAVwdrDecodeInverseDWT): Error calling QccWAVPerceptualWeightsRemove()");
        goto Error;
      }
  
  if (mask_subband_pyramid != NULL)
    {
      if (QccWAVSubbandPyramidInverseShapeAdaptiveDWT(image_subband_pyramid,
                                                      mask_subband_pyramid,
                                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVwdrDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseShapeAdaptiveDWT()");
          goto Error;
        }
    }
  else
    if (QccWAVSubbandPyramidInverseDWT(image_subband_pyramid, wavelet))
      {
        QccErrorAddMessage("(QccWAVwdrDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseDWT()");
        goto Error;
      }
  
  if (QccWAVSubbandPyramidAddMean(image_subband_pyramid, image_mean))
    {
      QccErrorAddMessage("(QccWAVwdrDecodeInverseDWT): Error calling QccWAVSubbandPyramidAddMean()");
      goto Error;
    }
  
  if (mask_subband_pyramid != NULL)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (QccAlphaTransparent(mask_subband_pyramid->matrix[row][col]))
          image_subband_pyramid->matrix[row][col] = 0;
  
  if (QccMatrixCopy(image->image, image_subband_pyramid->matrix,
                    image->num_rows, image->num_cols))
    {
      QccErrorAddMessage("(QccWAVwdrDecodeInverseDWT): Error calling QccMatrixCopy()");
      goto Error;
    }
  
  if (QccIMGImageComponentSetMaxMin(image))
    {
      QccErrorAddMessage("(QccWAVwdrDecodeInverseDWT): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVwdrAlgorithmInitialize(QccWAVSubbandPyramid *subband_pyramid,
                                        const QccWAVSubbandPyramid
                                        *mask_subband_pyramid,
                                        int num_levels,
                                        QccList *ICS,
                                        unsigned int *virtual_end)
{
  int return_value;
  int subband_num_rows, subband_num_cols;
  int row_offset, col_offset;
  int row, col, num_subbands, i;
  unsigned int current_index = 1;
  QccWAVwdrCoefficientBlock coefficient_block;
  QccListNode *new_node;
  int transparent;

  *virtual_end = 0;

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);
  for (i = 0; i < num_subbands; i++)
    {
      if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                          i,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVwdrAlgorithmInitialize): Error calling QccWAVSubbandPyramidSubbandSize()");
          goto Error;
        }
      
      if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                             i,
                                             &row_offset,
                                             &col_offset))
        {
          QccErrorAddMessage("(QccWAVwdrAlgorithmInitialize): Error calling QccWAVSubbandPyramidSubbandOffsets()");
        }
      
      if ((i % 3) == 2)
        for (col = 0; col < subband_num_cols; col++)
          for (row = 0; row < subband_num_rows; row++)
            {
              if (mask_subband_pyramid == NULL)
                transparent = 0;
              else
                transparent =
                  QccAlphaTransparent(mask_subband_pyramid->matrix
                                      [row + row_offset][col + col_offset]);
              if (!transparent)
                {
                  (*virtual_end)++;
                  coefficient_block.row = row + row_offset;
                  coefficient_block.col = col + col_offset;
                  coefficient_block.index = current_index;
                  current_index++;
                  if ((new_node =
                       QccListCreateNode(sizeof(QccWAVwdrCoefficientBlock),
                                         (void *)(&coefficient_block))) == NULL)
                    {
                      QccErrorAddMessage("(QccWAVwdrAlgorithmInitialize): Error calling QccListCreateNode()");
                      goto Error;
                    }
                  
                  if (QccListAppendNode(ICS, new_node))
                    {
                      QccErrorAddMessage("(QccWAVwdrAlgorithmInitialize): Error calling QccListAppendNode()");
                      goto Error;
                    }
                }
            }
      else
        for (row = 0; row < subband_num_rows; row++)
          for (col = 0; col < subband_num_cols; col++)
            {
              if (mask_subband_pyramid == NULL)
                transparent = 0;
              else
                transparent =
                  QccAlphaTransparent(mask_subband_pyramid->matrix
                                      [row + row_offset][col + col_offset]);
              if (!transparent)
                {
                  (*virtual_end)++;
                  coefficient_block.row = row + row_offset;
                  coefficient_block.col = col + col_offset;
                  coefficient_block.index = current_index;
                  current_index++;
                  if ((new_node =
                       QccListCreateNode(sizeof(QccWAVwdrCoefficientBlock),
                                         (void *)(&coefficient_block))) ==
                      NULL)
                    {
                      QccErrorAddMessage("(QccWAVwdrAlgorithmInitialize): Error calling QccListCreateNode()");
                      goto Error;
                    }
                  
                  if (QccListAppendNode(ICS, new_node))
                    {
                      QccErrorAddMessage("(QccWAVwdrAlgorithmInitialize): Error calling QccListAppendNode()");
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


static int QccWAVwdrInputOutputRefinementBit(QccWAVSubbandPyramid
                                             *subband_pyramid,
                                             QccBitBuffer *buffer, 
                                             double threshold,
                                             QccWAVwdrCoefficientBlock
                                             *coefficient,
                                             int method,
                                             QccENTArithmeticModel *model, 
                                             int target_bit_cnt) 
{
  int bit, return_value;
  
  if (method == QCCWAVWDR_ENCODE)
    {
      bit =
        (subband_pyramid->matrix[coefficient->row][coefficient->col] >=
         threshold);
      
      if (bit)
        subband_pyramid->matrix[coefficient->row][coefficient->col] -=
          threshold;
    }
  
  if (model != NULL)
    if (QccENTArithmeticSetModelContext(model, QCCWAVWDR_REFINEMENT_CONTEXT))
      {
        QccErrorAddMessage("(QccWAVwdrInputOutputRefinementBit): Error calling QccENTArithmeticSetModelContext()");
        goto Error;
      }

  if ((return_value = QccWAVwdrInputOutput(buffer,
                                           &bit,
                                           method,
                                           model,
                                           target_bit_cnt)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdrInputOutputRefinementBit): Error calling QccWAVwdrInputOutput()");
          goto Error;
        }
    }
  
  if (method == QCCWAVWDR_DECODE)
    subband_pyramid->matrix[coefficient->row][coefficient->col] += 
      (bit) ? (threshold / 2) : (-threshold / 2);
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVwdrOutputRunLength(QccWAVSubbandPyramid *subband_pyramid,
                                    QccBitBuffer *buffer,
                                    QccWAVwdrCoefficientBlock
                                    *coefficient_block,
                                    double threshold,
                                    int target_bit_cnt,
                                    unsigned int *relative_origin,
                                    unsigned char *significant,
                                    QccENTArithmeticModel *model)
{
  int return_value;
  int row, col, reduced_bits;
  unsigned int run_length, temp;
  
  row = coefficient_block->row;
  col = coefficient_block->col;
  
  if (subband_pyramid->matrix[row][col] >= threshold)
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
        if (QccENTArithmeticSetModelContext(model, QCCWAVWDR_RUN_CONTEXT))
          {
            QccErrorAddMessage("(QccWAVwdrOutputRunLength): Error calling QccENTArithmeticSetModelContext()");
            goto Error;
          }
      
      if ((return_value = QccWAVwdrOutputSymbol(buffer,
                                                run_length,
                                                reduced_bits,
                                                target_bit_cnt,
                                                model)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdrOutputRunLength): Error calling QccWAVwdrOutputSymbol()");
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


static int QccWAVwdrInputRunLength(QccWAVSubbandPyramid *subband_pyramid,
                                   QccBitBuffer *buffer,
                                   unsigned int *relative_origin,
                                   int *sign,
                                   QccENTArithmeticModel *model)
{
  int return_value;
  unsigned int run_length;
  
  if (model != NULL)
    if (QccENTArithmeticSetModelContext(model, QCCWAVWDR_RUN_CONTEXT))
      {
        QccErrorAddMessage("(QccWAVwdrInputRunLength): Error calling QccENTArithmeticSetModelContext()");
        goto Error;
      }
  
  if ((return_value = QccWAVwdrInputSymbol(buffer, &run_length, sign, model)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdrInputRunLength): Error calling QccWAVwdrInputSymbol()");
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


static int QccWAVwdrAddNodeToTPS(QccList *TPS,
                                 int row,
                                 int col,
                                 QccWAVSubbandPyramid *subband_pyramid,
                                 double threshold,
                                 int **sign_array,
                                 QccBitBuffer *buffer,
                                 int method,
                                 int target_bit_cnt,
                                 QccENTArithmeticModel *model)
{
  int return_value, symbol;
  QccWAVwdrCoefficientBlock coefficient_block;
  QccListNode *new_node = NULL;
  
  coefficient_block.row = row;
  coefficient_block.col = col;
  coefficient_block.index = 0;
  if ((new_node = QccListCreateNode(sizeof(QccWAVwdrCoefficientBlock),
                                    (void *)(&coefficient_block))) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdrAddNodeToTPS): Error calling QccListCreateNode()");
      goto Error;
    }
  
  if (QccListAppendNode(TPS, new_node))
    {
      QccErrorAddMessage("(QccWAVwdrAddNodeToTPS): Error calling QccListAppendNode()");
      goto Error;
    }
  
  if (method == QCCWAVWDR_ENCODE)
    {
      if (sign_array[row][col] == 0)
        symbol = POSITIVE;
      else
        symbol = NEGATIVE;
      
      if ((return_value = QccWAVwdrInputOutput(buffer,
                                               &symbol,
                                               QCCWAVWDR_ENCODE,
                                               model,
                                               target_bit_cnt)))
        {
          if (return_value == 2)
            return(2);
          else
            {
              QccErrorAddMessage("(QccWAVwdrAddNodeToTPS): Error calling QccWAVwdrInputOutput()");
              goto Error;
            }
        }
      
      subband_pyramid->matrix[row][col] -= threshold;		
    }
  else
    subband_pyramid->matrix[row][col] = 1.5 * threshold;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);    
}


static int QccWAVwdrSortingPass(QccWAVSubbandPyramid *subband_pyramid,
                                int **sign_array,
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
  QccWAVwdrCoefficientBlock *current_coefficient_block;
  QccListNode *current_list_node;
  QccListNode *next_list_node;
  
  current_list_node = ICS->start;
  
  if (method == QCCWAVWDR_ENCODE)
    while (current_list_node != NULL)
      {
        next_list_node = current_list_node->next;
        current_coefficient_block =
          QccWAVwdrGetCoefficientBlockFromNode(current_list_node);
        significant = FALSE;
        
        if ((return_value = QccWAVwdrOutputRunLength(subband_pyramid,
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
                QccErrorAddMessage("(QccWAVwdrSortingPass): Error calling QccWAVwdrOutputRunLength()");
                goto Error;
              }
          }
        
        if (significant == TRUE)
          {
            if ((return_value =
                 QccWAVwdrAddNodeToTPS(TPS,
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
                    QccErrorAddMessage("(QccWAVwdrSortingPass): Error calling QccWAVwdrAddNodeToTPS()");
                    goto Error;
                  }
              }
            
            if (QccListDeleteNode(ICS, current_list_node))
              {
                QccErrorAddMessage("(QccWAVwdrSortingPass): Error calling QccListDeleteNode()");
                goto Error;
              }
          }
        
        current_list_node = next_list_node;
      }
  else
    {
      while (*relative_origin != virtual_end)
        {
          if ((return_value = QccWAVwdrInputRunLength(subband_pyramid,
                                                      buffer,
                                                      relative_origin,
                                                      &sign,
                                                      model)))
            {
              if (return_value == 2)
                return(2);
              else
                {
                  QccErrorAddMessage("(QccWAVwdrSortingPass): Error calling QccWAVwdrInputRunLength()");
                  goto Error;
                }
            }
          
          if (*relative_origin == virtual_end)
            break;
          
          if (*relative_origin > virtual_end)
            {
              QccErrorAddMessage("(QccWAVwdrSortingPass): Synchronization Error In Decoding");
              goto Error;
            }
          
          current_coefficient_block =
            QccWAVwdrGetCoefficientBlockFromNode(current_list_node);
          
          while (current_coefficient_block->index != *relative_origin)
            {
              current_list_node = current_list_node->next;
              current_coefficient_block =
                QccWAVwdrGetCoefficientBlockFromNode(current_list_node);
            }
          
          if (QccWAVwdrAddNodeToTPS(TPS,
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
              QccErrorAddMessage("(QccWAVwdrSortingPass): Error calling QccWAVwdrAddNodeToTPS()");
              goto Error;
            }
          
          sign_array[current_coefficient_block->row]
            [current_coefficient_block->col] = sign;
          
          next_list_node = current_list_node->next;
          if (QccListDeleteNode(ICS, current_list_node))
            {
              QccErrorAddMessage("(QccWAVwdrSortingPass): Error calling QccListDeleteNode()");
              goto Error;
            }
          current_list_node = next_list_node;
        }
      
      if (sign == 1)
        {
          QccErrorAddMessage("(QccWAVwdrSortingPass): Synchronization Error In Decoding");
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


static int QccWAVwdrOutputVirtualEnd(QccBitBuffer *buffer,
                                     unsigned int relative_origin,
                                     unsigned int virtual_end,
                                     int target_bit_cnt, 
                                     QccENTArithmeticModel *model)
{
  unsigned int run_length, temp;
  int symbol = POSITIVE, reduced_bits, return_value;
  
  run_length = virtual_end - relative_origin;
  temp = run_length; reduced_bits = 0;
  while (temp != 0)
    {
      temp >>= 1;
      reduced_bits++;
    }
  reduced_bits--;
  
  if (model != NULL)
    if (QccENTArithmeticSetModelContext(model, QCCWAVWDR_RUN_CONTEXT))
      {
        QccErrorAddMessage("(QccWAVwdrOutputVirtualEnd): Error calling QccENTArithmeticSetModelContext()");
        goto Error;
      }
  
  if ((return_value = QccWAVwdrOutputSymbol(buffer,
                                            run_length,
                                            reduced_bits,
                                            target_bit_cnt,
                                            model)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdrOutputVirtualEnd): Error calling QccWAVwdrOutputSymbol()");
          goto Error;
        }
    }
  
  if ((return_value = QccWAVwdrInputOutput(buffer,
                                           &symbol,
                                           QCCWAVWDR_ENCODE,
                                           model,
                                           target_bit_cnt)))
    {
      if (return_value == 2)
        return(2);
      else
        {
          QccErrorAddMessage("(QccWAVwdrOutputVirtualEnd): Error calling QccWAVwdrInputOutput()");
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


static int QccWAVwdrUpdateIndex(QccList *ICS)
{
  QccListNode *current_node, *stop;
  QccWAVwdrCoefficientBlock *current_coefficient;
  unsigned int adjust_index = 1;
  
  current_node = ICS->start;
  stop = ICS->end;
  if ((current_node == NULL) || (stop == NULL))
    return(0);
  stop = stop->next;
  
  while (current_node != stop)
    {
      current_coefficient = QccWAVwdrGetCoefficientBlockFromNode(current_node);
      current_coefficient->index = adjust_index;
      adjust_index++;
      current_node = current_node->next;
    }
  
  return(0);
}


static int QccWAVwdrRefinementPass(QccWAVSubbandPyramid *subband_pyramid,
                                   QccBitBuffer *buffer, double threshold, 
                                   QccList *SCS,
                                   QccListNode *stop,
                                   int method,
                                   int target_bit_cnt,
                                   QccENTArithmeticModel *model)
{
  QccListNode *current_node;
  QccWAVwdrCoefficientBlock *current_coefficient;
  int return_value;
  
  current_node = SCS->start;
  if ((current_node == NULL) || (stop == NULL))
    return(0);
  stop = stop->next;
  
  while (current_node != stop)
    {
      current_coefficient = QccWAVwdrGetCoefficientBlockFromNode(current_node);
      
      if ((return_value =
           QccWAVwdrInputOutputRefinementBit(subband_pyramid,
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
              QccErrorAddMessage("(QccWAVwdrRefinementPass): Error calling QccWAVwdrInputOutputRefinementBit()");
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


int QccWAVwdrEncode(const QccIMGImageComponent *image,
                    const QccIMGImageComponent *mask,
                    QccBitBuffer *buffer,
                    int num_levels,
                    const QccWAVWavelet *wavelet,
                    const QccWAVPerceptualWeights *perceptual_weights,
                    int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;    
  QccWAVSubbandPyramid image_subband_pyramid;
  QccWAVSubbandPyramid mask_subband_pyramid;
  int **sign_array = NULL;
  double image_mean;
  int max_coefficient_bits;
  QccList ICS;
  QccList SCS;
  QccList TPS;
  QccListNode *stop;
  double threshold;
  unsigned int relative_origin;
  int row;
  unsigned int virtual_end;
  int bitplane = 0;

  if (image == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramidInitialize(&mask_subband_pyramid);
  QccListInitialize(&ICS);
  QccListInitialize(&SCS);
  QccListInitialize(&TPS);
  if ((model = QccENTArithmeticEncodeStart(QccWAVwdrArithmeticContexts,
                                           QCCWAVWDR_NUM_CONTEXTS, NULL,
                                           QCCENT_ANYNUMBITS)) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  
  image_subband_pyramid.num_levels = 0;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccWAVwdrEncode): Mask and image must be same size");
          goto Error;
        }
      
      mask_subband_pyramid.num_levels = 0;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramidAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }
    }
  
  if ((sign_array = (int **)malloc(sizeof(int *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdrEncode): Error allocating memory");
      goto Error;
    }
  
  for (row = 0; row < image->num_rows; row++)
    if ((sign_array[row] =
         (int *)malloc(sizeof(int) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVwdrEncode): Error allocating memory");
        goto Error;
      }
  
  if (QccWAVwdrEncodeDWT(&image_subband_pyramid,
                         sign_array,
                         image,
                         num_levels,
                         &image_mean,
                         &max_coefficient_bits,
                         &mask_subband_pyramid,
                         mask,
                         wavelet,
                         perceptual_weights))
    {
      QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccWAVwdrEncodeDWT()");
      goto Error;
    }
  
  if (QccWAVwdrEncodeHeader(buffer,
                            num_levels,
                            image->num_rows,
                            image->num_cols,
                            image_mean,
                            max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccWAVwdrEncodeHeader()");
      goto Error;
    }
  
  if (QccWAVwdrAlgorithmInitialize(&image_subband_pyramid,
                                   ((mask != NULL) ?
                                    &mask_subband_pyramid : NULL),
                                   num_levels,
                                   &ICS,
                                   &virtual_end))
    {
      QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccWAVwdrAlgorithmInitialize()");
      goto Error;
    }
  
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  while (bitplane < QCCWAVWDR_MAXBITPLANES)
    {
      stop = SCS.end; 
      relative_origin = 0;
      return_value =
        QccWAVwdrSortingPass(&image_subband_pyramid,
                             sign_array,
                             buffer,
                             threshold,
                             &ICS,
                             &TPS,
                             QCCWAVWDR_ENCODE,
                             target_bit_cnt,
                             &relative_origin,
                             virtual_end,
                             model);
      
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccWAVwdrSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      return_value = QccWAVwdrOutputVirtualEnd(buffer,
                                               relative_origin,
                                               virtual_end,
                                               target_bit_cnt,
                                               model);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccWAVwdrOutputVirtualEnd()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      QccWAVwdrUpdateIndex(&ICS);
      return_value =
        QccWAVwdrRefinementPass(&image_subband_pyramid,
                                buffer,
                                threshold,
                                &SCS,
                                stop,
                                QCCWAVWDR_ENCODE,
                                target_bit_cnt,
                                model);
      
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccWAVwdrRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      if (QccListConcatenate(&SCS, &TPS))
        {
          QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccListConcatenate()");
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
  QccWAVSubbandPyramidFree(&image_subband_pyramid);
  QccWAVSubbandPyramidFree(&mask_subband_pyramid);
  if (sign_array != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (sign_array[row] != NULL)
          QccFree(sign_array[row]);
      QccFree(sign_array);
    }
  QccListFree(&ICS);
  QccListFree(&SCS);
  QccListFree(&TPS);
  QccENTArithmeticFreeModel(model);    
  return(return_value);
}


int QccWAVwdrDecodeHeader(QccBitBuffer *buffer,
                          int *num_levels,
                          int *num_rows,
                          int *num_cols,
                          double *image_mean,
                          int *max_coefficient_bits)
{
  int return_value;
  unsigned char ch;

  if (QccBitBufferGetChar(buffer, &ch))
    {
      QccErrorAddMessage("(QccWAVwdrDecodeHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  *num_levels = (int)ch;
  
  if (QccBitBufferGetInt(buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVwdrDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVwdrDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVwdrDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVwdrDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVwdrDecode(QccBitBuffer *buffer,
                    QccIMGImageComponent *image,
                    const QccIMGImageComponent *mask,
                    int num_levels,
                    const QccWAVWavelet *wavelet,
                    const QccWAVPerceptualWeights *perceptual_weights,
                    double image_mean,
                    int max_coefficient_bits,
                    int target_bit_cnt)
{
  int return_value;
  QccWAVSubbandPyramid image_subband_pyramid;
  QccWAVSubbandPyramid mask_subband_pyramid;
  QccENTArithmeticModel *model = NULL;    
  int **sign_array = NULL;
  QccList ICS;
  QccList SCS;
  QccList TPS;
  QccListNode *stop;
  double threshold;
  int row, col;
  QccWAVWavelet lazy_wavelet_transform;
  unsigned int relative_origin;
  unsigned int virtual_end;
  
  if (image == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramidInitialize(&mask_subband_pyramid);
  QccListInitialize(&ICS);
  QccListInitialize(&SCS);
  QccListInitialize(&TPS);
  QccWAVWaveletInitialize(&lazy_wavelet_transform);
  
  image_subband_pyramid.num_levels = num_levels;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWDRDecode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccWDRDecode): Mask and image must be same size");
          goto Error;
        }
      
      if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
        {
          QccErrorAddMessage("(QccWDRDecode): Error calling QccWAVWaveletCreate()");
          goto Error;
        }
      
      mask_subband_pyramid.num_levels = 0;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramidAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWDRDecode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }
      
      if (QccMatrixCopy(mask_subband_pyramid.matrix, mask->image,
                        mask->num_rows, mask->num_cols))
        {
          QccErrorAddMessage("(QccWDRDecode): Error calling QccMatrixCopy()");
          goto Error;
        }
      
      if (QccWAVSubbandPyramidDWT(&mask_subband_pyramid, num_levels,
                                  &lazy_wavelet_transform))
        {
          QccErrorAddMessage("(QccWDRDecode): Error calling QccWAVSubbandPyramidDWT()");
          goto Error;
        }
    }
  
  if ((sign_array = (int **)malloc(sizeof(int *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWDRDecode): Error allocating memory");
      goto Error;
    }
  
  for (row = 0; row < image->num_rows; row++)
    if ((sign_array[row] =
         (int *)malloc(sizeof(int) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWDRDecode): Error allocating memory");
        goto Error;
      }
  
  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      {
        image_subband_pyramid.matrix[row][col] = 0.0;
        sign_array[row][col] = 0;
      }
  
  if ((model = QccENTArithmeticDecodeStart(buffer,
                                           QccWAVwdrArithmeticContexts,
                                           QCCWAVWDR_NUM_CONTEXTS,
                                           NULL,
                                           target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVwdrDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  
  if (QccWAVwdrAlgorithmInitialize(&image_subband_pyramid,
                                   ((mask != NULL) ?
                                    &mask_subband_pyramid : NULL),
                                   num_levels,
                                   &ICS,
                                   &virtual_end))
    {
      QccErrorAddMessage("(QccWAVwdrDecode): Error calling QccWAVwdrAlgorithmInitialize()");
      goto Error;
    }
  
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  while (1)
    {
      stop = SCS.end;
      relative_origin = 0;		
      return_value =
        QccWAVwdrSortingPass(&image_subband_pyramid,
                             sign_array,
                             buffer,
                             threshold,
                             &ICS,
                             &TPS,
                             QCCWAVWDR_DECODE,
                             0,
                             &relative_origin,
                             virtual_end,
                             model);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdrDecode): Error calling QccWAVwdrSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      QccWAVwdrUpdateIndex(&ICS);		
      return_value =
        QccWAVwdrRefinementPass(&image_subband_pyramid,
                                buffer,
                                threshold,
                                &SCS,
                                stop,
                                QCCWAVWDR_DECODE,
                                0,
                                model);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVwdrDecode): Error calling QccWAVwdrRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          break;
      
      if (QccListConcatenate(&SCS, &TPS))
        {
          QccErrorAddMessage("(QccWAVwdrEncode): Error calling QccListConcatenate()");
          return(1);
        }

      threshold /= 2.0;
    }
  
  if (QccWAVwdrDecodeInverseDWT(&image_subband_pyramid,
                                ((mask != NULL) ?
                                 &mask_subband_pyramid : NULL),
                                sign_array,
                                image,
                                image_mean,
                                wavelet,
                                perceptual_weights))
    {
      QccErrorAddMessage("(QccWDRDecode): Error calling QccWDRDecodeInverseDWT()");
      goto Error;
    }
  
  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramidFree(&image_subband_pyramid);
  QccWAVSubbandPyramidFree(&mask_subband_pyramid);
  QccWAVWaveletFree(&lazy_wavelet_transform);
  if (sign_array != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (sign_array[row] != NULL)
          QccFree(sign_array[row]);
      QccFree(sign_array);
    }
  QccListFree(&ICS);
  QccListFree(&SCS);
  QccListFree(&TPS);
  QccENTArithmeticFreeModel(model);
  return(return_value);
}
