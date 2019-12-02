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


#define QCCWAVSR_EOF -1

#define QCCWAVSR_ZERO  0
#define QCCWAVSR_ONE   1
#define QCCWAVSR_MINUS 2
#define QCCWAVSR_PLUS  3

#define QCCWAVSR_RUNCONTEXT   0
#define QCCWAVSR_STACKCONTEXT 1


static int QccWAVsrEncodeOutputRunLength(int run_length,
                                         QccBitBuffer *buffer,
                                         QccENTArithmeticModel *model)
{
  int retain_msb = 1;
  int symbol_stream;

  if (!run_length)
    return(0);

  if (QccENTArithmeticSetModelContext(model, QCCWAVSR_RUNCONTEXT))
    {
      QccErrorAddMessage("(QccWAVsrEncodeOutputRunLength): Error calling QccENTArithmeticSetModelContext()");
      return(1);
    }

  while (run_length > 1)
    {
      if (run_length & 1)
        {
          symbol_stream = QCCWAVSR_PLUS;
          if (QccENTArithmeticEncode(&symbol_stream, 1,
                                     model, buffer))
            {
              QccErrorAddMessage("(QccWAVsrEncodeOutputRunLength): Error calling QccENTArithmeticEncode()");
              return(1);
            }
        }
      else
        {
          symbol_stream = QCCWAVSR_MINUS;
          if (QccENTArithmeticEncode(&symbol_stream, 1,
                                     model, buffer))
            {
              QccErrorAddMessage("(QccWAVsrEncodeOutputRunLength): Error calling QccENTArithmeticEncode()");
              return(1);
            }
          retain_msb = 0;
        }
      run_length >>= 1;
    }

  if (retain_msb)
    {
      symbol_stream = QCCWAVSR_PLUS;
      if (QccENTArithmeticEncode(&symbol_stream, 1,
                                 model, buffer))
        {
          QccErrorAddMessage("(QccWAVsrEncodeOutputRunLength): Error calling QccENTArithmeticEncode()");
          return(1);
        }
    }

  return(0);
}


static int QccWAVsrEncodeOutputStackValue(int stack_value,
                                          QccBitBuffer *buffer,
                                          QccENTArithmeticModel *model)
{
  int positive;
  int symbol_stream;

  if (!stack_value)
    return(0);
  positive = (stack_value > 0);
  stack_value = abs(stack_value) + 1;

  if (QccENTArithmeticSetModelContext(model, QCCWAVSR_RUNCONTEXT))
    {
      QccErrorAddMessage("(QccWAVsrEncodeOutputStackValue): Error calling QccENTArithmeticSetModelContext()");
      return(1);
    }

  while (stack_value > 1)
    {
      if (stack_value & 1)
        {
          symbol_stream = QCCWAVSR_ONE;
          if (QccENTArithmeticEncode(&symbol_stream, 1,
                                     model, buffer))
            {
              QccErrorAddMessage("(QccWAVsrEncodeOutputStackValue): Error calling QccENTArithmeticEncode()");
              return(1);
            }
        }
      else
        {
          symbol_stream = QCCWAVSR_ZERO;
          if (QccENTArithmeticEncode(&symbol_stream, 1,
                                     model, buffer))
            {
              QccErrorAddMessage("(QccWAVsrEncodeOutputStackValue): Error calling QccENTArithmeticEncode()");
              return(1);
            }
        }

      stack_value >>= 1;
      if (QccENTArithmeticSetModelContext(model, QCCWAVSR_STACKCONTEXT))
        {
          QccErrorAddMessage("(QccWAVsrEncodeOutputStackValue): Error calling QccENTArithmeticSetModelContext()");
          return(1);
        }
    }

  if (positive)
    {
      symbol_stream = QCCWAVSR_PLUS;
      if (QccENTArithmeticEncode(&symbol_stream, 1,
                                 model, buffer))
        {
          QccErrorAddMessage("(QccWAVsrEncodeOutputStackValue): Error calling QccENTArithmeticEncode()");
          return(1);
        }
    }
  else
    {
      symbol_stream = QCCWAVSR_MINUS;
      if (QccENTArithmeticEncode(&symbol_stream, 1,
                                 model, buffer))
        {
          QccErrorAddMessage("(QccWAVsrEncodeOutputStackValue): Error calling QccENTArithmeticEncode()");
          return(1);
        }
    }

  return(0);
}


