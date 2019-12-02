/*
 * 
 * QccPack: Quantization, compression, and coding libraries
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 * 
 */


#include "libQccPack.h"


int QccVQMultiStageCodebookInitialize(QccVQMultiStageCodebook
                                      *multistage_codebook)
{
  if (multistage_codebook == NULL)
    return(0);

  QccStringMakeNull(multistage_codebook->filename);
  QccStringCopy(multistage_codebook->magic_num,
                QCCVQMULTISTAGECODEBOOK_MAGICNUM);
  QccGetQccPackVersion(&multistage_codebook->major_version,
                       &multistage_codebook->minor_version,
                       NULL);

  multistage_codebook->num_codebooks = 0;
  multistage_codebook->codebooks = NULL;

  return(0);
}


int QccVQMultiStageCodebookAllocCodebooks(QccVQMultiStageCodebook 
                                          *multistage_codebook)
{
  int codebook;
  
  if (multistage_codebook == NULL)
    return(0);
  
  if (multistage_codebook->num_codebooks <= 0)
    return(0);
  
  if (multistage_codebook->codebooks == NULL)
    {
      if ((multistage_codebook->codebooks = 
           (QccVQCodebook *)malloc(sizeof(QccVQCodebook)*
                                   multistage_codebook->num_codebooks)) ==
          NULL)
        {
          QccErrorAddMessage("(QccVQMultiStageCodebookAllocCodebooks): Error allocating memory");
          return(1);
        }
      
      for (codebook = 0; codebook < multistage_codebook->num_codebooks; 
           codebook++)
        QccVQCodebookInitialize(&(multistage_codebook->codebooks[codebook]));
    }
  
  return(0);
}


void QccVQMultiStageCodebookFreeCodebooks(QccVQMultiStageCodebook
                                          *multistage_codebook)
{
  int codebook;
  
  if (multistage_codebook == NULL)
    return;
  if (multistage_codebook->num_codebooks <= 0)
    return;
  
  if (multistage_codebook->codebooks != NULL)
    {
      for (codebook = 0; codebook < multistage_codebook->num_codebooks; 
           codebook++)
        QccVQCodebookFree(&(multistage_codebook->codebooks[codebook]));
      QccFree(multistage_codebook->codebooks);
    }
}


int QccVQMultiStageCodebookReadHeader(FILE *infile, 
                                      QccVQMultiStageCodebook
                                      *multistage_codebook)
{
  int codebook;
  
  if ((infile == NULL) || (multistage_codebook == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             multistage_codebook->magic_num,
                             &multistage_codebook->major_version,
                             &multistage_codebook->minor_version))
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookReadHeader): Error calling QccFileReadMagicNumber()");
      return(1);
    }
  
  if (strcmp(multistage_codebook->magic_num,
             QCCVQMULTISTAGECODEBOOK_MAGICNUM))
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookReadHeader): %s is not of multistage codebook (%s) type",
                         multistage_codebook->filename,
                         QCCVQMULTISTAGECODEBOOK_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(multistage_codebook->num_codebooks));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookReadHeader): Error reading number of codebooks in multistage codebook %s",
                         multistage_codebook->filename);
      return(1);
    }
  
  if (QccVQMultiStageCodebookAllocCodebooks(multistage_codebook))
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookReadHeader): Error calling QccVQMultiStageCodebookAllocCodebooks()");
      return(1);
    }
  
  for (codebook = 0; codebook < multistage_codebook->num_codebooks;
       codebook++)
    {
      fscanf(infile, "%d", 
             &(multistage_codebook->codebooks[codebook].num_codewords));
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccVQMultiStageCodebookReadHeader): Error reading number of codewords for codebook %d in multistage codebook %s",
                             codebook,
                             multistage_codebook->filename);
          return(1);
        }
      
      fscanf(infile, "%d", 
             &(multistage_codebook->codebooks[codebook].codeword_dimension));
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccVQMultiStageCodebookReadHeader): Error reading codeword dimension for codebook %d in multistage codebook %s",
                             codebook,
                             multistage_codebook->filename);
          return(1);
        }
    }
  
  fscanf(infile, "%*1[\n]");
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookReadHeader): Error reading multistage codebook %s",
                         multistage_codebook->filename);
      return(1);
    }

  return(0);
}


int QccVQMultiStageCodebookRead(QccVQMultiStageCodebook *multistage_codebook)
{
  int codebook;
  FILE *infile = NULL;
  
  if (multistage_codebook == NULL)
    return(0);
  
  if ((infile = QccFileOpen(multistage_codebook->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookRead): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccVQMultiStageCodebookReadHeader(infile, multistage_codebook))
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookRead): Error calling QccVQMultiStageCodebookReadHeader()");
      return(1);
    }
  
  for (codebook = 0; codebook < multistage_codebook->num_codebooks; codebook++)
    if (QccVQCodebookReadData(infile, 
                              &(multistage_codebook->codebooks[codebook])))
      {
        QccErrorAddMessage("(QccVQMultiStageCodebookRead): Error calling QccVQCodebookReadData()");
        return(1);
      }
  
  QccFileClose(infile);
  
  return(0);
}


