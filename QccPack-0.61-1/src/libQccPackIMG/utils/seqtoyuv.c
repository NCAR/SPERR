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

#define USG_STRING "[-sf %: %d:startframe] [-ef %: %d:endframe] %s:seqfile %s:yuvfile"

int StartFrameNumSpecified = 0;
int StartFrameNum;
int EndFrameNumSpecified = 0;
int EndFrameNum;

QccIMGImageSequence ImageSequence;
QccString YUVFilename;


int main(int argc, char *argv[])
{
  int row, col;
  int num_rows, num_cols;
  int current_frame;
  FILE *outfile = NULL;

  QccInit(argc, argv);
  
  QccIMGImageSequenceInitialize(&ImageSequence);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &StartFrameNumSpecified,
			 &StartFrameNum,
                         &EndFrameNumSpecified,
			 &EndFrameNum,
                         ImageSequence.filename,
                         YUVFilename))
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
  
  if (QccIMGImageColor(&(ImageSequence.current_frame)))
    {
      QccErrorAddMessage("%s: Images must be grayscale",
                         argv[0]);
      QccErrorExit();
    }
  
  ImageSequence.current_frame_num = ImageSequence.start_frame_num - 1;

  if ((outfile = QccFileOpen(YUVFilename, "w")) == NULL)
    {
      QccErrorAddMessage("%s: Error calling QccFileOpen()",
                         argv[0]);
      QccErrorExit();
    }

  for (current_frame = ImageSequence.start_frame_num;
       current_frame <= ImageSequence.end_frame_num;
       current_frame++)
    {
      if (QccIMGImageSequenceIncrementFrameNum(&ImageSequence))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSequenceIncrementFrameNum()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageSequenceReadFrame(&ImageSequence))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSequenceReadFrame()",
                             argv[0]);
          QccErrorExit();
        }
      
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          fputc((int)(ImageSequence.current_frame.Y.image[row][col]),
                outfile);
      for (row = 0; row < num_rows / 2; row++)
        for (col = 0; col < num_cols / 2; col++)
          fputc(0, outfile);
      for (row = 0; row < num_rows / 2; row++)
        for (col = 0; col < num_cols / 2; col++)
          fputc(0, outfile);
    }

  QccFileClose(outfile);

  QccIMGImageSequenceFree(&ImageSequence);
  
  QccExit;
}
