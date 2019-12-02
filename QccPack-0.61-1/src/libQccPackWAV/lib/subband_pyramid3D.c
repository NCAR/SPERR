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


int QccWAVSubbandPyramid3DInitialize(QccWAVSubbandPyramid3D
                                     *subband_pyramid)
{
  if (subband_pyramid == NULL)
    return(0);
  
  QccStringMakeNull(subband_pyramid->filename);
  QccStringCopy(subband_pyramid->magic_num, QCCWAVSUBBANDPYRAMID3D_MAGICNUM);
  QccGetQccPackVersion(&subband_pyramid->major_version,
                       &subband_pyramid->minor_version,
                       NULL);
  subband_pyramid->transform_type = QCCWAVSUBBANDPYRAMID3D_PACKET;
  subband_pyramid->temporal_num_levels = 0;
  subband_pyramid->spatial_num_levels = 0;
  subband_pyramid->num_frames = 0;
  subband_pyramid->num_rows = 0;
  subband_pyramid->num_cols = 0;
  subband_pyramid->origin_frame = 0;
  subband_pyramid->origin_row = 0;
  subband_pyramid->origin_col = 0;
  subband_pyramid->subsample_pattern_frame = 0;
  subband_pyramid->subsample_pattern_row = 0;
  subband_pyramid->subsample_pattern_col = 0;

  subband_pyramid->volume = NULL;

  return(0);
}