int QccVQMultiStageCodebookWriteHeader(FILE *outfile, 
                                       const QccVQMultiStageCodebook 
                                       *multistage_codebook)
{
  int codebook;
  
  if ((outfile == NULL) || (multistage_codebook == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCVQMULTISTAGECODEBOOK_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d\n",
          multistage_codebook->num_codebooks);
  if (ferror(outfile))
    goto Error;
  
  for (codebook = 0; codebook < multistage_codebook->num_codebooks; codebook++)
    {
      fprintf(outfile, "%d %d\n",
              multistage_codebook->codebooks[codebook].num_codewords,
              multistage_codebook->codebooks[codebook].codeword_dimension);
      if (ferror(outfile))
        goto Error;
    }
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccVQMultiStageCodebookWriteHeader): Error writing header to %s",
                     multistage_codebook->filename);
  return(1);
}


int QccVQMultiStageCodebookWrite(const
                                 QccVQMultiStageCodebook *multistage_codebook)
{
  int codebook;
  FILE *outfile;
  
  if (multistage_codebook == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(multistage_codebook->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccVQMultiStageCodebookWriteHeader(outfile, multistage_codebook))
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookWrite): Error calling QccVQMultiStageCodebookWriteHeader()");
      return(1);
    }
  
  for (codebook = 0; codebook < multistage_codebook->num_codebooks; codebook++)
    if (QccVQCodebookWriteData(outfile,
                               &(multistage_codebook->codebooks[codebook])))
      {
        QccErrorAddMessage("(QccVQMultiStageCodebookWrite): Error calling QccVQCodebookWriteData()");
        return(1);
      }
  
  QccFileClose(outfile);
  return(0);
}


int QccVQMultiStageCodebookPrint(const
                                 QccVQMultiStageCodebook *multistage_codebook)
{
  int codebook;
  
  if (multistage_codebook == NULL)
    return(0);
  
  if (QccFilePrintFileInfo(multistage_codebook->filename,
                           multistage_codebook->magic_num,
                           multistage_codebook->major_version,
                           multistage_codebook->minor_version))
    return(1);
  
  printf("Num of codebooks: %d\n\n", multistage_codebook->num_codebooks);
  
  for (codebook = 0; codebook < multistage_codebook->num_codebooks; codebook++)
    {
      printf("Codebook %d\n", codebook);
      QccVQCodebookPrint(&(multistage_codebook->codebooks[codebook]));
      printf("\n\n");
    }

  return(0);
}


int QccVQMultiStageCodebookToCodebook(const QccVQMultiStageCodebook
                                      *multistage_codebook,
                                      QccVQCodebook *codebook)
{
  int return_value;
  int current_codebook;
  int total_codewords;
  int current_codeword;
  int *stage_codeword = NULL;
  QccDataset dataset;
  int component;

  QccDatasetInitialize(&dataset);

  if (multistage_codebook == NULL)
    return(0);
  if (codebook == NULL)
    return(0);

  if (multistage_codebook->codebooks == NULL)
    return(0);
  if (codebook->codewords == NULL)
    return(0);

  total_codewords = 1;
  for (current_codebook = 0;
       current_codebook < multistage_codebook->num_codebooks;
       current_codebook++)
    {
      if (multistage_codebook->codebooks[current_codebook].codewords == NULL)
        return(0);
      total_codewords *=
        multistage_codebook->codebooks[current_codebook].num_codewords;
    }

  if (total_codewords != codebook->num_codewords)
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookToCodebook): Multistage codebook and codebook have incompatible numbers of codewords");
      goto Error;
    }

  if (multistage_codebook->codebooks[0].codeword_dimension !=
      codebook->codeword_dimension)
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookToCodebook): Multistage codebook and codebook have incompatible codeword dimensions");
      goto Error;
    }

  if ((stage_codeword =
       (int *)malloc(sizeof(int) * multistage_codebook->num_codebooks)) ==
      NULL)
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookToCodebook): Error allocating memory");
      goto Error;
    }

  dataset.num_vectors = 1;
  dataset.vector_dimension = codebook->codeword_dimension;
  dataset.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  if (QccDatasetAlloc(&dataset))
    {
      QccErrorAddMessage("(QccVQMultiStageCodebookToCodebook): Error calling QccDatasetAlloc()");
      goto Error;
    }

  for (current_codebook = 0;
       current_codebook < multistage_codebook->num_codebooks;
       current_codebook++)
    stage_codeword[current_codebook] = 0;

  for (current_codeword = 0;
       current_codeword < total_codewords;
       current_codeword++)
    {
      if (QccVQMultiStageVQDecode(stage_codeword,
                                  multistage_codebook,
                                  &dataset,
                                  multistage_codebook->num_codebooks))
        {
          QccErrorAddMessage("(QccVQMultiStageCodebookToCodebook): Error calling QccVQMultiStageVQDecode()");
          goto Error;
        }

      for (component = 0;
           component < codebook->codeword_dimension;
           component++)
        codebook->codewords[current_codeword][component] =
          dataset.vectors[0][component];

      for (current_codebook = 0;
           current_codebook < multistage_codebook->num_codebooks;
           current_codebook++)
        {
          stage_codeword[current_codebook]++;
          if (stage_codeword[current_codebook] >=
              multistage_codebook->codebooks[current_codebook].num_codewords)
            stage_codeword[current_codebook] = 0;
          else
            break;
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccFree(stage_codeword);
  QccDatasetFree(&dataset);
  return(return_value);
}
