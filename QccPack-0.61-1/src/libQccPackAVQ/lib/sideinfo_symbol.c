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


int QccAVQSideInfoSymbolInitialize(QccAVQSideInfoSymbol *sideinfo_symbol)
{
  if (sideinfo_symbol == NULL)
    return(0);

  sideinfo_symbol->type = 0;
  sideinfo_symbol->flag = 0;
  sideinfo_symbol->address = 0;
  sideinfo_symbol->vector = NULL;

  return(0);
}


QccAVQSideInfoSymbol *QccAVQSideInfoSymbolAlloc(int vector_dimension)
{
  QccAVQSideInfoSymbol *symbol = NULL;

  if ((symbol = 
       (QccAVQSideInfoSymbol *)malloc(sizeof(QccAVQSideInfoSymbol))) == NULL)
    {
      QccErrorAddMessage("(QccAVQSideInfoSymbolAlloc): Error allocating memory");
      goto QccAVQError;
    }

  QccAVQSideInfoSymbolInitialize(symbol);

  if ((symbol->vector = QccVectorAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccAVQSideInfoSymbolAlloc): Error calling QccVectorAlloc()");
      goto QccAVQError;
    }

  if ((symbol->vector_indices = 
       (int *)malloc(sizeof(int) * vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccAVQSideInfoSymbolAlloc): Error allocating memory");
      goto QccAVQError;
    }

  symbol->type = QCCAVQSIDEINFO_NULLSYMBOL;

  goto QccAVQExit;

 QccAVQError:
  if (symbol != NULL)
    {
      if (symbol->vector != NULL)
        {
          QccVectorFree(symbol->vector);
          symbol->vector = NULL;
        }
      if (symbol->vector_indices != NULL)
        {
          QccFree(symbol->vector_indices);
          symbol->vector_indices = NULL;
        }
      symbol = NULL;
    }

 QccAVQExit:
  return(symbol);
}


int QccAVQSideInfoSymbolFree(QccAVQSideInfoSymbol *symbol)
{
  if (symbol == NULL)
    return(0);

  if (symbol->vector != NULL)
    QccVectorFree(symbol->vector);
  if (symbol->vector_indices != NULL)
    QccFree(symbol->vector_indices);

  QccFree(symbol);
  return(0);
}


QccAVQSideInfoSymbol *QccAVQSideInfoSymbolRead(QccAVQSideInfo *sideinfo,
                                               QccAVQSideInfoSymbol *symbol)
{
  int component;
  int symbol_allocated = 0;

  if (sideinfo == NULL)
    return(0);
  if (sideinfo->fileptr == NULL)
    return(0);

  if (symbol == NULL)
    {
      if ((symbol = 
           QccAVQSideInfoSymbolAlloc(sideinfo->vector_dimension)) == NULL)
        {
          QccErrorAddMessage("(QccAVQSideInfoSymbolRead): Error calling QccAVQSideInfoSymbolAlloc()");
          goto AVQReadError;
        }
      symbol_allocated = 1;
    }

  fscanf(sideinfo->fileptr, "%d", &(symbol->type));
  if (ferror(sideinfo->fileptr) || feof(sideinfo->fileptr))
    {
      QccErrorAddMessage("(QccAVQSideInfoSymbolRead): Error reading %s",
                         sideinfo->filename);
      goto AVQReadError;
    }

  switch (symbol->type)
    {
    case QCCAVQSIDEINFO_FLAG:
      fscanf(sideinfo->fileptr, "%d", &(symbol->flag));
      if (ferror(sideinfo->fileptr) || feof(sideinfo->fileptr))
        {
          QccErrorAddMessage("(QccAVQSideInfoSymbolRead): Error reading %s",
                             sideinfo->filename);
          goto AVQReadError;
        }
      break;
    case QCCAVQSIDEINFO_ADDRESS:
      fscanf(sideinfo->fileptr, "%d", &(symbol->address));
      if (ferror(sideinfo->fileptr) || feof(sideinfo->fileptr))
        {
          QccErrorAddMessage("(QccAVQSideInfoSymbolRead): Error reading %s",
                             sideinfo->filename);
          goto AVQReadError;
        }
      break;
    case QCCAVQSIDEINFO_VECTOR:
      for (component = 0; component < sideinfo->vector_dimension; component++)
        {
          fscanf(sideinfo->fileptr, "%d", 
                 &(symbol->vector_indices[component]));
          if (ferror(sideinfo->fileptr) || feof(sideinfo->fileptr))
            {
              QccErrorAddMessage("(QccAVQSideInfoSymbolRead): Error reading %s",
                                 sideinfo->filename);
              goto AVQReadError;
            }
        }
      if (QccSQInverseScalarQuantizeVector(symbol->vector_indices,
                                           &(sideinfo->codebook_coder),
                                           symbol->vector,
                                           sideinfo->vector_dimension))
        {
          QccErrorAddMessage("(QccAVQSideInfoSymbolRead): Error calling QccSQInverseScalarQuantizeVector()");
          goto AVQReadError;
        }
      break;
    default:
      QccErrorAddMessage("(QccAVQSideInfoSymbolRead): Symbol type %d is invalid",
                         symbol->type);
      goto AVQReadError;
    }

  return(symbol);

 AVQReadError:
  if (symbol_allocated)
    QccAVQSideInfoSymbolFree(symbol);
  return(NULL);
}


QccAVQSideInfoSymbol *QccAVQSideInfoSymbolReadDesired(QccAVQSideInfo *sideinfo,
                                                      int desired_symbol,
                                                      QccAVQSideInfoSymbol 
                                                      *symbol)
{
  int symbol_allocated = 0;

  if (symbol == NULL)
    symbol_allocated = 1;

  if (QccAVQSideInfoSymbolRead(sideinfo, symbol) == NULL)
    {
      QccErrorAddMessage("(QccAVQSideInfoSymbolReadDesired): Error calling QccAVQSideInfoSymbolRead()");
      return(NULL);
    }
  
  if (symbol->type != desired_symbol)
    {
      QccErrorAddMessage("(QccAVQSideInfoSymbolReadDesired): Error reading sideinfo %s, unexpected symbol of type %d",
                         sideinfo->filename,
                         symbol->type);
      if (symbol_allocated)
        QccAVQSideInfoSymbolFree(symbol);
      return(NULL);
    }

  return(symbol);
}


QccAVQSideInfoSymbol *QccAVQSideInfoSymbolReadNextFlag(QccAVQSideInfo 
                                                       *sideinfo,
                                                       QccAVQSideInfoSymbol 
                                                       *symbol)
{
  do
    {
      if ((symbol = 
           QccAVQSideInfoSymbolRead(sideinfo, symbol)) == NULL)
        {
          QccErrorAddMessage("(QccAVQSideInfoSymbolReadNextFlag): Error calling QccAVQSideInfoSymbolRead()");
          return(NULL);
        }
    }
  while (symbol->type != QCCAVQSIDEINFO_FLAG);

  return(symbol);
}


int QccAVQSideInfoSymbolWrite(QccAVQSideInfo *sideinfo, 
                              const QccAVQSideInfoSymbol *symbol)
{
  int component;

  if ((sideinfo == NULL) || (symbol == NULL))
    return(0);
  if (sideinfo->fileptr == NULL)
    return(0);

  switch (symbol->type)
    {
    case QCCAVQSIDEINFO_FLAG:
      fprintf(sideinfo->fileptr, "%d %d\n",
              symbol->type, symbol->flag);
      break;
    case QCCAVQSIDEINFO_ADDRESS:
      fprintf(sideinfo->fileptr, "%d %d\n",
              symbol->type, symbol->address);
      break;
    case QCCAVQSIDEINFO_VECTOR:
      fprintf(sideinfo->fileptr, "%d ",
              symbol->type);
      for (component = 0; component < sideinfo->vector_dimension;
           component++)
        fprintf(sideinfo->fileptr, "%d ",
                symbol->vector_indices[component]);
      fprintf(sideinfo->fileptr, "\n");
      break;
    default:
      QccErrorAddMessage("(QccAVQSideInfoSymbolWrite): Symbol type %d is invalid",
                         symbol->type);
      goto AVQWriteError;
    }

  if (ferror(sideinfo->fileptr))
    {
      QccErrorAddMessage("(QccAVQSideInfoSymbolWrite): Error writing to file %s",
                         sideinfo->filename);
      goto AVQWriteError;
    }

  return(0);

 AVQWriteError:
  return(1);
}


int QccAVQSideInfoSymbolPrint(QccAVQSideInfo *sideinfo, 
                              const QccAVQSideInfoSymbol *symbol)
{
  int component;

  if ((sideinfo == NULL) || (symbol == NULL))
    return(0);

  switch (symbol->type)
    {
    case QCCAVQSIDEINFO_FLAG:
      printf("Flag: ");
      if (symbol->flag == QCCAVQSIDEINFO_FLAG_NOUPDATE)
        printf("noupdate\n");
      else
        printf("update\n");
      break;
    case QCCAVQSIDEINFO_ADDRESS:
      printf("Address: %d\n", symbol->address);
      break;
    case QCCAVQSIDEINFO_VECTOR:
      printf("Vector: ");
      for (component = 0; component < sideinfo->vector_dimension; component++)
        printf("% 10.4f (%d) ", 
               symbol->vector[component],
               symbol->vector_indices[component]);
      printf("\n");
      break;
    default:
      QccErrorAddMessage("(AVQPrintSymbol): Unsupported symbol type %d",
                         symbol->type);
      return(1);
    }
  return(0);
}


int QccAVQSideInfoCodebookCoder(QccAVQSideInfo *sideinfo,
                                QccAVQSideInfoSymbol *new_codeword_symbol)
{
  if (sideinfo == NULL)
    return(0);
  if (new_codeword_symbol == NULL)
    return(0);

  if (QccSQScalarQuantizeVector(new_codeword_symbol->vector,
                                sideinfo->vector_dimension,
                                &(sideinfo->codebook_coder),
                                NULL,
                                new_codeword_symbol->vector_indices))
    {
      QccErrorAddMessage("(QccAVQSideInfoCodebookCoder): Error calling QccSQScalarQuantizeVector()");
      return(1);
    }

  if (QccSQInverseScalarQuantizeVector(new_codeword_symbol->vector_indices,
                                       &(sideinfo->codebook_coder),
                                       new_codeword_symbol->vector,
                                       sideinfo->vector_dimension))
    {
      QccErrorAddMessage("(QccAVQSideInfoCodebookCoder): Error calling QccSQInverseScalarQuantizeVector()");
      return(1);
    }

  return(0);
}


