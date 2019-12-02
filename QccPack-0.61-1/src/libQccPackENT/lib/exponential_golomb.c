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


static int QccENTExponentialGolombConvertFromSigned(int symbol)
{
  if (symbol == 0)
    return(0);

  if (symbol > 0)
    return(2 * symbol - 1);
  else
    return(2 * (-symbol));
}


static int QccENTExponentialGolombConvertToSigned(int symbol)
{
  int sign;

  if (symbol == 0)
    return(0);

  sign = (symbol & 1) ? 1 : -1;

  return(((symbol + 1) / 2) * sign);
}


static int QccENTExponentialGolombEncodePrefix(QccBitBuffer *output_buffer,
                                               int symbol,
                                               int *num_bits)
{
  int code;
  int cnt;

  *num_bits = 0;
  code = symbol + 1;
  while (code)
    {
      code >>= 1;
      (*num_bits)++;
    }

  (*num_bits)--;

  for (cnt = *num_bits; cnt; cnt--)
    if (QccBitBufferPutBit(output_buffer, 0))
      {
        QccErrorAddMessage("(QccENTExponentialGolombEncodePrefix): Error calling QccBitBufferPutBit()");
        return(1);
      }

  if (QccBitBufferPutBit(output_buffer, 1))
    {
      QccErrorAddMessage("(QccENTExponentialGolombEncodePrefix): Error calling QccBitBufferPutBit()");
      return(1);
    }

  return(0);
}


static int QccENTExponentialGolombDecodePrefix(QccBitBuffer *input_buffer,
                                               int *symbol,
                                               int *num_bits)
{
  int bit_value;

  for (*num_bits = -1, bit_value = 0;
       !bit_value;
       (*num_bits)++)
    if (QccBitBufferGetBit(input_buffer,
                           &bit_value))
      {
        QccErrorAddMessage("(QccENTExponentialGolombDecodePrefix): Error calling QccBitBufferGetBit()");
        return(1);
      }

  *symbol = (1 << (*num_bits)) - 1;

  return(0);
}


static int QccENTExponentialGolombEncodeSuffix(QccBitBuffer *output_buffer,
                                               int symbol,
                                               int num_bits)
{
  if (!num_bits)
    return(0);

  if (QccBitBufferPutBits(output_buffer,
                          symbol + 1,
                          num_bits))
    {
      QccErrorAddMessage("(QccENTExponentialGolombEncodeSuffix): Error calling QccBitBufferPutBits()");
      return(1);
    }

  return(0);
}


static int QccENTExponentialGolombDecodeSuffix(QccBitBuffer *input_buffer,
                                               int *symbol,
                                               int num_bits)
{
  int suffix = 0;

  if (!num_bits)
    return(0);

  if (QccBitBufferGetBits(input_buffer,
                          &suffix,
                          num_bits))
    {
      QccErrorAddMessage("(QccENTExponentialGolombDecodeSuffix): Error calling QccBitBufferGetBits()");
      return(1);
    }

  *symbol += suffix;

  return(0);
}


int QccENTExponentialGolombEncodeSymbol(QccBitBuffer *output_buffer,
                                        int symbol,
                                        int signed_symbol)
{
  int num_bits;

  if (signed_symbol)
    symbol = QccENTExponentialGolombConvertFromSigned(symbol);
  
  if (QccENTExponentialGolombEncodePrefix(output_buffer,
                                          symbol,
                                          &num_bits))
    {
      QccErrorAddMessage("(QccENTExponentialGolombEncodeSymbol): Error calling QccENTExponentialGolombEncodePrefix()");
      return(1);
    }

  if (QccENTExponentialGolombEncodeSuffix(output_buffer,
                                          symbol,
                                          num_bits))
    {
      QccErrorAddMessage("(QccENTExponentialGolombEncodeSymbol): Error calling QccENTExponentialGolombEncodeSuffix()");
      return(1);
    }

  return(0);
}


int QccENTExponentialGolombDecodeSymbol(QccBitBuffer *input_buffer,
                                        int *symbol,
                                        int signed_symbol)
{
  int num_bits;

  if (QccENTExponentialGolombDecodePrefix(input_buffer,
                                          symbol,
                                          &num_bits))
    {
      QccErrorAddMessage("(QccENTExponentialGolombDecodeSymbol): Error calling QccENTExponentialGolombDecodePrefix()");
      return(1);
    }

  if (QccENTExponentialGolombDecodeSuffix(input_buffer,
                                          symbol,
                                          num_bits))
    {
      QccErrorAddMessage("(QccENTExponentialGolombDecodeSymbol): Error calling QccENTExponentialGolombDecodeSuffix()");
      return(1);
    }

  if (signed_symbol)
    *symbol = QccENTExponentialGolombConvertToSigned(*symbol);

  return(0);
}


int QccENTExponentialGolombEncode(QccBitBuffer *output_buffer,
                                  const int *symbols,
                                  int num_symbols,
                                  int signed_symbols)
{
  int index;

  for (index = 0; index < num_symbols; index++)
    if (QccENTExponentialGolombEncodeSymbol(output_buffer,
                                            symbols[index],
                                            signed_symbols))
      {
        QccErrorAddMessage("(QccENTExponentialGolombEncode): Error calling QccENTExponentialGolombEncodeSymbol()");
        return(1);
      }

  return(0);
}


int QccENTExponentialGolombDecode(QccBitBuffer *input_buffer,
                                  int *symbols, 
                                  int num_symbols,
                                  int signed_symbols)
{
  int index;

  for (index = 0; index < num_symbols; index++)
    if (QccENTExponentialGolombDecodeSymbol(input_buffer,
                                            &symbols[index],
                                            signed_symbols))
      {
        QccErrorAddMessage("(QccENTExponentialGolombDecode): Error calling QccENTExponentialGolombDecodeSymbol()");
        return(1);
      }

  return(0);
}
