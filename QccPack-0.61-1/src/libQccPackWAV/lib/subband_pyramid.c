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


int QccWAVSubbandPyramidInitialize(QccWAVSubbandPyramid *subband_pyramid)
{
  if (subband_pyramid == NULL)
    return(0);
  
  QccStringMakeNull(subband_pyramid->filename);
  QccStringCopy(subband_pyramid->magic_num, QCCWAVSUBBANDPYRAMID_MAGICNUM);
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


int QccWAVSubbandPyramidAlloc(QccWAVSubbandPyramid *subband_pyramid)
{
  int return_value;
  
  if (subband_pyramid == NULL)
    return(0);
  
  if (subband_pyramid->matrix != NULL)
    return(0);

  if ((subband_pyramid->matrix =
       QccMatrixAlloc(subband_pyramid->num_rows,
                      subband_pyramid->num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidAlloc): Error calling QccMatrixAlloc()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  QccWAVSubbandPyramidFree(subband_pyramid);
  return_value = 1;
 Return:
  return(return_value);
}


void QccWAVSubbandPyramidFree(QccWAVSubbandPyramid *subband_pyramid)
{
  if (subband_pyramid == NULL)
    return;
  
  QccMatrixFree(subband_pyramid->matrix,
                subband_pyramid->num_rows);
  subband_pyramid->matrix = NULL;

}


int QccWAVSubbandPyramidNumLevelsToNumSubbands(int num_levels)
{
  return(num_levels*3 + 1);
}


int QccWAVSubbandPyramidNumSubbandsToNumLevels(int num_subbands)
{
  if (num_subbands <= 0)
    return(0);
  
  if ((num_subbands - 1) % 3)
    return(0);
  
  return((num_subbands - 1) / 3);
}


int QccWAVSubbandPyramidCalcLevelFromSubband(int subband, int num_levels)
{
  if (!num_levels)
    return(0);
  
  if (!subband)
    return(num_levels);
  
  return(num_levels - (subband - 1)/3);
}


#if 0
static int QccWAVSubbandPyramidPattern(const QccWAVSubbandPyramid
                                       *subband_pyramid,
                                       int *subsample_pattern_row,
                                       int *subsample_pattern_col)
{
  int subsample_pattern_row1 = 0;
  int subsample_pattern_col1 = 0;
  int scale;
  int mask = 1;
  int mask_row = 1;
  int mask_col = 1;
  
  for (scale = 0; scale < subband_pyramid->num_levels; scale++)
    {
      if (subband_pyramid->subsample_pattern & mask)
        subsample_pattern_row1 |= mask_row;
      mask <<= 1;
      if (subband_pyramid->subsample_pattern & mask)
        subsample_pattern_col1 |= mask_col;
      mask <<= 1;
      
      mask_row <<= 1;
      mask_col <<= 1;
    }
  
  if (subsample_pattern_row != NULL)
    *subsample_pattern_row = subsample_pattern_row1;
  if (subsample_pattern_col != NULL)
    *subsample_pattern_col = subsample_pattern_col1;
  
  return(0);
}
#endif


int QccWAVSubbandPyramidSubbandSize(const QccWAVSubbandPyramid
                                    *subband_pyramid,
                                    int subband,
                                    int *subband_num_rows,
                                    int *subband_num_cols)
{
  int num_rows = 0;
  int num_cols = 0;
  int level;

  level = 
    QccWAVSubbandPyramidCalcLevelFromSubband(subband,
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


int QccWAVSubbandPyramidSubbandOffsets(const QccWAVSubbandPyramid 
                                       *subband_pyramid,
                                       int subband,
                                       int *row_offset,
                                       int *col_offset)
{
  int subband_num_rows;
  int subband_num_cols;
  QccWAVSubbandPyramid temp;

  if (subband < 0)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&temp);

  temp.num_levels =
    QccWAVSubbandPyramidCalcLevelFromSubband(subband,
                                             subband_pyramid->num_levels);
  temp.num_rows = subband_pyramid->num_rows;
  temp.num_cols = subband_pyramid->num_cols;
  temp.origin_row = subband_pyramid->origin_row;
  temp.origin_col = subband_pyramid->origin_col;
  temp.subsample_pattern_row = subband_pyramid->subsample_pattern_row;
  temp.subsample_pattern_col = subband_pyramid->subsample_pattern_col;

  if (QccWAVSubbandPyramidSubbandSize(&temp,
                                      0,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidSubbandOffsets): Error calling QccWAVSubbandPyramidSubbandSize()");
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


void QccWAVSubbandPyramidSubbandToQccString(int subband, int num_levels,
                                            QccString qccstring)
{
  int level;
  
  level = QccWAVSubbandPyramidCalcLevelFromSubband(subband, num_levels);
  
  if (!subband)
    QccStringSprintf(qccstring, "B%d", level);
  else
    switch ((subband - 1) % 3)
      {
      case 0:
        QccStringSprintf(qccstring, "H%d", level);
        break;
      case 1:
        QccStringSprintf(qccstring, "V%d", level);
        break;
      case 2:
        QccStringSprintf(qccstring, "D%d", level);
        break;
      }
}


int QccWAVSubbandPyramidPrint(const QccWAVSubbandPyramid *subband_pyramid)
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
          printf("% 10.4f ", subband_pyramid->matrix[row][col]);
        }
      printf("\n");
    }      
  return(0);
}


static int QccWAVSubbandPyramidReadHeader(FILE *infile, 
                                          QccWAVSubbandPyramid
                                          *subband_pyramid)
{
  
  if ((infile == NULL) || (subband_pyramid == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             subband_pyramid->magic_num,
                             &subband_pyramid->major_version,
                             &subband_pyramid->minor_version))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading magic number in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (strcmp(subband_pyramid->magic_num, QCCWAVSUBBANDPYRAMID_MAGICNUM))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): %s is not of subband-pyramid (%s) type",
                         subband_pyramid->filename, 
                         QCCWAVSUBBANDPYRAMID_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->num_levels));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading number of levels in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading number of levels in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->num_cols));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading number of columns in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading number of columns in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(subband_pyramid->num_rows));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading number of rows in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading number of rows in %s",
                         subband_pyramid->filename);
      return(1);
    }
  
  if (QccCompareQccPackVersions(subband_pyramid->major_version,
                                subband_pyramid->minor_version,
                                0, 12) < 0)
    {
      QccErrorWarning("(QccWAVSubbandPyramidReadHeader): converting %s from outdated version %d.%d to current version",
                      subband_pyramid->filename,
                      subband_pyramid->major_version,
                      subband_pyramid->minor_version);

      /*  Min value  */
      fscanf(infile, "%*f");
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading min value in %s",
                             subband_pyramid->filename);
          return(1);
        }
      
      if (QccFileSkipWhiteSpace(infile, 0))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading min value in %s",
                             subband_pyramid->filename);
          return(1);
        }
      
      /*  Max val  */
      fscanf(infile, "%*f");
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading max value in %s",
                             subband_pyramid->filename);
          return(1);
        }
    }
  
  fscanf(infile, "%*1[\n]");
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidReadHeader): Error reading subband pyramid %s",
                         subband_pyramid->filename);
      return(1);
    }

  return(0);
}


