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


#include "chnarithmeticencode.h"

#define USG_STRING "[-s %:] [-vo %:] [-d %d:vector_dimension] [-o %d:order] %s:channel %s:outfile"


QccChannel Channel;

QccBitBuffer OutputBuffer;

int Order = 1;

int Silent = 0;
int ValueOnly = 0;
int VectorDimension = 1;


int main(int argc, char *argv[])
{
  double rate;

  QccInit(argc, argv);

  QccChannelInitialize(&Channel);
  QccBitBufferInitialize(&OutputBuffer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &Silent,
                         &ValueOnly,
                         &VectorDimension,
                         &Order,
                         Channel.filename,
                         OutputBuffer.filename))
    QccErrorExit();
      
  if ((Order != 1) && (Order != 2))
    {
      QccErrorAddMessage("%s: Only orders 1 and 2 are valid",
                         argv[0]);
      QccErrorExit();
    }

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
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
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
  if (QccBitBufferPutChar(&OutputBuffer, (char)Order))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferPutChar()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccENTArithmeticEncodeChannel(&Channel, &OutputBuffer, Order))
    {
      QccErrorAddMessage("%s: Error calling QccENTArithmeticEncodeChannel()",
                         argv[0]);
      QccErrorExit();
    }

  rate = (double)(OutputBuffer.bit_cnt + 8*(sizeof(int)*2 + sizeof(char))) /
    Channel.channel_length / VectorDimension;
    
  if (!Silent)
    {
      if (ValueOnly)
        printf("%f\n", rate);
      else
        printf("Channel %s arithmetic coded to:\n    %f (bits/symbol)\n",
               Channel.filename, rate);
    }

  if (QccBitBufferEnd(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()",
                         argv[0]);
      QccErrorExit();
    }

  QccChannelFree(&Channel);

  QccExit;
}
