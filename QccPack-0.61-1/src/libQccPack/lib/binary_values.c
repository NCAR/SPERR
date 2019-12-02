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


/*
 *  All binary-valued numbers are stored to files by QccPack as
 *  MSB first.  These routines convert the MSB-first values to int/double
 *  with the host byte order.  Files written by QccPack should then
 *  be portable across different architectures.
 */

int QccBinaryCharToInt(const unsigned char *ch, unsigned int *val)
{
  int byte_cnt;

  if ((val == NULL) || (ch == NULL))
    return(0);

  /*  MSB first  */
  *val = 0;
  for (byte_cnt = 0; byte_cnt < QCC_INT_SIZE; byte_cnt++)
    {
      *val <<= 8;
      *val |= ch[byte_cnt];
    }

  return(0);
}


int QccBinaryIntToChar(unsigned int val, unsigned char *ch)
{
  int byte_cnt;

  if (ch == NULL)
    return(0);

  /*  MSB first  */
  for (byte_cnt = QCC_INT_SIZE - 1; byte_cnt >= 0; byte_cnt--)
    {
      ch[byte_cnt] = (val & 0xff);
      val >>= 8;
    }

  return(0);
}


int QccBinaryCharToFloat(const unsigned char *ch, float *val)
{
  /*  Relies on floats being QCC_INT_SIZE bytes long  */
  union
  {
    float d;
    unsigned int i;
  } tmp;

  if ((val == NULL) || (ch == NULL))
    return(0);

  QccBinaryCharToInt(ch, &(tmp.i));
  *val = tmp.d;

  return(0);
}


int QccBinaryFloatToChar(float val, unsigned char *ch)
{
  /*  Relies on floats being QCC_INT_SIZE bytes long  */
  union
  {
    float d;
    unsigned int i;
  } tmp;

  if (ch == NULL)
    return(0);

  tmp.d = val;
  QccBinaryIntToChar(tmp.i, ch);

  return(0);
}
