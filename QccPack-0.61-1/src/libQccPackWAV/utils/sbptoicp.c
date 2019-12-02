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

#define USG_STRING "%s:subband_pyramid %s:icpfile"


QccWAVSubbandPyramid SubbandPyramid;
QccString Filename;
QccIMGImageComponent OutputImage;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);
  QccIMGImageComponentInitialize(&OutputImage);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         SubbandPyramid.filename,
                         Filename))
    QccErrorExit();

  if (QccWAVSubbandPyramidRead(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidRead()",
                         argv[0]);
      QccErrorExit();
    }

  OutputImage.image = SubbandPyramid.matrix;
  OutputImage.num_rows = SubbandPyramid.num_rows;
  OutputImage.num_cols = SubbandPyramid.num_cols;
  QccIMGImageComponentSetMin(&OutputImage);
  QccIMGImageComponentSetMax(&OutputImage);

  QccStringCopy(OutputImage.filename, Filename);

  if (QccIMGImageComponentWrite(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageComponentFree(&OutputImage);

  QccExit;
}

