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

#include "spatialblockdecode.h"


#define USG_STRING "[-fp %:] [-hp %:] [-qp %:] [-ep %:] [-f1 %: %s:filter1] [-f2 %: %s:filter2] [-f3 %: %s:filter3] [-w %s:wavelet] [-b %s:boundary] [-mv %: %s:mvfile] [-q %:] %s:bitstream %s:sequence"

QccIMGImageSequence ImageSequence;

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

int NumRows, NumCols;
int NumLevels;
int TargetBitCnt;

QccBitBuffer InputBuffer;

int IntraFramesOnly;

int Quiet = 0;

QccWAVWavelet Wavelet;
QccString WaveletFilename = QCCWAVWAVELET_DEFAULT_WAVELET;
QccString Boundary = "symmetric";

int Blocksize;

int MVFilenameSpecified = 0;
QccString MVFilename;


int main(int argc, char *argv[])
{
  FILE *infile;
  
  QccInit(argc, argv);
  
  QccBitBufferInitialize(&InputBuffer);
  QccWAVWaveletInitialize(&Wavelet);
  QccIMGImageSequenceInitialize(&ImageSequence);
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
                         &MVFilenameSpecified,
                         MVFilename,
			 &Quiet,
                         InputBuffer.filename,
			 ImageSequence.filename))
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
  
  InputBuffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&InputBuffer))
    {
      QccErrorAddMessage("%s: Error calling QccBitBufferStart()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccVIDSpatialBlockDecodeHeader(&InputBuffer,
                                     &NumRows,
                                     &NumCols,
                                     &ImageSequence.start_frame_num,
                                     &ImageSequence.end_frame_num,
                                     &Blocksize,
                                     &NumLevels,
                                     &TargetBitCnt))
    {
      QccErrorAddMessage("%s: Error calling QccVIDSpatialBlockDecodeHeader()",
                         argv[0]);
      QccErrorExit();
    }
  
  ImageSequence.current_frame.image_type = QCCIMGTYPE_PGM;
  
  QccIMGImageSetSize(&ImageSequence.current_frame, NumRows, NumCols);
  if (QccIMGImageSequenceAlloc(&ImageSequence))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceAlloc()",
			 argv[0]);
      QccErrorExit();
    }
  
  if (QccVIDSpatialBlockDecode(&ImageSequence,
                               ((FilterSpecified1) ?
                                &Filter1 : NULL),
                               ((FilterSpecified2) ?
                                &Filter2 : NULL),
                               ((FilterSpecified3) ?
                                &Filter3 : NULL),
                               MotionAccuracy,
                               &InputBuffer,
                               TargetBitCnt,
                               Blocksize,
                               NumLevels,
                               &Wavelet,
                               ((MVFilenameSpecified) ?
                                MVFilename : NULL),
                               Quiet))
    {
      QccErrorAddMessage("%s: Error calling QccVIDSpatialBlockDecode()",
			 argv[0]);
      QccErrorExit();
    }
  
  if (QccBitBufferEnd(&InputBuffer))
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