int QccWAVSubbandPyramidRead(QccWAVSubbandPyramid *subband_pyramid)
{
  FILE *infile = NULL;
  int row, col;
  
  if (subband_pyramid == NULL)
    return(0);
  
  if ((infile = QccFileOpen(subband_pyramid->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidRead): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidReadHeader(infile, subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidRead): Error calling QccWAVSubbandPyramidReadHeader()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidAlloc(subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidRead): Error calling QccIMGSubbandPyramidAlloc()");
      return(1);
    }
  
  for (row = 0; row < subband_pyramid->num_rows; row++)
    for (col = 0; col < subband_pyramid->num_cols; col++)
      if (QccFileReadDouble(infile,
                            &(subband_pyramid->matrix[row][col])))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidRead): Error calling QccFileReadDouble()",
                             subband_pyramid->filename);
          return(1);
        }
  
  QccFileClose(infile);
  return(0);
}


static int QccWAVSubbandPyramidWriteHeader(FILE *outfile, 
                                           const QccWAVSubbandPyramid
                                           *subband_pyramid)
{
  if ((outfile == NULL) || (subband_pyramid == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCWAVSUBBANDPYRAMID_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d\n%d %d\n",
          subband_pyramid->num_levels,
          subband_pyramid->num_cols,
          subband_pyramid->num_rows);
  if (ferror(outfile))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccWAVSubbandPyramidWriteHeader): Error writing header to %s",
                     subband_pyramid->filename);
  return(1);
}


