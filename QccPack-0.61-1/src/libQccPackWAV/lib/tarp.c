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


#define QCCTARP_BOUNDARY_VALUE 0

#define QCCTARP_INSIGNIFICANT 0
#define QCCTARP_PREVIOUSLY_SIGNIFICANT 1
#define QCCTARP_NEWLY_SIGNIFICANT 2

#define QCCTARP_MAXBITPLANES 128


typedef unsigned int QccWAVTarpFixedPoint;
#define QCCTARP_FIXEDPOINT_BITS 30
#define QccWAVTarpToFixedPoint(x) \
((QccWAVTarpFixedPoint)rint((x) * (1 << QCCTARP_FIXEDPOINT_BITS)))
#define QccWAVTarpFromFixedPoint(x) \
(((double)(x)) / (1 << QCCTARP_FIXEDPOINT_BITS))
#define QCCTARP_FIXEDPOINT_MULT_SHIFT (QCCTARP_FIXEDPOINT_BITS >> 1)
#define QCCTARP_FIXEDPOINT_MULT_MASK ((1 << QCCTARP_FIXEDPOINT_MULT_SHIFT) - 1)
#define QccWAVTarpFixedPointMultDiv(x) \
((QccWAVTarpFixedPoint)((x) >> QCCTARP_FIXEDPOINT_MULT_SHIFT))
#define QccWAVTarpFixedPointMultMask(x) \
((QccWAVTarpFixedPoint)((x) & QCCTARP_FIXEDPOINT_MULT_MASK))
#define QccWAVTarpFixedPointMult(x, y) \
((QccWAVTarpFixedPoint)(QccWAVTarpFixedPointMultDiv((x)) * \
QccWAVTarpFixedPointMultDiv((y)) + \
QccWAVTarpFixedPointMultDiv(QccWAVTarpFixedPointMultMask((x)) * \
QccWAVTarpFixedPointMultDiv((y))) + \
QccWAVTarpFixedPointMultDiv(QccWAVTarpFixedPointMultMask((y)) * \
QccWAVTarpFixedPointMultDiv((x)))))
                             

typedef QccWAVTarpFixedPoint *QccWAVTarpFixedPointVector;


static QccWAVTarpFixedPointVector
QccWAVTarpFixedPointVectorAlloc(int vector_dimension)
{
  QccWAVTarpFixedPointVector vector;

  if (vector_dimension <= 0)
    return(NULL);

  if ((vector =
       (QccWAVTarpFixedPointVector)calloc(vector_dimension,
                                          sizeof(QccWAVTarpFixedPoint))) ==
      NULL)
    QccErrorAddMessage("(QccWAVTarpFixedPointVectorAlloc): Error allocating memory");

  return(vector);
}


static void QccWAVTarpFixedPointVectorFree(QccWAVTarpFixedPointVector vector)
{
  if (vector != NULL)
    QccFree(vector);
}


static int QccWAVTarpUpdateModel(QccENTArithmeticModel *model,
                                 QccWAVTarpFixedPoint prob)
{
  int probabilities[2];
  int symbol;
  int cum = 0;
  int freq;
  int max_freq;
  int context = 0;

  probabilities[1] =
    prob >> (QCCTARP_FIXEDPOINT_BITS - QCCENT_FREQUENCY_BITS);
  probabilities[0] = QCCENT_MAXFREQUENCY - probabilities[1];
  
  max_freq = QCCENT_MAXFREQUENCY - model->num_symbols[context] + 1;

  for (symbol = model->num_symbols[context] - 2; symbol >= 0; symbol--)
    {
      freq = probabilities[symbol];
      
      if (freq > max_freq)
        model->frequencies[context]
          [model->translate_symbol_to_index[context][symbol]] =
          max_freq;
      else
        if (freq <= 0)
          model->frequencies[context]
            [model->translate_symbol_to_index[context][symbol]] = 1;
        else
          model->frequencies[context]
            [model->translate_symbol_to_index[context][symbol]] = freq;
    }

  for (symbol = model->num_symbols[context]; symbol >= 0; symbol--)
    {
      model->cumulative_frequencies[context][symbol] = cum;
      cum += model->frequencies[context][symbol];
    }

  return(0);
}


