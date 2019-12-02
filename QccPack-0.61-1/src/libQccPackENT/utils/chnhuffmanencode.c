/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "chnhuffmanencode.h"

#define USG_STRING "[-s %:] [-vo %:] [-d %d:vector_dimension] %s:channel %s:bitstream %s:huffman_table"


QccChannel Channel;

QccBitBuffer OutputBuffer;

QccENTHuffmanTable HuffmanTable;

int Silent = 0;
int ValueOnly = 0;
int VectorDimension = 1;


int main(int argc, char *argv[])
{
  double rate;

  QccInit(argc, argv);

  QccChannelInitialize(&Channel);
  QccBitBufferInitialize(&OutputBuffer);
  QccENTHuffmanTableInitialize(&HuffmanTable);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &Silent,
                         &ValueOnly,
                         &VectorDimension,
                         Channel.filename,
                         OutputBuffer.filename,
                         HuffmanTable.filename))
    QccErrorExit();
      
  if (QccChannelReadWholefile(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelReadWholefile()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccChannelRemoveNullSymbols(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelRemoveNullSymbols()",
                         argv[0]);
      QccErrorExit();
    }

  OutputBuffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStartup()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccBitBufferPutInt(&OutputBuffer, Channel.channel_length))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferPutInt()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccBitBufferPutInt(&OutputBuffer, Channel.alphabet_size))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferPutInt()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccENTHuffmanEncodeChannel(&Channel, &OutputBuffer, &HuffmanTable))
    {
      QccErrorAddMessage("%s: Error calling QccENTHuffmanEncodeChannel()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccBitBufferFlush(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferFlush()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccBitBufferEnd(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccENTHuffmanTableWrite(&HuffmanTable))
    {
      QccErrorAddMessage("%s: Error calling QccENTHuffmanTableWrite()",
                         argv[0]);
      QccErrorExit();
    }

  rate = (double)(OutputBuffer.bit_cnt) /
    Channel.channel_length / VectorDimension;
    
  if (!Silent)
    {
      if (ValueOnly)
        printf("%f\n", rate);
      else
        printf("Channel %s huffman coded to:\n    %f (bits/symbol)\n",
               Channel.filename, rate);
    }

  QccChannelFree(&Channel);
  QccENTHuffmanTableFree(&HuffmanTable);

  QccExit;
}