int QccWAVSubbandPyramidWrite(const QccWAVSubbandPyramid *subband_pyramid)
{
  FILE *outfile;
  int row, col;
  
  if (subband_pyramid == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(subband_pyramid->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidWriteHeader(outfile, subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidWrite): Error calling QccWAVSubbandPyramidWriteHeader()");
      return(1);
    }
  
  for (row = 0; row < subband_pyramid->num_rows; row++)
    for (col = 0; col < subband_pyramid->num_cols; col++)
      if (QccFileWriteDouble(outfile,
                             subband_pyramid->matrix[row][col]))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidWrite): Error calling QccFileWriteDouble()");
          return(1);
        }
  
  QccFileClose(outfile);
  return(0);
}


int QccWAVSubbandPyramidCopy(QccWAVSubbandPyramid *subband_pyramid1,
                             const QccWAVSubbandPyramid *subband_pyramid2)
{
  if (subband_pyramid1 == NULL)
    return(0);
  if (subband_pyramid2 == NULL)
    return(0);


  if ((subband_pyramid1->num_rows != subband_pyramid2->num_rows) ||
      (subband_pyramid1->num_cols != subband_pyramid2->num_cols))
    {
      QccWAVSubbandPyramidFree(subband_pyramid1);
      subband_pyramid1->num_rows = subband_pyramid2->num_rows;
      subband_pyramid1->num_cols = subband_pyramid2->num_cols;
      if (QccWAVSubbandPyramidAlloc(subband_pyramid1))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidCopy): Error calling QccWAVSubbandPyramidAlloc()");
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

  if (QccMatrixCopy(subband_pyramid1->matrix,
                    subband_pyramid2->matrix,
                    subband_pyramid1->num_rows,
                    subband_pyramid1->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidCopy): Error calling QccMatrixCopy()");
      return(1);
    }

  return(0);
}


int QccWAVSubbandPyramidZeroSubband(QccWAVSubbandPyramid *subband_pyramid,
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
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);

  if (subband > num_subbands)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidZeroSubband): specified subband is large for given subband pyramid");
      return(1);
    }
  
  if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                      subband,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidZeroSubband): Error calling QccWAVSubbandPyramidSubbandSize()");
      return(1);
    }
  
  if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                         subband,
                                         &subband_row,
                                         &subband_col))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidZeroSubband): Error calling QccWAVSubbandPyramidSubbandOffsets()");
      return(1);
    }
  
  for (row = subband_row; row < subband_row + subband_num_rows; row++)
    for (col = subband_col; col < subband_col + subband_num_cols; col++)
      subband_pyramid->matrix[row][col] = 0;

  return(0);
}


