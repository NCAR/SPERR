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


#include "seqtoicb.h"

#define USG_STRING "[-sf %: %d:startframe] [-ef %: %d:endframe] %s:seqfile %s:icbfile"

int StartFrameNumSpecified = 0;
int StartFrameNum;
int EndFrameNumSpecified = 0;
int EndFrameNum;

QccIMGImageSequence ImageSequence;
QccIMGImageCube ImageCube;


int main(int argc, char *argv[])
{
  int num_rows, num_cols;

  QccInit(argc, argv);

  QccIMGImageSequenceInitialize(&ImageSequence);
  QccIMGImageCubeInitialize(&ImageCube);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &StartFrameNumSpecified,
			 &StartFrameNum,
                         &EndFrameNumSpecified,
			 &EndFrameNum,
                         ImageSequence.filename,
                         ImageCube.filename))
    QccErrorExit();
  
  if (QccIMGImageSequenceFindFrameNums(&ImageSequence))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceFindFrameNums()",
                         argv[0]);
      QccErrorExit();
    }

  if (!StartFrameNumSpecified)
    StartFrameNum = ImageSequence.start_frame_num;

  if (StartFrameNum < ImageSequence.start_frame_num)
    {
      QccErrorAddMessage("%s: Start frame %d is out of the range of available frames which start at %d",
                         argv[0],
                         StartFrameNum,
                         ImageSequence.start_frame_num);
      QccErrorExit();
    }
  ImageSequence.start_frame_num = StartFrameNum;

  if (!EndFrameNumSpecified)
    EndFrameNum = ImageSequence.end_frame_num;

  if (EndFrameNum > ImageSequence.end_frame_num)
    {
      QccErrorAddMessage("%s: End frame %d is out of the range of available frames which end at %d",
                         argv[0],
                         EndFrameNum,
                         ImageSequence.end_frame_num);
      QccErrorExit();
    }
  ImageSequence.end_frame_num = EndFrameNum;

  if (QccIMGImageSequenceStartRead(&ImageSequence))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceStartRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageGetSize(&(ImageSequence.current_frame),
                         &num_rows, &num_cols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSize()",
                         argv[0]);
      QccErrorExit();
    }

  ImageCube.num_frames = QccIMGImageSequenceLength(&ImageSequence);
  ImageCube.num_rows = num_rows;
  ImageCube.num_cols = num_cols;

  if (QccIMGImageCubeAlloc(&(ImageCube)))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceToImageCube(&ImageSequence,
                                     &ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceToImageCube()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageCubeWrite(&ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageSequenceFree(&ImageSequence);
  QccIMGImageCubeFree(&ImageCube);

  QccExit;
}
