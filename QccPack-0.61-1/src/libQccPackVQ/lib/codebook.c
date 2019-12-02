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


int QccVQCodebookInitialize(QccVQCodebook *codebook)
{
  if (codebook == NULL)
    return(0);

  QccStringMakeNull(codebook->filename);
  QccStringCopy(codebook->magic_num, QCCVQCODEBOOK_MAGICNUM);
  QccGetQccPackVersion(&codebook->major_version,
                       &codebook->minor_version,
                       NULL);
  codebook->num_codewords = 0;
  codebook->codeword_dimension = 0;
  codebook->index_length = 0;
  codebook->codewords = NULL;
  codebook->codeword_probs = NULL;
  codebook->codeword_codelengths = NULL;

  return(0);
}


int QccVQCodebookAlloc(QccVQCodebook *codebook)
{
  int return_value;
  int codeword;
  
  if (codebook == NULL)
    return(0);

  if (codebook->num_codewords < 1)
    return(0);
  
  if (codebook->codewords == NULL)
    {
      if ((codebook->codewords = 
           (QccVQCodewordArray)malloc(sizeof(QccVQCodeword) * 
                                      codebook->num_codewords)) == 
          NULL)
        {
          QccErrorAddMessage("(QccVQCodebookAlloc): Error allocating memory");
          goto Error;
        }
      
      for (codeword = 0; codeword < codebook->num_codewords; codeword++)
        if ((codebook->codewords[codeword] = 
             (QccVQCodeword)QccVectorAlloc(codebook->codeword_dimension)) 
            == NULL)
          {
            QccErrorAddMessage("(QccVQCodebookAlloc): Error allocating memory");
            goto Error;
          }
    }
  if (codebook->codeword_probs == NULL)
    if ((codebook->codeword_probs = 
         QccVectorAlloc(codebook->num_codewords)) == NULL)
      {
        QccErrorAddMessage("(QccVQCodebookAlloc): Error allocating codebook probabilities");
        goto Error;
      }
  if (codebook->codeword_codelengths == NULL)
    if ((codebook->codeword_codelengths = 
         QccVectorAlloc(codebook->num_codewords)) == NULL)
      {
        QccErrorAddMessage("(QccVQCodebookAlloc): Error allocating memory");
        goto Error;
      }
  

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
  QccVQCodebookFree(codebook);
 Return:
  return(return_value);
}


int QccVQCodebookRealloc(QccVQCodebook *codebook,
                         int new_num_codewords)
{
  int return_value;
  int codeword;
  int num_common;

  if (new_num_codewords < 1)
    goto Error;
  
  if (new_num_codewords == codebook->num_codewords)
    return(0);

  if (codebook->codewords != NULL)
    {
      if ((codebook->codewords = 
           (QccVQCodewordArray)realloc(codebook->codewords,
                                       sizeof(QccVQCodeword) * 
                                       new_num_codewords)) 
          == NULL)
        {
          QccErrorAddMessage("(QccVQCodebookRealloc): Error reallocating memory");
          goto Error;
        }
      
      num_common = (new_num_codewords > codebook->num_codewords) ? 
        codebook->num_codewords :
        new_num_codewords;
      
      for (codeword = num_common; codeword < new_num_codewords;
           codeword++)
        if ((codebook->codewords[codeword] = 
             (QccVQCodeword)QccVectorAlloc(codebook->codeword_dimension)) == 
            NULL)
          {
            QccErrorAddMessage("(QccVQCodebookRealloc): Error calling QccVectorAlloc()");
            goto Error;
          }
    }
      
  if (codebook->codeword_probs != NULL)
    if ((codebook->codeword_probs = 
         QccVectorResize(codebook->codeword_probs,
                         codebook->num_codewords,
                         new_num_codewords)) == NULL)
      {
        QccErrorAddMessage("(QccVQCodebookRealloc): Error calling QccVectorResize()");
        goto Error;
      }
  if (codebook->codeword_codelengths != NULL)
    if ((codebook->codeword_codelengths = 
         QccVectorResize(codebook->codeword_codelengths,
                         codebook->num_codewords,
                         new_num_codewords)) == NULL)
      {
        QccErrorAddMessage("(QccVQCodebookRealloc): Error calling QccVectorResize()");
        goto Error;
      }
  codebook->num_codewords = new_num_codewords;

  return_value = 0;
  goto Return;
 Error:
  QccVQCodebookFree(codebook);
  return_value = 1;
 Return:
  return(return_value);
}