int QccWAVSubbandPyramidSubtractMean(QccWAVSubbandPyramid *subband_pyramid,
                                     double *mean,
                                     const QccSQScalarQuantizer *quantizer)
{
  int subband_row, subband_col;
  int subband_num_rows, subband_num_cols;
  int partition;
  
  double mean1;
  
  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->matrix == NULL)
    return(0);

  if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                      0,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidSubtractMean): Error calling QccWAVSubbandPyramidSubbandSize()");
      return(1);
    }
  
  mean1 = QccMatrixMean(subband_pyramid->matrix,
                        subband_num_rows,
                        subband_num_cols);
  
  if (quantizer != NULL)
    {
      if (QccSQScalarQuantization(mean1, quantizer, NULL, &partition))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidSubtractMean): Error calling QccSQScalarQuantization()");
          return(1);
        }
      if (QccSQInverseScalarQuantization(partition, quantizer, &mean1))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidSubtractMean): Error calling QccSQInverseScalarQuantization()");
          return(1);
        }
    }
  
  for (subband_row = 0; subband_row < subband_num_rows; subband_row++)
    for (subband_col = 0; subband_col < subband_num_cols; subband_col++)
      subband_pyramid->matrix[subband_row][subband_col] -= mean1;
  
  if (mean != NULL)
    *mean = mean1;
  
  return(0);
}


int QccWAVSubbandPyramidAddMean(QccWAVSubbandPyramid *subband_pyramid,
                                double mean)
{
  int subband_row, subband_col;
  int subband_num_rows, subband_num_cols;

  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->matrix == NULL)
    return(0);
  if (subband_pyramid->num_levels != 0)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidAddMean): Subband-pyramid number of levels is not zero");
      return(1);
    }
  
  if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                      0,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidAddMean): Error calling QccWAVSubbandPyramidSubbandSize()");
      return(1);
    }

  for (subband_row = 0; subband_row < subband_num_rows; subband_row++)
    for (subband_col = 0; subband_col < subband_num_cols; subband_col++)
      subband_pyramid->matrix[subband_row][subband_col] += mean;
  
  return(0);
}


int QccWAVSubbandPyramidCalcCoefficientRange(const QccWAVSubbandPyramid
                                             *subband_pyramid,
                                             const QccWAVWavelet *wavelet,
                                             int level,
                                             double *max_value,
                                             double *min_value)
{
  double mean;
  double sum = 0;
  int index;
  double matrix_max;
  double matrix_min;
  
  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->matrix == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (wavelet->implementation !=
      QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidCalcCoefficientRange): Wavelet must be of filter-bank implementation");
      return(1);
    }
  
  mean = QccMatrixMean(subband_pyramid->matrix,
                       subband_pyramid->num_rows,
                       subband_pyramid->num_cols);
  
  for (index = 0;
       index < wavelet->filter_bank.lowpass_analysis_filter.length;
       index++)
    sum +=
      fabs(wavelet->filter_bank.lowpass_analysis_filter.coefficients[index]);
  
  matrix_max = QccMatrixMaxValue(subband_pyramid->matrix,
                                 subband_pyramid->num_rows,
                                 subband_pyramid->num_cols);
  matrix_min = QccMatrixMaxValue(subband_pyramid->matrix,
                                 subband_pyramid->num_rows,
                                 subband_pyramid->num_cols);
  
  if (max_value != NULL)
    *max_value =
      (matrix_max - mean) * pow(sum, (double)level);
  
  if (min_value != NULL)
    *min_value =
      (matrix_min - mean) * pow(sum, (double)level);
  
  return(0);
}


