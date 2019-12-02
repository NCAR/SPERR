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


int QccWAVMultiwaveletInitialize(QccWAVMultiwavelet *multiwavelet)
{
  if (multiwavelet == NULL)
    return(0);

  QccWAVVectorFilterBankInitialize(&multiwavelet->vector_filter_bank);

  return(0);
}


int QccWAVMultiwaveletAlloc(QccWAVMultiwavelet *multiwavelet)
{
  if (multiwavelet == NULL)
    return(0);

  if (QccWAVVectorFilterBankAlloc(&multiwavelet->vector_filter_bank))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletAlloc): Error calling QccWAVVectorFilterBankAlloc()");
      return(1);
    }

  return(0);
}


void QccWAVMultiwaveletFree(QccWAVMultiwavelet *multiwavelet)
{
  if (multiwavelet == NULL)
    return;

  QccWAVVectorFilterBankFree(&multiwavelet->vector_filter_bank);
}


int QccWAVMultiwaveletCreate(QccWAVMultiwavelet *multiwavelet,
                             const QccString multiwavelet_filename)
{
  FILE *infile;
  QccString magic_number;
  QccString multiwavelet_filename1;

  if (multiwavelet == NULL)
    return(0);
  if (multiwavelet_filename == NULL)
    return(0);

  QccStringCopy(multiwavelet_filename1, multiwavelet_filename);

  if ((infile = 
       QccFilePathSearchOpenRead(multiwavelet_filename1, 
                                 QCCWAVWAVELET_PATH_ENV,
                                 QCCMAKESTRING(QCCPACK_WAVELET_PATH_DEFAULT)))
       == NULL)
    {
      QccErrorAddMessage("(QccWAVMultiwaveletCreate): Error calling QccFilePathSeachOpenRead()");
      return(1);
    }
  
  if (QccFileReadMagicNumber(infile, magic_number,
                             NULL, NULL))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletCreate): Error calling QccFileReadMagicNumber()");
      return(1);
    }
  
  QccFileClose(infile);

  QccStringCopy(multiwavelet->vector_filter_bank.filename,
                multiwavelet_filename1);
  if (QccWAVVectorFilterBankRead(&multiwavelet->vector_filter_bank))
    {
      QccErrorAddMessage("(QccWAVMultiwaveletCreate): Error calling QccWAVVectorFilterBankRead()");
      return(1);
    }

  return(0);
}


void QccWAVMultiwaveletPrint(const QccWAVMultiwavelet *multiwavelet)
{
  if (multiwavelet == NULL)
    return;

  printf("---------------------------------------------------------------------------\n\n");
  printf("  Multiwavelet\n");

  printf("    Implementation: vector filter bank\n");
  printf("\n    Filter Bank:\n");
  QccWAVVectorFilterBankPrint(&multiwavelet->vector_filter_bank);
}
