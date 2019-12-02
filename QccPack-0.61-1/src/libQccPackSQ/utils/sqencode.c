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


#include "sqencode.h"

#define USG_STRING "%s:datfile %s:quantizer %s:channel"


QccDataset Datfile;
QccSQScalarQuantizer Quantizer;
QccChannel Channel;


int main(int argc, char *argv[])
{
  int num_blocks, block_size;
  int block;
  int max_index;

  QccInit(argc, argv);

  QccDatasetInitialize(&Datfile);
  QccSQScalarQuantizerInitialize(&Quantizer);
  QccChannelInitialize(&Channel);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         Datfile.filename,
                         Quantizer.filename,
                         Channel.filename))
    QccErrorExit();
      
  Datfile.access_block_size = 1;
  if (QccDatasetStartRead(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccSQScalarQuantizerRead(&Quantizer))
    {
      QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  block_size = QccDatasetGetBlockSize(&Datfile);
  num_blocks = Datfile.num_vectors / block_size;

  Channel.channel_length = num_blocks * block_size *
    Datfile.vector_dimension;
  Channel.access_block_size = Datfile.access_block_size *
    Datfile.vector_dimension;

  if (Quantizer.type == QCCSQSCALARQUANTIZER_GENERAL)
    Channel.alphabet_size = Quantizer.num_levels;
  else
    {
      if (Datfile.min_val < 0)
        {
          QccErrorAddMessage("%s: Datfile cannot contain negative values when used with non-general scalar quantizer",
                             argv[0]);
          QccErrorExit();
        }
      if (QccSQScalarQuantization(Datfile.max_val,
                                  &Quantizer,
                                  NULL,
                                  &max_index))
        {
          QccErrorAddMessage("%s: Error calling QccSQScalarQuantization()",
                             argv[0]);
          QccErrorExit();
        }
      Channel.alphabet_size = max_index + 1;
    }

  if (QccChannelStartWrite(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartWrite()",
                         argv[0]);
      QccErrorExit();
    }

  for (block = 0; block < num_blocks; block++)
    {
      if (QccDatasetReadBlock(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccSQScalarQuantizeDataset(&Datfile, &Quantizer, NULL, 
                                     Channel.channel_symbols))
        {
          QccErrorAddMessage("%s: Error calling QccSQScalarQuantizeDataset()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccChannelWriteBlock(&Channel))
        {
          QccErrorAddMessage("%s: Error calling QccChannelWriteBlock()",
                             argv[0]);
          QccErrorExit();
        }
    }

  if (QccDatasetEndRead(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccChannelEndWrite(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelEndWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccDatasetFree(&Datfile);
  QccSQScalarQuantizerFree(&Quantizer);
  QccChannelFree(&Channel);

  QccExit;
}
