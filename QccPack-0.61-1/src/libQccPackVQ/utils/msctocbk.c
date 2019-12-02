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


#include "msctocbk.h"

#define USG_STRING "%s:mscfile %s:cbkfile"

QccVQMultiStageCodebook MultiStageCodebook;
QccVQCodebook Codebook;


int main(int argc, char *argv[])
{
  int current_codebook;

  QccInit(argc, argv);
  
  QccVQMultiStageCodebookInitialize(&MultiStageCodebook);
  QccVQCodebookInitialize(&Codebook);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         MultiStageCodebook.filename,
                         Codebook.filename))
    QccErrorExit();
  
  if (QccVQMultiStageCodebookRead(&MultiStageCodebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQMultiStageCodebookRead()",
                         argv[0]);
      QccErrorExit();
    }

  Codebook.codeword_dimension =
    MultiStageCodebook.codebooks[0].codeword_dimension;
  Codebook.num_codewords = 1;
  for (current_codebook = 0;
       current_codebook < MultiStageCodebook.num_codebooks;
       current_codebook++)
    Codebook.num_codewords *=
      MultiStageCodebook.codebooks[current_codebook].num_codewords;

  if (QccVQCodebookAlloc(&Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQCodebookAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccVQMultiStageCodebookToCodebook(&MultiStageCodebook,
                                        &Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQMultiStageCodebookToCodebook()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccVQCodebookWrite(&Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQCodebookWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccVQMultiStageCodebookFreeCodebooks(&MultiStageCodebook);
  QccVQCodebookFree(&Codebook);

  QccExit;
}
