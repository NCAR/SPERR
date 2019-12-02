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


int QccENTHuffmanCodewordWrite(QccBitBuffer *output_buffer,
                               const QccENTHuffmanCodeword *codeword)
{
  int bit;
  int mask;

  if (codeword == NULL)
    return(0);

  if (codeword->length < 0)
    {
      QccErrorAddMessage("(QccENTHuffmanCodewordWrite): Invalid codeword");
      return(1);
    }

  mask = (1 << (codeword->length - 1));

  for (bit = 0; bit < codeword->length; bit++, mask >>= 1)
    if (QccBitBufferPutBit(output_buffer,
                           ((codeword->codeword & mask) != 0)))
      {
        QccErrorAddMessage("(QccENTHuffmanCodewordWrite): Error calling QccBitBufferPutBit()");
        return(1);
      }

  return(0);
}


void QccENTHuffmanCodewordPrint(const QccENTHuffmanCodeword *codeword)
{
  int bit;
  int mask;

  if (codeword == NULL)
    return;

  if (codeword->length < 0)
    {
      printf("XXXXXX\n");
      return;
    }

  mask = (1 << (codeword->length - 1));

  for (bit = 0; bit < codeword->length; bit++, mask >>= 1)
    printf("%d",
           ((codeword->codeword & mask) != 0));
  printf("\n");
}
