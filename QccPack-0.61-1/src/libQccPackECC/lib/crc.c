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


void QccECCcrcInitialize(QccECCcrc *crc)
{
  QccStringMakeNull(crc->filename);
  QccStringCopy(crc->magic_num, QCCECCCRC_MAGICNUM);
  QccGetQccPackVersion(&crc->major_version,
                       &crc->minor_version,
                       NULL);
  crc->width = 0;
  crc->polynomial = 0;
  crc->current_state = 0;
}


static int QccECCcrcReadHeader(FILE *infile, QccECCcrc *crc)
{
  if ((infile == NULL) || (crc == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             crc->magic_num,
                             &crc->major_version,
                             &crc->minor_version))
    {
      QccErrorAddMessage("(QccECCcrcReadHeader): Error reading magic number in CRC %s",
                         crc->filename);
      return(1);
    }
  
  if (strcmp(crc->magic_num, QCCECCCRC_MAGICNUM))
    {
      QccErrorAddMessage("(QccECCcrcReadHeader): %s is not of CRC (%s) type",
                         crc->filename,
                         QCCECCCRC_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(crc->width));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccECCcrcReadHeader): Error reading width in CRC %s",
                         crc->filename);
      return(1);
    }
  
  if ((crc->width <= 0) || (crc->width > 32))
    {
      QccErrorAddMessage("(QccECCcrcReadHeader): Invalid width (%d) in CRC %s",
                         crc->width,
                         crc->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccECCcrcReadHeader): Error reading width in CRC %s",
                         crc->filename);
      return(1);
    }
  
  return(0);
}


static int QccECCcrcReadData(FILE *infile, QccECCcrc *crc)
{
  if ((infile == NULL) || (crc == NULL))
    return(0);
  
  fscanf(infile, "%x", &(crc->polynomial));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccECCcrcReadData): Error reading data from %s",
                         crc->filename);
      return(1);
    }
  
  return(0);
}


