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


#include "icbsam.h"

#define USG_STRING "[-vo %:] %s:icbfile1 %s:icbfile2"

int value_only_flag = 0;

QccIMGImageCube Image1;
QccIMGImageCube Image2;


int main(int argc, char *argv[])
{
  double mean_sam = 0;

  QccInit(argc, argv);

  QccIMGImageCubeInitialize(&Image1);
  QccIMGImageCubeInitialize(&Image2);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &value_only_flag,
                         Image1.filename,
                         Image2.filename))
    QccErrorExit();

  if (QccIMGImageCubeRead(&Image1))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageCubeRead(&Image2))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }

  mean_sam = QccHYPImageCubeMeanSAM(&Image1,
                                    &Image2);
      
  if (!value_only_flag)
    printf("The distortion between files\n   %s\n         and\n   %s\nis:\n",
           Image1.filename, Image2.filename);
  
  if (!value_only_flag)
    printf("%16.6f degrees (mean SAM)\n", mean_sam);
  else
    printf("%f\n", mean_sam);

  QccIMGImageCubeFree(&Image1);
  QccIMGImageCubeFree(&Image2);

  QccExit;
}
