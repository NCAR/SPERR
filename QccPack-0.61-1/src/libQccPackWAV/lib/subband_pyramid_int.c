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


int QccWAVSubbandPyramidIntInitialize(QccWAVSubbandPyramidInt *subband_pyramid)
{
  if (subband_pyramid == NULL)
    return(0);
  
  QccStringMakeNull(subband_pyramid->filename);
  QccStringCopy(subband_pyramid->magic_num, QCCWAVSUBBANDPYRAMIDINT_MAGICNUM);
  QccGetQccPackVersion(&subband_pyramid->major_version,
                       &subband_pyramid->minor_version,
                       NULL);
  subband_pyramid->num_levels = 0;
  
  subband_pyramid->num_rows = 0;
  subband_pyramid->num_cols = 0;
  subband_pyramid->origin_row = 0;
  subband_pyramid->origin_col = 0;
  subband_pyramid->subsample_pattern_row = 0;
  subband_pyramid->subsample_pattern_col = 0;

  subband_pyramid->matrix = NULL;
  
  return(0);
}


int QccWAVSubbandPyramidIntAlloc(QccWAVSubbandPyramidInt *subband_pyramid)
{
  int return_value;
  
  if (subband_pyramid == NULL)
    return(0);
  
  if (subband_pyramid->matrix != NULL)
    return(0);

  if ((subband_pyramid->matrix =
       QccMatrixIntAlloc(subband_pyramid->num_rows,
                         subband_pyramid->num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntAlloc): Error calling QccMatrixIntAlloc()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  QccWAVSubbandPyramidIntFree(subband_pyramid);
  return_value = 1;
 Return:
  return(return_value);
}


void QccWAVSubbandPyramidIntFree(QccWAVSubbandPyramidInt *subband_pyramid)
{
  if (subband_pyramid == NULL)
    return;
  
  QccMatrixIntFree(subband_pyramid->matrix,
                   subband_pyramid->num_rows);
  subband_pyramid->matrix = NULL;
  
}


int QccWAVSubbandPyramidIntNumLevelsToNumSubbands(int num_levels)
{
  return(QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels));
}


int QccWAVSubbandPyramidIntNumSubbandsToNumLevels(int num_subbands)
{
  return(QccWAVSubbandPyramidNumSubbandsToNumLevels(num_subbands));
}


int QccWAVSubbandPyramidIntCalcLevelFromSubband(int subband, int num_levels)
{
  return(QccWAVSubbandPyramidCalcLevelFromSubband(subband, num_levels));
}


int QccWAVSubbandPyramidIntSubbandSize(const QccWAVSubbandPyramidInt
                                       *subband_pyramid,
                                       int subband,
                                       int *subband_num_rows,
                                       int *subband_num_cols)
{
  int num_rows = 0;
  int num_cols = 0;
  int level;
  
  level = 
    QccWAVSubbandPyramidIntCalcLevelFromSubband(subband,
                                                subband_pyramid->num_levels);
  if (!subband)
    {
      num_rows = 
        QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                      level,
                                      0,
                                      subband_pyramid->origin_row,
                                      subband_pyramid->subsample_pattern_row);
      num_cols = 
        QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                      level,
                                      0,
                                      subband_pyramid->origin_col,
                                      subband_pyramid->subsample_pattern_col);
    }
  else
    switch ((subband - 1) % 3)
      {
      case 0:
        num_rows = 
          QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                        level,
                                        1,
                                        subband_pyramid->origin_row,
                                        subband_pyramid->subsample_pattern_row);
        num_cols = 
          QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                        level,
                                        0,
                                        subband_pyramid->origin_col,
                                        subband_pyramid->subsample_pattern_col);
        break;
      case 1:
        num_rows = 
          QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                        level,
                                        0,
                                        subband_pyramid->origin_row,
                                        subband_pyramid->subsample_pattern_row);
        num_cols = 
          QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                        level,
                                        1,
                                        subband_pyramid->origin_col,
                                        subband_pyramid->subsample_pattern_col);
        break;
      case 2:
        num_rows = 
          QccWAVWaveletDWTSubbandLength(subband_pyramid->num_rows,
                                        level,
                                        1,
                                        subband_pyramid->origin_row,
                                        subband_pyramid->subsample_pattern_row);
        num_cols = 
          QccWAVWaveletDWTSubbandLength(subband_pyramid->num_cols,
                                        level,
                                        1,
                                        subband_pyramid->origin_col,
                                        subband_pyramid->subsample_pattern_col);
        break;
      }
  
  if (!num_rows || !num_cols)
    num_rows = num_cols = 0;
  
  if (subband_num_rows != NULL)
    *subband_num_rows = num_rows;
  if (subband_num_cols != NULL)
    *subband_num_cols = num_cols;
  
  return(0);
}


