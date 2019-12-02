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


#include "datcat.h"


#define USG_STRING "%*s:datfiles..."
int NumFiles;

QccDataset *Datafiles;
QccDataset Output;

QccString *Filenames;

int main(int argc, char *argv[])
{
  int current_file;
  int current_block;
  int num_blocks;

  QccInit(argc, argv);

  QccDatasetInitialize(&Output);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();

  if ((Datafiles = 
       (QccDataset *)malloc(NumFiles * sizeof(QccDataset))) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory\n",
                         argv[0]);
      QccErrorExit();
    }
  
  Output.num_vectors = 0;
  Output.min_val = MAXDOUBLE;
  Output.max_val = -MAXDOUBLE;

  for (current_file = 0; current_file < NumFiles; current_file++)
    {
      QccDatasetInitialize(&(Datafiles[current_file]));
      QccStringCopy(Datafiles[current_file].filename,
             Filenames[current_file]);
      Datafiles[current_file].access_block_size = 1;
      if (QccDatasetStartRead(&Datafiles[current_file]))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetStartRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (!current_file)
        Output.vector_dimension = Datafiles[current_file].vector_dimension;
      else
        if (Datafiles[current_file].vector_dimension != 
            Output.vector_dimension)
          {
            QccErrorAddMessage("%s: Files have different vector dimensions\n",
                               argv[0]);
            QccErrorExit();
          }
      Output.num_vectors += Datafiles[current_file].num_vectors;
      if (Datafiles[current_file].min_val < Output.min_val)
        Output.min_val = Datafiles[current_file].min_val;
      if (Datafiles[current_file].max_val > Output.max_val)
        Output.max_val = Datafiles[current_file].max_val;
    }

  Output.filename[0] = '\0';
  Output.fileptr = stdout;
  Output.access_block_size = 1;
  Output.num_blocks_accessed = 0;
  if (QccDatasetAlloc(&Output))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccDatasetWriteHeader(&Output))
    {
      QccErrorAddMessage("%s: Error writing header",
                         argv[0]);
      QccErrorExit();
    }

  for (current_file = 0; current_file < NumFiles; current_file++)
    {
      num_blocks = Datafiles[current_file].num_vectors;
      for (current_block = 0; current_block < num_blocks; current_block++)
        {
          if (QccDatasetReadBlock(&Datafiles[current_file]))
            {
              QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                                 argv[0]);
              QccErrorExit();
            }
          QccVectorCopy(Output.vectors[0],
                        Datafiles[current_file].vectors[0],
                        Output.vector_dimension);
          if (QccDatasetWriteBlock(&Output))
            {
              QccErrorAddMessage("%s: Error calling QccDatasetWriteBlock()",
                                 argv[0]);
              QccErrorExit();
            }
        }
      if (QccDatasetEndRead(&Datafiles[current_file]))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetEndRead()",
                             argv[0]);
          QccErrorExit();
        }
    }

  fflush(stdout);

  QccDatasetFree(&Output);
  for (current_file = 0; current_file < NumFiles; current_file++)
    QccDatasetFree(&(Datafiles[current_file]));
  QccFree(Datafiles);

  QccExit;
}
