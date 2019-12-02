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


#include "imgtoicp.h"

#define USG_STRING "%s:imgfile %*s:icpfiles..."

QccIMGImage Image;

QccIMGImageComponent ImageComponent;

QccString *Filenames;
int NumFiles;

int main(int argc, char *argv[])
{
  int current_file;
  
  QccInit(argc, argv);
  
  QccIMGImageInitialize(&Image);
  QccIMGImageComponentInitialize(&ImageComponent);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         Image.filename,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();
  
  if (QccIMGImageRead(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccIMGImageColor(&Image))
    {
      if (NumFiles != 3)
        {
          QccErrorAddMessage("%s: Must have three output files for color input image",
                             argv[0]);
          QccErrorExit();
        }
    }
  else
    {
      if (NumFiles != 1)
        {
          QccErrorAddMessage("%s: Must have only one output file for grayscale input image",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  for (current_file = 0; current_file < NumFiles; current_file++)
    {
      
      switch (current_file)
        {
        case 0:
          ImageComponent = Image.Y;
          break;
        case 1:
          ImageComponent = Image.U;
          break;
        case 2:
          ImageComponent = Image.V;
          break;
        }
      
      QccStringCopy(ImageComponent.filename,
                    Filenames[current_file]);
      ImageComponent.magic_num[0] = '\0';
      
      QccIMGImageComponentSetMin(&ImageComponent);
      QccIMGImageComponentSetMax(&ImageComponent);
      
      if (QccIMGImageComponentWrite(&ImageComponent))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentWrite()",
                             argv[0]);
          QccErrorExit();
        }

    }

  QccIMGImageFree(&Image);

  QccExit;
}
