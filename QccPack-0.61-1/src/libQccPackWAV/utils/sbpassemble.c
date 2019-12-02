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


#include "sbpassemble.h"

#define USG_STRING "icpfiles... %*s:subband_pyramid"

QccString *Filenames;
int NumFiles;
int NumLevels;
int NumSubbands;

QccWAVSubbandPyramid SubbandPyramid;
QccIMGImageComponent *InputImages;



int main(int argc, char *argv[])
{
  int subband_num_cols, subband_num_rows;
  int subband;

  QccInit(argc, argv);
  
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();

  if (NumFiles < 2)
    {
      QccErrorAddMessage("%s: Must have one output file and at least one input file",
                         argv[0]);
      QccErrorExit();
    }

  NumSubbands = NumFiles - 1;

  if (!(NumLevels = QccWAVSubbandPyramidNumSubbandsToNumLevels(NumSubbands)))
    {
      QccErrorAddMessage("%s: Invalid number of input files",
                         argv[0]);
      QccErrorExit();
    }

  NumSubbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(NumLevels);

  if ((InputImages = 
       (QccIMGImageComponent *)malloc(sizeof(QccIMGImageComponent) *
                                      NumSubbands)) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory",
                         argv[0]);
      QccErrorExit();
    }

  for (subband = 0; subband < NumSubbands; subband++)
    {
      QccIMGImageComponentInitialize(&InputImages[subband]);
      QccStringCopy(InputImages[subband].filename, Filenames[subband]);

      if (QccIMGImageComponentRead(&InputImages[subband]))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentRead()",
                             argv[0]);
          QccErrorExit();
        }
    }

  QccStringCopy(SubbandPyramid.filename, Filenames[NumSubbands]);
  SubbandPyramid.num_levels = NumLevels;
  SubbandPyramid.num_rows = 
    InputImages[0].num_rows << NumLevels;
  SubbandPyramid.num_cols =
    InputImages[0].num_cols << NumLevels;

  for (subband = 1; subband < NumSubbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(&SubbandPyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidSubbandSize()",
                             argv[0]);
          QccErrorExit();
        }
      if ((subband_num_rows != InputImages[subband].num_rows) ||
          (subband_num_cols != InputImages[subband].num_cols))
        {
          QccErrorAddMessage("%s: Sizes of input images are not correct",
                             argv[0]);
          QccErrorExit();
        }
    }

  if (QccWAVSubbandPyramidAlloc(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccWAVSubbandPyramidAssembleFromImageComponent(InputImages,
                                                     &SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidAssembleFromImageComponent()",
                         argv[0]);
      QccErrorExit();
    }
    
  if (QccWAVSubbandPyramidWrite(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccWAVSubbandPyramidFree(&SubbandPyramid);

  for (subband = 0; subband < NumSubbands; subband++)
    QccIMGImageComponentFree(&InputImages[subband]);
  QccFree(InputImages);

  QccExit;
}

