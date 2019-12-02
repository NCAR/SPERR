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


int QccWAVPerceptualWeightsInitialize(QccWAVPerceptualWeights
                                      *perceptual_weights)
{
  if (perceptual_weights == NULL)
    return(0);

  QccStringMakeNull(perceptual_weights->filename);
  QccStringCopy(perceptual_weights->magic_num, QCCWAVPERCEPTUALWEIGHTS_MAGICNUM);
  QccGetQccPackVersion(&perceptual_weights->major_version,
                       &perceptual_weights->minor_version,
                       NULL);

  perceptual_weights->num_subbands = 0;
  perceptual_weights->perceptual_weights = NULL;
  
  return(0);
}


int QccWAVPerceptualWeightsAlloc(QccWAVPerceptualWeights *perceptual_weights)
{
  int return_value;

  if (perceptual_weights == NULL)
    return(0);

  if ((perceptual_weights->perceptual_weights == NULL) &&
      (perceptual_weights->num_subbands > 0))
    if ((perceptual_weights->perceptual_weights =
         QccVectorAlloc(perceptual_weights->num_subbands)) == NULL)
      {
        QccErrorAddMessage("(QccWAVPerceptualWeightsAlloc): Error calling QccVectorAlloc()");
        goto Error;
      }

  return_value = 0;
  goto Return;
 Error:
  QccWAVPerceptualWeightsFree(perceptual_weights);
  return_value = 1;
 Return:
  return(return_value);
}


void QccWAVPerceptualWeightsFree(QccWAVPerceptualWeights *perceptual_weights)
{
  if (perceptual_weights == NULL)
    return;

  QccVectorFree(perceptual_weights->perceptual_weights);
  perceptual_weights->perceptual_weights = NULL;
  perceptual_weights->num_subbands = 0;

}


static int QccWAVPerceptualWeightsReadHeader(FILE *infile, 
                                             QccWAVPerceptualWeights 
                                             *perceptual_weights)
{
  
  if ((infile == NULL) || (perceptual_weights == NULL))
    return(0);

  if (QccFileReadMagicNumber(infile,
                             perceptual_weights->magic_num,
                             &perceptual_weights->major_version,
                             &perceptual_weights->minor_version))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsReadHeader): Error reading magic number in perceptual_weights %s",
                         perceptual_weights->filename);
      return(1);
    }

  if (strcmp(perceptual_weights->magic_num,
             QCCWAVPERCEPTUALWEIGHTS_MAGICNUM))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsReadHeader): %s is not of perceptual_weights (%s) type",
                         perceptual_weights->filename,
                         QCCWAVPERCEPTUALWEIGHTS_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(perceptual_weights->num_subbands));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsReadHeader): Error reading num_subbands in perceptual_weights %s",
                         perceptual_weights->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsReadHeader): Error reading num_subbands in perceptual_weights %s",
                         perceptual_weights->filename);
      return(1);
    }

  return(0);
}


static int QccWAVPerceptualWeightsReadData(FILE *infile, 
                                           QccWAVPerceptualWeights
                                           *perceptual_weights)
{
  int component;

  if ((infile == NULL) || (perceptual_weights == NULL))
    return(0);

  if (perceptual_weights->perceptual_weights != NULL)
    for (component = 0; component < perceptual_weights->num_subbands; 
         component++)
      {
        fscanf(infile, "%lf",
               &(perceptual_weights->perceptual_weights[component]));
        if (ferror(infile) || feof(infile))
          {
            QccErrorAddMessage("(QccWAVPerceptualWeightsReadData): Error reading data from %s",
                               perceptual_weights->filename);
            return(1);
          }
      }

  return(0);
}


int QccWAVPerceptualWeightsRead(QccWAVPerceptualWeights *perceptual_weights)
{
  FILE *infile = NULL;

  if (perceptual_weights == NULL)
    return(0);

  if ((infile = 
       QccFilePathSearchOpenRead(perceptual_weights->filename,
                                 QCCWAVWAVELET_PATH_ENV,
                                 QCCMAKESTRING(QCCPACK_WAVELET_PATH_DEFAULT)))
      == NULL)
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsRead): Error calling QccFilePathSearchOpenRead()");
      return(1);
    }

  if (QccWAVPerceptualWeightsReadHeader(infile, perceptual_weights))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsRead): Error calling QccWAVPerceptualWeightsReadHeader()");
      return(1);
    }

  if (QccWAVPerceptualWeightsAlloc(perceptual_weights))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsRead): Error calling QccWAVPerceptualWeightsAlloc()");
      return(1);
    }

  if (QccWAVPerceptualWeightsReadData(infile, perceptual_weights))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsRead): Error calling QccWAVPerceptualWeightsReadData()");
      return(1);
    }

  QccFileClose(infile);
  return(0);
}


static int QccWAVPerceptualWeightsWriteHeader(FILE *outfile, 
                                              const QccWAVPerceptualWeights
                                              *perceptual_weights)
{
  if ((outfile == NULL) || (perceptual_weights == NULL))
    return(0);

  if (QccFileWriteMagicNumber(outfile, QCCWAVPERCEPTUALWEIGHTS_MAGICNUM))
    goto QccErr;

  fprintf(outfile, "%d\n",
          perceptual_weights->num_subbands);

  if (ferror(outfile))
    goto QccErr;

  return(0);

 QccErr:
  QccErrorAddMessage("(QccWAVPerceptualWeightsWriteHeader): Error writing header to %s",
                     perceptual_weights->filename);
  return(1);

}