int QccWAVSubbandPyramid3DAlloc(QccWAVSubbandPyramid3D *subband_pyramid)
{
  int return_value;

  if (subband_pyramid == NULL)
    return(0);

  if (subband_pyramid->volume != NULL)
    return(0);

  if ((subband_pyramid->volume =
       QccVolumeAlloc(subband_pyramid->num_frames,
                      subband_pyramid->num_rows,
                      subband_pyramid->num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DAlloc): Error calling QccVolumeAlloc()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  QccWAVSubbandPyramid3DFree(subband_pyramid);
  return_value = 1;
 Return:
  return(return_value);
}


void QccWAVSubbandPyramid3DFree(QccWAVSubbandPyramid3D *subband_pyramid)
{
  if (subband_pyramid == NULL)
    return;
  
  QccVolumeFree(subband_pyramid->volume,
  		subband_pyramid->num_frames,
                subband_pyramid->num_rows);
  subband_pyramid->volume = NULL;

}


int QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(int num_levels)
{
  return(num_levels * 7 + 1);
}


int QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(int temporal_num_levels,
                                                       int spatial_num_levels)
{
  return(QccWAVSubbandPyramidNumLevelsToNumSubbands(spatial_num_levels) *
         (temporal_num_levels + 1));
}


int QccWAVSubbandPyramid3DNumSubbandsToNumLevelsDyadic(int num_subbands)
{
  if (num_subbands <= 0)
    return(0);
  if ((num_subbands - 1) % 7)
    return(0);
  else
    return((num_subbands - 1) / 7);
}


int QccWAVSubbandPyramid3DCalcLevelFromSubbandDyadic(int subband,
                                                     int num_levels)
{
  if (!num_levels)
    return(0);

  if (!subband)
    return(num_levels);

  return(num_levels - (subband - 1) / 7);
}


int QccWAVSubbandPyramid3DCalcLevelFromSubbandPacket(int subband,
                                                     int temporal_num_levels,
                                                     int spatial_num_levels,
                                                     int *temporal_level,
                                                     int *spatial_level)
{
  int spatial_num_subbands;

  spatial_num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(spatial_num_levels);

  if (temporal_level != NULL)
    *temporal_level = (subband < spatial_num_subbands) ?
      temporal_num_levels :
      temporal_num_levels - (subband / spatial_num_subbands) + 1;
  if (spatial_level != NULL)
    *spatial_level = 
      QccWAVSubbandPyramidCalcLevelFromSubband(subband % spatial_num_subbands,
                                               spatial_num_levels);

  return(0);
}


int QccWAVSubbandPyramid3DSubbandSize(const QccWAVSubbandPyramid3D
                                      *subband_pyramid,
                                      int subband,
                                      int *subband_num_frames,
                                      int *subband_num_rows,
                                      int *subband_num_cols)
{
  int num_frames = 0;
  int num_rows = 0;
  int num_cols = 0;
  int level;
  QccWAVSubbandPyramid temp;
  int spatial_num_levels = subband_pyramid->spatial_num_levels;
  int temporal_num_levels = subband_pyramid->temporal_num_levels;
  int origin_frame = subband_pyramid->origin_frame;
  int origin_row = subband_pyramid->origin_row;
  int origin_col = subband_pyramid->origin_col;
  int subsample_pattern_frame = subband_pyramid->subsample_pattern_frame;
  int subsample_pattern_row = subband_pyramid->subsample_pattern_row;
  int subsample_pattern_col = subband_pyramid->subsample_pattern_col;
  int spatial_num_subbands;

  QccWAVSubbandPyramidInitialize(&temp);

  if (subband < 0)
    return(0);
  
  if (subband_pyramid->transform_type == QCCWAVSUBBANDPYRAMID3D_DYADIC)
    {
      level =
        QccWAVSubbandPyramid3DCalcLevelFromSubbandDyadic(subband,
                                                         spatial_num_levels);
      
      if (!subband)
        {
          num_frames =
            QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                          level,
                                          0,
                                          origin_frame,
                                          subsample_pattern_frame);
          num_rows =
            QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                          level,
                                          0,
                                          origin_row,
                                          subsample_pattern_row);
          num_cols =
            QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                          level,
                                          0,
                                          origin_col,
                                          subsample_pattern_col);
        }      
      else
        switch ((subband - 1) % 7)
          {
          case 0:
            num_frames =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                            level,
                                            0,
                                            origin_frame,
                                            subsample_pattern_frame);
            num_rows =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                            level,
                                            1,
                                            origin_row,
                                            subsample_pattern_row);
            num_cols =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                            level,
                                            0,
                                            origin_col,
                                            subsample_pattern_col);
            break;
          case 1:
            num_frames =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                            level,
                                            0,
                                            origin_frame,
                                            subsample_pattern_frame);
            num_rows =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                            level,
                                            0,
                                            origin_row,
                                            subsample_pattern_row);
            num_cols =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                            level,
                                            1,
                                            origin_col,
                                            subsample_pattern_col);
            break;
          case 2:
            num_frames =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                            level,
                                            1,
                                            origin_frame,
                                            subsample_pattern_frame);
            num_rows =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                            level,
                                            0,
                                            origin_row,
                                            subsample_pattern_row);
            num_cols =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                            level,
                                            0,
                                            origin_col,
                                            subsample_pattern_col);
            break;
          case 3:
            num_frames =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                            level,
                                            0,
                                            origin_frame,
                                            subsample_pattern_frame);
            num_rows =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                            level,
                                            1,
                                            origin_row,
                                            subsample_pattern_row);
            num_cols =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                            level,
                                            1,
                                            origin_col,
                                            subsample_pattern_col);
            break;
          case 4:
            num_frames =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                            level,
                                            1,
                                            origin_frame,
                                            subsample_pattern_frame);
            num_rows =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                            level,
                                            1,
                                            origin_row,
                                            subsample_pattern_row);
            num_cols =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                            level,
                                            0,
                                            origin_col,
                                            subsample_pattern_col);
            break;
          case 5:
            num_frames =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                            level,
                                            1,
                                            origin_frame,
                                            subsample_pattern_frame);
            num_rows =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                            level,
                                            0,
                                            origin_row,
                                            subsample_pattern_row);
            num_cols =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                            level,
                                            1,
                                            origin_col,
                                            subsample_pattern_col);
            break;
          case 6:
            num_frames =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                            level,
                                            1,
                                            origin_frame,
                                            subsample_pattern_frame);
            num_rows =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                            level,
                                            1,
                                            origin_row,
                                            subsample_pattern_row);
            num_cols =
              QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                            level,
                                            1,
                                            origin_col,
                                            subsample_pattern_col);
            break;
          }
    }
  else
    if (subband_pyramid->transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
      {
        temp.num_levels = spatial_num_levels;
        temp.num_rows = subband_pyramid->num_rows;
        temp.num_cols = subband_pyramid->num_cols;
        temp.origin_row = origin_row;
        temp.origin_col = origin_col;
        temp.subsample_pattern_row = subsample_pattern_row;
        temp.subsample_pattern_col = subsample_pattern_col;

        spatial_num_subbands =
          QccWAVSubbandPyramidNumLevelsToNumSubbands(spatial_num_levels);

        QccWAVSubbandPyramid3DCalcLevelFromSubbandPacket(subband,
                                                         temporal_num_levels,
                                                         spatial_num_levels,
                                                         &level,
                                                         NULL);

        num_frames =
          QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                        level,
                                        (subband >= spatial_num_subbands),
                                        origin_frame,
                                        subsample_pattern_frame);
        
        if (QccWAVSubbandPyramidSubbandSize(&temp,
                                            subband % spatial_num_subbands,
                                            &num_rows,
                                            &num_cols))
          {
            QccErrorAddMessage("(QccWAVSubbandPyramid3DSubbandSize): Error calling QccWAVSubbandPyramidSubbandSize()");
            return(1);
          }
      }
  
  if (!num_frames || !num_rows || !num_cols)
    num_frames = num_rows = num_cols = 0;
  
  if (subband_num_frames != NULL)
    *subband_num_frames = num_frames;
  if (subband_num_rows != NULL)
    *subband_num_rows = num_rows;
  if (subband_num_cols != NULL)
    *subband_num_cols = num_cols;
  
  return(0);
}


