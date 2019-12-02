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


#include "sbpsplit.h"

#define USG_STRING "%s:subband_pyramid %*s:icpfiles..."

QccString *OutputFilenames;
int NumOutputFiles;
int NumLevels;
int NumSubbands;

QccWAVSubbandPyramid SubbandPyramid;
QccIMGImageComponent *OutputImages;



int main(int argc, char *argv[])
{
  int smallest_subimage_num_cols, smallest_subimage_num_rows;
  int subband;

  QccInit(argc, argv);
  
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         SubbandPyramid.filename,
                         &NumOutputFiles,
                         &OutputFilenames))
    QccErrorExit();

  if (QccWAVSubbandPyramidRead(&SubbandPyramid))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidRead()",
                         argv[0]);
      QccErrorExit();
    }

  NumLevels = SubbandPyramid.num_levels;
  NumSubbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(NumLevels);

  if (NumSubbands < NumOutputFiles)
    {
      QccErrorAddMessage("%s: %s contains too few subbands (%d) to create the %d specified output files",
                         argv[0], SubbandPyramid.filename, 
                         NumSubbands, NumOutputFiles);
      QccErrorExit();
    }

  if (QccWAVSubbandPyramidSubbandSize(&SubbandPyramid,
                                      0,
                                      &smallest_subimage_num_rows,
                                      &smallest_subimage_num_cols))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidSubbandSize()",
                         argv[0]);
      QccErrorExit();
    }

  if (((smallest_subimage_num_cols << NumLevels) != 
       SubbandPyramid.num_cols) ||
      ((smallest_subimage_num_rows << NumLevels) != 
       SubbandPyramid.num_rows))
    {
      QccErrorAddMessage("%s: Image size (%d x %d) of %s is not a multiple of %d (as needed for a decomposition of %d levels",
                         argv[0], 
                         SubbandPyramid.num_cols,
                         SubbandPyramid.num_rows,
                         SubbandPyramid.filename,
                         (1 << NumLevels), NumLevels);
      QccErrorExit();
    }

  if ((OutputImages = (QccIMGImageComponent *)
       malloc(sizeof(QccIMGImageComponent)*NumSubbands)) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory",
                         argv[0]);
      QccErrorExit();
    }

  for (subband = 0; subband < NumSubbands; subband++)
    QccIMGImageComponentInitialize(&OutputImages[subband]);

  if (QccWAVSubbandPyramidSplitToImageComponent(&SubbandPyramid,
                                                OutputImages))
    {
      QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidSplitToImageComponent()",
                         argv[0]);
      QccErrorExit();
    }
    

  for (subband = 0; subband < NumOutputFiles; subband++)
    {
      QccStringCopy(OutputImages[subband].filename, 
                    OutputFilenames[subband]);
      if (QccIMGImageComponentWrite(&OutputImages[subband]))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentWrite()",
                             argv[0]);
          QccErrorExit();
        }
    }

  QccWAVSubbandPyramidFree(&SubbandPyramid);

  for (subband = 0; subband < NumSubbands; subband++)
    QccIMGImageComponentFree(&OutputImages[subband]);
  QccFree(OutputImages);

  QccExit;
}