static int QccWAVsrEncodeBitstream(QccChannel *channels,
                                   QccBitBuffer *buffer,
                                   int num_subbands)
{
  int return_value;
  int subband;
  int symbol;
  int run_length;
  int stack_value;
  QccENTArithmeticModel *model = NULL;
  int num_symbols[2] = { 4, 4 };

  if ((model = QccENTArithmeticEncodeStart(num_symbols, 2,
                                           NULL,
                                           QCCENT_ANYNUMBITS)) == 
      NULL)
    {
      QccErrorAddMessage("(QccWAVsrEncodeBitstream): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }

  run_length = 0;
  for (subband = 0; subband < num_subbands; subband++)
    for (symbol = 0; symbol < channels[subband].channel_length; symbol++)
      if (!channels[subband].channel_symbols[symbol])
        run_length++;
      else
        {
          if (QccWAVsrEncodeOutputRunLength(run_length, buffer, model))
            {
              QccErrorAddMessage("(QccWAVsrEncodeBitstream): Error calling QccWAVsrEncodeOutputRunLength()");
              goto Error;
            }
          run_length = 0;
          stack_value =
            channels[subband].channel_symbols[symbol];
          if (QccWAVsrEncodeOutputStackValue(stack_value, buffer, model))
            {
              QccErrorAddMessage("(QccWAVsrEncodeBitstream): Error calling QccWAVsrEncodeOutputStackValue()");
              goto Error;
            }
        }

  if (QccENTArithmeticEncodeEnd(model, QCCWAVSR_RUNCONTEXT, buffer))
    {
      QccErrorAddMessage("(QccWAVsrEncodeBitstream): Error calling QccENTArithmeticEncodeEnd()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccENTArithmeticFreeModel(model);
  return(return_value);
}


static int QccWAVsrEncodeHeader(QccBitBuffer *buffer, 
                                int num_levels, 
                                int num_rows, int num_cols,
                                const QccSQScalarQuantizer *quantizer,
                                double image_mean)
{
  if (buffer == NULL)
    return(0);

  if (QccBitBufferPutChar(buffer,
                          (unsigned char)num_levels))
    {
      QccErrorAddMessage("(QccWAVsrEncodeHeader): Error calling QccBitBufferPutChar()");
      return(1);
    }
  
  if (QccBitBufferPutInt(buffer,
                         num_rows))
    {
      QccErrorAddMessage("(QccWAVsrEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutInt(buffer,
                         num_cols))
    {
      QccErrorAddMessage("(QccWAVsrEncodeHeader): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  if (QccBitBufferPutChar(buffer,
                          (char)quantizer->type))
    {
      QccErrorAddMessage("(QccWAVsrEncodeHeader): Error calling QccBitBufferPutChar()");
      return(1);
    }

  if (QccBitBufferPutDouble(buffer,
                            quantizer->stepsize))
    {
      QccErrorAddMessage("(QccWAVsrEncodeHeader): Error calling QccBitBufferPutDouble()");
      return(1);
    }

  if (quantizer->type == QCCSQSCALARQUANTIZER_DEADZONE)
    if (QccBitBufferPutDouble(buffer,
                              quantizer->deadzone))
      {
        QccErrorAddMessage("(QccWAVsrEncodeHeader): Error calling QccBitBufferPutDouble()");
        return(1);
      }

  if (QccBitBufferPutDouble(buffer,
                            image_mean))
    {
      QccErrorAddMessage("(QccWAVsrEncodeHeader): Error calling QccBitBufferPutDouble()");
      return(1);
    }

  return(0);
}


int QccWAVsrEncode(const QccIMGImageComponent *image,
                   QccBitBuffer *buffer,
                   const QccSQScalarQuantizer *quantizer,
                   int num_levels,
                   const QccWAVWavelet *wavelet,
                   const QccWAVPerceptualWeights *perceptual_weights)
{
  int return_value;
  QccChannel *channels = NULL;
  int subband;
  int num_subbands = 0;
  int subband_num_rows, subband_num_cols;
  double image_mean;

  if (image == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  if ((quantizer->type != QCCSQSCALARQUANTIZER_UNIFORM) &&
      (quantizer->type != QCCSQSCALARQUANTIZER_DEADZONE))
    {
      QccErrorAddMessage("(QccWAVsrEncode): Only uniform or dead-zone type quantizers supported");
      goto Error;
    }
  if (quantizer->stepsize <= 0)
    {
      QccErrorAddMessage("(QccWAVsrEncode): Step size must be > 0");
      goto Error;
    }
  
  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);
  
  if ((channels = 
       (QccChannel *)malloc(sizeof(QccChannel) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsrEncode): Error allocating memory");
      goto Error;
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      QccWAVSubbandPyramid subband_pyramid;
      subband_pyramid.num_levels = num_levels;
      subband_pyramid.num_rows = image->num_rows;
      subband_pyramid.num_cols = image->num_cols;

      QccChannelInitialize(&channels[subband]);
      if (QccWAVSubbandPyramidSubbandSize(&subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVsrEncode): Error calling QccWAVSubbandPyramidSubbandSize");
          goto Error;
        }
      channels[subband].channel_length =
        subband_num_rows * subband_num_cols;
      channels[subband].access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
      if (QccChannelAlloc(&channels[subband]))
        {
          QccErrorAddMessage("(QccWAVsrEncode): Error calling QccChannelAlloc()");
          goto Error;
        }
    }
  
  if (QccWAVSubbandPyramidDWTSQEncode(image,
                                      channels,
                                      quantizer,
                                      &image_mean,
                                      num_levels,
                                      wavelet,
                                      perceptual_weights))
    {
      QccErrorAddMessage("(QccWAVsrEncode): Error calling QccWAVWaveletSynthesisQuantize()");
      goto Error;
    }

  if (QccWAVsrEncodeHeader(buffer, num_levels,
                           image->num_rows, image->num_cols, 
                           quantizer,
                           image_mean))
    {
      QccErrorAddMessage("(QccWAVsrEncode): Error calling QccWAVsrEncodeHeader()");
      goto Error;
    }
  
  if (QccWAVsrEncodeBitstream(channels,
                              buffer,
                              num_subbands))
    {
      QccErrorAddMessage("(QccWAVsrEncode): Error calling QccWAVsrEncodeBitstream");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (channels != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccChannelFree(&channels[subband]);
      QccFree(channels);
    }
  return(return_value);
}


static int QccWAVsrDecodeInputRunLength(QccBitBuffer *buffer,
                                        int *run_length,
                                        int *first_stack_bit,
                                        QccENTArithmeticModel *model)
{
  int symbol;
  int retain_msb = 1;
  int mask = 1;
  int val;

  if (QccENTArithmeticSetModelContext(model, QCCWAVSR_RUNCONTEXT))
    {
      QccErrorAddMessage("(QccWAVsrDecodeInputRunLength): Error calling QccENTArithmeticSetModelContext()");
      return(1);
    }

  *run_length = 0;

  do
    {
      val = QccENTArithmeticDecode(buffer,
                                   model,
                                   &symbol, 1);
      if (val)
        {
          if (val == 2)
            {
              *run_length = QCCWAVSR_EOF;
              return(0);
            }
          else
            {
              QccErrorAddMessage("(QccWAVsrDecodeInputRunLength): Error calling QccENTArithmeticDecode()");
              return(1);
            }
        }

      if (symbol == QCCWAVSR_PLUS)
        {
          *run_length |= mask;
          mask <<= 1;
        }
      else
        if (symbol == QCCWAVSR_MINUS)
          {
            retain_msb = 0;
            mask <<= 1;
          }
    }
  while ((symbol == QCCWAVSR_PLUS) || (symbol == QCCWAVSR_MINUS));

  if (!retain_msb)
    *run_length |= mask;

  *first_stack_bit = (symbol == QCCWAVSR_ONE);

  return(0);
}


static int QccWAVsrDecodeInputStackValue(QccBitBuffer *buffer,
                                         int *stack_value,
                                         QccENTArithmeticModel *model)
{
  int symbol;
  int mask = 2;

  if (QccENTArithmeticSetModelContext(model, QCCWAVSR_STACKCONTEXT))
    {
      QccErrorAddMessage("(QccWAVsrDecodeInputStackValue): Error calling QccENTArithmeticSetModelContext()");
      return(1);
    }

  do
    {
      if (QccENTArithmeticDecode(buffer,
                                 model,
                                 &symbol, 1))
        {
          QccErrorAddMessage("(QccWAVsrDecodeInputStackValue): Error calling QccENTArithmeticDecode()");
          return(1);
        }
      if (symbol == QCCWAVSR_ONE)
        {
          *stack_value |= mask;
          mask <<= 1;
        }
      else
        if (symbol == QCCWAVSR_ZERO)
          mask <<= 1;
    }
  while ((symbol == QCCWAVSR_ONE) || (symbol == QCCWAVSR_ZERO));

  *stack_value |= mask;
  (*stack_value)--;
  if (symbol == QCCWAVSR_MINUS)
    *stack_value *= -1;

  return(0);
}


static int QccWAVsrDecodeBitstream(QccBitBuffer *buffer,
                                   QccChannel *channels,
                                   int num_subbands)
{
  int return_value;
  int subband;
  int symbol;
  int run_length;
  int stack_value;
  QccENTArithmeticModel *model = NULL;
  int num_symbols[2] = { 4, 4 };

  for (subband = 0; subband < num_subbands; subband++)
    for (symbol = 0; symbol < channels[subband].channel_length; symbol++)
      channels[subband].channel_symbols[symbol] = 0;

  if ((model = QccENTArithmeticDecodeStart(buffer,
                                           num_symbols, 2,
                                           NULL,
                                           QCCENT_ANYNUMBITS)) ==
      NULL)
    {
      QccErrorAddMessage("(QcCWAVsrDecodeBitstream): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }

  run_length = 0;
  if (QccWAVsrDecodeInputRunLength(buffer, &run_length, &stack_value,
                                   model))
    {
      QccErrorAddMessage("(QcCWAVsrDecodeBitstream): Error calling QccWAVsrDecodeInputRunLength()");
      goto Error;
    }

  if (run_length == QCCWAVSR_EOF)
    goto QccFinished;
  for (subband = 0; subband < num_subbands; subband++)
    for (symbol = 0; symbol < channels[subband].channel_length; symbol++)
      if (run_length)
        run_length--;
      else
        {
          if (QccWAVsrDecodeInputStackValue(buffer, &stack_value, model))
            {
              QccErrorAddMessage("(QcCWAVsrDecodeBitstream): Error calling QccWAVsrDecodeInputStackValue()");
              goto Error;
            }
          channels[subband].channel_symbols[symbol] = stack_value;
          if (QccWAVsrDecodeInputRunLength(buffer, &run_length, &stack_value,
                                           model))
            {
              QccErrorAddMessage("(QcCWAVsrDecodeBitstream): Error calling QccWAVsrDecodeInputrunLength()");
              goto Error;
            }
          if (run_length == QCCWAVSR_EOF)
            goto QccFinished;
        }

 QccFinished:
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccENTArithmeticFreeModel(model);
  return(return_value);
}


int QccWAVsrDecodeHeader(QccBitBuffer *buffer, 
                         int *num_levels, 
                         int *num_rows, int *num_cols,
                         QccSQScalarQuantizer *quantizer,
                         double *image_mean)
{
  unsigned char ch;

  if (buffer == NULL)
    return(0);
  if (buffer->fileptr == NULL)
    return(0);
  
  if (QccBitBufferGetChar(buffer,
                          &ch))
    {
      QccErrorAddMessage("(QccWAVsrDecodeHeader): Error calling QccBitBufferGetChar()");
      return(1);
    }
  if (num_levels != NULL)
    *num_levels = ch;
  
  if (QccBitBufferGetInt(buffer,
                         num_rows))
    {
      QccErrorAddMessage("(QccWAVsrDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  
  if (QccBitBufferGetInt(buffer,
                         num_cols))
    {
      QccErrorAddMessage("(QccWAVsrDecodeHeader): Error calling QccBitBufferGetInt()");
      return(1);
    }
  
  if (QccBitBufferGetChar(buffer,
                          &ch))
    {
      QccErrorAddMessage("(QccWAVsrDecodeHeader): Error calling QccBitBufferGetChar()");
      return(1);
    }
  quantizer->type = ch;
  
  if (QccBitBufferGetDouble(buffer,
                            &(quantizer->stepsize)))
    {
      QccErrorAddMessage("(QccWAVsrDecodeHeader): Error calling QccBitBufferGetDouble()");
      return(1);
    }

  if (quantizer->type == QCCSQSCALARQUANTIZER_DEADZONE)
    if (QccBitBufferGetDouble(buffer,
                              &(quantizer->deadzone)))
      {
        QccErrorAddMessage("(QccWAVsrDecodeHeader): Error calling QccBitBufferGetDouble()");
        return(1);
      }

  if (QccBitBufferGetDouble(buffer,
                            image_mean))
    {
      QccErrorAddMessage("(QccWAVsrDecodeHeader): Error calling QccBitBufferGetDouble()");
      return(1);
    }

  return(0);
}


int QccWAVsrDecode(QccBitBuffer *buffer,
                   QccIMGImageComponent *image,
                   const QccSQScalarQuantizer *quantizer,
                   int num_levels,
                   const QccWAVWavelet *wavelet,
                   const QccWAVPerceptualWeights *perceptual_weights,
                   double image_mean)
{
  int return_value;
  int num_subbands;
  QccChannel *channels = NULL;
  int subband;
  int subband_num_rows, subband_num_cols;

  if (buffer == NULL)
    return(0);
  if (image == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);

  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);

  if ((quantizer->type != QCCSQSCALARQUANTIZER_UNIFORM) &&
      (quantizer->type != QCCSQSCALARQUANTIZER_DEADZONE))
    {
      QccErrorAddMessage("(QccWAVsrDecode): Only uniform or dead-zone type quantizers supported");
      goto Error;
    }
  if (quantizer->stepsize <= 0)
    {
      QccErrorAddMessage("(QccWAVsrDecode): Step size must be > 0");
      goto Error;
    }

  if ((channels = 
       (QccChannel *)malloc(sizeof(QccChannel) * num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVsrDecode): Error allocating memory");
      goto Error;
    }

  for (subband = 0; subband < num_subbands; subband++)
    {
      QccWAVSubbandPyramid subband_pyramid;
      subband_pyramid.num_levels = num_levels;
      subband_pyramid.num_rows = image->num_rows;
      subband_pyramid.num_cols = image->num_cols;

      QccChannelInitialize(&channels[subband]);
      if (QccWAVSubbandPyramidSubbandSize(&subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVsrDecode): Error calling QccWAVSubbandPyramidSubbandSize");
          goto Error;
        }
      channels[subband].channel_length =
        subband_num_rows * subband_num_cols;
      channels[subband].access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
      if (QccChannelAlloc(&channels[subband]))
        {
          QccErrorAddMessage("(QccWAVsrDecode): Error calling QccChannelAlloc()");
          goto Error;
        }
    }

  if (QccWAVsrDecodeBitstream(buffer,
                              channels,
                              num_subbands))
    {
      QccErrorAddMessage("(QccWAVsrDecode): Error calling QccWAVsrDecodeBitstream()");
      goto Error;
    }


  if (QccWAVSubbandPyramidDWTSQDecode(channels,
                                      image,
                                      quantizer,
                                      image_mean,
                                      num_levels,
                                      wavelet,
                                      perceptual_weights))
    {
      QccErrorAddMessage("(QccWAVsrDecode): Error calling QccWAVSubbandPyramidDWTSQDecode()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (channels != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccChannelFree(&channels[subband]);
      QccFree(channels);
    }
  return(return_value);
}
