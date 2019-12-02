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


#include "dattoascii.h"

#define USG_STRING "%s:datfile %s:asciifile"


QccString ASCIIFilename;
FILE *Outfile;

QccDataset Dataset;


int main(int argc, char *argv[])
{
  int vector, component;

  QccInit(argc, argv);

  QccDatasetInitialize(&Dataset);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         Dataset.filename,
                         ASCIIFilename))
    QccErrorExit();

  if (QccDatasetReadWholefile(&Dataset))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetReadWholefile()",
                         argv[0]);
      QccErrorExit();
    }

  if ((Outfile = QccFileOpen(ASCIIFilename, "w")) ==  NULL)
    {
      QccErrorAddMessage("%s: Error calling QccFileOpen()",
                         argv[0]);
      QccErrorExit();
    }

  for (vector = 0; vector < Dataset.num_vectors; vector++)
    {
      for (component = 0; component < Dataset.vector_dimension; component++)
        {
          fprintf(Outfile, "%e ",
                  Dataset.vectors[vector][component]);
          if (ferror(Outfile))
            {
              QccErrorAddMessage("%s: Error writing file %s",
                                 argv[0], ASCIIFilename);
              QccErrorExit();
            }
        }
      fprintf(Outfile, "\n");
      if (ferror(Outfile))
        {
          QccErrorAddMessage("%s: Error writing file %s",
                             argv[0], ASCIIFilename);
          QccErrorExit();
        }
    }

  QccFileClose(Outfile);

  QccDatasetFree(&Dataset);

  QccExit;
}