int QccWAVSubbandPyramid3DSubbandOffsets(const QccWAVSubbandPyramid3D
                                         *subband_pyramid,
                                         int subband,
					 int *frame_offset,
					 int *row_offset,
                                         int *col_offset)
{
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  int spatial_num_levels = subband_pyramid->spatial_num_levels;
  int temporal_num_levels = subband_pyramid->temporal_num_levels;
  int spatial_num_subbands;
  int level;
  QccWAVSubbandPyramid3D temp;
  QccWAVSubbandPyramid temp2;

  QccWAVSubbandPyramid3DInitialize(&temp);
  QccWAVSubbandPyramidInitialize(&temp2);

  if (subband < 0)
    return(0);

  if (subband_pyramid->transform_type == QCCWAVSUBBANDPYRAMID3D_DYADIC)
    {
      temp.spatial_num_levels =
      temp.temporal_num_levels =
        QccWAVSubbandPyramid3DCalcLevelFromSubbandDyadic(subband,
                                                         spatial_num_levels);
      temp.num_frames = subband_pyramid->num_frames;
      temp.num_rows = subband_pyramid->num_rows;
      temp.num_cols = subband_pyramid->num_cols;
      temp.origin_frame = subband_pyramid->origin_frame;
      temp.origin_row = subband_pyramid->origin_row;
      temp.origin_col = subband_pyramid->origin_col;
      temp.subsample_pattern_frame =
        subband_pyramid->subsample_pattern_frame;
      temp.subsample_pattern_row =
        subband_pyramid->subsample_pattern_row;
      temp.subsample_pattern_col =
        subband_pyramid->subsample_pattern_col;
      temp.transform_type = QCCWAVSUBBANDPYRAMID3D_DYADIC;

      if (QccWAVSubbandPyramid3DSubbandSize(&temp,
                                            0,
                                            &subband_num_frames,
                                            &subband_num_rows,
                                            &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DSubbandOffsets): Error calling QccWAVSubbandPyramid3DSubbandSize()");
          return(1);
        }
      
      if (!subband)
        {
          *frame_offset = 0;
          *row_offset = 0;
          *col_offset = 0;
        }
      else
        switch ((subband - 1) % 7)
          {
          case 0:
            *frame_offset = 0;
            *row_offset = subband_num_rows;
            *col_offset = 0;
            break;
          case 1:
            *frame_offset = 0;
            *row_offset = 0;
            *col_offset = subband_num_cols;
            break;
          case 2:
            *frame_offset = subband_num_frames;
            *row_offset = 0;
            *col_offset = 0;
            break;
          case 3:
            *frame_offset = 0;
            *row_offset = subband_num_rows;
            *col_offset = subband_num_cols;
            break;
          case 4:
            *frame_offset = subband_num_frames;
            *row_offset = subband_num_rows;
            *col_offset = 0;
            break;
          case 5:
            *frame_offset = subband_num_frames;
            *row_offset = 0;
            *col_offset = subband_num_cols;
            break;
          case 6:
            *frame_offset = subband_num_frames;
            *row_offset = subband_num_rows;
            *col_offset = subband_num_cols;
            break;
          }
    }
  else
    if (subband_pyramid->transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
      {
        temp2.num_levels = spatial_num_levels;
        temp2.num_rows = subband_pyramid->num_rows;
        temp2.num_cols = subband_pyramid->num_cols;
        temp2.origin_row = subband_pyramid->origin_row;
        temp2.origin_col = subband_pyramid->origin_col;
        temp2.subsample_pattern_row =
          subband_pyramid->subsample_pattern_row;
        temp2.subsample_pattern_col =
          subband_pyramid->subsample_pattern_col;

        spatial_num_subbands =
          QccWAVSubbandPyramidNumLevelsToNumSubbands(spatial_num_levels);

        QccWAVSubbandPyramid3DCalcLevelFromSubbandPacket(subband,
                                                         temporal_num_levels,
                                                         spatial_num_levels,
                                                         &level,
                                                         NULL);

        if (subband < spatial_num_subbands)
          *frame_offset = 0;
        else
          *frame_offset =
            QccWAVWaveletDWTSubbandLength(subband_pyramid->num_frames,
                                          level,
                                          0,
                                          subband_pyramid->origin_frame,
                                          subband_pyramid->subsample_pattern_frame);
        
        if (QccWAVSubbandPyramidSubbandOffsets(&temp2,
                                               subband % spatial_num_subbands,
                                               row_offset,
                                               col_offset))
          {
            QccErrorAddMessage("(QccWAVSubbandPyramid3DSubbandOffsets): Error calling QccWAVSubbandPyramidSubbandOffsets()");
            return(1);
          }
      }
  
  return(0);
}


int QccWAVSubbandPyramid3DPrint(const QccWAVSubbandPyramid3D *subband_pyramid)
{
  int frame, row, col;

  if (QccFilePrintFileInfo(subband_pyramid->filename,
                           subband_pyramid->magic_num,
                           subband_pyramid->major_version,
                           subband_pyramid->minor_version))
    return(1);

  printf("Transform type: ");
  switch (subband_pyramid->transform_type)
    {
    case QCCWAVSUBBANDPYRAMID3D_DYADIC:
      printf("dyadic\n");
      printf("Levels of decomposition: %d\n",
             subband_pyramid->spatial_num_levels);
      break;
    case QCCWAVSUBBANDPYRAMID3D_PACKET:
      printf("packet\n");
      printf("Levels of decomposition:\n");
      printf("  temporal: %d\n", subband_pyramid->temporal_num_levels);
      printf("   spatial: %d\n", subband_pyramid->spatial_num_levels);
      break;
    default:
      printf("unknown\n");
    }
  printf("Size: %dx%dx%d (cols x rows x frames)\n",
         subband_pyramid->num_cols,
         subband_pyramid->num_rows,
	 subband_pyramid->num_frames);
  printf("Origin: (%d, %d, %d)\n",
         subband_pyramid->origin_frame,
	 subband_pyramid->origin_row,
         subband_pyramid->origin_col);

  printf("\nVolume:\n\n");

  for (frame = 0; frame < subband_pyramid->num_frames; frame++)
    {
      printf("======================== Frame %d ===========================\n",
             frame);

      for (row = 0; row < subband_pyramid->num_rows; row++)
        {
          for (col = 0; col < subband_pyramid->num_cols; col++)
            printf("% 10.4f ", subband_pyramid->volume[frame][row][col]);
          printf("\n");
        }
    }
  return(0);
}


static int QccWAVSubbandPyramid3DReadHeader(FILE *infile,
                                            QccWAVSubbandPyramid3D
                                            *subband_pyramid)
{
  
  if ((infile == NULL) || (subband_pyramid == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             subband_pyramid->magic_num,
                             &subband_pyramid->major_version,
                             &subband_pyramid->minor_version))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading magic number in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (strcmp(subband_pyramid->magic_num, QCCWAVSUBBANDPYRAMID3D_MAGICNUM))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): %s is not of subband-pyramid (%s) type",
                         subband_pyramid->filename,
                         QCCWAVSUBBANDPYRAMID3D_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->transform_type));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading transform type in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading transform type in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  switch (subband_pyramid->transform_type)
    {
    case QCCWAVSUBBANDPYRAMID3D_DYADIC:
      fscanf(infile, "%d", &(subband_pyramid->spatial_num_levels));
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of levels in %s",
                             subband_pyramid->filename);
          return(1);
        }
      subband_pyramid->temporal_num_levels =
        subband_pyramid->spatial_num_levels;
      if (QccFileSkipWhiteSpace(infile, 0))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of levels in %s",
                             subband_pyramid->filename);
          return(1);
        }
      break;
    case QCCWAVSUBBANDPYRAMID3D_PACKET:
      fscanf(infile, "%d", &(subband_pyramid->temporal_num_levels));
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of temporal levels in %s",
                             subband_pyramid->filename);
          return(1);
        }
      if (QccFileSkipWhiteSpace(infile, 0))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of spatial levels in %s",
                             subband_pyramid->filename);
          return(1);
        }
      fscanf(infile, "%d", &(subband_pyramid->spatial_num_levels));
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of spatial levels in %s",
                             subband_pyramid->filename);
          return(1);
        }
      if (QccFileSkipWhiteSpace(infile, 0))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of temporal levels in %s",
                             subband_pyramid->filename);
          return(1);
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Unknown transform type (%d) in %s",
                         subband_pyramid->transform_type,
                         subband_pyramid->filename);
      return(1);
    }

  fscanf(infile, "%d", &(subband_pyramid->num_cols));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of columns in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of columns in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->num_rows));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of rows in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of rows in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->num_frames));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of frames in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading number of frames in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  fscanf(infile, "%*1[\n]");
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DReadHeader): Error reading subband pyramid %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  return(0);
}


