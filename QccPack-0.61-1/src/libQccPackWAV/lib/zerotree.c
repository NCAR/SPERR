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


int QccWAVZerotreeInitialize(QccWAVZerotree *zerotree)
{
  if (zerotree == NULL)
    return(0);

  QccStringMakeNull(zerotree->filename);
  QccStringCopy(zerotree->magic_num, QCCWAVZEROTREE_MAGICNUM);
  QccGetQccPackVersion(&zerotree->major_version,
                       &zerotree->minor_version,
                       NULL);
  zerotree->image_num_cols = 0;
  zerotree->image_num_rows = 0;
  zerotree->image_mean = 0.0;
  zerotree->num_levels = 0;
  zerotree->num_subbands = 0;
  zerotree->alphabet_size = QCCWAVZEROTREE_NUMSYMBOLS;
  zerotree->num_cols = NULL;
  zerotree->num_rows = NULL;
  zerotree->zerotree = NULL;

  return(0);
}


int QccWAVZerotreeCalcSizes(const QccWAVZerotree *zerotree)
{
  int subband;
  QccWAVSubbandPyramid subband_pyramid;
  
  if (zerotree == NULL)
    return(0);
  if ((zerotree->num_cols == NULL) ||
      (zerotree->num_rows == NULL))
    return(0);
  
  subband_pyramid.num_levels = zerotree->num_levels;
  subband_pyramid.num_rows = zerotree->image_num_rows;
  subband_pyramid.num_cols = zerotree->image_num_cols;

  for (subband = 0; subband < zerotree->num_subbands; subband++)
    if (QccWAVSubbandPyramidSubbandSize(&subband_pyramid,
                                        subband,
                                        &zerotree->num_rows[subband],
                                        &zerotree->num_cols[subband]))
      {
        QccErrorAddMessage("(QccWAVZerotreeCalcSizes): Error calling QccWAVSubbandPyramidSubbandSize()");
        return(1);
      }
  
  return(0);
}


int QccWAVZerotreeAlloc(QccWAVZerotree *zerotree)
{
  int return_value;
  int subband, row;

  if (zerotree == NULL)
    return(0);

  if ((zerotree->num_cols == NULL) || (zerotree->num_rows == NULL))
    {
      if (zerotree->num_cols == NULL)
        if ((zerotree->num_cols =
             (int *)malloc(sizeof(int)*zerotree->num_subbands)) == NULL)
          {
            QccErrorAddMessage("(QccWAVZerotreeAlloc): Error allocating memory");
            goto Error;
          }
      if (zerotree->num_rows == NULL)
        if ((zerotree->num_rows =
             (int *)malloc(sizeof(int)*zerotree->num_subbands)) == NULL)
          {
            QccErrorAddMessage("(QccWAVZerotreeAlloc): Error allocating memory");
            goto Error;
          }
      
      if (QccWAVZerotreeCalcSizes(zerotree))
        {
          QccErrorAddMessage("(QccWAVZerotreeAlloc): Error calling QccVZTZerotreeCalcSizes()");
          goto Error;
        }
    }
  
  if (zerotree->zerotree == NULL)
    {
      if ((zerotree->zerotree = 
           (char ***)malloc(sizeof(char **) * 
                            zerotree->num_subbands)) == NULL)
        {
          QccErrorAddMessage("(QccWAVZerotreeAlloc): Error allocating memory");
          goto Error;
        }
      
      for (subband = 0; subband < zerotree->num_subbands; subband++)
        {
          if ((zerotree->zerotree[subband] = 
               (char **)malloc(sizeof(char *) * 
                               zerotree->num_rows[subband])) == NULL)
            {
              QccErrorAddMessage("(QccWAVZerotreeAlloc): Error allocating memory");
              goto Error;
            }
      
          for (row = 0; row < zerotree->num_rows[subband]; row++)
            if ((zerotree->zerotree[subband][row] = 
                 (char *)calloc(zerotree->num_cols[subband], 
                                sizeof(char))) == NULL)
              {
                QccErrorAddMessage("(QccWAVZerotreeAlloc): Error allocating memory");
                goto Error;
              }
        }
    }
  
  return_value = 0;
  goto Return;
 Error:
  QccWAVZerotreeFree(zerotree);
  return_value = 1;
 Return:
  return(return_value);
}


