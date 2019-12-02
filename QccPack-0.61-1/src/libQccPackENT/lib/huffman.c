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


int QccENTHuffmanEncode(QccBitBuffer *output_buffer,
                        const int *symbols,
                        int num_symbols,
                        const QccENTHuffmanTable *table)
{
  int symbol;

  if (output_buffer == NULL)
    return(0);
  if (symbols == NULL)
    return(0);
  if (table == NULL)
    return(0);
  if (table->table == NULL)
    return(0);

  if (table->table_type != QCCENTHUFFMAN_ENCODETABLE)
    {
      QccErrorAddMessage("(QccENTHuffmanEncode): Table must be converted to an encode-type table first");
      return(1);
    }

  for (symbol = 0; symbol < num_symbols; symbol++)
    {
      if ((symbols[symbol] < 0) ||
          (symbols[symbol] > QCCENTHUFFMAN_MAXSYMBOL))
        {
          QccErrorAddMessage("(QccENTHuffmanEncode): Invalid symbol encountered");
          return(1);
        }

      if (symbols[symbol] >= table->table_length)
        {
          QccErrorAddMessage("(QccENTHuffmanEncode): Symbol does not have a valid codeword in Huffman table (invalid symbol?)");
          return(1);
        }

      if (table->table[symbols[symbol]].codeword.length <= 0)
        {
          QccErrorAddMessage("(QccENTHuffmanEncode): Symbol does not have a valid codeword in Huffman table (invalid symbol?)");
          return(1);
        }

      if (QccENTHuffmanCodewordWrite(output_buffer,
                                     &(table->table[symbols[symbol]].codeword)))
        {
          QccErrorAddMessage("(QccENTHuffmanEncode): Error calling QccENTHuffmanCodewordWrite()");
          return(1);
        }
    }
    
  return(0);
}


int QccENTHuffmanDecode(QccBitBuffer *input_buffer,
                        int *symbols,
                        int num_symbols,
                        const QccENTHuffmanTable *table)
{
  int symbol;
  int bit;
  int current_code;
  int current_length;
  int ptr;

  if (input_buffer == NULL)
    return(0);
  if (symbols == NULL)
    return(0);
  if (table == NULL)
    return(0);
  if (table->table == NULL)
    return(0);

  if (table->table_type != QCCENTHUFFMAN_DECODETABLE)
    {
      QccErrorAddMessage("(QccENTHuffmanDecode): Table must be a decode-type table");
      return(1);
    }

  for (symbol = 0; symbol < num_symbols; symbol++)
    {
      current_length = 0;
      current_code = 0;
      do
        {
          if (QccBitBufferGetBit(input_buffer, &bit))
            {
              QccErrorAddMessage("(QccENTHuffmanDecode): Error calling QccBitBufferGetBit()");
              return(1);
            }
          current_length++;
          current_code <<= 1;
          current_code |= bit;
          if (current_length > QCCENTHUFFMAN_MAXCODEWORDLEN)
            {
              QccErrorAddMessage("(QccENTHuffmanDecode): Error decoding Huffman codeword - corrupted bitstream?");
              return(1);
            }
        }
      while (current_code > table->codeword_max[current_length - 1]);

      ptr = table->codeword_ptr[current_length - 1] + current_code -
        table->codeword_min[current_length - 1];
      symbols[symbol] =
        table->table[ptr].symbol;
    }

  return(0);
}
