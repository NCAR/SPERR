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


#include "klt_train.h"

#define USG_STRING "%s:icbfile %s:kltfile"

QccIMGImageCube Image;
QccHYPklt Transform;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);

  QccIMGImageCubeInitialize(&Image);
  QccHYPkltInitialize(&Transform);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         Image.filename,
                         Transform.filename))
    QccErrorExit();

  if (QccIMGImageCubeRead(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }

  Transform.num_bands = Image.num_frames;
  if (QccHYPkltAlloc(&Transform))
    {
      QccErrorAddMessage("%s: Error calling QccHYPkltAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccHYPkltTrain(&Image,
                     &Transform))
    {
      QccErrorAddMessage("%s: Error calling QccHYPkltTrain()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccHYPkltWrite(&Transform))
    {
      QccErrorAddMessage("%s: Error calling QccHYPkltWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageCubeFree(&Image);
  QccHYPkltFree(&Transform);

  QccExit;
}

