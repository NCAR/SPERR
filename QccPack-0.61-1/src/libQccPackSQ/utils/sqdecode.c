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


#include "sqdecode.h"

#define USG_STRING "[-d %d:vector_dimension] %s:channel %s:quantizer %s:datfile"


QccChannel Channel;
QccSQScalarQuantizer Quantizer;
QccDataset Datfile;

int VectorDimension = 1;

int main(int argc, char *argv[])
{
  int num_blocks, block_size;
  int block;
  
  QccInit(argc, argv);

  QccChannelInitialize(&Channel);
  QccSQScalarQuantizerInitialize(&Quantizer);
  QccDatasetInitialize(&Datfile);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &VectorDimension,
                         Channel.filename,
                         Quantizer.filename,
                         Datfile.filename))
    QccErrorExit();
      
  Channel.access_block_size = VectorDimension;
  if (QccChannelStartRead(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccSQScalarQuantizerRead(&Quantizer))
    {
      QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  block_size = QccChannelGetBlockSize(&Channel) / VectorDimension;
  num_blocks = Channel.channel_length / VectorDimension / block_size;

  Datfile.num_vectors = num_blocks * block_size;
  Datfile.vector_dimension = VectorDimension;
  Datfile.access_block_size = block_size;
  if (Quantizer.type == QCCSQSCALARQUANTIZER_GENERAL)
    {
      Datfile.min_val = 
        Quantizer.levels[0];
      Datfile.max_val =
        Quantizer.levels[Quantizer.num_levels - 1];
    }
  else
    {
      Datfile.min_val = 0;
      if (QccSQInverseScalarQuantization(Channel.alphabet_size,
                                         &Quantizer,
                                         &Datfile.max_val))
        {
          QccErrorAddMessage("%s: Error calling QccSQInverseScalarQuantization()",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  if (QccDatasetStartWrite(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartWrite()",
                         argv[0]);
      QccErrorExit();
    }

  for (block = 0; block < num_blocks; block++)
    {
      if (QccChannelReadBlock(&Channel))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                             argv[0]);
          QccErrorExit();
        }

      if (QccSQInverseScalarQuantizeDataset(Channel.channel_symbols,
                                            &Quantizer, 
                                            &Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccSQInverseScalarQuantizeDataset()",
                             argv[0]);
          QccErrorExit();
        }

      if (QccDatasetWriteBlock(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccChannelWriteBlock()",
                             argv[0]);
          QccErrorExit();
        }
    }

  if (QccChannelEndRead(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelEndRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccDatasetEndWrite(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccChannelFree(&Channel);
  QccSQScalarQuantizerFree(&Quantizer);
  QccDatasetFree(&Datfile);

  QccExit;
}