void QccWAVZerotreeFree(QccWAVZerotree *zerotree)
{
  int subband, row;

  if (zerotree == NULL)
    return;

  if ((zerotree->zerotree != NULL) && (zerotree->num_rows != NULL))
    {
      for (subband = 0; subband < zerotree->num_subbands; subband++)
        if (zerotree->zerotree[subband] != NULL)
          {
            for (row = 0; row < zerotree->num_rows[subband]; row++)
              if (zerotree->zerotree[subband][row] != NULL)
                QccFree(zerotree->zerotree[subband][row]);
            QccFree(zerotree->zerotree[subband]);
          }
      QccFree(zerotree->zerotree);
      zerotree->zerotree = NULL;
    }

  if (zerotree->num_rows != NULL)
    {
      QccFree(zerotree->num_rows);
      zerotree->num_rows = NULL;
    }
  if (zerotree->num_cols != NULL)
    {
      QccFree(zerotree->num_cols);
      zerotree->num_cols = NULL;
    }
}


int QccWAVZerotreePrint(const QccWAVZerotree *zerotree)
{
  int subband;
  int row, col;
  QccString subband_string;
  int level;
  int space;
  
  if (QccFilePrintFileInfo(zerotree->filename,
                           zerotree->magic_num,
                           zerotree->major_version,
                           zerotree->minor_version))
    return(1);

  printf("Image size: %dx%d\n",
         zerotree->image_num_cols,
         zerotree->image_num_rows);
  printf("Image mean: %10.2f\n", zerotree->image_mean);
  printf("Num. of octave-band decomposition levels: %d\n",
         zerotree->num_levels);
  printf("Num. of subbands: %d\n",
         zerotree->num_subbands);
  printf("Alphabet size: %d\n\n\n", zerotree->alphabet_size);
  
  for (subband = 0; subband < zerotree->num_subbands; subband++)
    {
      level = 
        QccWAVSubbandPyramidCalcLevelFromSubband(subband,
                                                 zerotree->num_levels);
      space = 1 << (zerotree->num_levels - level);
      QccWAVSubbandPyramidSubbandToQccString(subband, zerotree->num_levels, 
                                             subband_string);
      printf("---Subband #%d (%s)---------\n", subband, subband_string);
      printf("  Subband size: %dx%d\n", 
             zerotree->num_cols[subband],
             zerotree->num_rows[subband]);
      printf("  Symbol array:\n");
      
      for (row = 0; row < zerotree->num_rows[subband]; row++)
        {
          if (!(row % space))
            {
              for (col = 0; col < zerotree->num_cols[subband]; col++)
                {
                  printf("-");
                  if (!(col % space))
                    printf("-");
                }              
              printf("\n");
            }
          for (col = 0; col < zerotree->num_cols[subband]; col++)
            {
              if (!(col % space))
                printf("|");
              if (QccWAVZerotreeNullSymbol(zerotree->zerotree
                                           [subband][row][col]))
                printf("%c", '.');
              else
                switch (zerotree->zerotree[subband][row][col])
                  {
                  case QCCWAVZEROTREE_SYMBOLSIGNIFICANT:
                    printf("%c", 'S');
                    break;
                  case QCCWAVZEROTREE_SYMBOLINSIGNIFICANT:
                    printf("%c", 'I');
                    break;
                  case QCCWAVZEROTREE_SYMBOLZTROOT:
                    printf("%c", 'Z');
                    break;
                  }
            }
          
          printf("\n");
        }      
      printf("\n");
    }
  
  return(0);
}


int QccWAVZerotreeNullSymbol(const char symbol)
{
  return((int)symbol < 0);
}


int QccWAVZerotreeMakeSymbolNull(char *symbol)
{
  if (symbol == NULL)
    return(0);
  
  *symbol = (char)(-abs((int)*symbol));
  return(0);
}


int QccWAVZerotreeMakeSymbolNonnull(char *symbol)
{
  if (symbol == NULL)
    return(0);
  
  *symbol = (char)abs((int)*symbol);
  return(0);
}


int QccWAVZerotreeMakeFullTree(QccWAVZerotree *zerotree)
{
  int subband, row, col;

  if (zerotree == NULL)
    return(0);
  
  if (QccWAVZerotreeAlloc(zerotree))
    {
      QccErrorAddMessage("(QccWAVZerotreeMakeFullTree): Error calling QccWAVZerotreeAlloc()");
      return(1);
    }

  for (subband = 0; subband < zerotree->num_subbands; subband++)
    for (row = 0; row < zerotree->num_rows[subband]; row++)
      for (col = 0; col < zerotree->num_cols[subband]; col++)
        zerotree->zerotree[subband][row][col] =
          QCCWAVZEROTREE_SYMBOLSIGNIFICANT;
  
  return(0);
}


