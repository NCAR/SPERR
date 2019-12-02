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


static int QccENTGolombPutBits(QccBitBuffer *bit_buffer,
                               unsigned int count,
                               unsigned int k)
{
  unsigned int i, bit_value, mask;
  
  if (bit_buffer == NULL)
    return(0);
  
  if (bit_buffer->type != QCCBITBUFFER_OUTPUT)
    {
      QccErrorAddMessage("(QccENTGolombPutBits): Not an output buffer");
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
          QccErrorAddMessage("(QccENTGolombPutBits): Error calling QccBitBufferPutBit()");
          return(1);
        }
    }
  
  return(0);
}


static int QccENTGolombGetBits(QccBitBuffer *bit_buffer,
                               unsigned int *count,
                               unsigned int k)
{
  unsigned int i, bit_value;
  
  if (bit_buffer == NULL)
    return(0);
  
  if (bit_buffer->type != QCCBITBUFFER_INPUT)
    {
      QccErrorAddMessage("(QccENTGolombPutBits): Not an input buffer");
      return(1);
    }
  
  for (i = 0, *count = 0; i < k; i++)
    {
      if (QccBitBufferGetBit(bit_buffer, (int *)(&bit_value)))
        {
          QccErrorAddMessage("(QccENTGolombGetBits): Error calling QccBitBufferGetBit()");
          return(1);
        }
      
      *count = (*count << 1) | bit_value;
    }
  
  return(0);
}


int QccENTGolombEncode(QccBitBuffer *output_buffer,
                       const int *symbols,
                       int num_symbols,
                       const float *p,
                       const int *m)
{
  int symbol, i;
  unsigned int m2, k, count, shorter_words, ones, rem;
  
  if (output_buffer == NULL)
    return(0);
  if (symbols == NULL)
    return(0);
  
  if (p != NULL)
    {
      if (m != NULL)
        {
          QccErrorAddMessage("(QccENTGolombEncode): p and m should not be specified together");
          return(1);
        }
      if (*p == 0.5)
        {
          QccErrorAddMessage("(QccENTGolombEncode): Nothing to compress when you have p = 0.5");
          return(1);
        }
      if (*p < 0.5)
        {
          QccErrorAddMessage("(QccENTGolombEncode): p = %f < 0.5", *p);
          return(1);
        }
      m2 = (unsigned int)(-1.0 / QccMathLog2(*p));
    }
  else
    if (m == NULL)
      {
        for (count = 0, symbol = 0; symbol < num_symbols; ++symbol)
          if (symbols[symbol] == QCCENTGOLOMB_STOP_SYMBOL)
            ++count;
        if (count) 
          m2 = (unsigned int)(-1.0 / QccMathLog2((num_symbols - count) /
                                                (double)num_symbols));
        else
          {
            QccErrorAddMessage("(QccENTGolombEncode): No Stop symbol present");
            return(1);
          }
      }
    else
      m2 = *m;
  
  count = m2 - 1;
  k = 1;
  while(count)
    {
      count >>= 1;
      k++;
    }
  shorter_words = (1 << (k - 1)) - m2;
  
  if (QccBitBufferPutInt(output_buffer, (int)m2))
    {
      QccErrorAddMessage("(QccENTGolombEncode): Error calling QccBitBufferPutInt()");
      return(1);
    }
  
  for (count = 0, symbol = 0; symbol < num_symbols; ++symbol)
    {
      if (symbols[symbol] == QCCENTGOLOMB_RUN_SYMBOL)
        ++count;
      else
        if (symbols[symbol] == QCCENTGOLOMB_STOP_SYMBOL)
          {
            ones = count / m2;
            rem = count % m2;
            for (i = 0; i < ones; ++i)
              {
                if (QccBitBufferPutBit(output_buffer, 1))
                  {
                    QccErrorAddMessage("(QccENTGolombEncode): Error calling QccBitBufferPutBit()");
                    return(1);
                  }
              }
            if (rem < shorter_words)
              {
                if (QccENTGolombPutBits(output_buffer, rem, k - 1))
                  {
                    QccErrorAddMessage("(QccENTGolombEncode): Error calling QccENTGolombPutBits()");
                    return(1);
                  }
              }
            else
              {
                if (QccENTGolombPutBits(output_buffer, rem + shorter_words, k))
                  {
                    QccErrorAddMessage("(QccENTGolombEncode): Error calling QccENTGolombPutBits()");
                    return(1);
                  }
              }
            count = 0;
          }
        else
          {
            QccErrorAddMessage("(QccENTGolombEncode): Invalid symbol encountered");
            return(1);
          }
    }
  
  if (count)
    {
      ones = count / m2;
      rem = count % m2;
      for (i = 0; i < ones; ++i)
        {
          if (QccBitBufferPutBit(output_buffer, 1))
            {
              QccErrorAddMessage("(QccENTGolombEncode): Error calling QccBitBufferPutBit()");
              return(1);
            }
        }
      if (rem < shorter_words)
        {
          if (QccENTGolombPutBits(output_buffer, rem, k - 1))
            {
              QccErrorAddMessage("(QccENTGolombEncode): Error calling QccENTGolombPutBits()");
              return(1);
            }
        }
      else
        {
          if (QccENTGolombPutBits(output_buffer, rem + shorter_words, k))
            {
              QccErrorAddMessage("(QccENTGolombEncode): Error calling QccENTGolombPutBits()");
              return(1);
            }
        }
    }
  
  return(0);
}


int QccENTGolombDecode(QccBitBuffer *input_buffer,
                       int *symbols,
                       int num_symbols)
{
  int symbol, i, m, value;
  unsigned int count, k, shorter_words, ones, run_length;
  
  if (input_buffer == NULL)
    return(0);
  if (symbols == NULL)
    return(0);
  
  if (QccBitBufferGetInt(input_buffer, &m))
    {
      QccErrorAddMessage("(QccENTGolombDecodeChannel): Error calling QccBitBufferGetInt()");
      return(1);
    }
  
  symbol = 0;
  ones = 0;
  run_length = 0;
  count = m - 1;
  k = 1;
  while(count)
    {
      count >>= 1;
      k++;
    }
  shorter_words = (1 << (k - 1)) - m;
  count = 0;
  
  while(symbol < num_symbols)
    {
      if (QccBitBufferGetBit(input_buffer, &value))
        {
          QccErrorAddMessage("(QccENTGolombDecode): Error calling QccBitBufferGetBit()");
          return(1);
        }
      
      if (value == 1)
        ++ones;
      else
        {
          if (QccENTGolombGetBits(input_buffer,
                                  &run_length,
                                  (k == 1) ? 0 : k - 2))
            {
              QccErrorAddMessage("(QccENTGolombDecode): Error calling QccENTGolombGetBits()");
              return(1);
            }
          
          if ((run_length > shorter_words - 1 || !shorter_words) && (k != 1))
            {
              if (QccBitBufferGetBit(input_buffer, &value))
                {
                  QccErrorAddMessage("(QccENTGolombDecode): Error calling QccBitBufferGetBit()");
                  return(1);
                }
              
              run_length = (run_length << 1) + value;
              run_length -= shorter_words;
            }
          run_length += m * ones;
          for (i = 0; i < run_length; ++i)
            symbols[symbol++] = QCCENTGOLOMB_RUN_SYMBOL;
          if (symbol < num_symbols)
            symbols[symbol++] = QCCENTGOLOMB_STOP_SYMBOL;
          ones = 0;
          run_length = 0;
        }
    }
  
  return(0);
}
