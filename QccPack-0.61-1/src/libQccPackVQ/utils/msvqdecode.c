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


#include "msvqdecode.h"

#define USG_STRING "[-n %: %d:num_stages] %s:channelfile %s:codebookfile %s:datfile"


QccChannel Channel;
QccVQMultiStageCodebook MultiStageCodebook;
QccDataset Datfile;

int NumStagesSpecified = 0;
int NumStagesToDecode;

int main(int argc, char *argv[])
{
  int num_blocks, block_size;
  int block;
  int num_stages;

  QccInit(argc, argv);

  QccDatasetInitialize(&Datfile);
  QccChannelInitialize(&Channel);
  QccVQMultiStageCodebookInitialize(&MultiStageCodebook);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &NumStagesSpecified,
                         &NumStagesToDecode,
                         Channel.filename,
                         MultiStageCodebook.filename,
                         Datfile.filename))
    QccErrorExit();
      
  if (QccVQMultiStageCodebookRead(&MultiStageCodebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQMultiStageCodebookRead()",
                         argv[0]);
      QccErrorExit();
    }

  num_stages = MultiStageCodebook.num_codebooks;

  if (!NumStagesSpecified)
    NumStagesToDecode = num_stages;
  else
    if (NumStagesToDecode <= 0)
      {
        QccErrorAddMessage("%s: Invalid number of stages to decode (%d)",
                           argv[0], NumStagesToDecode);
        QccErrorExit();
      }

  Channel.access_block_size = num_stages;
  if (QccChannelStartRead(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  block_size = QccChannelGetBlockSize(&Channel);
  num_blocks = Channel.channel_length / block_size;

  Datfile.num_vectors = num_blocks * (block_size / num_stages);
  Datfile.vector_dimension =
    MultiStageCodebook.codebooks[0].codeword_dimension;
  Datfile.access_block_size = Channel.access_block_size / num_stages;
  Datfile.min_val = -MAXFLOAT;
  Datfile.max_val = MAXFLOAT;

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

      if (QccVQMultiStageVQDecode(Channel.channel_symbols,
                                  &MultiStageCodebook,
                                  &Datfile,
                                  NumStagesToDecode))
        {
          QccErrorAddMessage("%s: Error calling QccVQMultiStageVQDecode()",
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
  QccVQMultiStageCodebookFreeCodebooks(&MultiStageCodebook);

  QccExit;
}
