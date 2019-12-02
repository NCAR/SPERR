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


#include "icbiklt.h"

#define USG_STRING "%s:kltfile %s:icbfile1 %s:icbfile2"

QccHYPklt Transform;
QccIMGImageCube Image;
QccString InputFilename;
QccString OutputFilename;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);

  QccHYPkltInitialize(&Transform);
  QccIMGImageCubeInitialize(&Image);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         Transform.filename,
                         InputFilename,
                         OutputFilename))
    QccErrorExit();

  if (QccHYPkltRead(&Transform))
    {
      QccErrorAddMessage("%s: Error calling QccHYPkltRead()",
                         argv[0]);
      QccErrorExit();
    }

  QccStringCopy(Image.filename, InputFilename);
  if (QccIMGImageCubeRead(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccHYPkltInverseTransform(&Image,
                                &Transform))
    {
      QccErrorAddMessage("%s: Error calling QccHYPkltInverseTransform()",
                         argv[0]);
      QccErrorExit();
    }

  QccStringCopy(Image.filename, OutputFilename);
  if (QccIMGImageCubeWrite(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccHYPkltFree(&Transform);
  QccIMGImageCubeFree(&Image);

  QccExit;
}

