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


int QccBitBufferInitialize(QccBitBuffer *bit_buffer)
{
  QccStringMakeNull(bit_buffer->filename);
  bit_buffer->fileptr = NULL;
  bit_buffer->type = QCCBITBUFFER_OUTPUT;
  bit_buffer->bit_cnt = 0;
  bit_buffer->byte_cnt = 0;
  bit_buffer->bits_to_go = 8;
  bit_buffer->buffer = 0;

  return(0);
}


int QccBitBufferStart(QccBitBuffer *bit_buffer)
{
  int return_value;

  if (bit_buffer == NULL)
    return(0);

  if (bit_buffer->fileptr == NULL)
    {
      if (bit_buffer->type == QCCBITBUFFER_OUTPUT)
        {
          if ((bit_buffer->fileptr = QccFileOpen(bit_buffer->filename,
                                                 "w")) == NULL)
            {
              QccErrorAddMessage("(QccBitBufferStart): Error calling QccFileOpen()");
              goto Error;
            }
        }
      else
        {
          if ((bit_buffer->fileptr = QccFileOpen(bit_buffer->filename,
                                                 "r")) == NULL)
            {
              QccErrorAddMessage("(QccBitBufferStart): Error calling QccFileOpen()");
              goto Error;
            }
        }
    }

  bit_buffer->buffer = 0;
  
  if (bit_buffer->type == QCCBITBUFFER_INPUT)
    bit_buffer->bits_to_go = 0;
  else
    bit_buffer->bits_to_go = 8;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccBitBufferEnd(QccBitBuffer *bit_buffer)
{
  if (bit_buffer == NULL)
    return(0);

  if (QccFileClose(bit_buffer->fileptr))
    {
      QccErrorAddMessage("(QccBitBufferEnd): Error calling QccFileClose()");
      return(1);
    }
  
  bit_buffer->fileptr = NULL;

  return(0);
}


int QccBitBufferFlush(QccBitBuffer *bit_buffer)
{
  if (bit_buffer == NULL)
    return(0);
  if (bit_buffer->fileptr == NULL)
    return(0);

  if (bit_buffer->type == QCCBITBUFFER_OUTPUT)
    {
      if (bit_buffer->bits_to_go != 8)
        {
          bit_buffer->buffer >>= bit_buffer->bits_to_go;
          
          if (QccFileWriteChar(bit_buffer->fileptr, 
                               (char)(bit_buffer->buffer)))
            {
              QccErrorAddMessage("(QccBitBufferFlush): Error calling QccFileWriteChar()");
              return(1);
            }

          bit_buffer->byte_cnt++;
        }

      bit_buffer->bits_to_go = 8;
      bit_buffer->buffer = 0;

      if (QccFileFlush(bit_buffer->fileptr))
        {
          QccErrorAddMessage("(QccBitBufferFlush): Error calling QccFileFlush()");
          return(1);
        }
    }
  else
    bit_buffer->bits_to_go = 0;
  
  return(0);
}


int QccBitBufferCopy(QccBitBuffer *output_buffer,
                     QccBitBuffer *input_buffer,
                     int num_bits)
{
  int cnt;
  int bit_value;

  if (output_buffer == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);

  for (cnt = 0; cnt < num_bits; cnt++)
    {
      if (QccBitBufferGetBit(input_buffer, &bit_value))
        {
          QccErrorAddMessage("(QccBitBufferCopy): Error calling QccBitBufferGetBit()");
          return(1);
        }
      if (QccBitBufferPutBit(output_buffer, bit_value))
        {
          QccErrorAddMessage("(QccBitBufferCopy): Error calling QccBitBufferPutBit()");
          return(1);
        }
    }

  return(0);
}


int QccBitBufferPutBit(QccBitBuffer *bit_buffer, int bit_value)
{
  if (bit_buffer == NULL)
    return(0);
  
  if (bit_buffer->type != QCCBITBUFFER_OUTPUT)
    {
      QccErrorAddMessage("(QccBitBufferPutBit): Not an output buffer");
      return(1);
    }

  bit_buffer->buffer >>= 1;
  if (bit_value)
    bit_buffer->buffer |= 0x80;
  bit_buffer->bits_to_go--;
  bit_buffer->bit_cnt++;
  if (!bit_buffer->bits_to_go)
    {
      if (QccFileWriteChar(bit_buffer->fileptr, 
                           (char)bit_buffer->buffer))
        {
          QccErrorAddMessage("(QccBitBufferPutBit): Error calling QccFileWriteChar()");
          return(1);
        }
      bit_buffer->buffer = 0;
      bit_buffer->bits_to_go = 8;
      bit_buffer->byte_cnt++;
    }

  return(0);
}


int QccBitBufferGetBit(QccBitBuffer *bit_buffer, int *bit_value)
{
  if (bit_buffer == NULL)
    return(0);
  
  if (bit_buffer->type != QCCBITBUFFER_INPUT)
    {
      QccErrorAddMessage("(QccBitBufferGetBit): Not an input buffer");
      return(1);
    }

  if (!bit_buffer->bits_to_go)
    {
      if (QccFileReadChar(bit_buffer->fileptr, (char *)&(bit_buffer->buffer)))
        {
          QccErrorAddMessage("(QccBitBufferGetBit): Error calling QccFileReadChar()");
          return(1);
        }

      bit_buffer->bits_to_go = 8;
      bit_buffer->byte_cnt++;
    }

  if (bit_value != NULL)
    *bit_value = bit_buffer->buffer & 0x01;

  bit_buffer->buffer >>= 1;
  bit_buffer->bits_to_go--;
  bit_buffer->bit_cnt++;

  return(0);
}


int QccBitBufferPutBits(QccBitBuffer *bit_buffer, int val, int num_bits)
{
  int bit_value;
  int bit;

  if (bit_buffer == NULL)
    return(0);
  
  for (bit = 0; bit < num_bits; bit++)
    {
      bit_value = 
        ((val & 0x01) != 0);
      if (QccBitBufferPutBit(bit_buffer, bit_value))
        {
          QccErrorAddMessage("(QccBitBufferPutBits): Error calling QccBitBufferPutBit()");
          return(1);
        }
      val >>= 1;
    }

  return(0);
}


int QccBitBufferGetBits(QccBitBuffer *bit_buffer, int *val, int num_bits)
{
  int bit_value;
  int bit;
  unsigned int mask = 0x01;
  int val2;

  if (bit_buffer == NULL)
    return(0);
  
  val2 = 0;
  for (bit = 0; bit < num_bits; bit++)
    {
      if (QccBitBufferGetBit(bit_buffer, &bit_value))
        {
          QccErrorAddMessage("(QccBitBufferGetBits): Error calling QccBitBufferGetBit()");
          return(1);
        }
      if (bit_value)
        val2 |= mask;
      mask <<= 1;
    }

  if (val != NULL)
    *val = val2;

  return(0);
}


int QccBitBufferPutChar(QccBitBuffer *bit_buffer, unsigned char val)
{
  int bit_value;
  int bit;

  if (bit_buffer == NULL)
    return(0);
  
  for (bit = 0; bit < 8; bit++)
    {
      bit_value = 
        ((val & 0x01) != 0);
      if (QccBitBufferPutBit(bit_buffer, bit_value))
        {
          QccErrorAddMessage("(QccBitBufferPutChar): Error calling QccBitBufferPutBit()");
          return(1);
        }
      val >>= 1;
    }

  return(0);
}


int QccBitBufferGetChar(QccBitBuffer *bit_buffer, unsigned char *val)
{
  int bit_value;
  int bit;
  unsigned char mask = 0x01;
  unsigned char val2;

  if (bit_buffer == NULL)
    return(0);
  
  val2 = 0;
  for (bit = 0; bit < 8; bit++)
    {
      if (QccBitBufferGetBit(bit_buffer, &bit_value))
        {
          QccErrorAddMessage("(QccBitBufferGetChar): Error calling QccBitBufferGetBit()");
          return(1);
        }
      if (bit_value)
        val2 |= mask;
      mask <<= 1;
    }

  if (val != NULL)
    *val = val2;

  return(0);
}


int QccBitBufferPutInt(QccBitBuffer *bit_buffer, int val)
{
  unsigned char ch[QCC_INT_SIZE];
  int byte_cnt;

  if (bit_buffer == NULL)
    return(0);
  
  QccBinaryIntToChar((unsigned int)val, ch);
  for (byte_cnt = 0; byte_cnt < QCC_INT_SIZE; byte_cnt++)
    if (QccBitBufferPutChar(bit_buffer, ch[byte_cnt]))
      {
        QccErrorAddMessage("(QccBitBufferPutInt): Error calling QccBitBufferPutChar()");
        return(1);
      }

  return(0);
}


int QccBitBufferGetInt(QccBitBuffer *bit_buffer, int *val)
{
  unsigned char ch[QCC_INT_SIZE];
  int byte_cnt;

  if (bit_buffer == NULL)
    return(0);
  
  for (byte_cnt = 0; byte_cnt < QCC_INT_SIZE; byte_cnt++)
    if (QccBitBufferGetChar(bit_buffer, &ch[byte_cnt]))
      {
        QccErrorAddMessage("(QccBitBufferGetInt): Error calling QccBitBufferGetChar()");
        return(1);
      }

  if (val != NULL)
    QccBinaryCharToInt(ch, (unsigned int *)val);

  return(0);
}


int QccBitBufferPutDouble(QccBitBuffer *bit_buffer, double val)
{
  unsigned char ch[QCC_INT_SIZE];
  int byte_cnt;
  float tmp;

  if (bit_buffer == NULL)
    return(0);
  
  tmp = val;
  QccBinaryFloatToChar(tmp, ch);
  for (byte_cnt = 0; byte_cnt < QCC_INT_SIZE; byte_cnt++)
    if (QccBitBufferPutChar(bit_buffer, ch[byte_cnt]))
      {
        QccErrorAddMessage("(QccBitBufferPutDouble): Error calling QccBitBufferPutChar()");
        return(1);
      }

  return(0);
}


int QccBitBufferGetDouble(QccBitBuffer *bit_buffer, double *val)
{
  unsigned char ch[QCC_INT_SIZE];
  int byte_cnt;
  float tmp;

  if (bit_buffer == NULL)
    return(0);
  
  for (byte_cnt = 0; byte_cnt < QCC_INT_SIZE; byte_cnt++)
    if (QccBitBufferGetChar(bit_buffer, &ch[byte_cnt]))
      {
        QccErrorAddMessage("(QccBitBufferGetDouble): Error calling QccBitBufferGetChar()");
        return(1);
      }

  if (val != NULL)
    {
      QccBinaryCharToFloat(ch, &tmp);
      *val = tmp;
    }

  return(0);
}


