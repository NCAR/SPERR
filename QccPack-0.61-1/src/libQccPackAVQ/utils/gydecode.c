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


#include "gydecode.h"

#define USG_STRING "%s:channelfile %s:sideinfofile %s:codebook %s:codebook_coder %s:datfile"


QccChannel Channel;
QccAVQSideInfo SideInfo;
QccVQCodebook Codebook;
QccDataset Datfile;



int main(int argc, char *argv[])
{
  int block_size;
  int current_block;
  int num_blocks;
  int num_vectors;
  int vector_dimension;
  int final_block_size;
  
  QccInit(argc, argv);
  
  QccChannelInitialize(&Channel);
  QccAVQSideInfoInitialize(&SideInfo);
  QccVQCodebookInitialize(&Codebook);
  QccDatasetInitialize(&Datfile);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         Channel.filename,
                         SideInfo.filename,
                         Codebook.filename,
                         SideInfo.codebook_coder.filename,
                         Datfile.filename))
    QccErrorExit();
  
  if (QccAVQSideInfoStartRead(&SideInfo))
    {
      QccErrorAddMessage("%s: Error calling QccAVQSideInfoStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  vector_dimension = SideInfo.vector_dimension;
  block_size = SideInfo.N;
  
  Channel.access_block_size = block_size;
  if (QccChannelStartRead(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  num_vectors = Channel.channel_length;
  num_blocks = num_vectors / block_size;
  final_block_size = num_vectors - num_blocks*block_size;
  
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
  
  if (QccVQCodebookRead(&Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQCodebookRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (Codebook.codeword_dimension != vector_dimension)
    {
      QccErrorAddMessage("%s: Codebook %s and sideinfo %s have different vector dimensions",
                         argv[0], Codebook.filename, SideInfo.filename);
      QccErrorExit();
    }
  
  Datfile.num_vectors = num_vectors;
  Datfile.vector_dimension = vector_dimension;
  Datfile.access_block_size = block_size;
  Datfile.min_val = 
    SideInfo.codebook_coder.levels[0];
  Datfile.max_val =
    SideInfo.codebook_coder.levels[SideInfo.codebook_coder.num_levels - 1];
  Datfile.min_val =
    QccMathMin(Datfile.min_val,
               QccMatrixMinValue(Codebook.codewords,
                                      Codebook.num_codewords,
                                      Codebook.codeword_dimension));
  Datfile.max_val =
    QccMathMax(Datfile.max_val,
               QccMatrixMaxValue(Codebook.codewords,
                                      Codebook.num_codewords,
                                      Codebook.codeword_dimension));
  if (QccDatasetStartWrite(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  for (current_block = 0; current_block < num_blocks; current_block++)
    {
      if (QccChannelReadBlock(&Channel))
        {
          QccErrorAddMessage("%s: Error calling QccChannelRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccAVQGershoYanoDecode(&Datfile, &Codebook, &Channel, &SideInfo))
        {
          QccErrorAddMessage("%s: Error calling QccAVQGershoYanoDecode()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccDatasetWriteBlock(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetWriteBlock()",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  /*  Process final block, which may have size < block_size  */
  if (final_block_size)
    {
      Channel.access_block_size = 
        Datfile.access_block_size = 
        block_size = final_block_size;
      
      if (QccChannelReadBlock(&Channel))
        {
          QccErrorAddMessage("%s: Error calling QccChannelRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccAVQGershoYanoDecode(&Datfile, &Codebook, &Channel, &SideInfo))
        {
          QccErrorAddMessage("%s: Error calling QccAVQGershoYanoDecode()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccDatasetWriteBlock(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetWriteBlock()",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  if (QccDatasetEndWrite(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndWrite()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccAVQSideInfoEndRead(&SideInfo))
    {
      QccErrorAddMessage("%s: Error calling QccAVQSideInfoEndRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccChannelEndRead(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelEndRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  QccChannelFree(&Channel);
  QccVQCodebookFree(&Codebook);
  QccDatasetFree(&Datfile);

  QccExit;
}
