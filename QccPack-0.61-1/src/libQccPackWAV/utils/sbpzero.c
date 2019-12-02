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


#include "sbpzero.h"

#define USG_STRING "[-z %d:subband] %s:subband_pyramid1 %s:subband_pyramid2"


QccWAVSubbandPyramid SubbandPyramid;
QccString OutputFilename;

int NumSubbands;

int StartSubband = 1;


int main(int argc, char *argv[])
{
  int subband;

  QccInit(argc, argv);
  
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &StartSubband,
                         SubbandPyramid.filename,
                         OutputFilename))
    QccErrorExit();

  if (QccWAVSubbandPyramidRead(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidRead()",
                         argv[0]);
      QccErrorExit();
    }

  NumSubbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(SubbandPyramid.num_levels);

  for (subband = StartSubband; subband < NumSubbands; subband++)
    if (QccWAVSubbandPyramidZeroSubband(&SubbandPyramid, subband))
      {
        QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidZeroSubband()",
                           argv[0]);
        QccErrorExit();
      }

  QccStringCopy(SubbandPyramid.filename, OutputFilename);

  if (QccWAVSubbandPyramidWrite(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccWAVSubbandPyramidFree(&SubbandPyramid);

  QccExit;
}

