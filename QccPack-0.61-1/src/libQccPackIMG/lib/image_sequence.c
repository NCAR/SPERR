/*
 * 
 * QccPack: Quantization, compression, and coding libraries
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 * 
 */


#include "libQccPack.h"


int QccIMGImageSequenceInitialize(QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return(0);

  QccStringMakeNull(image_sequence->filename);
  image_sequence->start_frame_num = 0;
  image_sequence->end_frame_num = 0;
  image_sequence->current_frame_num = 0;
  QccStringMakeNull(image_sequence->current_filename);
  QccIMGImageInitialize(&(image_sequence->current_frame));

  return(0);
}


int QccIMGImageSequenceAlloc(QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return(0);

  if (QccIMGImageAlloc(&(image_sequence->current_frame)))
    {
      QccErrorAddMessage("(QccIMGImageSequenceAlloc): Error calling QccIMGImageAlloc()");
      return(1);
    }
  
  return(0);
}


void QccIMGImageSequenceFree(QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return;

  QccIMGImageFree(&(image_sequence->current_frame));
}


int QccIMGImageSequenceLength(const QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return(0);
  
  return(image_sequence->end_frame_num - image_sequence->start_frame_num + 1);
}


int QccIMGImageSequenceSetCurrentFilename(QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return(0);

  QccStringSprintf(image_sequence->current_filename,
                   image_sequence->filename,
                   image_sequence->current_frame_num);

  return(0);
}


int QccIMGImageSequenceIncrementFrameNum(QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return(0);

  image_sequence->current_frame_num++;

  if (QccIMGImageSequenceSetCurrentFilename(image_sequence))
    {
      QccErrorAddMessage("(QccIMGImageSequenceIncrementFrameNum): Error calling QccIMGImageSequenceSetCurrentFilename()");
      return(1);
    }

  return(0);
}


int QccIMGImageSequenceFindFrameNums(QccIMGImageSequence *image_sequence)
{
  struct stat infile_stat;
  unsigned int filename_length;

  if (image_sequence == NULL)
    return(0);

  image_sequence->current_frame_num = 0;
  QccIMGImageSequenceSetCurrentFilename(image_sequence);
  filename_length = strlen(image_sequence->current_filename);

  image_sequence->current_frame_num = -1;
  do
    {
      (image_sequence->current_frame_num)++;
      QccIMGImageSequenceSetCurrentFilename(image_sequence);
    }
  while ((stat(image_sequence->current_filename, &infile_stat)) && 
         (strlen(image_sequence->current_filename) == filename_length));

  if (strlen(image_sequence->current_filename) != filename_length)
    {
      QccErrorAddMessage("(QccIMGImageSequenceFindFrameNums): Image sequence matching %s not found",
                         image_sequence->filename);
      return(1);
    }

  image_sequence->start_frame_num = image_sequence->current_frame_num;

  do
    {
      (image_sequence->current_frame_num)++;
      QccIMGImageSequenceSetCurrentFilename(image_sequence);
    }
  while (!stat(image_sequence->current_filename, &infile_stat));

  image_sequence->end_frame_num = image_sequence->current_frame_num - 1;
  image_sequence->current_frame_num = -1;

  return(0);
}


int QccIMGImageSequenceReadFrame(QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return(0);

  if (QccIMGImageSequenceSetCurrentFilename(image_sequence))
    {
      QccErrorAddMessage("(QccIMGImageSequenceReadFrame): Error calling QccIMGImageSequenceSetCurrentFilename()");
      return(1);
    }

  QccStringCopy(image_sequence->current_frame.filename, 
         image_sequence->current_filename);

  if (QccIMGImageRead(&(image_sequence->current_frame)))
    {
      QccErrorAddMessage("(QccIMGImageSequenceReadFrame): Error calling QccIMGImageRead()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageSequenceWriteFrame(QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return(0);
  
  if (QccIMGImageSequenceSetCurrentFilename(image_sequence))
    {
      QccErrorAddMessage("(QccIMGImageSequenceWriteFrame): Error calling QccIMGImageSequenceSetCurrentFilename()");
      return(1);
    }
  
  QccStringCopy(image_sequence->current_frame.filename, 
                image_sequence->current_filename);
  
  if (QccIMGImageWrite(&(image_sequence->current_frame)))
    {
      QccErrorAddMessage("(QccIMGImageSequenceReadFrame): Error calling QccIMGImageWrite()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageSequenceStartRead(QccIMGImageSequence *image_sequence)
{
  if (image_sequence == NULL)
    return(0);

  image_sequence->current_frame_num = image_sequence->start_frame_num;
  
  if (QccIMGImageSequenceReadFrame(image_sequence))
    {
      QccErrorAddMessage("(QccIMGImageSequenceStartRead): Error calling QccIMGImageSequenceReadFrame()");
      return(1);
    }

  return(0);
}