int QccWAVZerotreeMakeEmptyTree(QccWAVZerotree *zerotree)
{
  int subband, row, col;

  if (zerotree == NULL)
    return(0);
  
  if (zerotree->zerotree == NULL)
    {
      if (QccWAVZerotreeAlloc(zerotree))
        {
          QccErrorAddMessage("(QccWAVZerotreeMakeFullTree): Error calling QccWAVZerotreeAlloc()");
          return(1);
        }
      if (QccWAVZerotreeMakeFullTree(zerotree))
        {
          QccErrorAddMessage("(QccWAVZerotreeMakeFullTree): Error calling QccWAVZerotreeMakeFullTree()");
          return(1);
        }
    }

  for (subband = 0; subband < zerotree->num_subbands; subband++)
    for (row = 0; row < zerotree->num_rows[subband]; row++)
      for (col = 0; col < zerotree->num_cols[subband]; col++)
        zerotree->zerotree[subband][row][col] =
          QccWAVZerotreeMakeSymbolNull(&(zerotree->zerotree
                                         [subband][row][col]));

  
  return(0);
}


static int QccWAVZerotreeReadHeader(FILE *infile, QccWAVZerotree *zerotree)
{
  if ((infile == NULL) || (zerotree == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             zerotree->magic_num,
                             &zerotree->major_version,
                             &zerotree->minor_version))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading magic number in %s",
                         zerotree->filename);
      return(1);
    }
  
  if (strcmp(zerotree->magic_num, QCCWAVZEROTREE_MAGICNUM))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): %s is not of zerotree (%s) type",
                         zerotree->filename, QCCWAVZEROTREE_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(zerotree->image_num_cols));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading number of image columns in %s",
                         zerotree->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading number of image columns in %s",
                         zerotree->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(zerotree->image_num_rows));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading number of image rows in %s",
                         zerotree->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading number of image rows in %s",
                         zerotree->filename);
      return(1);
    }
  
  fscanf(infile, "%lf", &(zerotree->image_mean));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading image mean in %s",
                         zerotree->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading image mean in %s",
                         zerotree->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(zerotree->num_levels));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading number of levels in %s",
                         zerotree->filename);
      return(1);
    }
  
  zerotree->num_subbands = 
    QccWAVSubbandPyramidNumLevelsToNumSubbands(zerotree->num_levels);
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading number of subbands in %s",
                         zerotree->filename);
      return(1);
    }
  
  fscanf(infile, "%d%*1[\n]", &(zerotree->alphabet_size));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadHeader): Error reading alphabet size in %s",
                         zerotree->filename);
      return(1);
    }
  
  return(0);
  
}

static int QccWAVZerotreeReadData(FILE *infile, QccWAVZerotree *zerotree)
{
  int subband;
  int row, col;
  
  if (zerotree == NULL)
    return(0);
  
  if (QccWAVZerotreeAlloc(zerotree))
    {
      QccErrorAddMessage("(QccWAVZerotreeReadData): Error calling QccWAVZerotreeAlloc()");
      goto QccWAVZerotreeReadDataErr;
    }
  
  for (subband = 0; subband < zerotree->num_subbands; subband++)
    for (row = 0; row < zerotree->num_rows[subband]; row++)
      for (col = 0; col < zerotree->num_cols[subband]; col++)
        if (QccFileReadChar(infile,
                            &(zerotree->zerotree[subband][row][col])))
          {
            QccErrorAddMessage("(QccWAVZerotreeReadData): Error calling QccFileReadChar()");
            goto QccWAVZerotreeReadDataErr;
          }
  
  return(0);
  
 QccWAVZerotreeReadDataErr:
  QccWAVZerotreeFree(zerotree);
  return(1);
}


