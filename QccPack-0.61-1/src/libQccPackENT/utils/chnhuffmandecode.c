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


#include "chnarithmeticdecode.h"

#define USG_STRING "%s:bitstream %s:huffman_table %s:channel"


QccBitBuffer InputBuffer;

QccENTHuffmanTable HuffmanTable;

QccChannel Channel;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);

  QccChannelInitialize(&Channel);
  QccBitBufferInitialize(&InputBuffer);
  QccENTHuffmanTableInitialize(&HuffmanTable);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         InputBuffer.filename,
                         HuffmanTable.filename,
                         Channel.filename))
    QccErrorExit();
      
  HuffmanTable.table_type = QCCENTHUFFMAN_DECODETABLE;
  if (QccENTHuffmanTableRead(&HuffmanTable))
    {
      QccErrorAddMessage("%s: Error calling QccENTHuffmanTableRead()",
                         argv[0]);
      QccErrorExit();
    }

  InputBuffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccBitBufferGetInt(&InputBuffer, &(Channel.channel_length)))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferGetInt()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccBitBufferGetInt(&InputBuffer, &(Channel.alphabet_size)))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferGetInt()",
                         argv[0]);
      QccErrorExit();
    }

  Channel.access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
  if (QccChannelStartWrite(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartWrite()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccENTHuffmanDecodeChannel(&InputBuffer, &Channel, &HuffmanTable))
    {
      QccErrorAddMessage("%s: Error calling QccENTHuffmanDecodeChannel()",
                         argv[0]);
      QccErrorExit();
    }
                             
  if (QccBitBufferEnd(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccChannelWriteBlock(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelWriteBlock()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccChannelEndWrite(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelEndWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccChannelFree(&Channel);
  QccENTHuffmanTableFree(&HuffmanTable);

  QccExit;
}
