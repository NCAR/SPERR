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


#include "imgirdwt.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] icpfiles... %*s:imgfile"

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

int NumFiles;
QccIMGImage OutputImage;
QccIMGImageComponent *InputImageComponents;
QccString *Filenames;

int NumSubbands;
int NumLevels;

int ImageNumCols;
int ImageNumRows;


int main(int argc, char *argv[])
{
  int current_file;
  QccMatrix *matrices;

  QccInit(argc, argv);

  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageInitialize(&OutputImage);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();

  NumSubbands = NumFiles - 1;
  NumLevels = QccWAVSubbandPyramidNumSubbandsToNumLevels(NumSubbands);
  if (!NumLevels)
    {
      QccErrorAddMessage("%s: Invalid number of input files",
                         argv[0]);
      QccErrorExit();
    }

  QccStringCopy(OutputImage.filename, Filenames[NumSubbands]);

  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }
  
  if ((InputImageComponents = 
       (QccIMGImageComponent *)
       malloc(NumSubbands * sizeof(QccIMGImageComponent))) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory",
                         argv[0]);
      QccErrorExit();
    }
  
  if ((matrices =
       (QccMatrix *)malloc(NumSubbands * sizeof(QccMatrix))) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory",
                         argv[0]);
      QccErrorExit();
    }

  for (current_file = 0; current_file < NumSubbands; current_file++)
    {
      QccIMGImageComponentInitialize(&(InputImageComponents[current_file]));
      QccStringCopy(InputImageComponents[current_file].filename,
                    Filenames[current_file]);
      if (QccIMGImageComponentRead(&(InputImageComponents[current_file])))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentRead()",
                             argv[0]);
          QccErrorExit();
        }

      matrices[current_file] = InputImageComponents[current_file].image;

      if (!current_file)
        {
          ImageNumRows = InputImageComponents[current_file].num_rows;
          ImageNumCols = InputImageComponents[current_file].num_cols;
        }
      else
        if ((InputImageComponents[current_file].num_rows != ImageNumRows) ||
            (InputImageComponents[current_file].num_cols != ImageNumCols))
          {
            QccErrorAddMessage("%s: All input image components must be of the same size",
                               argv[0]);
            QccErrorExit();
          }
    }

  if (QccIMGImageSetSize(&OutputImage, ImageNumRows, ImageNumCols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageAlloc(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccWAVWaveletInverseRedundantDWT2D(matrices,
                                         OutputImage.Y.image,
                                         ImageNumRows,
                                         ImageNumCols,
                                         NumLevels,
                                         &Wavelet))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletInverseRedundantDWT2D()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageWrite(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccExit;
}
