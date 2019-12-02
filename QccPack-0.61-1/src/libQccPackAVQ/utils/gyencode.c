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


#include "gyencode.h"

#define AVQGY_DEFAULT_ADAPTION_INTERVAL  1000

#define USG_STRING "[-i %d:adaption_interval] [-fc %: %s:final_codebook] %s:datfile %s:codebook %s:codebook_coder %s:channelfile %s:sideinfofile"


int AdaptionInterval = AVQGY_DEFAULT_ADAPTION_INTERVAL;
QccDataset Datfile;
QccVQCodebook Codebook;
QccChannel Channel;
QccAVQSideInfo SideInfo;

int final_codebook_specified = 0;
QccString FinalCodebookName;



int main(int argc, char *argv[])
{
  int block_size;
  int current_block;
  int num_blocks;
  int vector_dimension;
  int final_block_size;
  

  QccInit(argc, argv);
  
  QccDatasetInitialize(&Datfile);
  QccVQCodebookInitialize(&Codebook);
  QccChannelInitialize(&Channel);
  QccAVQSideInfoInitialize(&SideInfo);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &AdaptionInterval, 
                         &final_codebook_specified,
                         FinalCodebookName,
                         Datfile.filename,
                         Codebook.filename,
                         SideInfo.codebook_coder.filename,
                         Channel.filename,
                         SideInfo.filename))
    QccErrorExit();
  
  if (AdaptionInterval < 1)
    {
      QccErrorAddMessage("%s: Adaption interval must be >= 1\n",
                         argv[0]);
      QccErrorExit();
    }
  
  Datfile.access_block_size = AdaptionInterval;
  if (QccDatasetStartRead(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartRead",
                         argv[0]);
      QccErrorExit();
    }
  block_size = QccDatasetGetBlockSize(&Datfile);
  num_blocks = Datfile.num_vectors / block_size;
  final_block_size = Datfile.num_vectors - num_blocks*block_size;
  vector_dimension = Datfile.vector_dimension;
  
  if (QccVQCodebookRead(&Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQCodebookRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (Codebook.codeword_dimension != vector_dimension)
    {
      QccErrorAddMessage("%s: Codebook %s and dataset %s have different vector dimensions\n",
                         Codebook.filename, Datfile.filename);
      QccErrorExit();
    }
  
  if (QccSQScalarQuantizerRead(&(SideInfo.codebook_coder)))
    {
      QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (SideInfo.codebook_coder.type != QCCSQSCALARQUANTIZER_GENERAL)
    {
      QccErrorAddMessage("%s: Only general scalar quantizer is supported for codebook coder",
                         argv[0]);
      QccErrorExit();
    }
  
  Channel.access_block_size = block_size;
  Channel.channel_length = Datfile.num_vectors;
  Channel.alphabet_size = 
    Codebook.num_codewords;
  if (QccChannelStartWrite(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  SideInfo.N = block_size;
  SideInfo.vector_dimension = vector_dimension;
  SideInfo.vector_component_alphabet_size =
    SideInfo.codebook_coder.num_levels;
  if (QccGetProgramName(SideInfo.program_name))
    {
      QccErrorAddMessage("%s: Error calling QccGetProgramName()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccAVQSideInfoStartWrite(&SideInfo))
    {
      QccErrorAddMessage("%s: Error calling QccAVQSideInfoStartWrite()",
                         argv[0]);
      QccErrorExit();
    }

  for (current_block = 0; current_block < num_blocks; current_block++)
    {
      if (QccDatasetReadBlock(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccAVQGershoYanoEncode(&Datfile, &Codebook, &Channel, &SideInfo,
                                 block_size, NULL, NULL))
        {
          QccErrorAddMessage("%s: Error calling QccAVQGershoYanoEncode()",
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
  
  /*  Process final block, which may have size < block_size  */
  if (final_block_size)
    {
      Datfile.access_block_size =
        Channel.access_block_size = 
        block_size = final_block_size;
      
      if (QccDatasetReadBlock(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccAVQGershoYanoEncode(&Datfile, &Codebook, &Channel, &SideInfo,
                                 block_size, NULL, NULL))
        {
          QccErrorAddMessage("%s: Error calling QccAVQGershoYanoEncode()",
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
  
  
  if (QccAVQSideInfoEndWrite(&SideInfo))
    {
      QccErrorAddMessage("%s: Error calling QccAVQSideInfoEndWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccChannelEndWrite(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelEndWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccDatasetEndRead(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (final_codebook_specified)
    {
      QccStringCopy(Codebook.filename, FinalCodebookName);
      if (QccVQCodebookWrite(&Codebook))
        {
          QccErrorAddMessage("%s: Error calling QccVQCodebookWrite()",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  QccDatasetFree(&Datfile);
  QccVQCodebookFree(&Codebook);
  QccChannelFree(&Channel);

  QccExit;
}
