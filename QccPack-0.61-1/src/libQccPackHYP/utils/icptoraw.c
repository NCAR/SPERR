/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "icptoraw.h"


#define USG_STRING "[-bpp %d:bits_per_pixel] [-bend %:] [lend %:] %s:icpfile %s:rawfile"

QccString RawFile;

QccIMGImageComponent ImageComponent;

int BitsPerPixel = 16;

int Endian;
int BigEndianSpecified = 0;
int LittleEndianSpecified = 0;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccIMGImageComponentInitialize(&ImageComponent);

  if (QccParseParameters(argc, argv,
			 USG_STRING,
                         &BitsPerPixel,
                         &BigEndianSpecified,
                         &LittleEndianSpecified,
                         ImageComponent.filename,
                         RawFile))
    QccErrorExit();
  
  if ((BitsPerPixel < 8) ||
      (BitsPerPixel > 32))
    {
      QccErrorAddMessage("%s: bpp must be between 1 and 32",
                         argv[0]);
      QccErrorExit();
    }
      
  if (BigEndianSpecified + LittleEndianSpecified > 1)
    {
      QccErrorAddMessage("%s: Only one of -be or -le can be specified",
                         argv[0]);
      QccErrorExit();
    }

  if (BigEndianSpecified)
    Endian = QCCHYP_RAWENDIAN_BIG;
  else
    if (LittleEndianSpecified)
      Endian = QCCHYP_RAWENDIAN_LITTLE;
    else
      Endian = QCCHYP_RAWENDIAN_BIG;

  if (QccIMGImageComponentRead(&ImageComponent))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccHYPRawWrite2D(RawFile, &ImageComponent,
                       BitsPerPixel, Endian))
    {
      QccErrorAddMessage("%s: Error calling QccHYPRawWrite2D()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageComponentFree(&ImageComponent);

  QccExit;
}