int QccWAVSubbandPyramid3DRead(QccWAVSubbandPyramid3D *subband_pyramid)
{
  FILE *infile = NULL;
  int frame, row, col;

  if (subband_pyramid == NULL)
    return(0);

  if ((infile = QccFileOpen(subband_pyramid->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DRead): Error calling QccFileOpen()");
      return(1);
    }

  if (QccWAVSubbandPyramid3DReadHeader(infile, subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DRead): Error calling QccWAVSubbandPyramid3DReadHeader()");
      return(1);
    }

  if (QccWAVSubbandPyramid3DAlloc(subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DRead): Error calling QccIMGSubbandPyramidAlloc()");
      return(1);
    }

  for (frame = 0; frame < subband_pyramid->num_frames; frame++)
    for (row = 0; row < subband_pyramid->num_rows; row++)
      for (col = 0; col < subband_pyramid->num_cols; col++)
        if (QccFileReadDouble(infile,
                              &(subband_pyramid->volume[frame][row][col])))
          {
            QccErrorAddMessage("(QccWAVSubbandPyramid3DRead): Error calling QccFileReadDouble()",
                               subband_pyramid->filename);
            return(1);
          }

  QccFileClose(infile);
  return(0);
}


static int QccWAVSubbandPyramid3DWriteHeader(FILE *outfile,
                                             const QccWAVSubbandPyramid3D
                                             *subband_pyramid)
{
  if ((outfile == NULL) || (subband_pyramid == NULL))
    return(0);

  if (QccFileWriteMagicNumber(outfile, QCCWAVSUBBANDPYRAMID3D_MAGICNUM))
    goto Error;

  fprintf(outfile, "%d\n", subband_pyramid->transform_type);
  if (ferror(outfile))
    goto Error;

  switch (subband_pyramid->transform_type)
    {
    case QCCWAVSUBBANDPYRAMID3D_DYADIC:
      fprintf(outfile, "%d\n", subband_pyramid->spatial_num_levels);
      if (ferror(outfile))
        goto Error;
      break;
    case QCCWAVSUBBANDPYRAMID3D_PACKET:
      fprintf(outfile, "%d %d\n",
              subband_pyramid->temporal_num_levels,
              subband_pyramid->spatial_num_levels);
      if (ferror(outfile))
        goto Error;
      break;
    default:
      QccErrorAddMessage("(QccWAVSubbandPyramid3DWriteHeader): Unrecognized transform type");
      goto Error;
    }
  
  fprintf(outfile, "%d %d %d\n",
          subband_pyramid->num_cols,
          subband_pyramid->num_rows,
	  subband_pyramid->num_frames);
  if (ferror(outfile))
    goto Error;

  return(0);

 Error:
  QccErrorAddMessage("(QccWAVSubbandPyramid3DWriteHeader): Error writing header to %s",
                     subband_pyramid->filename);
  return(1);
}


int QccWAVSubbandPyramid3DWrite(const QccWAVSubbandPyramid3D *subband_pyramid)
{
  FILE *outfile;
  int frame, row, col;

  if (subband_pyramid == NULL)
    return(0);

  if ((outfile = QccFileOpen(subband_pyramid->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DWrite): Error calling QccFileOpen()");
      return(1);
    }

  if (QccWAVSubbandPyramid3DWriteHeader(outfile, subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DWrite): Error calling QccWAVSubbandPyramid3DWriteHeader()");
      return(1);
    }

  for (frame = 0; frame < subband_pyramid->num_frames; frame++)
    for (row = 0; row < subband_pyramid->num_rows; row++)
      for (col = 0; col < subband_pyramid->num_cols; col++)
        if (QccFileWriteDouble(outfile,
                               subband_pyramid->volume[frame][row][col]))
          {
            QccErrorAddMessage("(QccWAVSubbandPyramid3DWrite): Error calling QccFileWriteDouble()");
            return(1);
          }

  QccFileClose(outfile);
  return(0);
}


int QccWAVSubbandPyramid3DCopy(QccWAVSubbandPyramid3D *subband_pyramid1,
                               const QccWAVSubbandPyramid3D *subband_pyramid2)
{
  if (subband_pyramid1 == NULL)
    return(0);
  if (subband_pyramid2 == NULL)
    return(0);

  if ((subband_pyramid1->num_rows != subband_pyramid2->num_rows) ||
      (subband_pyramid1->num_cols != subband_pyramid2->num_cols) ||
      (subband_pyramid1->num_frames != subband_pyramid2->num_frames))
    {
      QccWAVSubbandPyramid3DFree(subband_pyramid1);
      subband_pyramid1->num_frames = subband_pyramid2->num_frames;
      subband_pyramid1->num_rows = subband_pyramid2->num_rows;
      subband_pyramid1->num_cols = subband_pyramid2->num_cols;
      if (QccWAVSubbandPyramid3DAlloc(subband_pyramid1))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidd3DCopy): Error calling QccWAVSubbandPyramid3DAlloc()");
          return(1);
        }
    }

  subband_pyramid1->transform_type = subband_pyramid2->transform_type;

  subband_pyramid1->spatial_num_levels = subband_pyramid2->spatial_num_levels;
  subband_pyramid1->temporal_num_levels =
    subband_pyramid2->temporal_num_levels;

  subband_pyramid1->origin_frame = subband_pyramid2->origin_frame;
  subband_pyramid1->origin_row = subband_pyramid2->origin_row;
  subband_pyramid1->origin_col = subband_pyramid2->origin_col;

  subband_pyramid1->subsample_pattern_frame =
    subband_pyramid2->subsample_pattern_frame;
  subband_pyramid1->subsample_pattern_row =
    subband_pyramid2->subsample_pattern_row;
  subband_pyramid1->subsample_pattern_col =
    subband_pyramid2->subsample_pattern_col;

  if (QccVolumeCopy(subband_pyramid1->volume,
                    subband_pyramid2->volume,
		    subband_pyramid1->num_frames,
                    subband_pyramid1->num_rows,
                    subband_pyramid1->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DCopy): Error calling QccVolumeCopy()");
      return(1);
    }

  return(0);
}


