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


static int QccENTArithmeticEncodeChannelSecondOrderContext(const
                                                           int *symbol_stream,
                                                           int current_symbol)
{
  if ((symbol_stream == NULL) || (current_symbol <= 0))
    return(0);

  return(symbol_stream[current_symbol - 1]);
}


int QccENTArithmeticEncodeChannel(const QccChannel *channel,
                                  QccBitBuffer *buffer,
                                  int order)
{
  int return_value;
  int num_contexts;
  int *num_symbols;
  int context;
  QccENTArithmeticGetContext context_function = NULL;
  QccENTArithmeticModel *model = NULL;

  if (channel == NULL)
    return(0);
  if (buffer == NULL)
    return(0);

  if (channel->channel_symbols == NULL)
    return(0);

  num_contexts = (order == 2) ? channel->alphabet_size : 1;
  context_function = (order == 2) ?
    QccENTArithmeticEncodeChannelSecondOrderContext : NULL;
  if ((num_symbols = (int *)malloc(sizeof(int)*num_contexts)) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticEncodeChannel): Error allocating memory");
      goto Error;
    }
  for (context = 0; context < num_contexts; context++)
    num_symbols[context] = channel->alphabet_size;

  if ((model = QccENTArithmeticEncodeStart(num_symbols,
                                           num_contexts,
                                           context_function,
                                           QCCENT_ANYNUMBITS)) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticEncodeChannel): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }

  if (QccENTArithmeticEncode(channel->channel_symbols, 
                             channel->channel_length,
                             model,
                             buffer))
    {
      QccErrorAddMessage("(QccENTArithmeticEncodeChannel): Error calling QccENTArithmeticEncode()");
      goto Error;
    }

  context = (model->context_function != NULL) ?
    model->context_function(channel->channel_symbols,
                            channel->channel_length) : 0;

  if (QccENTArithmeticEncodeEnd(model,
                                context,
                                buffer))
    {
      QccErrorAddMessage("(QccENTArithmeticEncodeChannel): Error calling QccENTArithmeticEncodeEnd()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (num_symbols != NULL)
    QccFree(num_symbols);
  if (model != NULL)
    QccENTArithmeticFreeModel(model);
  
  return(return_value);
}


int QccENTArithmeticDecodeChannel(QccBitBuffer *buffer, QccChannel *channel,
                                  int order)
{
  int return_value;
  int num_contexts;
  int *num_symbols;
  int context;
  QccENTArithmeticGetContext context_function = NULL;
  QccENTArithmeticModel *model = NULL;

  if (buffer == NULL)
    return(0);
  if (channel == NULL)
    return(0);

  if (channel->channel_symbols == NULL)
    return(0);

  num_contexts = (order == 2) ? channel->alphabet_size : 1;
  context_function = (order == 2) ?
    QccENTArithmeticEncodeChannelSecondOrderContext : NULL;
  if ((num_symbols = (int *)malloc(sizeof(int)*num_contexts)) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticDecodeChannel): Error allocating memory");
      goto Error;
    }
  for (context = 0; context < num_contexts; context++)
    num_symbols[context] = channel->alphabet_size;

  if ((model = QccENTArithmeticDecodeStart(buffer,
                                           num_symbols,
                                           num_contexts,
                                           context_function,
                                           QCCENT_ANYNUMBITS)) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticDecodeChannel): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }

  if (QccENTArithmeticDecode(buffer,
                             model,
                             channel->channel_symbols,
                             channel->channel_length))
    {
      QccErrorAddMessage("(QccENTArithmeticDecodeChannel): Error calling QccENTArithmeticDecode()");
      goto Error;
    }

  return_value = 0;
  goto Return;

 Error:
  return_value = 1;

 Return:
  if (num_symbols != NULL)
    QccFree(num_symbols);
  if (model != NULL)
    QccENTArithmeticFreeModel(model);
  return(return_value);
}
