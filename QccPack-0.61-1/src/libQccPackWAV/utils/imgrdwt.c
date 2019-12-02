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

#include "imgrdwt.h"

#define USG_STRING "[-w %s:wavelet] [-b %s:boundary] [-nl %d:levels] %s:imgfile %*s:icpfiles..."

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

int NumFiles;
QccIMGImage InputImage;
QccIMGImageComponent *OutputImageComponents;
QccString *Filenames;

int NumLevels = -1;
int NumSubbands;

int ImageNumCols;
int ImageNumRows;


int main(int argc, char *argv[])
{
  int current_file;
  QccMatrix *matrices;

  QccInit(argc, argv);

  QccIMGImageInitialize(&InputImage);
  QccWAVWaveletInitialize(&Wavelet);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         WaveletFilename,
                         Boundary,
                         &NumLevels,
                         InputImage.filename,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();

  if (NumLevels < 0)
    {
      NumSubbands = NumFiles;
      NumLevels = QccWAVSubbandPyramidNumSubbandsToNumLevels(NumSubbands);
      if (!NumLevels)
        {
          QccErrorAddMessage("%s: Invalid number of output files",
                             argv[0]);
          QccErrorExit();
        }
    }
  else
    {
      NumSubbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(NumLevels);
      if (NumFiles > NumSubbands)
        {
          QccErrorAddMessage("%s: Too many output files for a %d-level transform (use %d or fewer output files)",
                             argv[0],
                             NumLevels,
                             NumSubbands);
          QccErrorExit();
        }
    }

  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccIMGImageRead(&InputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageColor(&InputImage))
    {
      QccErrorAddMessage("%s: Color images not supported",
                         argv[0]);
      QccErrorExit();
    }

  ImageNumCols = InputImage.Y.num_cols;
  ImageNumRows = InputImage.Y.num_rows;

  if ((OutputImageComponents = 
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
      QccIMGImageComponentInitialize(&(OutputImageComponents[current_file]));
      OutputImageComponents[current_file].num_rows = ImageNumRows;
      OutputImageComponents[current_file].num_cols = ImageNumCols;

      if (QccIMGImageComponentAlloc(&(OutputImageComponents[current_file])))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentAlloc()",
                             argv[0]);
          QccErrorExit();
        }

      matrices[current_file] = OutputImageComponents[current_file].image;
    }
  
  for (current_file = 0; current_file < NumFiles; current_file++)
    QccStringCopy(OutputImageComponents[current_file].filename,
                  Filenames[current_file]);

  if (QccWAVWaveletRedundantDWT2D(InputImage.Y.image,
                                  matrices,
                                  ImageNumRows,
                                  ImageNumCols,
                                  NumLevels,
                                  &Wavelet))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletRedundantDWT2D()",
                         argv[0]);
      QccErrorExit();
    }
  
  for (current_file = 0; current_file < NumFiles; current_file++)
    {
      if (QccIMGImageComponentSetMaxMin(&(OutputImageComponents[current_file])))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentSetMaxMin()",
                             argv[0]);
          QccErrorExit();
        }

      if (QccIMGImageComponentWrite(&(OutputImageComponents[current_file])))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentWrite()",
                             argv[0]);
          QccErrorExit();
        }
    }

  for (current_file = 0; current_file < NumSubbands; current_file++)
    QccIMGImageComponentFree(&(OutputImageComponents[current_file]));
  QccFree(OutputImageComponents);
  QccFree(matrices);

  QccIMGImageFree(&InputImage);
  QccWAVWaveletFree(&Wavelet);

  QccExit;
}
