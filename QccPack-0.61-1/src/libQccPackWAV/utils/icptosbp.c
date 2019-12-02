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


#include "sbptoicp.h"

#define USG_STRING "%d:num_levels %s:icpfile %s:subband_pyramid"


QccIMGImageComponent InputImage;
QccWAVSubbandPyramid SubbandPyramid;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccIMGImageComponentInitialize(&InputImage);
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &SubbandPyramid.num_levels,
                         &InputImage.filename,
                         SubbandPyramid.filename))
    QccErrorExit();

  if (QccIMGImageComponentRead(&InputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentRead()",
                         argv[0]);
      QccErrorExit();
    }

  SubbandPyramid.matrix = InputImage.image;
  SubbandPyramid.num_rows = InputImage.num_rows;
  SubbandPyramid.num_cols = InputImage.num_cols;

  if (QccWAVSubbandPyramidWrite(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageComponentFree(&InputImage);

  QccExit;
}

