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


int QccENTHuffmanEncodeChannel(const QccChannel *channel,
                               QccBitBuffer *output_buffer,
                               QccENTHuffmanTable *huffman_table)
{
  int return_value;
  int *symbols = NULL;
  double *probabilities = NULL;
  int symbol;

  if (channel == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);

  if (channel->channel_symbols == NULL)
    return(0);

  if (channel->alphabet_size == QCCCHANNEL_UNDEFINEDSIZE)
    {
      QccErrorAddMessage("(QccENTHuffmanEncodeChannel): Channel alphabet size is undefined - cannot determine number of distinct channel symbols");
      goto Error;
    }

  if (channel->alphabet_size < 0)
    {
      QccErrorAddMessage("(QccENTHuffmanEncodeChannel): Invalid channel alphabet size (%d)",
                         channel->alphabet_size);
      goto Error;
    }

  if ((symbols =
       (int *)malloc(sizeof(int) * channel->alphabet_size)) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanEncodeChannel): Error allocating memory");
      goto Error;
    }
  if ((probabilities =
       (double *)malloc(sizeof(double) * channel->alphabet_size)) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanEncodeChannel): Error allocating memory");
      goto Error;
    }

  for (symbol = 0; symbol < channel->alphabet_size; symbol++)
    {
      symbols[symbol] = symbol;
      probabilities[symbol] = 0;
    }

  for (symbol = 0; symbol < QccChannelGetBlockSize(channel); symbol++)
    {
      if ((channel->channel_symbols[symbol] < 0) ||
          (channel->channel_symbols[symbol] >=
           channel->alphabet_size))
        {
          QccErrorAddMessage("(QccENTHuffmanEncodeChannel): Invalid symbol encountered in channel");
          goto Error;
        }
      probabilities[channel->channel_symbols[symbol]]++;
    }

  for (symbol = 0; symbol < channel->alphabet_size; symbol++)
    probabilities[symbol] /= QccChannelGetBlockSize(channel);

  huffman_table->table_type = QCCENTHUFFMAN_ENCODETABLE;
  if (QccENTHuffmanDesign(symbols,
                          probabilities,
                          channel->alphabet_size,
                          huffman_table))
    {
      QccErrorAddMessage("(QccENTHuffmanEncodeChannel): Error calling QccENTHuffmanDesign()");
      goto Error;
    }

  if (QccENTHuffmanEncode(output_buffer,
                          channel->channel_symbols,
                          QccChannelGetBlockSize(channel),
                          huffman_table))
    {
      QccErrorAddMessage("(QccENTHuffmanEncodeChannel): Error calling QccENTHuffmanEncode()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (symbols != NULL)
    QccFree(symbols);
  if (probabilities != NULL)
    QccFree(probabilities);
  return(return_value);
}


int QccENTHuffmanDecodeChannel(QccBitBuffer *input_buffer,
                               const QccChannel *channel,
                               const QccENTHuffmanTable *huffman_table)
{
  int return_value;

  if (QccENTHuffmanDecode(input_buffer,
                          channel->channel_symbols,
                          QccChannelGetBlockSize(channel),
                          huffman_table))
    {
      QccErrorAddMessage("(QccENTHuffmanDecodeChannel): Error calling QccENTHuffmanDecode()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}

