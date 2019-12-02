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


int QccAVQSideInfoInitialize(QccAVQSideInfo *sideinfo)
{
  if (sideinfo == NULL)
    return(0);
  
  QccStringMakeNull(sideinfo->filename);
  sideinfo->fileptr = NULL;
  QccStringCopy(sideinfo->magic_num, QCCAVQSIDEINFO_MAGICNUM);
  QccGetQccPackVersion(&sideinfo->major_version,
                       &sideinfo->minor_version,
                       NULL);
  QccStringMakeNull(sideinfo->program_name);
  sideinfo->max_codebook_size = 0;
  sideinfo->N = 0;
  sideinfo->vector_dimension = 0;
  sideinfo->vector_component_alphabet_size = 0;
  QccSQScalarQuantizerInitialize(&sideinfo->codebook_coder);

  return(0);
}


int QccAVQSideInfoReadHeader(QccAVQSideInfo *sideinfo)
{
  if (sideinfo == NULL)
    return(0);

  if (QccFileReadMagicNumber(sideinfo->fileptr,
                             sideinfo->magic_num,
                             &sideinfo->major_version,
                             &sideinfo->minor_version))
    {
      QccErrorAddMessage("(QccAVQSideInfoReadHeader): Error reading magic number in sideinfo %s",
                         sideinfo->filename);
      return(1);
    }

  if (strcmp(sideinfo->magic_num, QCCAVQSIDEINFO_MAGICNUM))
    {
      QccErrorAddMessage("(QccAVQSideInfoReadHeader): %s is not of sideinfo (%s) type",
                         sideinfo->filename, QCCAVQSIDEINFO_MAGICNUM);
      return(1);
    }

  if (QccFileReadString(sideinfo->fileptr, sideinfo->program_name))
    {
      QccErrorAddMessage("(QccAVQSideInfoReadHeader): Error reading program name in sideinfo %s",
                         sideinfo->filename);
      return(1);
    }

  fscanf(sideinfo->fileptr, "%d", &(sideinfo->max_codebook_size));
  if (ferror(sideinfo->fileptr) || feof(sideinfo->fileptr))
    {
      QccErrorAddMessage("(QccAVQSideInfoReadHeader): Error reading max_codebook_size in sideinfo %s",
                         sideinfo->filename);
      return(1);
    }

  fscanf(sideinfo->fileptr, "%d", &(sideinfo->N));
  if (ferror(sideinfo->fileptr) || feof(sideinfo->fileptr))
    {
      QccErrorAddMessage("(QccAVQSideInfoReadHeader): Error reading N in sideinfo %s",
                         sideinfo->filename);
      return(1);
    }
  
  fscanf(sideinfo->fileptr, "%d", &(sideinfo->vector_dimension));
  if (ferror(sideinfo->fileptr) || feof(sideinfo->fileptr))
    {
      QccErrorAddMessage("(QccAVQSideInfoReadHeader): Error reading vector dimension in sideinfo %s",
                         sideinfo->filename);
      return(1);
    }
  
  fscanf(sideinfo->fileptr, "%d", &(sideinfo->vector_component_alphabet_size));
  if (ferror(sideinfo->fileptr) || feof(sideinfo->fileptr))
    {
      QccErrorAddMessage("(QccAVQSideInfoReadHeader): Error reading vector component alphabet size in sideinfo %s",
                         sideinfo->filename);
      return(1);
    }
  
  return(0);
}


int QccAVQSideInfoStartRead(QccAVQSideInfo *sideinfo)
{
  if ((sideinfo->fileptr = QccFileOpen(sideinfo->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccAVQSideInfoRead): Error opening %s for reading",
                         sideinfo->filename);
      return(1);
    }

  if (QccAVQSideInfoReadHeader(sideinfo))
    {
      QccErrorAddMessage("(QccAVQSideInfoRead): Error reading header of %s",
                         sideinfo->filename);
      return(1);
    }

  return(0);
}


int QccAVQSideInfoEndRead(QccAVQSideInfo *sideinfo)
{
  if (sideinfo == NULL)
    return(0);
  QccFileClose(sideinfo->fileptr);
  return(0);
}


int QccAVQSideInfoWriteHeader(const QccAVQSideInfo *sideinfo)
{
  if (sideinfo == NULL)
    return(0);
  if (sideinfo->fileptr == NULL)
    return(0);

  if (QccFileWriteMagicNumber(sideinfo->fileptr, QCCAVQSIDEINFO_MAGICNUM))
    goto QccErr;

  fprintf(sideinfo->fileptr, "%s\n",
          sideinfo->program_name);
  if (ferror(sideinfo->fileptr))
    goto QccErr;

  fprintf(sideinfo->fileptr, "%d\n%d\n%d %d\n",
          sideinfo->max_codebook_size,
          sideinfo->N,
          sideinfo->vector_dimension,
          sideinfo->vector_component_alphabet_size);
  if (ferror(sideinfo->fileptr))
    goto QccErr;

  return(0);

 QccErr:
  QccErrorAddMessage("(QccAVQSideInfoWriteHeader): Error writing header to %s",
                     sideinfo->filename);
  return(1);
}


int QccAVQSideInfoStartWrite(QccAVQSideInfo *sideinfo)
{

  if (sideinfo == NULL)
    return(0);

  if ((sideinfo->fileptr = QccFileOpen(sideinfo->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccAVQSideInfoWrite): Error opening %s for writing",
                         sideinfo->filename);
      return(1);
    }

  if (QccAVQSideInfoWriteHeader(sideinfo))
    {
      QccErrorAddMessage("(QccAVQSideInfoWrite): Error writing header to %s",
                         sideinfo->filename);
      return(1);
    }

  return(0);
}


int QccAVQSideInfoEndWrite(const QccAVQSideInfo *sideinfo)
{
  if (sideinfo == NULL)
    return(0);
  QccFileClose(sideinfo->fileptr);
  return(0);
}


int QccAVQSideInfoPrint(QccAVQSideInfo *sideinfo)
{
  int flag_cnt;
  QccAVQSideInfoSymbol *current_symbol;

  if (sideinfo == NULL)
    return(0);
  if (sideinfo->fileptr == NULL)
    return(0);

  if (QccFilePrintFileInfo(sideinfo->filename,
                           sideinfo->magic_num,
                           sideinfo->major_version,
                           sideinfo->minor_version))
    return(1);

  printf("Generating program: %s\n", sideinfo->program_name);
  printf("Max codebook size: %d\n", 
         sideinfo->max_codebook_size);
  printf("N: %d\n", sideinfo->N);
  printf("Vector dimension: %d\tvector component alphabet size: %d\n", 
         sideinfo->vector_dimension,
         sideinfo->vector_component_alphabet_size);
  printf("Codebook coder: %s\n", sideinfo->codebook_coder.filename);
  
  printf("------------------------------------\n\n");
  flag_cnt = 0;
  while (flag_cnt < sideinfo->N)
    {
      if ((current_symbol = 
           QccAVQSideInfoSymbolRead(sideinfo, NULL)) == NULL)
        {
          QccErrorAddMessage("(QccAVQSideInfoPrint): Error calling QccAVQSideInfoSymbolRead()");
          return(1);
        }
      if (QccAVQSideInfoSymbolPrint(sideinfo, current_symbol))
        {
          QccErrorAddMessage("(QccAVQSideInfoPrint): Error calling QccAVQSideInfoSymbolPrint()");
          return(1);
        }
      if (current_symbol->type == QCCAVQSIDEINFO_FLAG)
        flag_cnt++;
      QccAVQSideInfoSymbolFree(current_symbol);
    }

  return(0);
}
