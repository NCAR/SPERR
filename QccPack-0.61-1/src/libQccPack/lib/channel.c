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


int QccChannelInitialize(QccChannel *channel)
{
  if (channel == NULL)
    return(0);

  QccStringMakeNull(channel->filename);
  channel->fileptr = NULL;
  QccStringCopy(channel->magic_num, QCCCHANNEL_MAGICNUM);
  QccGetQccPackVersion(&channel->major_version,
                       &channel->minor_version,
                       NULL);
  channel->channel_length = 0;
  channel->access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
  channel->num_blocks_accessed = 0;
  channel->alphabet_size = 0;
  channel->channel_symbols = NULL;

  return(0);
}


int QccChannelAlloc(QccChannel *channel)
{
  int block_size;

  if (channel == NULL)
    return(0);

  block_size = QccChannelGetBlockSize(channel);

  if (block_size == 0)
    return(0);

  if (channel->channel_symbols == NULL)
    if ((channel->channel_symbols = 
         (int *)malloc(sizeof(int) * block_size)) == NULL)
      {
        QccErrorAddMessage("(QccChannelAlloc): Error allocating memory");
        return(1);
      }
      
  return(0);
}


void QccChannelFree(QccChannel *channel)
{
  if (channel == NULL)
    return;

  if (channel->channel_symbols != NULL)
    {
      QccFree(channel->channel_symbols);
      channel->channel_symbols = NULL;
    }
}


int QccChannelGetBlockSize(const QccChannel *channel)
{
  int block_size;

  if (channel == NULL)
    return(0);

  block_size = 
    (channel->access_block_size == QCCCHANNEL_ACCESSWHOLEFILE) ?
    channel->channel_length : channel->access_block_size;
  
  return(block_size);
}


int QccChannelPrint(const QccChannel *channel)
{
  int symbol;
  int block_size;
  
  if (QccFilePrintFileInfo(channel->filename,
                           channel->magic_num,
                           channel->major_version,
                           channel->minor_version))
    return(1);

  printf("Num of symbols: %d\n", channel->channel_length);
  printf("Alphabet size: %d\n", channel->alphabet_size);
  block_size = QccChannelGetBlockSize(channel);
  if (channel->access_block_size == QCCCHANNEL_ACCESSWHOLEFILE)
    printf("Access block size: whole file\n");
  else
    printf("Access block size: %d symbols\n",
           block_size);
  printf("Number of blocks accessed so far: %d\n",
         channel->num_blocks_accessed);
  if (channel->num_blocks_accessed)
    {
      printf("\nCurrent block:\nIndex\tSymbol\n\n");
      
      for (symbol = 0; symbol < block_size; symbol++)
        if (channel->channel_symbols[symbol] != QCCCHANNEL_NULLSYMBOL)
          printf("%5d\t%6d\n", symbol, channel->channel_symbols[symbol]);
        else
          printf("%5d\t-- NULL --\n", symbol);
    }
  fflush(stdout);
  return(0);
}


int QccChannelReadWholefile(QccChannel *channel)
{
  if (channel == NULL)
    return(0);

  channel->access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  
  if (QccChannelStartRead(channel))
    {
      QccErrorAddMessage("(QccChannelReadWholefile): Error calling QccChannelStartRead()");
      return(1);
    }
  if (QccChannelReadBlock(channel))
    {
      QccErrorAddMessage("(QccChannelReadWholefile): Error calling QccChannelReadBlock()");
      return(1);
    }

  QccFileClose(channel->fileptr);

  return(0);
}


