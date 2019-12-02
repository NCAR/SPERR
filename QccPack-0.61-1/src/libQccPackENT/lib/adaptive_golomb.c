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


static int QccENTAdaptiveGolombPutBits(QccBitBuffer *bit_buffer,
                                       unsigned int count, unsigned int k)
{
  unsigned int i, bit_value, mask;
  
  if (bit_buffer == NULL)
    return(0);
  
  if (bit_buffer->type != QCCBITBUFFER_OUTPUT)
    {
      QccErrorAddMessage("(QccENTAdaptiveGolombPutBits): Not an output buffer");
      return(1);
    }
  
  mask = 1 << (k - 1);
  
  for (i = 0; i < k; i++)
    {
      if (count & mask)
        bit_value = 1;
      else
        bit_value = 0;

      mask >>= 1;
    
      if (QccBitBufferPutBit(bit_buffer, (int)bit_value))
        {
          QccErrorAddMessage("(QccENTAdaptiveGolombPutBits): Error calling QccBitBufferPutBit()");
          return(1);
        }
    }

  return(0);
}


static int QccENTAdaptiveGolombGetBits(QccBitBuffer *bit_buffer,
                                       unsigned int *count,
                                       unsigned int k)
{
  unsigned int i, bit_value;
  
  if (bit_buffer == NULL)
    return(0);
  
  if (bit_buffer->type != QCCBITBUFFER_INPUT)
    {
      QccErrorAddMessage("(QccENTAdaptiveGolombPutBits): Not an input buffer");
      return(1);
    }
  
  for (i = 0, *count = 0; i < k; i++)
    {
      if (QccBitBufferGetBit(bit_buffer, (int *)(&bit_value)))
        {
          QccErrorAddMessage("(QccENTAdaptiveGolombGetBits): Error calling QccBitBufferGetBit()");
          return(1);
        }

      *count = (*count << 1) | bit_value;
    }

  return(0);
}


int QccENTAdaptiveGolombEncode(QccBitBuffer *output_buffer,
                               const int *symbols, 
                               int num_symbols)
{
  int symbol;
  unsigned count, k, limit, value;
  
  if (output_buffer == NULL)
    return(0);
  if (symbols == NULL)
    return(0);
  
  for (count = 0, limit = 1, k = 0, symbol = 0;
       symbol < num_symbols;
       symbol++)
    if (symbols[symbol] == QCCENTADAPTIVEGOLOMB_RUN_SYMBOL)
      {
        if (k == 0)
          {
            value = 1;
            if (QccBitBufferPutBit(output_buffer, (int)value))
              {
                QccErrorAddMessage("(QccENTAdaptiveGolombEncode): Error calling QccBitBufferPutBit()");
                return(1);
              }
            k++;
            limit = ((unsigned int) 1) << k;
            count = 0;
          }
        else
          {
            count++;
            if (count == limit)
              {
                value = 1;
                if (QccBitBufferPutBit(output_buffer, (int)value))
                  {
                    QccErrorAddMessage("(QccENTAdaptiveGolombEncode): Error calling QccBitBufferPutBit()");
                    return(1);
                  }
                if (k < QCCENTADAPTIVEGOLOMB_KMAX)
                  {
                    k++;
                    limit = ((unsigned int) 1) << k;
                  }
                count = 0;
              }
          }
      }
    else
      if (symbols[symbol] == QCCENTADAPTIVEGOLOMB_STOP_SYMBOL)
        {
          value = 0;
          if (QccBitBufferPutBit(output_buffer, (int)value))
            {
              QccErrorAddMessage("(QccENTAdaptiveGolombEncode): Error calling QccBitBufferPutBit()");
              return(1);
            }
          if (QccENTAdaptiveGolombPutBits(output_buffer, count, k))
            {
              QccErrorAddMessage("(QccENTAdaptiveGolombEncode): Error calling QccENTAdaptiveGolombPutBits()");
              return(1);
            }
          count = 0;
          if(k != 0)
            {
              k--;
              limit = ((unsigned int) 1) << k;
            }
        }
      else
        {
          QccErrorAddMessage("(QccENTAdaptiveGolombEncode): Invalid symbol encountered");
          return(1);
        }

  return(0);
}


int QccENTAdaptiveGolombDecode(QccBitBuffer *input_buffer,
                               int *symbols, 
                               int num_symbols)
{
  int symbol;
  unsigned int i, count, k, limit, value;
  
  if (input_buffer == NULL)
    return(0);
  if (symbols == NULL)
    return(0);
  
  count = 0;
  k = 0;
  symbol = 0;

  while (symbol < num_symbols)
    {
      if (QccBitBufferGetBit(input_buffer, (int *)(&value)))
        {
          QccErrorAddMessage("(QccENTAdaptiveGolombDecode): Error calling QccBitBufferGetBit()");
          return(1);
        }
    if (value == 1)
      {
        limit = ((unsigned int) 1) << k;
        for (i = 0; i < limit; i++)
          {
            symbols[symbol] = QCCENTADAPTIVEGOLOMB_RUN_SYMBOL;
            symbol++;
          }
        if (k < QCCENTADAPTIVEGOLOMB_KMAX)
          k++;
        count = 0;
      }
    else
      {
        if (QccENTAdaptiveGolombGetBits(input_buffer, &count, k))
          {
            QccErrorAddMessage("(QccENTAdaptiveGolombDecode): Error calling QccENTAdaptiveGolombGetBits()");
            return(1);
          }
      for (i = 0; i < count; i++)
        {
          symbols[symbol] = QCCENTADAPTIVEGOLOMB_RUN_SYMBOL;
          symbol++;
        }
      symbols[symbol] = QCCENTADAPTIVEGOLOMB_STOP_SYMBOL;
      symbol++;
      if (k > 0)
        k--;
      }
    }

  return(0);
}
