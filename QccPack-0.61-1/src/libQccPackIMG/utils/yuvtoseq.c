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


#include "yuvtoseq.h"

#define USG_STRING "[-sf %d:start_frame] %d:num_rows %d:num_cols %s:yuvfile %s:seqfile"

QccString YUVFilename;
QccIMGImageSequence OutputImageSequence;

int StartFrame = 0;

int NumRows;
int NumCols;


int main(int argc, char *argv[])
{
  FILE *infile = NULL;
  int row, col;

  QccInit(argc, argv);

  QccIMGImageSequenceInitialize(&OutputImageSequence);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &StartFrame,
                         &NumRows,
                         &NumCols,
                         YUVFilename,
                         OutputImageSequence.filename))
    QccErrorExit();
  
  if ((NumRows <= 0) || (NumCols <= 0))
    {
      QccErrorAddMessage("%s: Invalid image size (%d x %d)",
                         argv[0],
                         NumCols, NumRows);
      QccErrorExit();
    }

  OutputImageSequence.current_frame.image_type = QCCIMGTYPE_PGM;
  OutputImageSequence.start_frame_num = StartFrame;
  if (QccIMGImageSetSize(&(OutputImageSequence.current_frame),
                         NumRows,
                         NumCols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceAlloc(&(OutputImageSequence)))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  OutputImageSequence.current_frame_num =
    OutputImageSequence.start_frame_num;

  if ((infile = QccFileOpen(YUVFilename, "r")) == NULL)
    {
      QccErrorAddMessage("%s: Error calling QccFileOpen()",
                         argv[0]);
      QccErrorExit();
    }

  while (!feof(infile))
    {
      for (row = 0; row < NumRows; row++)
        for (col = 0; col < NumCols; col++)
          OutputImageSequence.current_frame.Y.image[row][col] =
            fgetc(infile);
      for (row = 0; row < NumRows / 2; row++)
        for (col = 0; col < NumCols / 2; col++)
          fgetc(infile);
      for (row = 0; row < NumRows / 2; row++)
        for (col = 0; col < NumCols / 2; col++)
          fgetc(infile);

      if (!feof(infile))
        {
          if (QccIMGImageSequenceWriteFrame(&OutputImageSequence))
            {
              QccErrorAddMessage("%s: Error calling QccIMGImageSequenceWriteFrame()",
                                 argv[0]);
              QccErrorExit();
            }
          
          if (QccIMGImageSequenceIncrementFrameNum(&OutputImageSequence))
            {
              QccErrorAddMessage("%s: Error calling QccIMGImageSequenceIncrementFrameNum()",
                                 argv[0]);
              QccErrorExit();
            }
        }
    }
  
  QccFileClose(infile);

  QccIMGImageSequenceFree(&OutputImageSequence);

  QccExit;
}
