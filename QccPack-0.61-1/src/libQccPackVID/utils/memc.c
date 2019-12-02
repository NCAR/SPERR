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


#include "memc.h"


#define USG_STRING "[-fp %:] [-hp %:] [-qp %:] [-ep %:] [-f1 %: %s:filter1] [-f2 %: %s:filter2] [-f3 %: %s:filter3] [-w %d:window] [-b %d:block_size] [-mc %: %s:motion_compensated_frame] [-srf %: %s:subpixel_reference_frame] [-r %: %s:residual_frame] %s:reference_frame %s:current_frame %s:mvfile"

QccIMGImage ReferenceFrame;
QccIMGImage CurrentFrame;

QccString MVFilename;
int NumCols1, NumRows1;
int NumCols2, NumRows2;

int WindowSize = 15;
int BlockSize = 16;

QccIMGImageComponent HorizontalMotion;
QccIMGImageComponent VerticalMotion;

int FullPixelSpecified = 0;
int HalfPixelSpecified = 0;
int QuarterPixelSpecified = 0;
int EighthPixelSpecified = 0;
int MotionAccuracy = QCCVID_ME_FULLPIXEL;

QccIMGImageComponent ReferenceFrame2;

int FilterSpecified1 = 0;
QccString FilterFilename1;
QccFilter Filter1;
int FilterSpecified2 = 0;
QccString FilterFilename2;
QccFilter Filter2;
int FilterSpecified3 = 0;
QccString FilterFilename3;
QccFilter Filter3;

int CompensatedFrameSpecified = 0;
QccIMGImage CompensatedFrame;

int SubpixelReferenceFrameSpecified = 0;
QccIMGImage SubpixelReferenceFrame;

int ResidualFrameSpecified = 0;
QccIMGImage ResidualFrame;