int QccWAVSubbandPyramidAddNoiseSquare(QccWAVSubbandPyramid *subband_pyramid,
                                       int subband,
                                       double start,
                                       double width,
                                       double noise_variance)
{
  int return_value;
  int subband_num_cols;
  int subband_num_rows;
  int subband_row_offset;
  int subband_col_offset;
  
  if (subband_pyramid == NULL)
    return(0);
  
  if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                      subband,
                                      &subband_num_rows,
                                      &subband_num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidAddNoiseSquare): Error calling QccWAVSubbandPyramidSubbandSize()");
      goto Error;
    }
  
  if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                         subband,
                                         &subband_row_offset,
                                         &subband_col_offset))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidAddNoiseSquare): Error calling QccWAVSubbandPyramidSubbandSize()");
      goto Error;
    }
  
  if (QccMatrixAddNoiseToRegion(subband_pyramid->matrix,
                                subband_pyramid->num_rows,
                                subband_pyramid->num_cols,
                                start * subband_num_rows + subband_row_offset,
                                start * subband_num_cols + subband_col_offset,
                                width * subband_num_rows,
                                width * subband_num_cols,
                                noise_variance))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidAddNoiseSquare): Error calling QccMatrixAddNoiseToRegion()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramidSplitToImageComponent(const QccWAVSubbandPyramid
                                              *subband_pyramid,
                                              QccIMGImageComponent
                                              *output_images)
{
  int num_subbands;
  int subband;
  int subband_num_rows, subband_num_cols;
  int subband_row_offset, subband_col_offset;
  int row, col;
  
  if ((subband_pyramid == NULL) ||
      (output_images == NULL))
    return(0);
  
  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidSplitToImageComponent): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      
      output_images[subband].num_rows = subband_num_rows;
      output_images[subband].num_cols = subband_num_cols;
      if (QccIMGImageComponentAlloc(&(output_images[subband])))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidSplitToImageComponent): Error calling QccIMGImageComponentAlloc()");
          return(1);
        }
      
      if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                             subband,
                                             &subband_row_offset,
                                             &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidSplitToImageComponent): Error calling QccWAVSubbandPyramidSubbandOffsets()");
          return(1);
        }
      
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          output_images[subband].image[row][col] =
            subband_pyramid->matrix[subband_row_offset + row]
            [subband_col_offset + col];
      
      QccIMGImageComponentSetMin(&output_images[subband]);
      QccIMGImageComponentSetMax(&output_images[subband]);
    }
  
  return(0);
}


int QccWAVSubbandPyramidSplitToDat(const QccWAVSubbandPyramid *subband_pyramid,
                                   QccDataset *output_datasets,
                                   const int *vector_dimensions)
{
  int num_subbands;
  int return_value;
  int subband;
  int tile_dimension;
  QccIMGImageComponent *subband_images = NULL;
  
  if (subband_pyramid == NULL)
    return(0);
  if (vector_dimensions == NULL)
    return(0);
  if (output_datasets == NULL)
    return(0);
  
  num_subbands = 
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);
  
  if ((subband_images = (QccIMGImageComponent *)
       malloc(sizeof(QccIMGImageComponent)*num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidSplitToDat): Error allocating memory");
      goto Error;
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    QccIMGImageComponentInitialize(&(subband_images[subband]));
  
  if (QccWAVSubbandPyramidSplitToImageComponent(subband_pyramid,
                                                subband_images))
    {
      QccErrorAddMessage("(QccWAVsubbandPyramidSplitToDat): Error calling QccWAVSubbandPyramidSplitToImageComponent()");
      goto Error;
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      tile_dimension = 
        sqrt((double)vector_dimensions[subband]);
      
      if (QccIMGImageComponentToDataset(&subband_images[subband],
                                        &output_datasets[subband], 
                                        tile_dimension, tile_dimension))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidSplitToDat): Error calling QccIMGImageComponentToDat()");
          goto Error;
        }
    }
  
  return_value = 0;
  goto Return;
  
 Error:
  return_value = 1;
  
 Return:
  if (subband_images != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccIMGImageComponentFree(&(subband_images[subband]));
      QccFree(subband_images);
    }
  return(return_value);
}