int QccWAVSubbandPyramid3DZeroSubband(QccWAVSubbandPyramid3D *subband_pyramid,
                                      int subband)
{
  int num_subbands;
  int subband_num_frames;
  int subband_num_rows;
  int subband_num_cols;
  int subband_frame;
  int subband_row;
  int subband_col;
  int frame, row, col;
  int spatial_num_levels = subband_pyramid->spatial_num_levels;
  int temporal_num_levels = subband_pyramid->temporal_num_levels;

  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->volume == NULL)
    return(0);
  if (subband < 0)
    return(0);
  
  switch (subband_pyramid->transform_type)
    {
    case QCCWAVSUBBANDPYRAMID3D_DYADIC:
      num_subbands =
        QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(spatial_num_levels);
      break;
    case QCCWAVSUBBANDPYRAMID3D_PACKET:
      num_subbands =
        QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(spatial_num_levels,
                                                           temporal_num_levels);
      break;
    default:
      QccErrorAddMessage("(QccWAVSubbandPyramid3DZeroSubband): Unrecognized transform type");
      return(1);
    }

  if (subband > num_subbands)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DZeroSubband): specified subband is large for given subband pyramid");
      return(1);
    }
  
  if (QccWAVSubbandPyramid3DSubbandSize(subband_pyramid,
                                        subband,
                                        &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DZeroSubband): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      return(1);
    }
  
  if (QccWAVSubbandPyramid3DSubbandOffsets(subband_pyramid,
                                           subband,
                                           &subband_frame,
                                           &subband_row,
                                           &subband_col))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DZeroSubband): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      return(1);
    }
  
  for (frame = subband_frame; frame < subband_frame + subband_num_frames;
       frame++)
    for (row = subband_row; row < subband_row + subband_num_rows; row++)
      for (col = subband_col; col < subband_col + subband_num_cols; col++)
        subband_pyramid->volume[frame][row][col] = 0;
  
  return(0);
}