int QccChannelReadHeader(QccChannel *channel)
{
  if (QccFileReadMagicNumber(channel->fileptr,
                             channel->magic_num,
                             &channel->major_version,
                             &channel->minor_version))
    {
      QccErrorAddMessage("(QccChannelReadHeader): Error reading magic number in channel %s",
                         channel->filename);
      return(1);
    }

  if (strcmp(channel->magic_num, QCCCHANNEL_MAGICNUM))
    {
      QccErrorAddMessage("(QccChannelReadHeader): %s is not of channel (%s) type",
                         channel->filename, QCCCHANNEL_MAGICNUM);
      return(1);
    }

  fscanf(channel->fileptr, "%d", &(channel->channel_length));
  if (ferror(channel->fileptr) || feof(channel->fileptr))
    {
      QccErrorAddMessage("(QccChannelReadHeader): Error reading number of symbols in channel %s",
                         channel->filename);
      return(1);
    }
  
  fscanf(channel->fileptr, "%d", &(channel->alphabet_size));
  if (ferror(channel->fileptr) || feof(channel->fileptr))
    {
      QccErrorAddMessage("(QccChannelReadHeader): Error reading alphabet size in channel %s",
                         channel->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(channel->fileptr, 0))
    {
      QccErrorAddMessage("(QccChannelReadHeader): Error reading number of symbols in channel %s",
                         channel->filename);
      return(1);
    }

  return(0);
}


int QccChannelStartRead(QccChannel *channel)
{
  int block_size;

  if (channel == NULL)
    return(0);

  if ((channel->fileptr = QccFileOpen(channel->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccChannelRead): Error opening %s for reading",
                         channel->filename);
      return(1);
    }

  if (QccChannelReadHeader(channel))
    {
      QccErrorAddMessage("(QccChannelRead): Error reading header of %s",
                         channel->filename);
      return(1);
    }

  block_size = QccChannelGetBlockSize(channel);
    
  if (block_size > channel->channel_length)
    {
      QccErrorAddMessage("(QccChannelRead): Channel %s contains fewer symbols (%d) than requested block size (%d)",
                         channel->filename, channel->channel_length, block_size);
      return(1);
    }

  if (QccChannelAlloc(channel))
    {
      QccErrorAddMessage("(QccChannelRead): Error calling QccChannelAlloc()");
      return(1);
    }

  channel->num_blocks_accessed = 0;

  return(0);
}


int QccChannelEndRead(QccChannel *channel)
{
  if (channel == NULL)
    return(0);

  QccFileClose(channel->fileptr);

  QccChannelFree(channel);

  return(0);
}


int QccChannelReadBlock(QccChannel *channel)
{
  int symbol;
  int num_symbols_to_read;

  if (channel == NULL)
    return(0);
  if ((channel->fileptr == NULL) || (!channel->access_block_size))
    return(0);

  num_symbols_to_read = QccChannelGetBlockSize(channel);

  for (symbol = 0; symbol < num_symbols_to_read; symbol++)
    {
      fscanf(channel->fileptr, "%d", &channel->channel_symbols[symbol]);
      if (ferror(channel->fileptr) || feof(channel->fileptr))
        {
          QccErrorAddMessage("(QccChannelReadBlock): Error reading data from %s",
                             channel->filename);
          return(1);
        }
    }

  channel->num_blocks_accessed++;

  return(0);
}


int QccChannelWriteWholefile(QccChannel *channel)
{
  if (channel == NULL)
    return(0);

  channel->access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  
  if (QccChannelStartWrite(channel))
    {
      QccErrorAddMessage("(QccChannelWriteWholefile): Error calling QccChannelStartWrite()");
      return(1);
    }
  if (QccChannelWriteBlock(channel))
    {
      QccErrorAddMessage("(QccChannelWriteWholefile): Error calling QccChannelWriteBlock()");
      return(1);
    }

  QccFileClose(channel->fileptr);

  return(0);
}


int QccChannelWriteHeader(QccChannel *channel)
{
  if (channel == NULL)
    return(0);
  if (channel->fileptr == NULL)
    return(0);

  if (QccFileWriteMagicNumber(channel->fileptr, QCCCHANNEL_MAGICNUM))
    goto QccErr;

  fprintf(channel->fileptr, "%d\n%d\n",
          channel->channel_length, channel->alphabet_size);
  if (ferror(channel->fileptr))
    goto QccErr;

  return(0);

 QccErr:
  QccErrorAddMessage("(QccChannelWriteHeader): Error writing header to %s",
                     channel->filename);
  return(1);
  
}


int QccChannelStartWrite(QccChannel *channel)
{

  if (channel == NULL)
    return(0);

  if ((channel->fileptr = QccFileOpen(channel->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccChannelWrite): Error opening %s for writing",
                         channel->filename);
      return(1);
    }

  if (QccChannelWriteHeader(channel))
    {
      QccErrorAddMessage("(QccChannelWrite): Error writing header to %s",
                         channel->filename);
      return(1);
    }

  if (QccChannelAlloc(channel))
    {
      QccErrorAddMessage("(QccChannelWrite): Error calling QccChannelAlloc()");
      return(1);
    }
      
  channel->num_blocks_accessed = 0;

  return(0);
}


int QccChannelEndWrite(QccChannel *channel)
{
  if (channel == NULL)
    return(0);

  QccFileClose(channel->fileptr);

  QccChannelFree(channel);

  return(0);
}


int QccChannelWriteBlock(QccChannel *channel)
{
  int symbol;
  int num_symbols_to_write;

  if (channel == NULL)
    return(0);
  if ((channel->fileptr == NULL) || (!channel->access_block_size))
    return(0);

  num_symbols_to_write = QccChannelGetBlockSize(channel);

  for (symbol = 0; symbol < num_symbols_to_write; symbol++)
    fprintf(channel->fileptr, "%d\n",
            channel->channel_symbols[symbol]);

  channel->num_blocks_accessed++;

  return(0);
}


int QccChannelNormalize(QccChannel *channel)
{
  int symbol;
  int offset;

  if (channel == NULL)
    return(0);

  offset = channel->alphabet_size / 2;

  for (symbol = 0; symbol < QccChannelGetBlockSize(channel); symbol++)
    channel->channel_symbols[symbol] += offset;

  return(0);
}


int QccChannelDenormalize(QccChannel *channel)
{
  int symbol;
  int offset;

  if (channel == NULL)
    return(0);

  offset = channel->alphabet_size / 2;

  for (symbol = 0; symbol < QccChannelGetBlockSize(channel); symbol++)
    channel->channel_symbols[symbol] -= offset;

  return(0);
}


int QccChannelGetNumNullSymbols(const QccChannel *channel)
{
  int cnt;
  int symbol;

  if (channel == NULL)
    return(0);
  if (channel->channel_symbols == NULL)
    return(0);

  for (symbol = 0, cnt = 0; symbol < QccChannelGetBlockSize(channel); symbol++)
    if (channel->channel_symbols[symbol] == QCCCHANNEL_NULLSYMBOL)
      cnt++;

  return(cnt);
}


int QccChannelRemoveNullSymbols(QccChannel *channel)
{
  int num_null_symbols;
  int *new_channel_symbols;
  int symbol1, symbol2;
  int block_size;

  if (channel == NULL)
    return(0);

  if (channel->channel_symbols == NULL)
    return(0);

  block_size = QccChannelGetBlockSize(channel);

  if (block_size != channel->channel_length)
    {
      QccErrorAddMessage("(QccChannelRemoveNullSymbols): Block size must be whole channel");
      return(1);
    }

  num_null_symbols = QccChannelGetNumNullSymbols(channel);

  if ((new_channel_symbols = (int *)malloc(sizeof(int) *
                                           (block_size -
                                            num_null_symbols))) == NULL)
    {
      QccErrorAddMessage("(QccChannelRemoveNullSymbols): Error allocating memory");
      return(1);
    }

  for (symbol1 = 0, symbol2 = 0; symbol1 < block_size; symbol1++)
    if (channel->channel_symbols[symbol1] != QCCCHANNEL_NULLSYMBOL)
      new_channel_symbols[symbol2++] = channel->channel_symbols[symbol1];

  QccFree(channel->channel_symbols);
  channel->channel_symbols = new_channel_symbols;

  channel->channel_length -= num_null_symbols;

  return(0);
}


double QccChannelEntropy(const QccChannel *channel, int order)
{
  double entropy = 0;
  int block_size;
  int symbol;
  QccMatrix probs = NULL;
  int symbol_count;
  int context = 0;
  int num_contexts = 0;

  if (channel == NULL)
    return((double)-1.0);

  if (channel->channel_symbols == NULL)
    return((double)-1.0);

  if ((order != 1) && (order != 2))
    {
      QccErrorAddMessage("(QccChannelEntropy): only order 1 or 2 entropy is supported");
      goto Error;
    }

  if (channel->alphabet_size < 1)
    {
      QccErrorAddMessage("(QccChannelEntropy): channel %s has undefined or illegal alphabet size (%d)",
                         channel->filename, channel->alphabet_size);
      goto Error;
    }

  num_contexts = (order == 2) ? channel->alphabet_size : 1;

  block_size = QccChannelGetBlockSize(channel);
  
  if (!block_size)
    return((double)0);

  if ((probs = QccMatrixAlloc(num_contexts,
                                   channel->alphabet_size)) == NULL)
    {
      QccErrorAddMessage("(QccChannelEntropy): Error calling QccMatrixAlloc()");
      goto Error;
    }

  for (context = 0; context < num_contexts; context++)
    for (symbol = 0; symbol < channel->alphabet_size; symbol++)
      probs[context][symbol] = 0.0;

  for (symbol = 0, context = 0, symbol_count = 0; 
       symbol < block_size; symbol++)
    {
      if (channel->channel_symbols[symbol] != QCCCHANNEL_NULLSYMBOL)
        {
          if ((channel->channel_symbols[symbol] >= channel->alphabet_size) ||
              (channel->channel_symbols[symbol] < 0))
            {
              QccErrorAddMessage("(QccChannelEntropy): Invalid symbol %d in channel %s",
                                 channel->channel_symbols[symbol],
                                 channel->filename);
              goto Error;
            }
          
          probs[context][channel->channel_symbols[symbol]] += 1;
          context = (order == 2) ? channel->channel_symbols[symbol] : 0;
          symbol_count++;
        }
    }

  /*  Joint probability  */
  if (symbol_count)
    {
      for (context = 0; context < num_contexts; context++)
        for (symbol = 0; symbol < channel->alphabet_size; symbol++)
          probs[context][symbol] /= (double)symbol_count;
      
      entropy = QccENTConditionalEntropy(probs, num_contexts,
                                         channel->alphabet_size);
    }
  else
    entropy = 0;

  goto Return;

 Error:
  entropy = -1.0;

 Return:
  if (probs != NULL)
    QccMatrixFree(probs, num_contexts);
  return(entropy);
}


int QccChannelAddSymbolToChannel(QccChannel *channel, int symbol)
{
  
  if (channel == NULL)
    return(0);

  if (channel->access_block_size != QCCCHANNEL_ACCESSWHOLEFILE)
    {
      QccErrorAddMessage("(QccChannelAddSymbolToChannel): Block size must be whole file\n");
      return(1);
    }

  if (channel->channel_symbols == NULL)
    {
      if ((channel->channel_symbols = (int *)malloc(sizeof(int))) == NULL)
        {
          QccErrorAddMessage("(QccChannelAddSymbolToChannel): Error allocating memory");
          return(1);
        }
      channel->channel_symbols[0] = symbol;
      channel->channel_length = 1;
    }
  else
    {
      channel->channel_length++;
      if ((channel->channel_symbols = 
           (int *)realloc((void *)channel->channel_symbols, 
                          sizeof(int)*channel->channel_length)) == NULL)
        {
          QccErrorAddMessage("(QccChannelAddSymbolToChannel): Error reallocating memory");
          return(1);
        }
      channel->channel_symbols[channel->channel_length - 1] = symbol;
    }

  return(0);
}


