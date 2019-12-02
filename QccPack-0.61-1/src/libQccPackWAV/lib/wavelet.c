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


int QccWAVInit()
{
  return(0);
}


int QccWAVWaveletInitialize(QccWAVWavelet *wavelet)
{
  if (wavelet == NULL)
    return(0);

  wavelet->implementation = -1;
  wavelet->boundary = -1;
  QccWAVFilterBankInitialize(&wavelet->filter_bank);
  QccWAVLiftingSchemeInitialize(&wavelet->lifting_scheme);

  return(0);
}


int QccWAVWaveletAlloc(QccWAVWavelet *wavelet)
{
  if (wavelet == NULL)
    return(0);

  if (QccWAVFilterBankAlloc(&wavelet->filter_bank))
    {
      QccErrorAddMessage("(QccWAVWaveletAlloc): Error calling QccWAVFilterBankAlloc()");
      return(1);
    }

  return(0);
}


void QccWAVWaveletFree(QccWAVWavelet *wavelet)
{
  if (wavelet == NULL)
    return;

  QccWAVFilterBankFree(&wavelet->filter_bank);
}


void QccWAVWaveletPrint(const QccWAVWavelet *wavelet)
{
  if (wavelet == NULL)
    return;

  printf("---------------------------------------------------------------------------\n\n");
  printf("  Wavelet\n");

  switch (wavelet->implementation)
    {
    case QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK:
      printf("    Implementation: filter bank\n");
      printf("    Boundary: ");
      switch (wavelet->boundary)
        {
        case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
          printf("symmetric\n");
          break;
        case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
          printf("periodic\n");
          break;
        }
      printf("\n    Filter Bank:\n");
      QccWAVFilterBankPrint(&wavelet->filter_bank);
      break;
    case QCCWAVWAVELET_IMPLEMENTATION_LIFTED:
      break;
    }
}


int QccWAVWaveletCreate(QccWAVWavelet *wavelet,
                        const QccString wavelet_filename,
                        const QccString boundary)
{
  FILE *infile;
  QccString magic_number;
  QccString wavelet_filename1;

  if (wavelet == NULL)
    return(0);
  if (wavelet_filename == NULL)
    return(0);
  if (boundary == NULL)
    return(0);

  QccStringCopy(wavelet_filename1, wavelet_filename);

  if (QccSetEnv(QCCWAVWAVELET_PATH_ENV,
                QCCMAKESTRING(QCCPACK_WAVELET_PATH_DEFAULT)))
    {
      QccErrorAddMessage("(QccWAVInit): Error calling QccSetEnv()");
      QccErrorExit();
    }

  if ((infile = 
       QccFilePathSearchOpenRead(wavelet_filename1, 
                                 QCCWAVWAVELET_PATH_ENV,
                                 QCCMAKESTRING(QCCPACK_WAVELET_PATH_DEFAULT)))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVWaveletCreate): Error calling QccFilePathSearchOpenRead()");
      return(1);
    }
  
  if (QccFileReadMagicNumber(infile, magic_number,
                             NULL, NULL))
    {
      QccErrorAddMessage("(QccWAVWaveletCreate): Error calling QccFileReadMagicNumber()");
      return(1);
    }
  
  QccFileClose(infile);

  if (!strncmp(magic_number, QCCWAVFILTERBANK_MAGICNUM, 3))
    wavelet->implementation =  QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK;
  else
    if (!strncmp(magic_number, QCCWAVLIFTINGSCHEME_MAGICNUM, 3))
      wavelet->implementation =  QCCWAVWAVELET_IMPLEMENTATION_LIFTED       ;
    else
      {
        QccErrorAddMessage("(QccWAVWaveletCreate): Unrecognized wavelet implementation");
        return(1);
      }

  if (!strncmp(boundary, "periodic", 3))
    wavelet->boundary = QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION;
  else
    if (!strncmp(boundary, "symmetric", 3))
      wavelet->boundary = QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION;
    else
      if (!strncmp(boundary, "boundary", 3))
        wavelet->boundary = QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET;
      else
        {
          QccErrorAddMessage("(QccWAVWaveletCreate): Undefined boundary");
          return(1);
        }

  switch (wavelet->implementation)
    {
    case QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK:
      QccStringCopy(wavelet->filter_bank.filename, wavelet_filename1);
      if (QccWAVFilterBankRead(&wavelet->filter_bank))
        {
          QccErrorAddMessage("(QccWAVWaveletCreate): Error calling QccWAVFilterBankRead()");
          return(1);
        }
      break;
    case QCCWAVWAVELET_IMPLEMENTATION_LIFTED:
      QccStringCopy(wavelet->lifting_scheme.filename, wavelet_filename1);
      if (QccWAVLiftingSchemeRead(&wavelet->lifting_scheme))
        {
          QccErrorAddMessage("(QccWAVWaveletCreate): Error calling QccWAVLiftingSchemeRead()");
          return(1);
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVWaveletCreate): Undefined wavelet implementation");
      return(1);
    }

  return(0);
}


int QccWAVWaveletBiorthogonal(const QccWAVWavelet *wavelet)
{
  switch (wavelet->implementation)
    {
    case QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK:
      return(QccWAVFilterBankBiorthogonal(&wavelet->filter_bank));
      break;
    case QCCWAVWAVELET_IMPLEMENTATION_LIFTED:
      return(QccWAVLiftingSchemeBiorthogonal(&wavelet->lifting_scheme));
      break;
    default:
      return(0);
    }

  return(0);
}