static int QccWAVTarpEncodeDWT(QccWAVSubbandPyramid *image_subband_pyramid,
                               char **sign_array,
                               const QccIMGImageComponent *image,
                               int num_levels,
                               double *image_mean,
                               int *max_coefficient_bits,
                               QccWAVSubbandPyramid *mask_subband_pyramid,
                               const QccIMGImageComponent *mask,
                               const QccWAVWavelet *wavelet)
{
  double coefficient_magnitude;
  double max_coefficient = -MAXFLOAT;
  int row, col;
  
  if (mask == NULL)
    {
      if (QccMatrixCopy(image_subband_pyramid->matrix, image->image,
                        image->num_rows, image->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarpEncodeDWT): Error calling QccMatrixCopy()");
          return(1);
        }
      
      if (QccWAVSubbandPyramidSubtractMean(image_subband_pyramid,
                                           image_mean,
                                           NULL))
        {
          QccErrorAddMessage("(QccWAVTarpEncodeDWT): Error calling QccWAVSubbandPyramidSubtractMean()");
          return(1);
        }
    }
  else
    {
      if (QccMatrixCopy(mask_subband_pyramid->matrix, mask->image,
                        mask->num_rows, mask->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarpEncodeDWT): Error calling QccMatrixCopy()");
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
          QccErrorAddMessage("(QccWAVTarpEncodeDWT): Error calling QccWAVSubbandPyramidShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramidDWT(image_subband_pyramid,
                                num_levels,
                                wavelet))
      {
        QccErrorAddMessage("(QccWAVTarpEncodeDWT): Error calling QccWAVSubbandPyramidDWT()");
        return(1);
      }
  
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    for (col = 0; col < image_subband_pyramid->num_cols; col++)
      {
        coefficient_magnitude = fabs(image_subband_pyramid->matrix[row][col]);
        if (image_subband_pyramid->matrix[row][col] != coefficient_magnitude)
          {
            image_subband_pyramid->matrix[row][col] = coefficient_magnitude;
            sign_array[row][col] = 1;
          }
        else
          sign_array[row][col] = 0;
        if (coefficient_magnitude > max_coefficient)
          max_coefficient = coefficient_magnitude;
      }
  
  *max_coefficient_bits = (int)floor(QccMathLog2(max_coefficient));
  
  return(0);
}


int QccWAVTarpEncodeHeader(QccBitBuffer *output_buffer, 
                           double alpha,
                           int num_levels, 
                           int num_rows, int num_cols,
                           double image_mean,
                           int max_coefficient_bits)
{
  int return_value;
  
  if (QccBitBufferPutDouble(output_buffer, alpha))
    {
      QccErrorAddMessage("(QccWAVTarpEncodeHeader): Error calling QccBitBufferPuDouble()");
      goto Error;
    }

  if (QccBitBufferPutChar(output_buffer, (unsigned char)num_levels))
    {
      QccErrorAddMessage("(QccWAVTarpEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVTarpEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVTarpEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(output_buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVTarpEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVTarpEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVTarpSignificanceInputOutput(double *coefficient,
                                             QccWAVTarpFixedPoint p,
                                             double threshold,
                                             char *sign,
                                             char *significance,
                                             QccENTArithmeticModel *model,
                                             QccBitBuffer *buffer)
{
  int symbol;
  int return_value;

  if (*significance != QCCTARP_INSIGNIFICANT)
    return(0);

  if (QccWAVTarpUpdateModel(model, p))
    {
      QccErrorAddMessage("(QccWAVTarpSignificanceInputOutput): Error calling QccWAVTarpUpdateModel()");
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
            QccErrorAddMessage("(QccWAVTarpSignificanceInputOutput): Error calling QccENTArithmeticEncode()");
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
      *significance = QCCTARP_NEWLY_SIGNIFICANT;
          
      if (QccWAVTarpUpdateModel(model, QccWAVTarpToFixedPoint(0.5)))
        {
          QccErrorAddMessage("(QccWAVTarpSignificanceInputOutput): Error calling QccWAVTarpUpdateModel()");
          return(1);
        }

      if (buffer->type == QCCBITBUFFER_OUTPUT)
        {
          symbol = (int)(*sign);
      
          return_value =
            QccENTArithmeticEncode(&symbol, 1,
                                   model, buffer);
          if (return_value == 2)
            return(2);
          else
            if (return_value)
              {
                QccErrorAddMessage("(QccWAVTarpSignificanceInputOutput): Error calling QccENTArithmeticEncode()");
                return(1);
              }
        }
      else
        {
          if (QccENTArithmeticDecode(buffer,
                                     model,
                                     &symbol, 1))
            return(2);

          *sign = (char)symbol;
        }
    }

  return(0);
}


static int QccWAVTarpSignificancePassSubband(QccWAVSubbandPyramid
                                             *coefficients,
                                             const QccWAVSubbandPyramid *mask,
                                             char **significance_map,
                                             char **sign_array,
                                             QccWAVTarpFixedPoint alpha,
                                             QccWAVTarpFixedPoint beta,
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
  QccWAVTarpFixedPoint p1;
  QccWAVTarpFixedPointVector p2 = NULL;
  QccWAVTarpFixedPoint p3;
  QccWAVTarpFixedPoint p;
  QccWAVTarpFixedPoint v;
  int transparent;

  if (QccWAVSubbandPyramidSubbandSize(coefficients,
                                      subband,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVTarpSignificancePassSubband): Error calling QccWAVSubbandPyramidSubbandSize()");
      goto Error;
    }
  if (QccWAVSubbandPyramidSubbandOffsets(coefficients,
                                         subband,
                                         &subband_origin_row,
                                         &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVTarpSignificancePassSubband): Error calling QccWAVSubbandPyramidSubbandOffsets()");
      goto Error;
    }
  
  if ((!subband_num_rows) || (!subband_num_cols))
    return(0);

  if ((p2 = QccWAVTarpFixedPointVectorAlloc(subband_num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarpSignificancePassSubband): Error calling QccWAVTarpFixedPointVectorAlloc()");
      goto Error;
    }

  for (col = 0; col < subband_num_cols; col++)
    p2[col] = QCCTARP_BOUNDARY_VALUE;

  for (row = 0; row < subband_num_rows; row++)
    {
      p1 = QCCTARP_BOUNDARY_VALUE;

      for (col = 0; col < subband_num_cols; col++)
        {
          transparent =
            (mask != NULL) ?
            QccAlphaTransparent(mask->matrix
                                [subband_origin_row + row]
                                [subband_origin_col + col]) : 0;

          if (!transparent)
            {
              p = QccWAVTarpFixedPointMult(alpha, (p1 + p2[col]));
              
              return_value =
                QccWAVTarpSignificanceInputOutput(&coefficients->matrix
                                                  [subband_origin_row + row]
                                                  [subband_origin_col + col],
                                                  p,
                                                  threshold,
                                                  &sign_array
                                                  [subband_origin_row + row]
                                                  [subband_origin_col + col],
                                                  &significance_map
                                                  [subband_origin_row + row]
                                                  [subband_origin_col + col],
                                                  model,
                                                  buffer);
              if (return_value == 2)
                goto Return;
              else
                if (return_value)
                  {
                    QccErrorAddMessage("(QccWAVTarpSignificancePassSubband): Error calling QccWAVTarpSignificanceInputOutput()");
                    goto Error;
                  }
              
              v =
                (significance_map[subband_origin_row + row]
                 [subband_origin_col + col] != QCCTARP_INSIGNIFICANT) ?
                QccWAVTarpToFixedPoint(1.0) : 0;
              p1 =
                QccWAVTarpFixedPointMult(alpha, p1) +
                QccWAVTarpFixedPointMult(beta, v);
            }

          p2[col] = p1 + QccWAVTarpFixedPointMult(alpha, p2[col]);

        }

      p3 = QCCTARP_BOUNDARY_VALUE;

      for (col = subband_num_cols - 1; col >= 0; col--)
        {
          transparent =
            (mask != NULL) ?
            QccAlphaTransparent(mask->matrix
                                [subband_origin_row + row]
                                [subband_origin_col + col]) : 0;

          p2[col] += QccWAVTarpFixedPointMult(alpha, p3);

          if (!transparent)
            {
              v =
                (significance_map[subband_origin_row + row]
                 [subband_origin_col + col] != QCCTARP_INSIGNIFICANT) ?
                QccWAVTarpToFixedPoint(1.0) : 0;
              p3 =
                QccWAVTarpFixedPointMult(alpha, p3) +
                QccWAVTarpFixedPointMult(beta, v);
            }
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVTarpFixedPointVectorFree(p2);
  return(return_value);
}


static int QccWAVTarpSignificancePass(QccWAVSubbandPyramid *coefficients,
                                      const QccWAVSubbandPyramid *mask,
                                      char **significance_map,
                                      char **sign_array,
                                      QccWAVTarpFixedPoint alpha,
                                      QccWAVTarpFixedPoint beta,
                                      double threshold,
                                      QccENTArithmeticModel *model,
                                      QccBitBuffer *buffer)
{
  int subband;
  int num_subbands;
  int return_value;

  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(coefficients->num_levels);

  for (subband = 0; subband < num_subbands; subband++)
    {
      return_value =
        QccWAVTarpSignificancePassSubband(coefficients,
                                          mask,
                                          significance_map,
                                          sign_array,
                                          alpha,
                                          beta,
                                          subband,
                                          threshold,
                                          model,
                                          buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVTarpSignificancePass): Error calling QccWAVTarpSignificancePassSubband()");
          return(1);
        }
      else
        if (return_value == 2)
          return(2);
    }

  return(0);
}


static int QccWAVTarpRefinementInputOutput(double *coefficient,
                                           double threshold,
                                           QccENTArithmeticModel *model,
                                           QccBitBuffer *buffer)
{
  int return_value;
  int symbol;

  if (QccWAVTarpUpdateModel(model, QccWAVTarpToFixedPoint(0.5)))
    {
      QccErrorAddMessage("(QccWAVTarpRefinementInputOutput): Error calling QccWAVTarpUpdateModel()");
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
            QccErrorAddMessage("(QccWAVTarpRefinementInputOutput): Error calling QccENTArithmeticEncode()");
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


static int QccWAVTarpRefinementPassSubband(QccWAVSubbandPyramid *coefficients,
                                           const QccWAVSubbandPyramid *mask,
                                           char **significance_map,
                                           double threshold,
                                           int subband,
                                           QccENTArithmeticModel *model,
                                           QccBitBuffer *buffer)
{
  int return_value;
  int subband_origin_row;
  int subband_origin_col;
  int subband_num_rows;
  int subband_num_cols;
  int row, col;
  int transparent;

  if (QccWAVSubbandPyramidSubbandSize(coefficients,
                                      subband,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVTarpRefinementPassSubband): Error calling QccWAVSubbandPyramidSubbandSize()");
      return(1);
    }
  if (QccWAVSubbandPyramidSubbandOffsets(coefficients,
                                         subband,
                                         &subband_origin_row,
                                         &subband_origin_col))
    {
      QccErrorAddMessage("(QccWAVTarpRefinementPassSubband): Error calling QccWAVSubbandPyramidSubbandOffsets()");
      return(1);
    }
  
  for (row = 0; row < subband_num_rows; row++)
    for (col = 0; col < subband_num_cols; col++)
      {
        transparent =
          (mask != NULL) ?
          QccAlphaTransparent(mask->matrix
                              [subband_origin_row + row]
                              [subband_origin_col + col]) : 0;
        
        if (!transparent)
          {
            if (significance_map[subband_origin_row + row]
                [subband_origin_col + col] == QCCTARP_PREVIOUSLY_SIGNIFICANT)
              {
                return_value =
                  QccWAVTarpRefinementInputOutput(&coefficients->matrix
                                                  [subband_origin_row + row]
                                                  [subband_origin_col + col],
                                                  threshold,
                                                  model,
                                                  buffer);
                if (return_value == 1)
                  {
                    QccErrorAddMessage("(QccWAVTarpRefinementPassSubband): Error calling QccWAVTarpRefinementPassInputOutput()");
                    return(1);
                  }
                else
                  if (return_value == 2)
                    return(2);
              }
            else
              if (significance_map[subband_origin_row + row]
                  [subband_origin_col + col] == QCCTARP_NEWLY_SIGNIFICANT)
                significance_map[subband_origin_row + row]
                  [subband_origin_col + col] = QCCTARP_PREVIOUSLY_SIGNIFICANT;
          }
      }

  return(0);
}


static int QccWAVTarpRefinementPass(QccWAVSubbandPyramid *coefficients,
                                    const QccWAVSubbandPyramid *mask,
                                    char **significance_map,
                                    double threshold,
                                    QccENTArithmeticModel *model,
                                    QccBitBuffer *buffer)
{
  int subband;
  int num_subbands;
  int return_value;

  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(coefficients->num_levels);

  for (subband = 0; subband < num_subbands; subband++)
    {
      return_value =
        QccWAVTarpRefinementPassSubband(coefficients,
                                        mask,
                                        significance_map,
                                        threshold,
                                        subband,
                                        model,
                                        buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVTarpRefinementPass): Error calling QccWAVTarpRefinementPassSubband()");
          return(1);
        }
      else
        if (return_value == 2)
          return(2);
    }

  return(0);
}


int QccWAVTarpEncode(const QccIMGImageComponent *image,
                     const QccIMGImageComponent *mask,
                     double alpha,
                     int num_levels,
                     int target_bit_cnt,
                     const QccWAVWavelet *wavelet,
                     QccBitBuffer *output_buffer)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid image_subband_pyramid;
  QccWAVSubbandPyramid mask_subband_pyramid;
  char **sign_array = NULL;
  char **significance_map = NULL;
  double image_mean;
  int max_coefficient_bits;
  double threshold;
  int num_symbols[1] = { 2 };
  int row, col;
  QccWAVTarpFixedPoint beta;
  int bitplane = 0;

  if (image == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramidInitialize(&mask_subband_pyramid);

  image_subband_pyramid.num_levels = 0;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVTarpEncode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarpEncode): Mask and image must be same size");
          goto Error;
        }

      mask_subband_pyramid.num_levels = 0;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramidAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVTarpEncode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }
    }
  
  if ((sign_array = (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarpEncode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((sign_array[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVTarpEncode): Error allocating memory");
        goto Error;
      }
  
  if ((significance_map =
       (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarpEncode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((significance_map[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVTarpEncode): Error allocating memory");
        goto Error;
      }
  
  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      significance_map[row][col] = QCCTARP_INSIGNIFICANT;

  if (QccWAVTarpEncodeDWT(&image_subband_pyramid,
                          sign_array,
                          image,
                          num_levels,
                          &image_mean,
                          &max_coefficient_bits,
                          ((mask != NULL) ? &mask_subband_pyramid : NULL),
                          mask,
                          wavelet))
    {
      QccErrorAddMessage("(QccWAVTarpEncode): Error calling QccWAVTarpEncodeDWT()");
      goto Error;
    }
  
  alpha = (double)((float)alpha);

  if (QccWAVTarpEncodeHeader(output_buffer,
                             alpha,
                             num_levels,
                             image->num_rows, image->num_cols, 
                             image_mean,
                             max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVTarpEncode): Error calling QccWAVTarpEncodeHeader()");
      goto Error;
    }
  
  if ((model = QccENTArithmeticEncodeStart(num_symbols,
                                           1,
                                           NULL,
                                           target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarpEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);

  threshold = pow((double)2, (double)max_coefficient_bits);
  
  beta = QccWAVTarpToFixedPoint((1 - alpha) * (1 - alpha) / 2 / alpha);

  while (bitplane < QCCTARP_MAXBITPLANES)
    {
      return_value = QccWAVTarpSignificancePass(&image_subband_pyramid,
                                                ((mask != NULL) ?
                                                 &mask_subband_pyramid : NULL),
                                                significance_map,
                                                sign_array,
                                                QccWAVTarpToFixedPoint(alpha),
                                                beta,
                                                threshold,
                                                model,
                                                output_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVTarpEncode): Error calling QccWAVTarpSignificancePass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      return_value = QccWAVTarpRefinementPass(&image_subband_pyramid,
                                              ((mask != NULL) ?
                                               &mask_subband_pyramid : NULL),
                                              significance_map,
                                              threshold,
                                              model,
                                              output_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVTarpEncode): Error calling QccWAVTarpRefinementPass()");
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
  QccWAVSubbandPyramidFree(&image_subband_pyramid);
  QccWAVSubbandPyramidFree(&mask_subband_pyramid);
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
  QccENTArithmeticFreeModel(model);
  return(return_value);
}


static int QccWAVTarpDecodeInverseDWT(QccWAVSubbandPyramid
                                      *image_subband_pyramid,
                                      QccWAVSubbandPyramid
                                      *mask_subband_pyramid,
                                      char **sign_array,
                                      QccIMGImageComponent *image,
                                      double image_mean,
                                      const QccWAVWavelet *wavelet)
{
  int row, col;
  
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    for (col = 0; col < image_subband_pyramid->num_cols; col++)
      if (sign_array[row][col])
        image_subband_pyramid->matrix[row][col] *= -1;
  
  if (mask_subband_pyramid != NULL)
    {
      if (QccWAVSubbandPyramidInverseShapeAdaptiveDWT(image_subband_pyramid,
                                                      mask_subband_pyramid,
                                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVTarpDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramidInverseDWT(image_subband_pyramid,
                                       wavelet))
      {
        QccErrorAddMessage("(QccWAVTarpDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseDWT()");
        return(1);
      }
  
  if (QccWAVSubbandPyramidAddMean(image_subband_pyramid,
                                  image_mean))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeInverseDWT): Error calling QccWAVSubbandPyramidAddMean()");
      return(1);
    }
  
  if (mask_subband_pyramid != NULL)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (QccAlphaTransparent(mask_subband_pyramid->matrix[row][col]))
          image_subband_pyramid->matrix[row][col] = 0;

  if (QccMatrixCopy(image->image, image_subband_pyramid->matrix,
                    image->num_rows, image->num_cols))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeInverseDWT): Error calling QccMatrixCopy()");
      return(1);
    }
  
  if (QccIMGImageComponentSetMaxMin(image))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeInverseDWT): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccWAVTarpDecodeHeader(QccBitBuffer *input_buffer, 
                           double *alpha,
                           int *num_levels, 
                           int *num_rows, int *num_cols,
                           double *image_mean,
                           int *max_coefficient_bits)
{
  int return_value;
  unsigned char ch;

  if (QccBitBufferGetDouble(input_buffer, alpha))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }

  if (QccBitBufferGetChar(input_buffer, &ch))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  *num_levels = (int)ch;
  
  if (QccBitBufferGetInt(input_buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(input_buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVTarpDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVTarpDecode(QccBitBuffer *input_buffer,
                     QccIMGImageComponent *image,
                     const QccIMGImageComponent *mask,
                     double alpha,
                     int num_levels,
                     const QccWAVWavelet *wavelet,
                     double image_mean,
                     int max_coefficient_bits,
                     int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid image_subband_pyramid;
  QccWAVSubbandPyramid mask_subband_pyramid;
  char **sign_array = NULL;
  char **significance_map = NULL;
  double threshold;
  int row, col;
  int num_symbols[1] = { 2 };
  QccWAVWavelet lazy_wavelet_transform;
  QccWAVTarpFixedPoint beta;
  
  if (image == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramidInitialize(&mask_subband_pyramid);
  QccWAVWaveletInitialize(&lazy_wavelet_transform);
  
  image_subband_pyramid.num_levels = num_levels;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarpDecode): Mask and image must be same size");
          goto Error;
        }

      if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
        {
          QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccWAVWaveletCreate()");
          goto Error;
        }

      mask_subband_pyramid.num_levels = 0;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramidAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }

      if (QccMatrixCopy(mask_subband_pyramid.matrix,
                        mask->image,
                        mask->num_rows,
                        mask->num_cols))
        {
          QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccMatrixCopy()");
          goto Error;
        }

      if (QccWAVSubbandPyramidDWT(((mask != NULL) ?
                                   &mask_subband_pyramid : NULL),
                                  num_levels,
                                  &lazy_wavelet_transform))
        {
          QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccWAVSubbandPyramidDWT()");
          goto Error;
        }
    }
  
  if ((sign_array = (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarpDecode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((sign_array[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVTarpDecode): Error allocating memory");
        goto Error;
      }
  
  if ((significance_map =
       (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarpDecode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((significance_map[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccWAVTarpDecode): Error allocating memory");
        goto Error;
      }
  
  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      {
        image_subband_pyramid.matrix[row][col] = 0.0;
        sign_array[row][col] = 0;
        significance_map[row][col] = QCCTARP_INSIGNIFICANT;
      }
  
  if ((model =
       QccENTArithmeticDecodeStart(input_buffer,
                                   num_symbols,
                                   1,
                                   NULL,
                                   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  QccENTArithmeticSetModelAdaption(model, QCCENT_NONADAPTIVE);

  threshold = pow((double)2, (double)max_coefficient_bits);
  
  beta = QccWAVTarpToFixedPoint((1 - alpha) * (1 - alpha) / 2 / alpha);

  while (1)
    {
      return_value = QccWAVTarpSignificancePass(&image_subband_pyramid,
                                                ((mask != NULL) ?
                                                 &mask_subband_pyramid : NULL),
                                                significance_map,
                                                sign_array,
                                                QccWAVTarpToFixedPoint(alpha),
                                                beta,
                                                threshold,
                                                model,
                                                input_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccWAVTarpSignificancePass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      return_value = QccWAVTarpRefinementPass(&image_subband_pyramid,
                                              ((mask != NULL) ?
                                               &mask_subband_pyramid : NULL),
                                              significance_map,
                                              threshold,
                                              model,
                                              input_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccWAVTarpRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      threshold /= 2.0;
    }

 Finished:
  if (QccWAVTarpDecodeInverseDWT(&image_subband_pyramid,
                                 ((mask != NULL) ?
                                  &mask_subband_pyramid : NULL),
                                 sign_array,
                                 image,
                                 image_mean,
                                 wavelet))
    {
      QccErrorAddMessage("(QccWAVTarpDecode): Error calling QccWAVTarpDecodeInverseDWT()");
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
  QccENTArithmeticFreeModel(model);
  return(return_value);
}
