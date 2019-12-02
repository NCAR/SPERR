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


#include "icbtocolor.h"

#define USG_STRING "[-r %d:red] [-g %d:green] [-b %d:blue] %s:icbfile %s:imgfile"

int RedBand = 52;
int GreenBand = 30;
int BlueBand = 4;

QccIMGImageCube ImageCube;
QccIMGImage Image;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);

  QccIMGImageCubeInitialize(&ImageCube);
  QccIMGImageInitialize(&Image);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &RedBand,
                         &GreenBand,
                         &BlueBand,
                         ImageCube.filename,
                         Image.filename))
    QccErrorExit();

  if (QccIMGImageDetermineType(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageDetermineType()",
                         argv[0]);
      QccErrorExit();
    }
  if (!QccIMGImageColor(&Image))
    {
      QccErrorAddMessage("%s: %s must be a color image",
                         argv[0],
                         Image.filename);
      QccErrorExit();
    }

  if (QccIMGImageCubeRead(&ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }

  if ((RedBand < 0) || (RedBand >= ImageCube.num_frames))
    {
      QccErrorAddMessage("%s: Invalid red band",
                         argv[0]);
      QccErrorExit();
    }
  if ((GreenBand < 0) || (GreenBand >= ImageCube.num_frames))
    {
      QccErrorAddMessage("%s: Invalid green band",
                         argv[0]);
      QccErrorExit();
    }
  if ((BlueBand < 0) || (BlueBand >= ImageCube.num_frames))
    {
      QccErrorAddMessage("%s: Invalid blue band",
                         argv[0]);
      QccErrorExit();
    }
      
  if (QccIMGImageSetSize(&Image, ImageCube.num_rows, ImageCube.num_cols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageAlloc(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
                         argv[0]);
      QccErrorExit();
    }
      
  if (QccHYPImageCubeToColor(&ImageCube,
                             &Image,
                             RedBand,
                             GreenBand,
                             BlueBand))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeToColor()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageWrite(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageCubeFree(&ImageCube);
  QccIMGImageFree(&Image);

  QccExit;
}