int QccWAVSubbandPyramidIntSubbandOffsets(const QccWAVSubbandPyramidInt 
                                          *subband_pyramid,
                                          int subband,
                                          int *row_offset,
                                          int *col_offset)
{
  int subband_num_rows;
  int subband_num_cols;
  QccWAVSubbandPyramidInt temp;
  
  if (subband < 0)
    return(0);
  
  QccWAVSubbandPyramidIntInitialize(&temp);
  
  temp.num_levels =
    QccWAVSubbandPyramidIntCalcLevelFromSubband(subband,
                                                subband_pyramid->num_levels);
  temp.num_rows = subband_pyramid->num_rows;
  temp.num_cols = subband_pyramid->num_cols;
  temp.origin_row = subband_pyramid->origin_row;
  temp.origin_col = subband_pyramid->origin_col;
  temp.subsample_pattern_row = subband_pyramid->subsample_pattern_row;
  temp.subsample_pattern_col = subband_pyramid->subsample_pattern_col;
  
  if (QccWAVSubbandPyramidIntSubbandSize(&temp,
                                         0,
                                         &subband_num_rows,
                                         &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntSubbandOffsets): Error calling QccWAVSubbandPyramidIntSubbandSize()");
      return(1);
    }
  
  if (!subband)
    {
      *row_offset = 0;
      *col_offset = 0;
    }
  else
    switch ((subband - 1) % 3)
      {
      case 0:
        *row_offset = subband_num_rows;
        *col_offset = 0;
        break;
      case 1:
        *row_offset = 0;
        *col_offset = subband_num_cols;
        break;
      case 2:
        *row_offset = subband_num_rows;
        *col_offset = subband_num_cols;
        break;
      }
  
  return(0);
}


void QccWAVSubbandPyramidIntSubbandToQccString(int subband, int num_levels,
                                               QccString qccstring)
{
  QccWAVSubbandPyramidSubbandToQccString(subband,
                                         num_levels,
                                         qccstring);
}


int QccWAVSubbandPyramidIntPrint(const QccWAVSubbandPyramidInt
                                 *subband_pyramid)
{
  int row, col;
  
  if (QccFilePrintFileInfo(subband_pyramid->filename,
                           subband_pyramid->magic_num,
                           subband_pyramid->major_version,
                           subband_pyramid->minor_version))
    return(1);
  
  printf("Levels of decomposition: %d\n",
         subband_pyramid->num_levels);
  printf("size: %dx%d\n",
         subband_pyramid->num_cols,
         subband_pyramid->num_rows);
  printf("Origin: (%d, %d)\n",
         subband_pyramid->origin_row,
         subband_pyramid->origin_col);

  printf("\nMatrix:\n\n");
  
  for (row = 0; row < subband_pyramid->num_rows; row++)
    {
      for (col = 0; col < subband_pyramid->num_cols; col++)
        {
          printf("% 10d ", subband_pyramid->matrix[row][col]);
        }
      printf("\n");
    }      
  return(0);
}


static int QccWAVSubbandPyramidIntReadHeader(FILE *infile, 
                                             QccWAVSubbandPyramidInt
                                             *subband_pyramid)
{
  
  if ((infile == NULL) || (subband_pyramid == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             subband_pyramid->magic_num,
                             &subband_pyramid->major_version,
                             &subband_pyramid->minor_version))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): Error reading magic number in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (strcmp(subband_pyramid->magic_num, QCCWAVSUBBANDPYRAMIDINT_MAGICNUM))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): %s is not of subband-pyramid (%s) type",
                         subband_pyramid->filename, 
                         QCCWAVSUBBANDPYRAMIDINT_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->num_levels));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): Error reading number of levels in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): Error reading number of levels in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->num_cols));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): Error reading number of columns in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): Error reading number of columns in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->num_rows));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): Error reading number of rows in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): Error reading number of rows in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  fscanf(infile, "%*1[\n]");
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntReadHeader): Error reading subband pyramid %s",
                         subband_pyramid->filename);
      return(1);
    }

  return(0);
}


