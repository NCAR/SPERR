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

/*
 *  This code was written by Joe Boettcher <jbb15@msstate.edu> based on
 *  the originally developed algorithm and code by Suxia Cui.
 */

#include "rdwtblockencode.h"


#define USG_STRING "[-fp %:] [-hp %:] [-qp %:] [-ep %:] [-f1 %: %s:filter1] [-f2 %: %s:filter2] [-f3 %: %s:filter3] [-w %s:wavelet] [-b %s:boundary] [-nl %d:num_levels] [-mv %: %s:mvfile] [-rmv %:] [-bs %d:blocksize] [-sf %: %d:startframe] [-ef %: %d:endframe] [-q %:] %f:rate %s:sequence %s:bitstream"

QccIMGImageSequence ImageSequence;

QccBitBuffer OutputBuffer;

int FullPixelSpecified = 0;
int HalfPixelSpecified = 0;
int QuarterPixelSpecified = 0;
int EighthPixelSpecified = 0;
int MotionAccuracy = QCCVID_ME_FULLPIXEL;

int FilterSpecified1 = 0;
QccString FilterFilename1;
QccFilter Filter1;
int FilterSpecified2 = 0;
QccString FilterFilename2;
QccFilter Filter2;
int FilterSpecified3 = 0;
QccString FilterFilename3;
QccFilter Filter3;

int StartFrameNumSpecified = 0;
int StartFrameNum;
int EndFrameNumSpecified = 0;
int EndFrameNum;

int Quiet = 0;
float Rate = 0.5;
int TargetBitCnt;

int NumRows;
int NumCols;

int NumLevels = 3;

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

int MVFilenameSpecified = 0;
QccString MVFilename;
int ReadMotionVectors = 0;

int Blocksize = 8;


int main(int argc, char *argv[])
{
  FILE *infile;

  QccInit(argc, argv);

  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageSequenceInitialize(&ImageSequence);
  QccBitBufferInitialize(&OutputBuffer);
  QccFilterInitialize(&Filter1);
  QccFilterInitialize(&Filter2);
  QccFilterInitialize(&Filter3);

  if (QccParseParameters(argc, argv, USG_STRING,
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
                         WaveletFilename,
                         Boundary, 
                         &NumLevels,
                         &MVFilenameSpecified,
                         &MVFilename,
                         &ReadMotionVectors,
                         &Blocksize,
                         &StartFrameNumSpecified,
			 &StartFrameNum,
                         &EndFrameNumSpecified,
			 &EndFrameNum,
			 &Quiet,
			 &Rate,
			 ImageSequence.filename,
                         OutputBuffer.filename))
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

  if (QccWAVWaveletCreate(&Wavelet, WaveletFilename, Boundary))
    {
      QccErrorAddMessage("%s: Error calling QccWAVWaveletCreate()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceFindFrameNums(&ImageSequence))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceFindFrameNums()",
			 argv[0]);
      QccErrorExit();
    }

  if (StartFrameNumSpecified)
    {
      if (StartFrameNum < ImageSequence.start_frame_num)
        {
          QccErrorAddMessage("%s: Start frame %d is out of the range of available frames which start at %d",
                             argv[0],
                             ImageSequence.start_frame_num);
          QccErrorExit();
        }
      ImageSequence.start_frame_num = StartFrameNum;
    }
  if (EndFrameNumSpecified)
    {
      if (EndFrameNum > ImageSequence.end_frame_num)
        {
          QccErrorAddMessage("%s: End frame %d is out of the range of available frames which end at %d",
                             argv[0],
                             EndFrameNum,
                             ImageSequence.end_frame_num);
          QccErrorExit();
        }
      ImageSequence.end_frame_num = EndFrameNum;
    }

  if (QccIMGImageSequenceStartRead(&ImageSequence))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceStartRead()",
			 argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageColor(&ImageSequence.current_frame))
    {
      QccErrorAddMessage("%s: Images must be grayscale",
			 argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageGetSize(&ImageSequence.current_frame,
                         &NumRows, &NumCols))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSize()",
			 argv[0]);
      QccErrorExit();
    }

  TargetBitCnt = (int)(Rate * NumRows * NumCols);

  if ((NumRows % Blocksize) ||
      (NumCols % Blocksize))
    {
      QccErrorAddMessage("%s: Image size must be a multiple of %d",
			 argv[0], Blocksize);
      QccErrorExit();
    }

  OutputBuffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
  if (!QccFileSeekable(OutputBuffer.fileptr))
    {
      QccErrorAddMessage("%s: Output bitstream is not a seekable stream",
                         argv[0]);
      QccErrorExit();
    }

  if (ReadMotionVectors && (!MVFilenameSpecified))
    {
      QccErrorAddMessage("%s: Must provide motion-vectors filename for reading",
                         argv[0]);
      QccErrorExit();
    }

  if (QccVIDRDWTBlockEncode(&ImageSequence,
                            ((FilterSpecified1) ?
                             &Filter1 : NULL),
                            ((FilterSpecified2) ?
                             &Filter2 : NULL),
                            ((FilterSpecified3) ?
                             &Filter3 : NULL),
                            MotionAccuracy,
                            &OutputBuffer,
                            Blocksize,
                            NumLevels,
                            TargetBitCnt,
                            &Wavelet,
                            ((MVFilenameSpecified) ?
                             MVFilename : NULL),
                            ReadMotionVectors,
                            Quiet))
    {
      QccErrorAddMessage("%s: Error calling QccVIDRDWTBlockEncode()",
			 argv[0]);
      QccErrorExit();
    }

  if (QccBitBufferEnd(&OutputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferEnd()",
                         argv[0]);
      QccErrorExit();
    }

  QccWAVWaveletFree(&Wavelet);
  QccIMGImageSequenceFree(&ImageSequence);
  QccFilterFree(&Filter1);
  QccFilterFree(&Filter2);
  QccFilterFree(&Filter3);

  QccExit;
}