int QccWAVSubbandPyramid3DSubtractMean(QccWAVSubbandPyramid3D *subband_pyramid,
                                       double *mean,
                                       const QccSQScalarQuantizer *quantizer)
{
  int subband_frame, subband_row, subband_col;
  int subband_num_frames, subband_num_rows, subband_num_cols;
  int partition;
  
  double mean1;
  
  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->volume == NULL)
    return(0);
  
  if (QccWAVSubbandPyramid3DSubbandSize(subband_pyramid,
                                        0,
  			                &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DSubtractMean): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      return(1);
    }
  
  mean1 = QccVolumeMean(subband_pyramid->volume,
                        subband_num_frames,
			subband_num_rows,
                        subband_num_cols);
  
  if (quantizer != NULL)
    {
      if (QccSQScalarQuantization(mean1, quantizer, NULL, &partition))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DSubtractMean): Error calling QccSQScalarQuantization()");
          return(1);
        }
      if (QccSQInverseScalarQuantization(partition, quantizer, &mean1))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DSubtractMean): Error calling QccSQInverseScalarQuantization()");
          return(1);
        }
    }
  
  for (subband_frame = 0; subband_frame < subband_num_frames; subband_frame++)
    for (subband_row = 0; subband_row < subband_num_rows; subband_row++)
      for (subband_col = 0; subband_col < subband_num_cols; subband_col++)
        subband_pyramid->volume[subband_frame][subband_row][subband_col] -=
          mean1;
  
  if (mean != NULL)
    *mean = mean1;
  
  return(0);
}


int QccWAVSubbandPyramid3DAddMean(QccWAVSubbandPyramid3D *subband_pyramid,
                                  double mean)
{
  int subband_frame, subband_row, subband_col;
  int subband_num_frames, subband_num_rows, subband_num_cols;
  
  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->volume == NULL)
    return(0);
  if (subband_pyramid->spatial_num_levels ||
      subband_pyramid->temporal_num_levels)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DAddMean): Subband-pyramid number of levels is not zero");
      return(1);
    }
  
  if (QccWAVSubbandPyramid3DSubbandSize(subband_pyramid,
                                        0,
                                        &subband_num_frames,
                                        &subband_num_rows,
                                        &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DAddMean): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      return(1);
    }
  
  for (subband_frame = 0; subband_frame < subband_num_frames; subband_frame++)
    for (subband_row = 0; subband_row < subband_num_rows; subband_row++)
      for (subband_col = 0; subband_col < subband_num_cols; subband_col++)
        subband_pyramid->volume[subband_frame][subband_row][subband_col] +=
          mean;
  
  return(0);
}