int QccWAVSubbandPyramidIntRead(QccWAVSubbandPyramidInt *subband_pyramid)
{
  FILE *infile = NULL;
  int row, col;
  
  if (subband_pyramid == NULL)
    return(0);
  
  if ((infile = QccFileOpen(subband_pyramid->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntRead): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidIntReadHeader(infile, subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntRead): Error calling QccWAVSubbandPyramidIntReadHeader()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidIntAlloc(subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntRead): Error calling QccIMGSubbandPyramidIntAlloc()");
      return(1);
    }
  
  for (row = 0; row < subband_pyramid->num_rows; row++)
    for (col = 0; col < subband_pyramid->num_cols; col++)
      if (QccFileReadInt(infile,
                         &(subband_pyramid->matrix[row][col])))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidIntRead): Error calling QccFileReadInt()",
                             subband_pyramid->filename);
          return(1);
        }
  
  QccFileClose(infile);
  return(0);
}


static int QccWAVSubbandPyramidIntWriteHeader(FILE *outfile, 
                                              const QccWAVSubbandPyramidInt
                                              *subband_pyramid)
{
  if ((outfile == NULL) || (subband_pyramid == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCWAVSUBBANDPYRAMIDINT_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d\n%d %d\n",
          subband_pyramid->num_levels,
          subband_pyramid->num_cols,
          subband_pyramid->num_rows);
  if (ferror(outfile))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccWAVSubbandPyramidIntWriteHeader): Error writing header to %s",
                     subband_pyramid->filename);
  return(1);
}


int QccWAVSubbandPyramidIntWrite(const QccWAVSubbandPyramidInt *subband_pyramid)
{
  FILE *outfile;
  int row, col;
  
  if (subband_pyramid == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(subband_pyramid->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidIntWriteHeader(outfile, subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntWrite): Error calling QccWAVSubbandPyramidIntWriteHeader()");
      return(1);
    }
  
  for (row = 0; row < subband_pyramid->num_rows; row++)
    for (col = 0; col < subband_pyramid->num_cols; col++)
      if (QccFileWriteInt(outfile,
                          subband_pyramid->matrix[row][col]))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidIntWrite): Error calling QccFileWriteInt()");
          return(1);
        }
  
  QccFileClose(outfile);
  return(0);
}


int QccWAVSubbandPyramidIntCopy(QccWAVSubbandPyramidInt *subband_pyramid1,
                                const QccWAVSubbandPyramidInt
                                *subband_pyramid2)
{
  if (subband_pyramid1 == NULL)
    return(0);
  if (subband_pyramid2 == NULL)
    return(0);

  if ((subband_pyramid1->num_rows != subband_pyramid2->num_rows) ||
      (subband_pyramid1->num_cols != subband_pyramid2->num_cols))
    {
      QccWAVSubbandPyramidIntFree(subband_pyramid1);
      subband_pyramid1->num_rows = subband_pyramid2->num_rows;
      subband_pyramid1->num_cols = subband_pyramid2->num_cols;
      if (QccWAVSubbandPyramidIntAlloc(subband_pyramid1))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidIntCopy): Error calling QccWAVSubbandPyramidIntAlloc()");
          return(1);
        }
    }

  subband_pyramid1->num_levels = subband_pyramid2->num_levels;
  
  subband_pyramid1->origin_row = subband_pyramid2->origin_row;
  subband_pyramid1->origin_col = subband_pyramid2->origin_col;

  subband_pyramid1->subsample_pattern_row =
    subband_pyramid2->subsample_pattern_row;
  subband_pyramid1->subsample_pattern_col =
    subband_pyramid2->subsample_pattern_col;

  if (QccMatrixIntCopy(subband_pyramid1->matrix,
                       subband_pyramid2->matrix,
                       subband_pyramid1->num_rows,
                       subband_pyramid1->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntCopy): Error calling QccMatrixIntCopy()");
      return(1);
    }

  return(0);
}


int QccWAVSubbandPyramidIntZeroSubband(QccWAVSubbandPyramidInt
                                       *subband_pyramid,
                                       int subband)
{
  int num_subbands;
  int subband_num_rows;
  int subband_num_cols;
  int subband_row;
  int subband_col;
  int row, col;

  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->matrix == NULL)
    return(0);
  if (subband < 0)
    return(0);

  num_subbands =
    QccWAVSubbandPyramidIntNumLevelsToNumSubbands(subband_pyramid->num_levels);

  if (subband > num_subbands)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntZeroSubband): specified subband is large for given subband pyramid");
      return(1);
    }
  
  if (QccWAVSubbandPyramidIntSubbandSize(subband_pyramid,
                                         subband,
                                         &subband_num_rows,
                                         &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntZeroSubband): Error calling QccWAVSubbandPyramidIntSubbandSize()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidIntSubbandOffsets(subband_pyramid,
                                            subband,
                                            &subband_row,
                                            &subband_col))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntZeroSubband): Error calling QccWAVSubbandPyramidIntSubbandOffsets()");
      return(1);
    }
  
  for (row = subband_row; row < subband_row + subband_num_rows; row++)
    for (col = subband_col; col < subband_col + subband_num_cols; col++)
      subband_pyramid->matrix[row][col] = 0;

  return(0);
}


