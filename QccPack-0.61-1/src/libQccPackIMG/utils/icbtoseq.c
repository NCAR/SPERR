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


#include "icbtoseq.h"

#define USG_STRING "[-sf %d:start_frame] %s:icbfile %s:seqfile"

QccIMGImageCube ImageCube;
QccIMGImageSequence ImageSequence;

int StartFrame = 0;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);

  QccIMGImageCubeInitialize(&ImageCube);
  QccIMGImageSequenceInitialize(&ImageSequence);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &StartFrame,
                         ImageCube.filename,
                         ImageSequence.filename))
    QccErrorExit();
  
  if (QccIMGImageCubeRead(&ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }

  ImageSequence.current_frame.image_type = QCCIMGTYPE_PGM;
  ImageSequence.start_frame_num = StartFrame;
  if (QccIMGImageSetSize(&(ImageSequence.current_frame),
                         ImageCube.num_rows,
                         ImageCube.num_cols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceAlloc(&(ImageSequence)))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageCubeToImageSequence(&ImageCube,
                                     &ImageSequence))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeToImageSequence()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageCubeFree(&ImageCube);
  QccIMGImageSequenceFree(&ImageSequence);

  QccExit;
}
