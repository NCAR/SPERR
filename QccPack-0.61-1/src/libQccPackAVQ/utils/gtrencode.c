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


#include "gtrencode.h"

/*  Default parameters  */
#define QCCAVQGTR_DEFAULT_LAMBDA  16.0
#define QCCAVQGTR_DEFAULT_OMEGA  100.0
#define QCCAVQGTR_DEFAULT_SIDEINFOQUANTIZER_NUMLEVELS 256
#define QCCAVQGTR_DEFAULT_MAX_CODEBOOK_SIZE 256

#define USG_STRING "[-l %f:lambda] [-w %f:omega] [-ic %: %s:initial_codebook] [-cs %: %d:max_codebook_size] [-fc %: %s:final_codebook] %s:datfile %s:codebook_coder %s:channelfile %s:sideinfofile"


float Lambda = QCCAVQGTR_DEFAULT_LAMBDA;
float Omega = QCCAVQGTR_DEFAULT_OMEGA;

int initial_codebook_specified = 0;
int max_codebook_size_specified = 0;
int MaxCodebookSize = QCCAVQGTR_DEFAULT_MAX_CODEBOOK_SIZE;

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
  

  QccInit(argc, argv);
  
  QccDatasetInitialize(&Datfile);
  QccVQCodebookInitialize(&Codebook);
  QccChannelInitialize(&Channel);
  QccAVQSideInfoInitialize(&SideInfo);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &Lambda, &Omega,
                         &initial_codebook_specified,
                         Codebook.filename,
                         &max_codebook_size_specified,
                         &MaxCodebookSize,
                         &final_codebook_specified,
                         FinalCodebookName,
                         Datfile.filename,
                         SideInfo.codebook_coder.filename,
                         Channel.filename,
                         SideInfo.filename))
    QccErrorExit();
  
  if (MaxCodebookSize < 1)
    {
      QccErrorAddMessage("%s: invalid max_codebook_size %d",
                         argv[0], MaxCodebookSize);
      QccErrorExit();
    }

  Datfile.access_block_size = 1;
  if (QccDatasetStartRead(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartRead",
                         argv[0]);
      QccErrorExit();
    }
  block_size = QccDatasetGetBlockSize(&Datfile);
  num_blocks = Datfile.num_vectors / block_size;
  vector_dimension = Datfile.vector_dimension;
  
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
          QccErrorAddMessage("%s: Codebook %s and dataset %s have different vector dimensions\n",
                             Codebook.filename, Datfile.filename);
          QccErrorExit();
        }
      if (!max_codebook_size_specified)
        MaxCodebookSize = Codebook.num_codewords;
    }
  else
    {
      Codebook.num_codewords = 0;
      Codebook.codeword_dimension = vector_dimension;
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

  Channel.channel_length = Datfile.num_vectors;
  Channel.alphabet_size = MaxCodebookSize;
  Channel.access_block_size = block_size;
  
  SideInfo.max_codebook_size = MaxCodebookSize;
  SideInfo.N = Datfile.num_vectors;
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
  
  if (QccChannelStartWrite(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelStartWrite()",
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
      if (QccAVQgtrEncode(&Datfile, 
                          &Codebook, 
                          &Channel, &SideInfo,
                          (double)Lambda, (double)Omega, NULL))
        {
          QccErrorAddMessage("%s: Error calling QccAVQgtrEncode()",
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
