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


int QccENTGolombEncodeChannel(const QccChannel *channel,
                              QccBitBuffer *output_buffer,
                              const float *p,
                              const int *m)
{
  int return_value;
  int symbol;
  
  if (channel == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  
  if (channel->channel_symbols == NULL)
    return(0);
  
  for (symbol = 0; symbol < QccChannelGetBlockSize(channel); symbol++)
    if ((channel->channel_symbols[symbol] != QCCENTGOLOMB_RUN_SYMBOL) &&
        (channel->channel_symbols[symbol] != QCCENTGOLOMB_STOP_SYMBOL))
      {
        QccErrorAddMessage("(QccENTGolombEncodeChannel): Invalid symbol encountered in channel");
        goto Error;
      }
  
  if (QccENTGolombEncode(output_buffer,
                         channel->channel_symbols,
                         QccChannelGetBlockSize(channel), p, m))
    {
      QccErrorAddMessage("(QccENTGolombEncodeChannel): Error calling QccENTGolombEncode()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
  
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccENTGolombDecodeChannel(QccBitBuffer *input_buffer,
                              const QccChannel *channel)
{
  int return_value;
  
  if (QccENTGolombDecode(input_buffer,
                         channel->channel_symbols,
                         QccChannelGetBlockSize(channel)))
    {
      QccErrorAddMessage("(QccENTGolombDecodeChannel): Error calling QccENTGolombDecode()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}
