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


#include "icptoimg.h"

#define USG_STRING "[-n %:] icpfiles... %*s:imgfile"

QccIMGImageComponent ImageComponent[3];
QccIMGImage Image;

int NumFiles;
QccString *Filenames;

int normalize = 0;

int NumRows, NumCols;


int main(int argc, char *argv[])
{
  int current_file;

  QccInit(argc, argv);

  QccIMGImageInitialize(&Image);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &normalize,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();
      
  QccStringCopy(Image.filename, Filenames[NumFiles - 1]);
  if (QccIMGImageDetermineType(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageDetermineType()",
                         argv[0]);
      QccErrorExit();
    }
  if ((QccIMGImageColor(&Image) && (NumFiles != 4)) ||
      (!QccIMGImageColor(&Image) && (NumFiles != 2)))
    {
      QccErrorAddMessage("%s: Invalid number of files specified",
                         argv[0]);
      QccErrorExit();
    }

  for (current_file = 0; current_file < NumFiles - 1; current_file++)
    {
      QccIMGImageComponentInitialize(&ImageComponent[current_file]);
      QccStringCopy(ImageComponent[current_file].filename,
             Filenames[current_file]);
      
      if (QccIMGImageComponentRead(&ImageComponent[current_file]))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (!current_file)
        {
          NumRows = ImageComponent[current_file].num_rows;
          NumCols = ImageComponent[current_file].num_cols;
        }
      else
        if ((NumRows != ImageComponent[current_file].num_rows) ||
            (NumCols != ImageComponent[current_file].num_cols))
          {
            QccErrorAddMessage("%s: icpfiles have different sizes",
                               argv[0]);
            QccErrorExit();
          }
      if (normalize)
        if (QccIMGImageComponentNormalize(&ImageComponent[current_file]))
          {
            QccErrorAddMessage("%s: Error calling QccIMGImageComponentNormalize()",
                               argv[0]);
            QccErrorExit();
          }
              
    }

  QccIMGImageSetSize(&Image, NumRows, NumCols);

  Image.Y = ImageComponent[0];
  if (QccIMGImageColor(&Image))
    {
      Image.U = ImageComponent[1];
      Image.V = ImageComponent[2];
    }

  if (QccIMGImageWrite(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                         argv[0]);
      QccErrorExit();
    }

  for (current_file = 0; current_file < NumFiles - 1; current_file++)
    QccIMGImageComponentFree(&ImageComponent[current_file]);

  QccExit;
}
