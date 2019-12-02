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


#include "asciitodat.h"

#define USG_STRING "[-d %d:vector_dimension] %s:asciifile %s:datfile"


QccString ascii_filename;
FILE *infile;

QccDataset Dataset;

int VectorDimension = 1;


int main(int argc, char *argv[])
{
  double val;
  double min = MAXDOUBLE;
  double max = -MAXDOUBLE;
  int entry;
  int number_of_values = 0;
  int component;


  QccInit(argc, argv);

  QccDatasetInitialize(&Dataset);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &VectorDimension,
                         ascii_filename,
                         Dataset.filename))
    QccErrorExit();

  if ((infile = QccFileOpen(ascii_filename, "r")) ==  NULL)
    {
      QccErrorAddMessage("%s: Error calling QccFileOpen()",
                         argv[0]);
      QccErrorExit();
    }

  while (!feof(infile))
    {
      fscanf(infile, "%lf", &val);
      if (ferror(infile) || feof(infile))
        {
          if (!feof(infile))
            {
              QccErrorAddMessage("%s: Error reading %s\n",
                                 argv[0], ascii_filename);
              QccErrorExit();
            }
        }
      if (val < min)
        min = val;
      if (val > max)
        max = val;
      number_of_values++;
    }
  
  number_of_values--;

  QccFileClose(infile);
  if ((infile = QccFileOpen(ascii_filename, "r")) ==  NULL)
    {
      QccErrorAddMessage("%s: Error calling QccFileOpen()",
                         argv[0]);
      QccErrorExit();
    }

  Dataset.num_vectors = (int)floor(number_of_values / VectorDimension);
  Dataset.vector_dimension = VectorDimension;
  Dataset.access_block_size = 1;
  Dataset.min_val = min;
  Dataset.max_val = max;
  if (QccDatasetStartWrite(&Dataset))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartWrite()",
                         argv[0]);
      QccErrorExit();
    }

  for (entry = 0;
       entry < Dataset.num_vectors * VectorDimension;
       entry++)
    {
      for (component = 0; component < VectorDimension; component++)
        {
          fscanf(infile, "%lf", &val);
          if (ferror(infile) || feof(infile))
            {
              if (!feof(infile))
                {
                  QccErrorAddMessage("%s: Error reading %s\n",
                                     argv[0], ascii_filename);
                  QccErrorExit();
                }
              Dataset.vectors[0][component] = 0;
            }
          else
            Dataset.vectors[0][component] = val;
        }
      
      if (QccDatasetWriteBlock(&Dataset))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetWriteBlock()",
                             argv[0]);
          QccErrorExit();
        }
    }

  if (QccDatasetEndWrite(&Dataset))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccFileClose(infile);

  QccDatasetFree(&Dataset);

  QccExit;
}
