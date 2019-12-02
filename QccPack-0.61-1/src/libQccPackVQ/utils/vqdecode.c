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


#include "vqdecode.h"

#define USG_STRING "%s:channelfile %s:codebookfile %s:datfile"


QccChannel Channel;
QccVQCodebook Codebook;
QccDataset Datfile;



int main(int argc, char *argv[])
{
  int num_blocks, block_size;
  int block;
  
  QccInit(argc, argv);

  QccDatasetInitialize(&Datfile);
  QccChannelInitialize(&Channel);
  QccVQCodebookInitialize(&Codebook);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         Channel.filename,
                         Codebook.filename,
                         Datfile.filename))
    QccErrorExit();
      
  Channel.access_block_size = 1;
  if (QccChannelStartRead(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccVQCodebookRead(&Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQCodebookRead()",
                         argv[0]);
      QccErrorExit();
    }

  block_size = QccChannelGetBlockSize(&Channel);
  num_blocks = Channel.channel_length / block_size;

  Datfile.num_vectors = num_blocks * block_size;
  Datfile.vector_dimension = Codebook.codeword_dimension;
  Datfile.access_block_size = Channel.access_block_size;
  Datfile.min_val = 
    QccMatrixMinValue(Codebook.codewords,
                           Codebook.num_codewords,
                           Codebook.codeword_dimension);
  Datfile.max_val =
    QccMatrixMaxValue(Codebook.codewords,
                           Codebook.num_codewords,
                           Codebook.codeword_dimension);
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

      if (QccVQInverseVectorQuantization(Channel.channel_symbols,
                                         &Codebook, &Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccVQInverseVectorQuantization()",
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

  QccDatasetFree(&Datfile);
  QccChannelFree(&Channel);
  QccVQCodebookFree(&Codebook);

  QccExit;
}
