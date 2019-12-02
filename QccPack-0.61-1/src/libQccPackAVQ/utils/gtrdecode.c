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


#include "gtrdecode.h"

#define USG_STRING "[-ic %: %s:initial_codebook] %s:channelfile %s:sideinfofile %s:codebook_coder %s:datfile"


int initial_codebook_specified = 0;
QccVQCodebook Codebook;

QccChannel Channel;
QccAVQSideInfo SideInfo;
QccDataset Datfile;



int main(int argc, char *argv[])
{
  int block_size;
  int current_block;
  int num_blocks;
  int vector_dimension;
  
  QccInit(argc, argv);
  
  QccVQCodebookInitialize(&Codebook);
  QccChannelInitialize(&Channel);
  QccAVQSideInfoInitialize(&SideInfo);
  QccDatasetInitialize(&Datfile);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &initial_codebook_specified,
                         Codebook.filename,
                         Channel.filename,
                         SideInfo.filename,
                         SideInfo.codebook_coder.filename,
                         Datfile.filename))
    QccErrorExit();
  
  block_size = 1;

  Channel.access_block_size = block_size;
  if (QccChannelStartRead(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartRead()",
                         argv[0]);
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
  
  if (QccAVQSideInfoStartRead(&SideInfo))
    {
      QccErrorAddMessage("%s: Error calling QccAVQSideInfoStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  vector_dimension = SideInfo.vector_dimension;
  
  if (initial_codebook_specified)
    {
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
    }
  else
    {
      Codebook.num_codewords = 0;
      Codebook.codeword_dimension = vector_dimension;
    }

  Datfile.num_vectors = SideInfo.N;
  Datfile.vector_dimension = vector_dimension;
  Datfile.access_block_size = block_size;
  Datfile.min_val = 
    SideInfo.codebook_coder.levels[0];
  Datfile.max_val =
    SideInfo.codebook_coder.levels[SideInfo.codebook_coder.num_levels - 1];
  if (initial_codebook_specified)
    {
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
    }
  if (QccDatasetStartWrite(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  num_blocks = Datfile.num_vectors;

  for (current_block = 0; current_block < num_blocks; current_block++)
    {
      if (QccChannelReadBlock(&Channel))
        {
          QccErrorAddMessage("%s: Error calling QccChannelReadBlock()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccAVQgtrDecode(&Datfile, &Codebook, &Channel, &SideInfo))
        {
          QccErrorAddMessage("%s: Error calling QccAVQgtrDecode()",
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
  
  QccVQCodebookFree(&Codebook);
  QccChannelFree(&Channel);
  QccDatasetFree(&Datfile);

  QccExit;
}