int QccWAVSubbandPyramidAssembleFromImageComponent(const QccIMGImageComponent
                                                   *input_images,
                                                   QccWAVSubbandPyramid
                                                   *subband_pyramid)
{
  int num_subbands;
  int subband;
  int subband_num_cols, subband_num_rows;
  int subband_col_offset, subband_row_offset;
  int row, col;
  
  if ((input_images == NULL) || (subband_pyramid == NULL))
    return(0);
  
  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidAssembleFromImageComponent): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      
      if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                             subband,
                                             &subband_row_offset,
                                             &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidAssembleFromImageComponent): Error calling QccWAVSubbandPyramidSubbandOffsets()");
          return(1);
        }
      
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          subband_pyramid->matrix[subband_row_offset + row]
            [subband_col_offset + col] =
            input_images[subband].image[row][col];
    }
  
  return(0);
}


int QccWAVSubbandPyramidAssembleFromDat(const QccDataset *input_datasets,
                                        QccWAVSubbandPyramid *subband_pyramid,
                                        const int *vector_dimensions)
{
  QccIMGImageComponent *subband_images;
  int subband;
  int num_subbands;
  int return_value;
  int tile_dimension;
  int subband_num_rows, subband_num_cols;
  
  if ((input_datasets == NULL) ||
      (subband_pyramid == NULL))
    return(0);
  
  num_subbands = 
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);
  
  if ((subband_images = (QccIMGImageComponent *)
       malloc(sizeof(QccIMGImageComponent)*num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidAssembleFromDat): Error allocating memory");
      goto Error;
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidAssembleFromDat): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      
      QccIMGImageComponentInitialize(&(subband_images[subband]));
      subband_images[subband].num_rows = subband_num_rows;
      subband_images[subband].num_cols = subband_num_cols;
      if (QccIMGImageComponentAlloc(&(subband_images[subband])))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidAssembleFromDat): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
      
      tile_dimension = 
        sqrt((double)vector_dimensions[subband]);
      if (QccIMGDatasetToImageComponent(&subband_images[subband],
                                        &input_datasets[subband], 
                                        tile_dimension, tile_dimension))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidAssembleFromDat): Error calling QccIMGDatToImageComponent()");
          goto Error;
        }
    }
  
  if (QccWAVSubbandPyramidAssembleFromImageComponent(subband_images,
                                                     subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidAssembleFromDat): Error calling QccWAVSubbandPyramidAssembleFromImageComponent()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (subband_images != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccIMGImageComponentFree(&(subband_images[subband]));
      QccFree(subband_images);
    }
  return(return_value);
}


int QccWAVSubbandPyramidRasterScan(const QccWAVSubbandPyramid *subband_pyramid,
                                   QccVector scanned_coefficients)
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
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);
  
  coefficient = 0;
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidRasterScan): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                             subband,
                                             &subband_row_offset,
                                             &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidRasterScan): Error calling QccWAVSubbandPyramidSubbandOffsets()");
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


int QccWAVSubbandPyramidInverseRasterScan(QccWAVSubbandPyramid
                                          *subband_pyramid,
                                          const QccVector
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
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);
  
  coefficient = 0;
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidInverseRasterScan): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                             subband,
                                             &subband_row_offset,
                                             &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidInverseRasterScan): Error calling QccWAVSubbandPyramidSubbandOffsets()");
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