int main(int argc, char *argv[])
{
  FILE *infile;

  QccInit(argc, argv);

  QccIMGImageInitialize(&ReferenceFrame);
  QccIMGImageInitialize(&CurrentFrame);
  QccIMGImageComponentInitialize(&HorizontalMotion);
  QccIMGImageComponentInitialize(&VerticalMotion);
  QccIMGImageComponentInitialize(&ReferenceFrame2);
  QccFilterInitialize(&Filter1);
  QccFilterInitialize(&Filter2);
  QccFilterInitialize(&Filter3);
  QccIMGImageInitialize(&CompensatedFrame);
  QccIMGImageInitialize(&SubpixelReferenceFrame);
  QccIMGImageInitialize(&ResidualFrame);

  if (QccParseParameters(argc, argv,
			 USG_STRING,
                         &FullPixelSpecified,
                         &HalfPixelSpecified,
                         &QuarterPixelSpecified,
                         &EighthPixelSpecified,
                         &FilterSpecified1,
                         FilterFilename1,
                         &FilterSpecified2,
                         FilterFilename2,
                         &FilterSpecified3,
                         FilterFilename3,
			 &WindowSize,
                         &BlockSize,
                         &CompensatedFrameSpecified,
                         CompensatedFrame.filename,
                         &SubpixelReferenceFrameSpecified,
                         SubpixelReferenceFrame.filename,
                         &ResidualFrameSpecified,
                         ResidualFrame.filename,
			 ReferenceFrame.filename,
			 CurrentFrame.filename,
                         MVFilename))
    QccErrorExit();
  
  if (EighthPixelSpecified)
    MotionAccuracy = QCCVID_ME_EIGHTHPIXEL;
  else
    if (QuarterPixelSpecified)
      MotionAccuracy = QCCVID_ME_QUARTERPIXEL;
    else
      if (HalfPixelSpecified)
        MotionAccuracy = QCCVID_ME_HALFPIXEL;
      else
        if (FullPixelSpecified)
          MotionAccuracy = QCCVID_ME_FULLPIXEL;
  
  if (FilterSpecified1)
    {
      if ((infile = QccFileOpen(FilterFilename1, "r")) == NULL)
        {
          QccErrorAddMessage("%s: Error calling QccFileOpen()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccFilterRead(infile, &Filter1))
        {
          QccErrorAddMessage("%s: Error calling QccFilterRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccFileClose(infile);
    }
  if (FilterSpecified2)
    {
      if ((infile = QccFileOpen(FilterFilename2, "r")) == NULL)
        {
          QccErrorAddMessage("%s: Error calling QccFileOpen()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccFilterRead(infile, &Filter2))
        {
          QccErrorAddMessage("%s: Error calling QccFilterRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccFileClose(infile);
    }
  if (FilterSpecified3)
    {
      if ((infile = QccFileOpen(FilterFilename3, "r")) == NULL)
        {
          QccErrorAddMessage("%s: Error calling QccFileOpen()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccFilterRead(infile, &Filter3))
        {
          QccErrorAddMessage("%s: Error calling QccFilterRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccFileClose(infile);
    }

  if (QccIMGImageRead(&ReferenceFrame))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
			 argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageRead(&CurrentFrame))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
			 argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageColor(&ReferenceFrame) || QccIMGImageColor(&CurrentFrame))
    {
      QccErrorAddMessage("%s: Both images must be grayscale",
			 argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageGetSize(&ReferenceFrame,
                         &NumRows1, &NumCols1))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSize()",
			 argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageGetSize(&CurrentFrame,
                         &NumRows2, &NumCols2))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSize()",
			 argv[0]);
      QccErrorExit();
    }

  if ((NumRows1 % BlockSize) || (NumCols1 % BlockSize))
    {
      QccErrorAddMessage("%s: Image dimensions must be integer multiple of block size %d",
			 argv[0],
			 BlockSize);
      QccErrorExit();
    }

  if ((NumRows1 != NumRows2) || (NumCols1 != NumCols2))
    {
      QccErrorAddMessage("%s: Both images must be same size",
			 argv[0]);
      QccErrorExit();
    }

  HorizontalMotion.num_rows = NumRows1 / BlockSize;
  HorizontalMotion.num_cols = NumCols1 / BlockSize;
  if (QccIMGImageComponentAlloc(&HorizontalMotion))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentAlloc()",
			 argv[0]);
      QccErrorExit();
    }
  VerticalMotion.num_rows = NumRows1 / BlockSize;
  VerticalMotion.num_cols = NumCols1 / BlockSize;
  if (QccIMGImageComponentAlloc(&VerticalMotion))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentAlloc()",
			 argv[0]);
      QccErrorExit();
    }

  if (WindowSize <= 0)
    {
      QccErrorAddMessage("%s: Window size must be positive, non-zero",
			 argv[0]);
      QccErrorExit();
    }

  switch (MotionAccuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      ReferenceFrame2.num_rows = ReferenceFrame.Y.num_rows;
      ReferenceFrame2.num_cols = ReferenceFrame.Y.num_cols;
      break;
    case QCCVID_ME_HALFPIXEL:
      ReferenceFrame2.num_rows = 2 * ReferenceFrame.Y.num_rows;
      ReferenceFrame2.num_cols = 2 * ReferenceFrame.Y.num_cols;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      ReferenceFrame2.num_rows = 4 * ReferenceFrame.Y.num_rows;
      ReferenceFrame2.num_cols = 4 * ReferenceFrame.Y.num_cols;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      ReferenceFrame2.num_rows = 8 * ReferenceFrame.Y.num_rows;
      ReferenceFrame2.num_cols = 8 * ReferenceFrame.Y.num_cols;
      break;
    }

  if (QccIMGImageComponentAlloc(&ReferenceFrame2))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentAlloc()",
			 argv[0]);
      QccErrorExit();
    }

  if (QccVIDMotionEstimationCreateReferenceFrame(&ReferenceFrame.Y,
                                                 &ReferenceFrame2,
                                                 MotionAccuracy,
                                                 ((FilterSpecified1) ?
                                                  &Filter1 : NULL),
                                                 ((FilterSpecified2) ?
                                                  &Filter2 : NULL),
                                                 ((FilterSpecified3) ?
                                                  &Filter3 : NULL)))
    {
      QccErrorAddMessage("%s: Error calling QccVIDMotionEstimationCreateReferenceFrame()",
			 argv[0]);
      QccErrorExit();
    }

  if (QccVIDMotionEstimationFullSearch(&CurrentFrame.Y,
                                       &ReferenceFrame2,
                                       &HorizontalMotion,
                                       &VerticalMotion,
                                       BlockSize,
                                       WindowSize,
                                       MotionAccuracy))
    {
      QccErrorAddMessage("%s: Error calling QccVIDMotionEstimationFullSearch()",
			 argv[0]);
      QccErrorExit();
    }

  if (QccVIDMotionVectorsWriteFile(&HorizontalMotion,
                                   &VerticalMotion,
                                   MVFilename,
                                   0))
    {
      QccErrorAddMessage("%s: Error calling QccVIDMotionVectorWriteFile()",
			 argv[0]);
      QccErrorExit();
    }

  if (CompensatedFrameSpecified || ResidualFrameSpecified)
    {
      if (QccIMGImageSetSize(&CompensatedFrame,
                             NumRows1,
                             NumCols1))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageAlloc(&CompensatedFrame))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
                             argv[0]);
          QccErrorExit();
        }
      
      if (QccVIDMotionEstimationCreateCompensatedFrame(&CompensatedFrame.Y,
                                                       &ReferenceFrame2,
                                                       &HorizontalMotion,
                                                       &VerticalMotion,
                                                       BlockSize,
                                                       MotionAccuracy))
        {
          QccErrorAddMessage("%s: Error calling QccVIDMotionEstimationCreateCompensatedFrame()",
                             argv[0]);
          QccErrorExit();
        }
    }
                                                       
  if (CompensatedFrameSpecified)
    {
      if (QccIMGImageDetermineType(&CompensatedFrame))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageDetermineType()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageColor(&CompensatedFrame))
        {
          QccErrorAddMessage("%s: Motion-compensated frame must be grayscale",
                             argv[0]);
          QccErrorExit();
        }
      
      if (QccIMGImageWrite(&CompensatedFrame))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  if (ResidualFrameSpecified)
    {
      if (QccIMGImageDetermineType(&ResidualFrame))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageDetermineType()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageColor(&ResidualFrame))
        {
          QccErrorAddMessage("%s: Motion-compensated frame must be grayscale",
                             argv[0]);
          QccErrorExit();
        }

      if (QccIMGImageSetSize(&ResidualFrame,
                             NumRows1,
                             NumCols1))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageAlloc(&ResidualFrame))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
                             argv[0]);
          QccErrorExit();
        }

      if (QccIMGImageComponentSubtract(&CurrentFrame.Y,
                                       &CompensatedFrame.Y,
                                       &ResidualFrame.Y))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentSubtract()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageComponentAbsoluteValue(&ResidualFrame.Y))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentAbsoluteValue()",
                             argv[0]);
          QccErrorExit();
        }

      if (QccIMGImageWrite(&ResidualFrame))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                             argv[0]);
          QccErrorExit();
        }
    }

  if (SubpixelReferenceFrameSpecified)
    {
      if (QccIMGImageDetermineType(&SubpixelReferenceFrame))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageDetermineType()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageColor(&SubpixelReferenceFrame))
        {
          QccErrorAddMessage("%s: Motion-compensated frame must be grayscale",
                             argv[0]);
          QccErrorExit();
        }
      
      if (QccIMGImageSetSize(&SubpixelReferenceFrame,
                             ReferenceFrame2.num_rows,
                             ReferenceFrame2.num_cols))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSetSize()",
                             argv[0]);
          QccErrorExit();
        }
      SubpixelReferenceFrame.Y.image = ReferenceFrame2.image;
      
      if (QccIMGImageWrite(&SubpixelReferenceFrame))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  QccIMGImageFree(&ReferenceFrame);
  QccIMGImageFree(&CurrentFrame);
  QccIMGImageComponentFree(&HorizontalMotion);
  QccIMGImageComponentFree(&VerticalMotion);
  QccIMGImageComponentFree(&ReferenceFrame2);
  QccFilterFree(&Filter1);
  QccFilterFree(&Filter2);
  QccFilterFree(&Filter3);
  QccIMGImageFree(&CompensatedFrame);

  QccExit;
}
