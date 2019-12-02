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


#include "icbextractframe.h"

#define USG_STRING "%d:frame_num %s:icbfile %s:icpfile"

int FrameNum;
QccIMGImageCube ImageCube;
QccIMGImageComponent ImageComponent;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);

  QccIMGImageCubeInitialize(&ImageCube);
  QccIMGImageComponentInitialize(&ImageComponent);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &FrameNum,
                         ImageCube.filename,
                         ImageComponent.filename))
    QccErrorExit();

  if (QccIMGImageCubeRead(&ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }

  if ((FrameNum < 0) || (FrameNum >= ImageCube.num_frames))
    {
      QccErrorAddMessage("%s: Invalid frame number",
                         argv[0]);
      QccErrorExit();
    }
      
  ImageComponent.num_rows = ImageCube.num_rows;
  ImageComponent.num_cols = ImageCube.num_cols;
  if (QccIMGImageComponentAlloc(&ImageComponent))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentAlloc()",
                         argv[0]);
      QccErrorExit();
    }
      
  if (QccIMGImageCubeExtractFrame(&ImageCube,
                                  FrameNum,
                                  &ImageComponent))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeExtractFrame()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageComponentWrite(&ImageComponent))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageCubeFree(&ImageCube);
  QccIMGImageComponentFree(&ImageComponent);

  QccExit;
}
