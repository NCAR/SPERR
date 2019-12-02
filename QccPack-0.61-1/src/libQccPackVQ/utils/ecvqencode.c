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


#include "ecvqencode.h"

#define USG_STRING "%f:lambda %s:datafile %s:codebookfile %s:channelfile"


float Lambda;
QccDataset Datfile;
QccVQCodebook Codebook;
QccChannel Channel;


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
                         &Lambda,
                         Datfile.filename,
                         Codebook.filename,
                         Channel.filename))
    QccErrorExit();
      
  Datfile.access_block_size = 1;
  if (QccDatasetStartRead(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccVQCodebookRead(&Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQCodebookRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (Codebook.codeword_dimension !=
      Datfile.vector_dimension)
    {
      QccErrorAddMessage("%s: Vector dimensions of %s and %s must be the same\n",
                         argv[0], Codebook.filename, Datfile.filename);
      QccErrorExit();
    }
  
  block_size = QccDatasetGetBlockSize(&Datfile);
  num_blocks = Datfile.num_vectors / block_size;

  Channel.channel_length = num_blocks * block_size;
  Channel.access_block_size = Datfile.access_block_size;
  Channel.alphabet_size = Codebook.num_codewords;
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
      if (QccVQEntropyConstrainedVQ(&Datfile, &Codebook, (double)Lambda,
                                    NULL, Channel.channel_symbols, NULL))
        {
          QccErrorAddMessage("%s: Error calling QccVQVectorQuantization()",
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
  QccChannelFree(&Channel);
  QccVQCodebookFree(&Codebook);

  QccExit;
}