int QccWAVSubbandPyramidDWT(QccWAVSubbandPyramid *subband_pyramid,
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
      QccErrorAddMessage("(QccWAVSubbandPyramidDWT): Subband pyramid is already decomposed");
      goto Error;
    }

  subband_pyramid->num_levels = num_levels;
  
  if (QccWAVWaveletDWT2D(subband_pyramid->matrix,
                         subband_pyramid->num_rows,
                         subband_pyramid->num_cols,
                         subband_pyramid->origin_row,
                         subband_pyramid->origin_col,
                         subband_pyramid->subsample_pattern_row,
                         subband_pyramid->subsample_pattern_col,
                         num_levels,
                         wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWT): Error calling QccWAVWaveletDWT2D()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVSubbandPyramidInverseDWT(QccWAVSubbandPyramid *subband_pyramid,
                                   const QccWAVWavelet *wavelet)
{
  int return_value;
  
  if (subband_pyramid == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  if (subband_pyramid->num_levels <= 0)
    return(0);

  if (QccWAVWaveletInverseDWT2D(subband_pyramid->matrix,
                                subband_pyramid->num_rows,
                                subband_pyramid->num_cols,
                                subband_pyramid->origin_row,
                                subband_pyramid->origin_col,
                                subband_pyramid->subsample_pattern_row,
                                subband_pyramid->subsample_pattern_col,
                                subband_pyramid->num_levels,
                                wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidInverseDWT): Error calling QccWAVWaveletDWT2D()");
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


int QccWAVSubbandPyramidShapeAdaptiveDWT(QccWAVSubbandPyramid *subband_pyramid,
                                         QccWAVSubbandPyramid *mask,
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
      QccErrorAddMessage("(QccWAVSubbandPyramidShapeAdaptiveDWT): Subband pyramid is already decomposed");
      goto Error;
    }

  if ((subband_pyramid->num_rows != mask->num_rows) ||
      (subband_pyramid->num_cols != mask->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidShapeAdaptiveDWT): Subband pyramid and mask must have same number of rows and columns");
      goto Error;
    }

  if (QccWAVWaveletShapeAdaptiveDWT2D(subband_pyramid->matrix,
                                      mask->matrix,
                                      subband_pyramid->num_rows,
                                      subband_pyramid->num_cols,
                                      num_levels,
                                      wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidShapeAdaptiveDWT): Error calling QccWAVWaveletShapeAdaptiveDWT2D()");
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


int QccWAVSubbandPyramidInverseShapeAdaptiveDWT(QccWAVSubbandPyramid
                                                *subband_pyramid,
                                                QccWAVSubbandPyramid *mask,
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
      QccErrorAddMessage("(QccWAVSubbandPyramidInverseShapeAdaptiveDWT): Subband pyramid and mask must have same number of rows and columns");
      goto Error;
    }
  
  if (subband_pyramid->num_levels != mask->num_levels)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidInverseShapeAdaptiveDWT): Subband pyramid and mask must have same number of levels of decomposition");
      goto Error;
    }
  
  if (QccWAVWaveletInverseShapeAdaptiveDWT2D(subband_pyramid->matrix,
                                             mask->matrix,
                                             subband_pyramid->num_rows,
                                             subband_pyramid->num_cols,
                                             subband_pyramid->num_levels,
                                             wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidInverseShapeAdaptiveDWT): Error calling QccWAVWaveletInverseShapeAdaptiveDWT2D()");
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


int QccWAVSubbandPyramidRedundantDWTSubsample(const QccMatrix *input_matrices,
                                              QccWAVSubbandPyramid
                                              *subband_pyramid,
                                              const QccWAVWavelet *wavelet)
{
  if (input_matrices == NULL)
    return(0);
  if (subband_pyramid == NULL)
    return(0);
  if (subband_pyramid->matrix == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidRedundantDWTSubsample): Subband pyramid is not allocated");
      return(1);
    }
  if ((subband_pyramid->num_rows <= 0) ||
      (subband_pyramid->num_cols <= 0) ||
      (subband_pyramid->num_levels < 0))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidRedundantDWTSubsample): Subband pyramid is not setup properly");
      return(1);
    }

  if (QccWAVWaveletRedundantDWT2DSubsample(input_matrices,
                                           subband_pyramid->matrix,
                                           subband_pyramid->num_rows,
                                           subband_pyramid->num_cols,
                                           subband_pyramid->num_levels,
                                           subband_pyramid->subsample_pattern_row,
                                           subband_pyramid->subsample_pattern_col,
                                           wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidRedundantDWTSubsample): Error calling QccWAVWaveletRedundantDWT2DSubsample()");
      return(1);
    }

  return(0);
}