int QccWAVSubbandPyramidIntSubtractMean(QccWAVSubbandPyramidInt
                                        *subband_pyramid,
                                        int *mean)
{
  int subband_row, subband_col;
  int subband_num_rows, subband_num_cols;
  int mean1;

  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->matrix == NULL)
    return(0);

  if (QccWAVSubbandPyramidIntSubbandSize(subband_pyramid,
                                         0,
                                         &subband_num_rows,
                                         &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntSubtractMean): Error calling QccWAVSubbandPyramidIntSubbandSize()");
      return(1);
    }
  
  mean1 = (int)rint(QccMatrixIntMean(subband_pyramid->matrix,
                                     subband_num_rows,
                                     subband_num_cols));
  for (subband_row = 0; subband_row < subband_num_rows; subband_row++)
    for (subband_col = 0; subband_col < subband_num_cols; subband_col++)
      subband_pyramid->matrix[subband_row][subband_col] -= mean1;
  
  if (mean != NULL)
    *mean = mean1;
  
  return(0);
}


int QccWAVSubbandPyramidIntAddMean(QccWAVSubbandPyramidInt *subband_pyramid,
                                   int mean)
{
  int subband_row, subband_col;
  int subband_num_rows, subband_num_cols;

  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->matrix == NULL)
    return(0);
  if (subband_pyramid->num_levels != 0)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntAddMean): Subband-pyramid number of levels is not zero");
      return(1);
    }
  
  if (QccWAVSubbandPyramidIntSubbandSize(subband_pyramid,
                                         0,
                                         &subband_num_rows,
                                         &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntAddMean): Error calling QccWAVSubbandPyramidIntSubbandSize()");
      return(1);
    }

  for (subband_row = 0; subband_row < subband_num_rows; subband_row++)
    for (subband_col = 0; subband_col < subband_num_cols; subband_col++)
      subband_pyramid->matrix[subband_row][subband_col] += mean;
  
  return(0);
}


int QccWAVSubbandPyramidIntRasterScan(const QccWAVSubbandPyramidInt
                                      *subband_pyramid,
                                      QccVectorInt scanned_coefficients)
{
  int num_subbands;
  int subband;
  int subband_num_rows, subband_num_cols;
  int subband_row_offset, subband_col_offset;
  int row, col;
  int coefficient;
  
  if (subband_pyramid == NULL)
    return(0);
  if (scanned_coefficients == NULL)
    return(0);
  
  num_subbands =
    QccWAVSubbandPyramidIntNumLevelsToNumSubbands(subband_pyramid->num_levels);
  
  coefficient = 0;
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidIntSubbandSize(subband_pyramid,
                                             subband,
                                             &subband_num_rows,
                                             &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidIntRasterScan): Error calling QccWAVSubbandPyramidIntSubbandSize()");
          return(1);
        }
      if (QccWAVSubbandPyramidIntSubbandOffsets(subband_pyramid,
                                                subband,
                                                &subband_row_offset,
                                                &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidIntRasterScan): Error calling QccWAVSubbandPyramidIntSubbandOffsets()");
          return(1);
        }
      
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          scanned_coefficients[coefficient++] =
            subband_pyramid->matrix
            [subband_row_offset + row]
            [subband_col_offset + col];
    }
  
  return(0);
}


int QccWAVSubbandPyramidIntInverseRasterScan(QccWAVSubbandPyramidInt
                                             *subband_pyramid,
                                             const QccVectorInt
                                             scanned_coefficients)
{
  int num_subbands;
  int subband;
  int subband_num_rows, subband_num_cols;
  int subband_row_offset, subband_col_offset;
  int row, col;
  int coefficient;

  if (subband_pyramid == NULL)
    return(0);
  if (scanned_coefficients == NULL)
    return(0);
  
  num_subbands =
    QccWAVSubbandPyramidIntNumLevelsToNumSubbands(subband_pyramid->num_levels);
  
  coefficient = 0;
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidIntSubbandSize(subband_pyramid,
                                             subband,
                                             &subband_num_rows,
                                             &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidIntInverseRasterScan): Error calling QccWAVSubbandPyramidIntSubbandSize()");
          return(1);
        }
      if (QccWAVSubbandPyramidIntSubbandOffsets(subband_pyramid,
                                                subband,
                                                &subband_row_offset,
                                                &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidIntInverseRasterScan): Error calling QccWAVSubbandPyramidIntSubbandOffsets()");
          return(1);
        }
      
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          subband_pyramid->matrix
            [subband_row_offset + row]
            [subband_col_offset + col] =
            scanned_coefficients[coefficient++];
    }
  
  return(0);
}