void QccVQCodebookFree(QccVQCodebook *codebook)
{
  int codeword;
  
  if (codebook == NULL)
    return;

  if (codebook->codewords != NULL)
    {
      for (codeword = 0; codeword < codebook->num_codewords; codeword++)
        QccVectorFree((QccVector)codebook->codewords[codeword]);
      QccFree(codebook->codewords);
      codebook->codewords = NULL;
    }
  QccVectorFree(codebook->codeword_probs);
  codebook->codeword_probs = NULL;
  QccVectorFree(codebook->codeword_codelengths);
  codebook->codeword_codelengths = NULL;

}


int QccVQCodebookCopy(QccVQCodebook *codebook1,
                      const QccVQCodebook *codebook2)
{
  int codeword;
  int component;

  if (codebook1 == NULL)
    return(0);
  if (codebook2 == NULL)
    return(0);
  if (codebook1->codewords == NULL)
    return(0);
  if (codebook2->codewords == NULL)
    return(0);
  
  if ((codebook1->num_codewords != codebook2->num_codewords) ||
      (codebook1->codeword_dimension != codebook2->codeword_dimension))
    {
      QccErrorAddMessage("(QccVQCodebookRealloc): codebooks must have same codeword dimension and number of codewords");
      return(1);
    }

  codebook1->index_length = codebook2->index_length;

  for (codeword = 0; codeword < codebook1->num_codewords; codeword++)
    {
      for (component = 0; component < codebook1->codeword_dimension;
           component++)
        codebook1->codewords[codeword][component] =
          codebook2->codewords[codeword][component];
      codebook1->codeword_probs[codeword] =
        codebook2->codeword_probs[codeword];
      codebook1->codeword_codelengths[codeword] =
        codebook2->codeword_codelengths[codeword];
    }

  return(0);
}


int QccVQCodebookMoveCodewordToFront(QccVQCodebook *codebook, int index)
{
  int codeword;
  QccVQCodeword tmp_codeword = NULL;
  
  if (!index)
    return(0);
  if (index >= codebook->num_codewords)
    {
      QccErrorAddMessage("(QccVQCodebookMoveCodewordToFront): Index is out of range");
      return(1);
    }

  if ((tmp_codeword = 
       (QccVQCodeword)QccVectorAlloc(codebook->codeword_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccVQCodebookMoveCodewordToFront): Error allocating memory");
      return(1);
    }
  QccVectorCopy((QccVector)tmp_codeword, 
                (QccVector)codebook->codewords[index],
                codebook->codeword_dimension);
  
  for (codeword = index; codeword; codeword--)
    QccVectorCopy((QccVector)codebook->codewords[codeword],
                  (QccVector)codebook->codewords[codeword - 1],
                  codebook->codeword_dimension);
  QccVectorCopy((QccVector)codebook->codewords[0],
                (QccVector)tmp_codeword,
                codebook->codeword_dimension);

  QccVectorFree((QccVector)tmp_codeword);

  return(0);
}


int QccVQCodebookSetProbsFromPartitions(QccVQCodebook *codebook,
                                        const int *partition,
                                        int num_partitions)
{
  if ((codebook == NULL) ||
      (partition == NULL) ||
      (num_partitions <= 0))
    return(0);
  
  if (codebook->codeword_probs == NULL)
    return(0);
  
  return(QccVectorGetSymbolProbs(partition, num_partitions,
                                    codebook->codeword_probs,
                                    codebook->num_codewords));
  
}


int QccVQCodebookSetCodewordLengths(QccVQCodebook *codebook)
{
  int codeword;
  int codebook_size;
  
  codebook_size = codebook->num_codewords;
  
  if (codebook == NULL)
    return(0);
  
  if ((codebook->codeword_probs == NULL) ||
      (codebook->codeword_codelengths == NULL))
    return(0);
  
  for (codeword = 0; codeword < codebook_size; codeword++)
    if (codebook->codeword_probs[codeword] > 0.0)
      codebook->codeword_codelengths[codeword] = 
        -QccMathLog2(codebook->codeword_probs[codeword]);
    else
      codebook->codeword_codelengths[codeword] = -1;
  
  return(0);
}


int QccVQCodebookSetIndexLength(QccVQCodebook *codebook)
{
  if (codebook->num_codewords > 0)
    codebook->index_length = 
      (int)(log((double)codebook->num_codewords)/log(2.0) + 0.5);
  else
    codebook->index_length = 0;
  
  return(0);
}


int QccVQCodebookReadHeader(FILE *infile, QccVQCodebook *codebook)
{
  
  if ((infile == NULL) || (codebook == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             codebook->magic_num,
                             &codebook->major_version,
                             &codebook->minor_version))
    {
      QccErrorAddMessage("(QccVQCodebookReadHeader): Error calling QccFileReadMagicNumber()");
      return(1);
    }
  
  if (strcmp(codebook->magic_num, QCCVQCODEBOOK_MAGICNUM))
    {
      QccErrorAddMessage("(QccVQCodebookReadHeader): %s is not of codebook (%s) type",
                         codebook->filename, QCCVQCODEBOOK_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(codebook->num_codewords));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccVQCodebookReadHeader): Error reading number of codewords in codebook %s",
                         codebook->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccVQCodebookReadHeader): Error reading codeword dimension in codebook %s",
                         codebook->filename);
      return(1);
    }
  fscanf(infile, "%d%*1[\n]", &(codebook->codeword_dimension));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccVQCodebookReadHeader): Error reading codeword dimension in codebook %s",
                         codebook->filename);
      return(1);
    }
  
  return(0);
}


