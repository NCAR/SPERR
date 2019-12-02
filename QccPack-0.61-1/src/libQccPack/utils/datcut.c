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


#include "datcut.h"

#define USG_STRING "[-s %d:start] %d:length %s:datfile1 %s:datfile2"


int Length;

int StartPoint = 0;

QccDataset Dataset1;
QccDataset Dataset2;


int main(int argc, char *argv[])
{
  QccString output_filename;
  int current_vector;

  QccInit(argc, argv);

  QccDatasetInitialize(&Dataset1);
  QccDatasetInitialize(&Dataset2);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &StartPoint,
                         &Length,
                         Dataset1.filename,
                         output_filename))
    QccErrorExit();

  Dataset1.access_block_size = Length;

  if (QccDatasetStartRead(&Dataset1))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (StartPoint + Length > Dataset1.num_vectors)
    {
      QccErrorAddMessage("%s: Dataset %s contains too few vectors (vectors 0-%d) to extract requested block (vectors %d-%d)",
                         argv[0], Dataset1.filename, Dataset1.num_vectors-1,
                         StartPoint, StartPoint+Length+1);
      QccErrorExit();
    }

  Dataset1.access_block_size = 1;

  for (current_vector = 0; current_vector < StartPoint; current_vector++)
    if (QccDatasetReadBlock(&Dataset1))
      {
        QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                           argv[0]);
        QccErrorExit();
      }

  Dataset1.access_block_size = Length;

  if (QccDatasetReadBlock(&Dataset1))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccDatasetCopy(&Dataset2, &Dataset1))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetCopy()",
                         argv[0]);
      QccErrorExit();
    }

  Dataset2.num_vectors = Length;
  QccStringCopy(Dataset2.filename, output_filename);
  QccDatasetSetMaxMinValues(&Dataset2);

  if (QccDatasetStartWrite(&Dataset2))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartWrite()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccDatasetWriteBlock(&Dataset2))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetWriteBlock()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccDatasetEndRead(&Dataset1))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccDatasetEndWrite(&Dataset2))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccDatasetFree(&Dataset1);
  QccDatasetFree(&Dataset2);
      
  QccExit;
}