int QccECCcrcRead(QccECCcrc *crc)
{
  FILE *infile = NULL;
  
  if (crc == NULL)
    return(0);
  
  if ((infile = 
       QccFilePathSearchOpenRead(crc->filename,
                                 QCCECCCRC_PATH_ENV,
                                 QCCMAKESTRING(QCCPACK_CODES_PATH_DEFAULT)))
      == NULL)
    {
      QccErrorAddMessage("(QccECCcrcRead): Error calling QccFilePathSearchOpenRead()");
      return(1);
    }
  
  if (QccECCcrcReadHeader(infile, crc))
    {
      QccErrorAddMessage("(QccECCcrcRead): Error calling QccECCcrcReadHeader()");
      return(1);
    }
  
  if (QccECCcrcReadData(infile, crc))
    {
      QccErrorAddMessage("(QccECCcrcRead): Error calling QccECCcrcReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


int QccECCcrcPredefined(QccECCcrc *crc, int predefined_crc_code)
{
  int return_value = 0;
  
  switch (predefined_crc_code)
    {
    case QCCECCCRC_CRC32:
      crc->width = QCCECCCRC_CRC32_WIDTH;
      crc->polynomial = QCCECCCRC_CRC32_POLYNOMIAL;
      break;
    default:
      QccErrorAddMessage("(QccECCcrcPredefined): Undefined CRC code (%d)",
                         predefined_crc_code);
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccECCcrcPrint(const QccECCcrc *crc)
{
  if (crc == NULL)
    return(0);
  
  if (QccFilePrintFileInfo(crc->filename,
                           crc->magic_num,
                           crc->major_version,
                           crc->minor_version))
    return(1);
  
  printf("CRC:\n");
  printf("  Width: %d\n",
         crc->width);
  printf("  Polynomial: %0*x\n",
         (crc->width >> 4) + ((crc->width & 0xf) != 0),
         crc->polynomial);
  
  return(0);
}


int QccECCcrcProcessStart(unsigned int initial_state, QccECCcrc *crc)
{
  if (crc == NULL)
    return(0);

  crc->current_state = initial_state;

  return(0);
}


int QccECCcrcProcessBit(int bit_value, QccECCcrc *crc)
{
  int feedback_bit;

  if (crc == NULL)
    return(0);

  feedback_bit = (crc->current_state & (1 << (crc->width - 1)));

  crc->current_state <<= 1;
  if (crc->width < 32)
    crc->current_state &= ((unsigned int)(1 << crc->width)) - 1;
  crc->current_state |= (bit_value & 1);

  if (feedback_bit)
    crc->current_state ^= crc->polynomial;

  return(0);
}


int QccECCcrcProcessBits(int val, int num_bits, QccECCcrc *crc)
{
  int bit_value;
  int bit;

  if (crc == NULL)
    return(0);
  
  for (bit = 0; bit < num_bits; bit++)
    {
      bit_value = 
        ((val & 0x01) != 0);
      if (QccECCcrcProcessBit(bit_value, crc))
        {
          QccErrorAddMessage("(QccECCcrcProcessBits): Error calling QccECCcrcProcessBit()");
          return(1);
        }
      val >>= 1;
    }

  return(0);
}


int QccECCcrcProcessChar(char ch, QccECCcrc *crc)
{
  int cnt;
  int bit_value;

  if (crc == NULL)
    return(0);

  for (cnt = 0; cnt < 8; cnt++)
    {
      bit_value = (ch & 1);
      if (QccECCcrcProcessBit(bit_value, crc))
        {
          QccErrorAddMessage("(QccECCcrcProcessChar): Error calling QccECCcrcProcessBit()");
          return(1);
        }
      ch >>= 1;
    }

  return(0);
}


int QccECCcrcProcessInt(int val, QccECCcrc *crc)
{
  unsigned char ch[QCC_INT_SIZE];
  int byte_cnt;

  if (crc == NULL)
    return(0);
  
  QccBinaryIntToChar((unsigned int)val, ch);
  for (byte_cnt = 0; byte_cnt < QCC_INT_SIZE; byte_cnt++)
    if (QccECCcrcProcessChar(ch[byte_cnt], crc))
      {
        QccErrorAddMessage("(QccECCcrcProcessInt): Error calling QccECCcrcProcessChar()");
        return(1);
      }

  return(0);
}


int QccECCcrcFlush(unsigned int *checksum, QccECCcrc *crc)
{
  int cnt;

  if (crc == NULL)
    return(0);

  for (cnt = 0; cnt < crc->width; cnt++)
    if (QccECCcrcProcessBit(0, crc))
      {
        QccErrorAddMessage("(QccECCcrcProcessChar): Error calling QccECCcrcProcessBit()");
        return(1);
      }
  
  if (checksum != NULL)
    *checksum = crc->current_state;

  return(0);
}


int QccECCcrcCheck(unsigned int checksum, QccECCcrc *crc)
{
  unsigned int checksum2;

  if (QccECCcrcFlush(&checksum2, crc))
    {
      QccErrorAddMessage("(QccECCcrcCheck): Error calling QccECCcrcFlush()");
      return(1);
    }
  
  if (checksum2 != checksum)
    return(2);

  return(0);
}


int QccECCcrcPutChecksum(QccBitBuffer *output_buffer,
                         unsigned int checksum,
                         const QccECCcrc *crc)
{
  if (QccBitBufferPutBits(output_buffer,
                          (int)checksum,
                          crc->width))
    {
      QccErrorAddMessage("(QccECCcrcPutChecksum): Error calling QccBitBufferPutBits()");
          return(1);
    }

  return(0);
}


int QccECCcrcGetChecksum(QccBitBuffer *input_buffer,
                         unsigned int *checksum,
                         const QccECCcrc *crc)
{
  if (QccBitBufferGetBits(input_buffer, (int *)checksum, crc->width))
    {
      QccErrorAddMessage("(QccECCcrcGetChecksum): Error calling QccBitBufferGetBits()");
      return(1);
    }

  return(0);
}
