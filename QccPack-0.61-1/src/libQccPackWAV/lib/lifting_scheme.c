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


int QccWAVLiftingSchemeInitialize(QccWAVLiftingScheme *lifting_scheme)
{
  if (lifting_scheme == NULL)
    return(0);
  
  QccStringMakeNull(lifting_scheme->filename);
  QccStringCopy(lifting_scheme->magic_num, QCCWAVLIFTINGSCHEME_MAGICNUM);
  QccGetQccPackVersion(&lifting_scheme->major_version,
                       &lifting_scheme->minor_version,
                       NULL);
  lifting_scheme->scheme = -1;

  return(0);
}


int QccWAVLiftingSchemePrint(const QccWAVLiftingScheme *lifting_scheme)
{
  if (lifting_scheme == NULL)
    return(0);
  
  if (QccFilePrintFileInfo(lifting_scheme->filename,
                           lifting_scheme->magic_num,
                           lifting_scheme->major_version,
                           lifting_scheme->minor_version))
    return(1);
  
  
  switch (lifting_scheme->scheme)
    {
    case QCCWAVLIFTINGSCHEME_LWT:
      printf("  Lazy Wavelet transform\n\n");
      break;
    case QCCWAVLIFTINGSCHEME_Daubechies4:
      printf("  Daubechies Orthonormal Wavelet, Length 4\n");
      printf("        -- Lifted Implementation --\n\n");
      break;
    case QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau9_7:
      printf("  Cohen-Daubechies-Feauveau Biorthogonal Wavelet, Length 9-7\n");
      printf("        -- Lifted Implementation --\n\n");
      break;
    case QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau5_3:
      printf("  Cohen-Daubechies-Feauveau Biorthogonal Wavelet, Length 5-3\n");
      printf("        -- Lifted Implementation --\n");
      printf("    (linear lifting with unitary scaling)\n\n");
      break;
    case QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau9_7:
      printf("  Integer Cohen-Daubechies-Feauveau Biorthogonal Wavelet, Length 9-7\n");
      printf("        -- Lifted Implementation --\n\n");
      break;
    default:
      QccErrorAddMessage("(QccWAVLiftingSchemePrint): Unrecognized lifting scheme");
      return(1);
      break;
    }
  
  return(0);
}


static int QccWAVLiftingSchemeReadHeader(FILE *infile,
                                         QccWAVLiftingScheme *lifting_scheme)
{
  
  if ((infile == NULL) || (lifting_scheme == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             lifting_scheme->magic_num, 
                             &lifting_scheme->major_version,
                             &lifting_scheme->minor_version))
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeReadHeader): Error reading magic number in lifting scheme %s",
                         lifting_scheme->filename);
      return(1);
    }
  
  if (strcmp(lifting_scheme->magic_num, QCCWAVLIFTINGSCHEME_MAGICNUM))
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeReadHeader): %s is not of filter-bank (%s) type",
                         lifting_scheme->filename,
                         QCCWAVLIFTINGSCHEME_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(lifting_scheme->scheme));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeReadHeader): Error reading scheme in lifting scheme %s",
                         lifting_scheme->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeReadHeader): Error reading in lifting scheme %s",
                         lifting_scheme->filename);
      return(1);
    }
  
  return(0);
}


int QccWAVLiftingSchemeRead(QccWAVLiftingScheme *lifting_scheme)
{
  FILE *infile = NULL;

  if (lifting_scheme == NULL)
    return(0);
  
  if ((infile = 
       QccFileOpen(lifting_scheme->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeRead): Error calling QccFileOpen()");
      return(1);
    }

  if (QccWAVLiftingSchemeReadHeader(infile, lifting_scheme))
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeRead): Error calling QccWAVLiftingSchemeReadHeader()");
      return(1);
    }
  
  /*
  if (QccWAVLiftingSchemeReadData(infile, lifting_scheme))
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeRead): Error calling QccWAVLiftingSchemeReadData()");
      return(1);
    }
  */
  
  QccFileClose(infile);
  return(0);
}


static int QccWAVLiftingSchemeWriteHeader(FILE *outfile,
                                          const QccWAVLiftingScheme
                                          *lifting_scheme)
{
  if ((outfile == NULL) || (lifting_scheme == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCWAVLIFTINGSCHEME_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d\n",
          lifting_scheme->scheme);
  
  if (ferror(outfile))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccWAVLiftingSchemeWriteHeader): Error writing header to %s",
                     lifting_scheme->filename);
  return(1);
  
}


int QccWAVLiftingSchemeWrite(const QccWAVLiftingScheme *lifting_scheme)
{
  FILE *outfile;
  
  if (lifting_scheme == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(lifting_scheme->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVLiftingSchemeWriteHeader(outfile, lifting_scheme))
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeWrite): Error calling QccWAVLiftingSchemeWriteHeader()");
      return(1);
    }
  
  /*
  if (QccWAVLiftingSchemeWriteData(outfile, lifting_scheme))
    {
      QccErrorAddMessage("(QccWAVLiftingSchemeWrite): Error calling QccWAVLiftingSchemeWriteData()");
      return(1);
    }
  */
  
  QccFileClose(outfile);
  return(0);
}


int QccWAVLiftingSchemeBiorthogonal(const QccWAVLiftingScheme *lifting_scheme)
{
  switch (lifting_scheme->scheme)
    {
    case QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau9_7:
      return(1);
      break;
    case QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau5_3:
      return(1);
      break;
    case QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau9_7:
      return(1);
      break;
    default:
      return(0);
      break;
    }

  return(0);
}


int QccWAVLiftingSchemeInteger(const QccWAVLiftingScheme *lifting_scheme)
{
  switch (lifting_scheme->scheme)
    {
    case QCCWAVLIFTINGSCHEME_IntLWT:
      return(1);
      break;
    case QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau9_7:
      return(1);
      break;
    case QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau5_3:
      return(1);
      break;
    default:
      return(0);
      break;
    }

  return(0);
}