int QccWAVSubbandPyramid3DDWT(QccWAVSubbandPyramid3D *subband_pyramid,
                              int transform_type,
                              int temporal_num_levels,
                              int spatial_num_levels,
                              const QccWAVWavelet *wavelet)
{
  int return_value;
  
  if (subband_pyramid == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (subband_pyramid->spatial_num_levels ||
      subband_pyramid->temporal_num_levels)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DDWT): Subband pyramid is already decomposed");
      goto Error;
    }
  
  switch (transform_type)
    {
    case QCCWAVSUBBANDPYRAMID3D_DYADIC:
      subband_pyramid->transform_type = QCCWAVSUBBANDPYRAMID3D_DYADIC;
      if (spatial_num_levels != temporal_num_levels)
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DDWT): Spatial and temporal number of levels must be identical for a dyadic transform");
          goto Error;
        }
      subband_pyramid->spatial_num_levels =
        subband_pyramid->temporal_num_levels = spatial_num_levels;
      if (QccWAVWaveletDyadicDWT3D(subband_pyramid->volume,
				   subband_pyramid->num_frames,
				   subband_pyramid->num_rows,
                                   subband_pyramid->num_cols,
                                   subband_pyramid->origin_frame,
                                   subband_pyramid->origin_row,
                                   subband_pyramid->origin_col,
                                   subband_pyramid->subsample_pattern_frame,
                                   subband_pyramid->subsample_pattern_row,
                                   subband_pyramid->subsample_pattern_col,
				   spatial_num_levels,
				   wavelet))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DDWT): Error calling QccWAVWaveletDyadicDWT3D()");
          goto Error;
        }
      break;
    case QCCWAVSUBBANDPYRAMID3D_PACKET:
      subband_pyramid->transform_type = QCCWAVSUBBANDPYRAMID3D_PACKET;
      subband_pyramid->spatial_num_levels = spatial_num_levels;
      subband_pyramid->temporal_num_levels = temporal_num_levels;
      if (QccWAVWaveletPacketDWT3D(subband_pyramid->volume, 
                                   subband_pyramid->num_frames,
                                   subband_pyramid->num_rows,
                                   subband_pyramid->num_cols,
                                   subband_pyramid->origin_frame,
                                   subband_pyramid->origin_row,
                                   subband_pyramid->origin_col,
                                   subband_pyramid->subsample_pattern_frame,
                                   subband_pyramid->subsample_pattern_row,
                                   subband_pyramid->subsample_pattern_col,
                                   temporal_num_levels,
                                   spatial_num_levels,
                                   wavelet))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DDWT): Error calling QccWAVWaveletPacketDWT3D()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVSubbandPyramid3DDWT): Unrecognized transform type");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramid3DInverseDWT(QccWAVSubbandPyramid3D *subband_pyramid,
                                     const QccWAVWavelet *wavelet)
{
  int return_value;
  
  if (subband_pyramid == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if ((subband_pyramid->spatial_num_levels <= 0) &&
      (subband_pyramid->temporal_num_levels <= 0))
    return(0);

  switch (subband_pyramid->transform_type)
    {
    case QCCWAVSUBBANDPYRAMID3D_DYADIC:
      if (QccWAVWaveletInverseDyadicDWT3D(subband_pyramid->volume,
                                          subband_pyramid->num_frames,
                                          subband_pyramid->num_rows,
                                          subband_pyramid->num_cols,
                                          subband_pyramid->origin_frame,
                                          subband_pyramid->origin_row,
                                          subband_pyramid->origin_col,
                                          subband_pyramid->subsample_pattern_frame,
                                          subband_pyramid->subsample_pattern_row,
                                          subband_pyramid->subsample_pattern_col,
                                          subband_pyramid->spatial_num_levels,
                                          wavelet))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DInverseDWT): Error calling QccWAVWaveletDyadicInverseDWT3D()");
          goto Error;
        }
      break;
    case QCCWAVSUBBANDPYRAMID3D_PACKET:
      if (QccWAVWaveletInversePacketDWT3D(subband_pyramid->volume,
                                          subband_pyramid->num_frames,
                                          subband_pyramid->num_rows,
                                          subband_pyramid->num_cols,
                                          subband_pyramid->origin_frame,
                                          subband_pyramid->origin_row,
                                          subband_pyramid->origin_col,
                                          subband_pyramid->subsample_pattern_frame,
                                          subband_pyramid->subsample_pattern_row,
                                          subband_pyramid->subsample_pattern_col,
                                          subband_pyramid->temporal_num_levels,
                                          subband_pyramid->spatial_num_levels,
                                          wavelet))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DInverseDWT): Error calling QccWAVWaveletPacketInverseDWT3D()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVSubbandPyramid3DInverseDWT): Unrecognized transform type");
      goto Error;
    }
  
  subband_pyramid->spatial_num_levels = 0;
  subband_pyramid->temporal_num_levels = 0;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramid3DShapeAdaptiveDWT(QccWAVSubbandPyramid3D
                                           *subband_pyramid,
                                           QccWAVSubbandPyramid3D *mask,
                                           int transform_type,
                                           int temporal_num_levels,
                                           int spatial_num_levels,
                                           const QccWAVWavelet *wavelet)
{
  int return_value;
  
  if (subband_pyramid == NULL)
    return(0);
  if (mask == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (subband_pyramid->spatial_num_levels ||
      subband_pyramid->temporal_num_levels)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DShapeAdaptiveDWT): Subband pyramid is already decomposed");
      goto Error;
    }
  
  if ((subband_pyramid->num_frames != mask->num_frames) ||
      (subband_pyramid->num_rows != mask->num_rows) ||
      (subband_pyramid->num_cols != mask->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DShapeAdaptiveDWT): Subband pyramid and mask must have same number of frames, rows, and columns");
      goto Error;
    }
  
  switch (transform_type)
    {
    case QCCWAVSUBBANDPYRAMID3D_DYADIC:
      subband_pyramid->transform_type = QCCWAVSUBBANDPYRAMID3D_DYADIC;
      mask->transform_type = QCCWAVSUBBANDPYRAMID3D_DYADIC;
      if (spatial_num_levels != temporal_num_levels)
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DDWT): Spatial and temporal number of levels must be identical for a dyadic transform");
          goto Error;
        }
      subband_pyramid->spatial_num_levels =
        subband_pyramid->temporal_num_levels = spatial_num_levels;
      mask->spatial_num_levels =
        mask->temporal_num_levels = spatial_num_levels;
      if (QccWAVWaveletShapeAdaptiveDyadicDWT3D(subband_pyramid->volume,
                                                mask->volume,
                                                subband_pyramid->num_frames,
                                                subband_pyramid->num_rows,
                                                subband_pyramid->num_cols,
                                                spatial_num_levels,
                                                wavelet))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DShapeAdaptiveDWT): Error calling QccWAVWaveletShapeAdaptiveDyadicDWT3D()");
          goto Error;
        }
      break;
    case QCCWAVSUBBANDPYRAMID3D_PACKET:
      subband_pyramid->transform_type = QCCWAVSUBBANDPYRAMID3D_PACKET;
      subband_pyramid->spatial_num_levels = spatial_num_levels;
      subband_pyramid->temporal_num_levels = temporal_num_levels;
      mask->transform_type = QCCWAVSUBBANDPYRAMID3D_PACKET;
      mask->spatial_num_levels = spatial_num_levels;
      mask->temporal_num_levels = temporal_num_levels;
      if (QccWAVWaveletShapeAdaptivePacketDWT3D(subband_pyramid->volume,
                                                mask->volume,
                                                subband_pyramid->num_frames,
                                                subband_pyramid->num_rows,
                                                subband_pyramid->num_cols,
                                                temporal_num_levels,
                                                spatial_num_levels,
                                                wavelet))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DShapeAdaptiveDWT): Error calling QccWAVWaveletShapeAdaptivePacketDWT3D()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVSubbandPyramid3DShapeAdaptiveDWT): Unrecognized transform type");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT(QccWAVSubbandPyramid3D
                                                  *subband_pyramid,
                                                  QccWAVSubbandPyramid3D *mask,
                                                  const QccWAVWavelet *wavelet)
{
  int return_value;
  
  if (subband_pyramid == NULL)
    return(0);
  if (mask == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if ((subband_pyramid->spatial_num_levels <= 0) &&
      (subband_pyramid->temporal_num_levels <= 0))
    return(0);
  
  if ((subband_pyramid->num_frames != mask->num_frames) ||
      (subband_pyramid->num_rows != mask->num_rows) ||
      (subband_pyramid->num_cols != mask->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT): Subband pyramid and mask must have same number of frames, rows, and columns");
      goto Error;
    }
  
  if ((subband_pyramid->spatial_num_levels != mask->spatial_num_levels) ||
      (subband_pyramid->temporal_num_levels != mask->temporal_num_levels))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT): Subband pyramid and mask must have same number of levels of decomposition");
      goto Error;
    }
  
  switch (subband_pyramid->transform_type)
    {
    case QCCWAVSUBBANDPYRAMID3D_DYADIC:
      if (QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D(subband_pyramid->volume,
                                                       mask->volume,
                                                       subband_pyramid->num_frames,
                                                       subband_pyramid->num_rows,
                                                       subband_pyramid->num_cols,
                                                       subband_pyramid->spatial_num_levels,
                                                       wavelet))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT): Error calling QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D)");
          goto Error;
        }
      break;
    case QCCWAVSUBBANDPYRAMID3D_PACKET:
      if (QccWAVWaveletInverseShapeAdaptivePacketDWT3D(subband_pyramid->volume,
                                                       mask->volume,
                                                       subband_pyramid->num_frames,
                                                       subband_pyramid->num_rows,
                                                       subband_pyramid->num_cols,
                                                       subband_pyramid->temporal_num_levels,
                                                       subband_pyramid->spatial_num_levels,
                                                       wavelet))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT): Error calling QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D)");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT): Unrecognized transform type");
      goto Error;
    }  
  subband_pyramid->spatial_num_levels = 0;
  subband_pyramid->temporal_num_levels = 0;
  mask->spatial_num_levels = 0;
  mask->temporal_num_levels = 0;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramid3DPacketToDyadic(QccWAVSubbandPyramid3D
                                         *subband_pyramid,
                                         int num_levels)
{
  int return_value;
  QccWAVWavelet lazy_wavelet_transform;

  QccWAVWaveletInitialize(&lazy_wavelet_transform);

  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->volume == NULL)
    return(0);

  if (subband_pyramid->transform_type != QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DPacketToDyadic): Subband pyramid is not a packet decomposition");
      goto Error;
    }

  if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DPacketToDyadic): Error calling QccWAVWaveletCreate()");
      goto Error;
    }
      
  if (QccWAVSubbandPyramid3DInverseDWT(subband_pyramid,
                                       &lazy_wavelet_transform))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DPacketToDyadic): Error calling QccWAVSubbandPyramid3DInverseDWT()");
      goto Error;
    }

  if (QccWAVSubbandPyramid3DDWT(subband_pyramid,
                                QCCWAVSUBBANDPYRAMID3D_DYADIC,
                                num_levels,
                                num_levels,
                                &lazy_wavelet_transform))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DPacketToDyadic): Error calling QccWAVSubbandPyramid3DDWT()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVWaveletFree(&lazy_wavelet_transform);
  return(return_value);
}


