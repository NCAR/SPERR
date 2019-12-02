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


#include "mask_bar.h"

#define USG_STRING "[-vo %:] %s:mask"

QccIMGImage Mask;

int ValueOnly = 0;

double BarValue = 0;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);

  QccIMGImageInitialize(&Mask);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &ValueOnly,
                         Mask.filename))
    QccErrorExit();

  if (QccIMGImageRead(&Mask))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageColor(&Mask))
    {
      QccErrorAddMessage("%s: Mask must be grayscale",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccWAVShapeAdaptiveMaskBAR(Mask.Y.image,
                                 Mask.Y.num_rows,
                                 Mask.Y.num_cols,
                                 &BarValue))
    {
      QccErrorAddMessage("%s: Error calling QccWAVShapeAdaptiveMaskBAR()",
                         argv[0]);
      QccErrorExit();
    }

  if (ValueOnly)
    printf("%f\n", BarValue);
  else
    {
      printf("The boundary-area ratio (BAR) for %s is\n",
             Mask.filename);
      printf("  %f\n", BarValue);
    }

  QccIMGImageFree(&Mask);

  QccExit;
}