int QccWAVZerotreeRead(QccWAVZerotree *zerotree)
{
  FILE *infile = NULL;
  
  if (zerotree == NULL)
    return(0);
  
  if ((infile = QccFileOpen(zerotree->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccWAVZerotreeRead): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVZerotreeReadHeader(infile, zerotree))
    {
      QccErrorAddMessage("(QccWAVZerotreeRead): Error calling QccWAVZerotreeReadHeader()");
      return(1);
    }
  
  if (QccWAVZerotreeReadData(infile, zerotree))
    {
      QccErrorAddMessage("(QccWAVZerotreeRead): Error calling QccWAVZerotreeReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


static int QccWAVZerotreeWriteHeader(FILE *outfile, 
                                     const QccWAVZerotree *zerotree)
{
  if ((outfile == NULL) || (zerotree == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCWAVZEROTREE_MAGICNUM))
    goto QccErr;
  
  fprintf(outfile, "%d %d\n% 16.9e\n%d\n%d\n",
          zerotree->image_num_cols,
          zerotree->image_num_rows,
          zerotree->image_mean,
          zerotree->num_levels,
          QCCWAVZEROTREE_NUMSYMBOLS);

  if (ferror(outfile))
    goto QccErr;

  return(0);

 QccErr:
  QccErrorAddMessage("(QccWAVZerotreeWriteHeader): Error writing header to %s",
                     zerotree->filename);
  return(1);
}


static int QccWAVZerotreeWriteData(FILE *outfile,
                                   const QccWAVZerotree *zerotree)
{
  int subband;
  int row, col;
  
  for (subband = 0; subband < zerotree->num_subbands; subband++)
    for (row = 0; row < zerotree->num_rows[subband]; row++)
      for (col = 0; col < zerotree->num_cols[subband]; col++)
        if (QccFileWriteChar(outfile,
                             zerotree->zerotree[subband][row][col]))
          {
            QccErrorAddMessage("(QccWAVZerotreeWrite): Error calling QccFileWriteChar()");
            return(1);
          }
  
  return(0);
}


int QccWAVZerotreeWrite(const QccWAVZerotree *zerotree)
{
  FILE *outfile;
  
  if (zerotree == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(zerotree->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccWAVZerotreeWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVZerotreeWriteHeader(outfile, zerotree))
    {
      QccErrorAddMessage("(QccWAVZerotreeWrite): Error calling QccWAVZerotreeWriteHeader()");
      return(1);
    }
  
  if (QccWAVZerotreeWriteData(outfile, zerotree))
    {
      QccErrorAddMessage("(QccWAVZerotreeWrite): Error calling QccWAVZerotreeWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


int QccWAVZerotreeCarveOutZerotree(QccWAVZerotree *zerotree,
                                   int subband, int row, int col)
{
  int child_row, child_col;
  int num_children;
  int num_subbands;
  int child_subband;
  int child_subband_start, child_subband_end;
  
  num_subbands = zerotree->num_subbands;
  
  if (subband >= num_subbands)
    return(0);
  
  if (subband)
    child_subband_start = child_subband_end =
      subband + 3;
  else
    {
      child_subband_start = 1;
      child_subband_end = 3;
    }
  
  for (child_subband = child_subband_start;
       child_subband <= child_subband_end;
       child_subband++)
    if (child_subband < num_subbands)
      {
        num_children = (child_subband < 4) ? 1 : 2;
        
        for (child_row = num_children*row;
             child_row < num_children*(row + 1);
             child_row++)
          for (child_col = num_children*col;
               child_col < num_children*(col + 1);
               child_col++)
            {        
              QccWAVZerotreeCarveOutZerotree(zerotree,
                                             child_subband,
                                             child_row,
                                             child_col);
              QccWAVZerotreeMakeSymbolNull(&(zerotree->zerotree
                                             [child_subband]
                                             [child_row][child_col]));
            }
      }
  
  return(0);
}


int QccWAVZerotreeUndoZerotree(QccWAVZerotree *zerotree,
                               int subband, int row, int col)
{
  int child_row, child_col;
  int num_children;
  int num_subbands;
  int child_subband;
  int child_subband_start, child_subband_end;
  
  num_subbands = zerotree->num_subbands;
  
  if (subband >= num_subbands)
    return(0);
  
  if (subband)
    child_subband_start = child_subband_end =
      subband + 3;
  else
    {
      child_subband_start = 1;
      child_subband_end = 3;
    }
  
  for (child_subband = child_subband_start;
       child_subband <= child_subband_end;
       child_subband++)
    if (child_subband < num_subbands)
      {
        num_children = (child_subband < 4) ? 1 : 2;
        
        for (child_row = num_children*row;
             child_row < num_children*(row + 1);
             child_row++)
          for (child_col = num_children*col;
               child_col < num_children*(col + 1);
               child_col++)
            {        
              QccWAVZerotreeMakeSymbolNonnull(&(zerotree->zerotree
                                                [child_subband]
                                                [child_row][child_col]));
              QccWAVZerotreeUndoZerotree(zerotree,
                                         child_subband,
                                         child_row,
                                         child_col);
            }
      }
  
  return(0);
}
