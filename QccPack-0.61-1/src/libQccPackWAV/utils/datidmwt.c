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


#include "datidmwt.h"

#define USG_STRING "[-w %s:multiwavelet] [-nl %d:num_levels] %s:infile %s:outfile"

QccDataset InputDataset;
QccDataset OutputDataset;

QccWAVMultiwavelet Multiwavelet;
QccString MultiwaveletFilename = QCCWAVWAVELET_DEFAULT_MULTIWAVELET;

int NumLevels = 3;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);

  QccDatasetInitialize(&InputDataset);
  QccDatasetInitialize(&OutputDataset);
  QccWAVMultiwaveletInitialize(&Multiwavelet);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         MultiwaveletFilename,
                         &NumLevels,
                         InputDataset.filename,
                         OutputDataset.filename))
    QccErrorExit();

  if (QccWAVMultiwaveletCreate(&Multiwavelet, MultiwaveletFilename))
    {
      QccErrorAddMessage("%s: Error calling QccWAVMultiwaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccDatasetReadWholefile(&InputDataset))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetReadWholefile()",
                         argv[0]);
      QccErrorExit();
    }

  OutputDataset.num_vectors = InputDataset.num_vectors;
  OutputDataset.vector_dimension = InputDataset.vector_dimension;
  OutputDataset.access_block_size = QCCDATASET_ACCESSWHOLEFILE;

  if (QccDatasetAlloc(&OutputDataset))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccWAVMultiwaveletInverseDMWT1D(&InputDataset,
                                      &OutputDataset,
                                      NumLevels,
                                      &Multiwavelet))
    {
      QccErrorAddMessage("%s: Error calling QccWAVMultiwaveletInverseDMWT1D()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccDatasetWriteWholefile(&OutputDataset))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetWriteWholefile()",
                         argv[0]);
      QccErrorExit();
    }

  QccDatasetFree(&InputDataset);
  QccDatasetFree(&OutputDataset);
  QccWAVMultiwaveletFree(&Multiwavelet);

  QccExit;
}
