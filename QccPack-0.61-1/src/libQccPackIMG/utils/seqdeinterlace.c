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


#include "seqdeinterlace.h"

#define USG_STRING "[-ss %:] %s:seqfile1 %s:seqfile2"

QccIMGImageSequence InputImageSequence;
QccIMGImageSequence OutputImageSequence;

int SuperSample = 0;


int main(int argc, char *argv[])
{
  int num_rows_Y, num_cols_Y;
  int num_rows_U, num_cols_U;
  int num_rows_V, num_cols_V;
  int image_type;

  QccInit(argc, argv);

  QccIMGImageSequenceInitialize(&InputImageSequence);
  QccIMGImageSequenceInitialize(&OutputImageSequence);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &SuperSample,
                         InputImageSequence.filename,
                         OutputImageSequence.filename))
    QccErrorExit();
  
  if (QccIMGImageSequenceFindFrameNums(&InputImageSequence))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceFindFrameNums()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceStartRead(&InputImageSequence))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceStartRead()",
                         argv[0]);
      QccErrorExit();
    }

  image_type =
    InputImageSequence.current_frame.image_type;

  if (QccIMGImageGetSizeYUV(&(InputImageSequence.current_frame),
                            &num_rows_Y, &num_cols_Y,
                            &num_rows_U, &num_cols_U,
                            &num_rows_V, &num_cols_V))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSizeYUV()",
                         argv[0]);
      QccErrorExit();
    }

  OutputImageSequence.current_frame.image_type = image_type;
  if (QccIMGImageSetSizeYUV(&(OutputImageSequence.current_frame),
                            num_rows_Y, num_cols_Y,
                            num_rows_U, num_cols_U,
                            num_rows_V, num_cols_V))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSetSizeYUV()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceAlloc(&(OutputImageSequence)))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceDeinterlace(&InputImageSequence,
                                     &OutputImageSequence,
                                     SuperSample))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceDeinterlace()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageSequenceFree(&InputImageSequence);
  QccIMGImageSequenceFree(&OutputImageSequence);

  QccExit;
}