int QccVQCodebookReadData(FILE *infile, QccVQCodebook *codebook)
{
  int codeword, component;
  
  if ((infile == NULL) || (codebook == NULL))
    return(0);
  
  if (QccVQCodebookAlloc(codebook))
    {
      QccErrorAddMessage("(QccVQCodebookReadData): Error calling QccVQCodebookAlloc()");
      return(1);
    }

  QccVQCodebookSetIndexLength(codebook);

  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    for (component = 0; component < codebook->codeword_dimension; component++)
      if (QccFileReadDouble(infile,
                            &(codebook->codewords[codeword][component])))
        {
          QccErrorAddMessage("(QccVQCodebookReadData): Error calling QccFileReadDouble()");
          return(1);
        }
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    if (QccFileReadDouble(infile,
                          &(codebook->codeword_probs[codeword])))
      {
        QccErrorAddMessage("(QccVQCodebookReadData): Error calling QccFileReadDouble()");
        return(1);
      }
  
  if (QccVQCodebookSetCodewordLengths(codebook))
    {
      QccErrorAddMessage("(QccVQCodebookReadData): Error calling QccVQCodebookSetCodewordLengths()");
      return(1);
    }
  
  return(0);
}


int QccVQCodebookRead(QccVQCodebook *codebook)
{
  FILE *infile = NULL;
  
  if (codebook == NULL)
    return(0);
  
  if ((infile = QccFileOpen(codebook->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccVQCodebookRead): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccVQCodebookReadHeader(infile, codebook))
    {
      QccErrorAddMessage("(QccVQCodebookRead): Error calling QccVQCodebookReadHeader()");
      return(1);
    }
  
  if (QccVQCodebookReadData(infile, codebook))
    {
      QccErrorAddMessage("(QccVQCodebookRead): Error calling QccVQCodebookReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


int QccVQCodebookWriteHeader(FILE *outfile, const QccVQCodebook *codebook)
{
  if ((outfile == NULL) || (codebook == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCVQCODEBOOK_MAGICNUM))
    goto Error;

  fprintf(outfile, "%d %d\n",
          codebook->num_codewords,
          codebook->codeword_dimension);
  if (ferror(outfile))
    goto Error;
  
  return(0);

 Error:
  QccErrorAddMessage("(QccVQCodebookWriteHeader): Error writing header to %s",
                     codebook->filename);
  return(1);
  
}


int QccVQCodebookWriteData(FILE *outfile, const QccVQCodebook *codebook)
{
  int codeword, component;
  
  if ((outfile == NULL) || (codebook == NULL))
    return(0);
  
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    {
      for (component = 0; component < codebook->codeword_dimension; 
           component++)
        if (QccFileWriteDouble(outfile,
                               codebook->codewords[codeword][component]))
          {
            QccErrorAddMessage("(QccVQCodebookWriteData): Error writing data to %s",
                               codebook->filename);
            return(1);
          }
    }
  
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    if (QccFileWriteDouble(outfile,
                           codebook->codeword_probs[codeword]))
      {
        QccErrorAddMessage("(QccVQCodebookWrite): Error writing data to %s",
                           codebook->filename);
        return(1);
      }
  
  return(0);
}


int QccVQCodebookWrite(const QccVQCodebook *codebook)
{
  FILE *outfile;
  
  if (codebook == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(codebook->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccVQCodebookWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccVQCodebookWriteHeader(outfile, codebook))
    {
      QccErrorAddMessage("(QccVQCodebookWrite): Error calling QccVQCodebookWriteHeader()");
      return(1);
    }
  
  if (QccVQCodebookWriteData(outfile, codebook))
    {
      QccErrorAddMessage("(QccVQCodebookWrite): Error calling QccVQCodebookWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


int QccVQCodebookPrint(const QccVQCodebook *codebook)
{
  int component, codeword;
  
  if (codebook == NULL)
    return(0);
  
  if (QccFilePrintFileInfo(codebook->filename,
                           codebook->magic_num,
                           codebook->major_version,
                           codebook->minor_version))
    return(1);

  printf("Num of codewords: %d\n", codebook->num_codewords);
  printf("Codeword dimension: %d\n\n", codebook->codeword_dimension);
  
  if (!codebook->num_codewords)
    return(0);

  printf("Index\tProb\tCodeword\n\n");
  
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    {
      printf("%4d\t%.3f\t", codeword, codebook->codeword_probs[codeword]);
      for (component = 0; component < codebook->codeword_dimension; 
           component++)
        printf("% 10.4f ", codebook->codewords[codeword][component]);
      printf("\n");
    }
  
  return(0);
}


int QccVQCodebookCreateRandomCodebook(QccVQCodebook *codebook, 
                                      double max, double min)
{
  int codeword, component;
  
  if (codebook == NULL)
    return(0);
  
  if (QccVQCodebookAlloc(codebook))
    {
      QccErrorAddMessage("(QccVQCodebookCreateRandomCodebook): Error calling QccVQCodebookAlloc()");
      return(1);
    }
  
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    for (component = 0; component < codebook->codeword_dimension; component++)
      {
        codebook->codewords[codeword][component] =
          QccMathRand()*(max - min) + min;
        codebook->codeword_probs[codeword] = 
          1.0/(double)codebook->num_codewords;
      }
  
  if (QccVQCodebookSetCodewordLengths(codebook))
    {
      QccErrorAddMessage("(QccVQCodebookCreateRandomCodebook): Error calling QccVQCodebookSetCodewordLengths()");
      return(1);
    }
  
  return(0);
}


int QccVQCodebookAddCodeword(QccVQCodebook *codebook,
                             QccVQCodeword codeword)
{

  if (codebook == NULL)
    return(0);
  if (codeword == NULL)
    return(0);

  if (codebook->num_codewords > 0)
    {
      if (QccVQCodebookRealloc(codebook,
                               codebook->num_codewords + 1))
        {
          QccErrorAddMessage("(QccVQCodebookAddCodeword): Error calling QccVQCodebookRealloc()");
          return(1);
        }
    }
  else
    {
      codebook->num_codewords++;
      if (QccVQCodebookAlloc(codebook))
        {
          QccErrorAddMessage("(QccVQCodebookAddCodeword): Error calling QccVQCodebookAlloc()");
          return(1);
        }
      codebook->codeword_probs[codebook->num_codewords - 1] = 1.0;
    }

  QccVectorCopy((QccVector)codebook->codewords[codebook->num_codewords - 1],
                (QccVector)codeword, codebook->codeword_dimension);
  QccVQCodebookSetIndexLength(codebook);
  QccVQCodebookSetCodewordLengths(codebook);

  return(0);
}