static int QccWAVPerceptualWeightsWriteData(FILE *outfile,
                                            const QccWAVPerceptualWeights
                                            *perceptual_weights)
{
  int component;

  if ((outfile == NULL) || (perceptual_weights == NULL))
    return(0);

  for (component = 0; component < perceptual_weights->num_subbands; 
       component++)
    fprintf(outfile, "% 16.9e ",
            perceptual_weights->perceptual_weights[component]);

  fprintf(outfile, "\n");

  if (ferror(outfile))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsWriteData): Error writing data to %s",
                         perceptual_weights->filename);
      return(1);
    }

  return(0);
}


int QccWAVPerceptualWeightsWrite(const QccWAVPerceptualWeights
                                 *perceptual_weights)
{
  FILE *outfile;

  if (perceptual_weights == NULL)
    return(0);

  if ((outfile = QccFileOpen(perceptual_weights->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsWrite): Error calling QccFileOpen()");
      return(1);
    }

  if (QccWAVPerceptualWeightsWriteHeader(outfile, perceptual_weights))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsWrite): Error calling QccWAVPerceptualWeightsWriteHeader()");
      return(1);
    }

  if (QccWAVPerceptualWeightsWriteData(outfile, perceptual_weights))
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsWrite): Error calling QccWAVPerceptualWeightsWriteData()");
      return(1);
    }

  QccFileClose(outfile);
  return(0);
}


int QccWAVPerceptualWeightsPrint(const QccWAVPerceptualWeights
                                 *perceptual_weights)
{
  int component;
  
  if (perceptual_weights == NULL)
    return(0);

  if (QccFilePrintFileInfo(perceptual_weights->filename,
                           perceptual_weights->magic_num,
                           perceptual_weights->major_version,
                           perceptual_weights->minor_version))
    return(1);
  
  printf("Perceptual Weights:\n");
  for (component = 0; component < perceptual_weights->num_subbands; 
       component++)
    {
      printf("% 16.9f\n", 
             perceptual_weights->perceptual_weights[component]);
    }

  return(0);
}


int QccWAVPerceptualWeightsApply(QccWAVSubbandPyramid *subband_pyramid,
                                 const QccWAVPerceptualWeights 
                                 *perceptual_weights)
{
  int subband;
  int num_subbands;
  int subband_num_rows, subband_num_cols;
  int subband_row_offset, subband_col_offset;
  int row, col;
  
  if ((subband_pyramid == NULL) ||
      (perceptual_weights == NULL))
    return(0);
  
  if (subband_pyramid->matrix == NULL)
    return(0);
  if (perceptual_weights->perceptual_weights == NULL)
    return(0);
  
  num_subbands = 
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);
  if (num_subbands != perceptual_weights->num_subbands)
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsApply): subband_pyramid and pereptual_weights have different numbers of subbands");
      return(1);
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVPerceptualWeightsApply): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      
      if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                             subband,
                                             &subband_row_offset,
                                             &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVPerceptualWeightsApply): Error calling QccWAVSubbandPyramidSubbandOffsets()");
          return(1);
        }
      
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          subband_pyramid->matrix[subband_row_offset + row]
            [subband_col_offset + col] *= 
            perceptual_weights->perceptual_weights[subband];
    }
  
  return(0);
}


int QccWAVPerceptualWeightsRemove(QccWAVSubbandPyramid 
                                  *subband_pyramid,
                                  const QccWAVPerceptualWeights 
                                  *perceptual_weights)
{
  int subband;
  int num_subbands;
  int subband_num_rows, subband_num_cols;
  int subband_row_offset, subband_col_offset;
  int row, col;
  
  if ((subband_pyramid == NULL) ||
      (perceptual_weights == NULL))
    return(0);
  
  if (subband_pyramid->matrix == NULL)
    return(0);
  if (perceptual_weights->perceptual_weights == NULL)
    return(0);
  
  num_subbands = 
    QccWAVSubbandPyramidNumLevelsToNumSubbands(subband_pyramid->num_levels);
  if (num_subbands != perceptual_weights->num_subbands)
    {
      QccErrorAddMessage("(QccWAVPerceptualWeightsRemove): subband_pyramid and pereptual_weights have different numbers of subbands");
      return(1);
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (QccWAVSubbandPyramidSubbandSize(subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVPerceptualWeightsRemove): Error calling QccWAVSubbandPyramidSubbandSize()");
          return(1);
        }
      
      if (QccWAVSubbandPyramidSubbandOffsets(subband_pyramid,
                                             subband,
                                             &subband_row_offset,
                                             &subband_col_offset))
        {
          QccErrorAddMessage("(QccWAVPerceptualWeightsRemove): Error calling QccWAVSubbandPyramidSubbandOffsets()");
          return(1);
        }
      
      for (row = 0; row < subband_num_rows; row++)
        for (col = 0; col < subband_num_cols; col++)
          subband_pyramid->matrix[subband_row_offset + row]
            [subband_col_offset + col] /= 
            perceptual_weights->perceptual_weights[subband];
    }
  
  return(0);
}