int QccWAVSubbandPyramid3DDyadicToPacket(QccWAVSubbandPyramid3D
                                         *subband_pyramid,
                                         int temporal_num_levels,
                                         int spatial_num_levels)
{
  int return_value;
  QccWAVWavelet lazy_wavelet_transform;

  QccWAVWaveletInitialize(&lazy_wavelet_transform);

  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->volume == NULL)
    return(0);

  if (subband_pyramid->transform_type != QCCWAVSUBBANDPYRAMID3D_DYADIC)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DDyadicToPacket): Subband pyramid is not a dyadic decomposition");
      goto Error;
    }

  if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DDyadicToPacket): Error calling QccWAVWaveletCreate()");
      goto Error;
    }
      
  if (QccWAVSubbandPyramid3DInverseDWT(subband_pyramid,
                                       &lazy_wavelet_transform))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DDyadicToPacket): Error calling QccWAVSubbandPyramid3DInverseDWT()");
      goto Error;
    }

  if (QccWAVSubbandPyramid3DDWT(subband_pyramid,
                                QCCWAVSUBBANDPYRAMID3D_PACKET,
                                temporal_num_levels,
                                spatial_num_levels,
                                &lazy_wavelet_transform))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramid3DDyadicToPacket): Error calling QccWAVSubbandPyramid3DDWT()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVWaveletFree(&lazy_wavelet_transform);
  return(return_value);
}