int QccWAVSubbandPyramidIntDWT(QccWAVSubbandPyramidInt *subband_pyramid,
                               int num_levels,
                               const QccWAVWavelet *wavelet)
{
  int return_value;

  if (subband_pyramid == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (subband_pyramid->num_levels != 0)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntDWT): Subband pyramid is already decomposed");
      goto Error;
    }

  subband_pyramid->num_levels = num_levels;
  
  if (QccWAVWaveletDWT2DInt(subband_pyramid->matrix,
                            subband_pyramid->num_rows,
                            subband_pyramid->num_cols,
                            subband_pyramid->origin_row,
                            subband_pyramid->origin_col,
                            subband_pyramid->subsample_pattern_row,
                            subband_pyramid->subsample_pattern_col,
                            num_levels,
                            wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntDWT): Error calling QccWAVWaveletDWT2DInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramidIntInverseDWT(QccWAVSubbandPyramidInt *subband_pyramid,
                                      const QccWAVWavelet *wavelet)
{
  int return_value;
  
  if (subband_pyramid == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (subband_pyramid->num_levels <= 0)
    return(0);

  if (QccWAVWaveletInverseDWT2DInt(subband_pyramid->matrix,
                                   subband_pyramid->num_rows,
                                   subband_pyramid->num_cols,
                                   subband_pyramid->origin_row,
                                   subband_pyramid->origin_col,
                                   subband_pyramid->subsample_pattern_row,
                                   subband_pyramid->subsample_pattern_col,
                                   subband_pyramid->num_levels,
                                   wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntInverseDWT): Error calling QccWAVWaveletDWT2DInt()");
      goto Error;
    }
  
  subband_pyramid->num_levels = 0;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramidIntShapeAdaptiveDWT(QccWAVSubbandPyramidInt
                                            *subband_pyramid,
                                            QccWAVSubbandPyramidInt *mask,
                                            int num_levels,
                                            const QccWAVWavelet *wavelet)
{
  int return_value;

  if (subband_pyramid == NULL)
    return(0);
  if (mask == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (subband_pyramid->num_levels != 0)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntShapeAdaptiveDWT): Subband pyramid is already decomposed");
      goto Error;
    }

  if ((subband_pyramid->num_rows != mask->num_rows) ||
      (subband_pyramid->num_cols != mask->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntShapeAdaptiveDWT): Subband pyramid and mask must have same number of rows and columns");
      goto Error;
    }

  if (QccWAVWaveletShapeAdaptiveDWT2DInt(subband_pyramid->matrix,
                                         mask->matrix,
                                         subband_pyramid->num_rows,
                                         subband_pyramid->num_cols,
                                         num_levels,
                                         wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntShapeAdaptiveDWT): Error calling QccWAVWaveletShapeAdaptiveDWT2DInt()");
      goto Error;
    }
  
  subband_pyramid->num_levels = num_levels;
  mask->num_levels = num_levels;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramidIntInverseShapeAdaptiveDWT(QccWAVSubbandPyramidInt
                                                   *subband_pyramid,
                                                   QccWAVSubbandPyramidInt
                                                   *mask,
                                                   const QccWAVWavelet *wavelet)
{
  int return_value;

  if (subband_pyramid == NULL)
    return(0);
  if (mask == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (subband_pyramid->num_levels <= 0)
    return(0);
  
  if ((subband_pyramid->num_rows != mask->num_rows) ||
      (subband_pyramid->num_cols != mask->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntInverseShapeAdaptiveDWT): Subband pyramid and mask must have same number of rows and columns");
      goto Error;
    }
  
  if (subband_pyramid->num_levels != mask->num_levels)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntInverseShapeAdaptiveDWT): Subband pyramid and mask must have same number of levels of decomposition");
      goto Error;
    }
  
  if (QccWAVWaveletInverseShapeAdaptiveDWT2DInt(subband_pyramid->matrix,
                                                mask->matrix,
                                                subband_pyramid->num_rows,
                                                subband_pyramid->num_cols,
                                                subband_pyramid->num_levels,
                                                wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidIntInverseShapeAdaptiveDWT): Error calling QccWAVWaveletInverseShapeAdaptiveDWT2DInt()");
      goto Error;
    }
  
  subband_pyramid->num_levels = 0;
  mask->num_levels = 0;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}
