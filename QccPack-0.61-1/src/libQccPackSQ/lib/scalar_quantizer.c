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


int QccSQScalarQuantizerInitialize(QccSQScalarQuantizer *scalar_quantizer)
{
  if (scalar_quantizer == NULL)
    return(0);

  QccStringMakeNull(scalar_quantizer->filename);
  QccStringCopy(scalar_quantizer->magic_num, QCCSQSCALARQUANTIZER_MAGICNUM);
  QccGetQccPackVersion(&scalar_quantizer->major_version,
                       &scalar_quantizer->minor_version,
                       NULL);
  scalar_quantizer->type = QCCSQSCALARQUANTIZER_GENERAL;
  scalar_quantizer->num_levels = 0;
  scalar_quantizer->levels = NULL;
  scalar_quantizer->probs = NULL;
  scalar_quantizer->boundaries = NULL;
  scalar_quantizer->stepsize = 0.0;
  scalar_quantizer->deadzone = 0.0;

  return(0);
}


int QccSQScalarQuantizerAlloc(QccSQScalarQuantizer *scalar_quantizer)
{
  if (scalar_quantizer == NULL)
    return(0);

  if (scalar_quantizer->num_levels < 1)
    return(0);

  if (scalar_quantizer->levels == NULL)
    if ((scalar_quantizer->levels =
         QccVectorAlloc(scalar_quantizer->num_levels)) == NULL)
      {
        QccErrorAddMessage("(QccSQScalarQuantizerAlloc): Error calling QccVectorAlloc()");
        return(1);
      }
  
  if (scalar_quantizer->probs == NULL)
    if ((scalar_quantizer->probs =
         QccVectorAlloc(scalar_quantizer->num_levels)) == NULL)
      {
        QccErrorAddMessage("(QccSQScalarQuantizerAlloc): Error calling QccVectorAlloc()");
        return(1);
      }
  
  if (scalar_quantizer->num_levels > 1)
    if (scalar_quantizer->boundaries == NULL)
      if ((scalar_quantizer->boundaries =
           QccVectorAlloc(scalar_quantizer->num_levels - 1)) == NULL)
        {
          QccErrorAddMessage("(QccSQScalarQuantizerAlloc): Error calling QccVectorAlloc()");
          return(1);
        }

  return(0);
}


void QccSQScalarQuantizerFree(QccSQScalarQuantizer *scalar_quantizer)
{
  if (scalar_quantizer == NULL)
    return;

  if (scalar_quantizer->type == QCCSQSCALARQUANTIZER_GENERAL)
    {
      QccVectorFree(scalar_quantizer->levels);
      QccVectorFree(scalar_quantizer->probs);
      QccVectorFree(scalar_quantizer->boundaries);
      scalar_quantizer->levels = NULL;
      scalar_quantizer->probs = NULL;
      scalar_quantizer->boundaries = NULL;
    }
}


int QccSQScalarQuantizerSetProbsFromPartitions(const QccSQScalarQuantizer 
                                               *scalar_quantizer,
                                               const int *partition, 
                                               int num_partitions)
{
  if ((scalar_quantizer == NULL) ||
      (partition == NULL) ||
      (num_partitions <= 0))
    return(0);
  
  if (scalar_quantizer->type != QCCSQSCALARQUANTIZER_GENERAL)
    return(0);
  
  if (scalar_quantizer->probs == NULL)
    return(0);
  
  return(QccVectorGetSymbolProbs(partition, num_partitions,
                                 scalar_quantizer->probs,
                                 scalar_quantizer->num_levels));
  
}


int QccSQScalarQuantizerReadHeader(FILE *infile, 
                                   QccSQScalarQuantizer *scalar_quantizer)
{
  if ((infile == NULL) || (scalar_quantizer == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             scalar_quantizer->magic_num,
                             &scalar_quantizer->major_version,
                             &scalar_quantizer->minor_version))
    {
      QccErrorAddMessage("(QccSQScalarQuantizerReadHeader): Error reading magic number in scalar quantizer %s",
                         scalar_quantizer->filename);
      return(1);
    }
  
  if (strcmp(scalar_quantizer->magic_num, QCCSQSCALARQUANTIZER_MAGICNUM))
    {
      QccErrorAddMessage("(QccSQScalarQuantizerReadHeader): %s is not of scalar quantizer (%s) type",
                         scalar_quantizer->filename, QCCSQSCALARQUANTIZER_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(scalar_quantizer->type));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccSQScalarQuantizerReadHeader): Error reading type in scalar quantizer %s",
                         scalar_quantizer->filename);
      return(1);
    }

  switch (scalar_quantizer->type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      fscanf(infile, "%d%*1[\n]", &(scalar_quantizer->num_levels));
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerReadHeader): Error reading number of levels in scalar quantizer %s",
                             scalar_quantizer->filename);
          return(1);
        }
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:
      fscanf(infile, "%*1[\n]");
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerReadHeader): Error reading type in scalar quantizer %s",
                             scalar_quantizer->filename);
          return(1);
        }
      break;

    case QCCSQSCALARQUANTIZER_DEADZONE:
      fscanf(infile, "%*1[\n]");
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerReadHeader): Error reading type in scalar quantizer %s",
                             scalar_quantizer->filename);
          return(1);
        }
      break;
    }
  
  return(0);
}


int QccSQScalarQuantizerReadData(FILE *infile, 
                                 QccSQScalarQuantizer *scalar_quantizer)
{
  int level;
  
  if ((infile == NULL) || (scalar_quantizer == NULL))
    return(0);
  
  if (QccSQScalarQuantizerAlloc(scalar_quantizer))
    {
      QccErrorAddMessage("(QccSQScalarQuantizerReadData): Error calling QccSQScalarQuantizerAlloc()");
      return(1);
    }
  
  switch (scalar_quantizer->type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      for (level = 0; level < scalar_quantizer->num_levels; level++)
        if (QccFileReadDouble(infile,
                              &(scalar_quantizer->levels[level])))
          {
            QccErrorAddMessage("(QccSQScalarQuantizerReadData): Error calling QccFileReadDouble()");
            return(1);
          }
      for (level = 0; level < scalar_quantizer->num_levels; level++)
        if (QccFileReadDouble(infile,
                              &(scalar_quantizer->probs[level])))
          {
            QccErrorAddMessage("(QccSQScalarQuantizerReadData): Error calling QccFileReadDouble()");
            return(1);
          }
      for (level = 0; level < scalar_quantizer->num_levels - 1; level++)
        if (QccFileReadDouble(infile,
                              &(scalar_quantizer->boundaries[level])))
          {
            QccErrorAddMessage("(QccSQScalarQuantizerReadData): Error calling QccFileReadDouble()");
            return(1);
          }
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:
      if (QccFileReadDouble(infile,
                            &(scalar_quantizer->stepsize)))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerReadData): Error calling QccFileReadDouble()");
          return(1);
        }
      break;

    case QCCSQSCALARQUANTIZER_DEADZONE:
      if (QccFileReadDouble(infile,
                            &(scalar_quantizer->stepsize)))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerReadData): Error calling QccFileReadDouble()");
          return(1);
        }
      if (QccFileReadDouble(infile,
                            &(scalar_quantizer->deadzone)))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerReadData): Error calling QccFileReadDouble()");
          return(1);
        }
      break;
    }
  
  return(0);
}


int QccSQScalarQuantizerRead(QccSQScalarQuantizer *scalar_quantizer)
{
  FILE *infile = NULL;
  
  if (scalar_quantizer == NULL)
    return(0);
  
  if ((infile = QccFileOpen(scalar_quantizer->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccSQScalarQuantizerRead): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccSQScalarQuantizerReadHeader(infile, scalar_quantizer))
    {
      QccErrorAddMessage("(QccSQScalarQuantizerRead): Error calling QccSQScalarQuantizerReadHeader()");
      return(1);
    }
  
  if (QccSQScalarQuantizerReadData(infile, scalar_quantizer))
    {
      QccErrorAddMessage("(QccSQScalarQuantizerRead): Error calling QccSQScalarQuantizerReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


int QccSQScalarQuantizerWriteHeader(FILE *outfile, 
                                    const
                                    QccSQScalarQuantizer *scalar_quantizer)
{
  if ((outfile == NULL) || (scalar_quantizer == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCSQSCALARQUANTIZER_MAGICNUM))
    goto Error;

  fprintf(outfile, "%d\n", scalar_quantizer->type);
  if (ferror(outfile))
    goto Error;

  switch (scalar_quantizer->type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      fprintf(outfile, "%d\n",
              scalar_quantizer->num_levels);
      if (ferror(outfile))
        goto Error;
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:

    case QCCSQSCALARQUANTIZER_DEADZONE:
      break;
    }
  
  return(0);

 Error:
  QccErrorAddMessage("(QccSQScalarQuantizerWriteHeader): Error writing header to %s",
                     scalar_quantizer->filename);
  return(1);
  
}


int QccSQScalarQuantizerWriteData(FILE *outfile, 
                                  const 
                                  QccSQScalarQuantizer *scalar_quantizer)
{
  int level;
  
  if ((outfile == NULL) || (scalar_quantizer == NULL))
    return(0);
  
  switch (scalar_quantizer->type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      for (level = 0; level < scalar_quantizer->num_levels; level++)
        if (QccFileWriteDouble(outfile,
                               scalar_quantizer->levels[level]))
          {
            QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccFileWriteDouble()");
            return(1);
          }
      for (level = 0; level < scalar_quantizer->num_levels; level++)
        if (QccFileWriteDouble(outfile,
                               scalar_quantizer->probs[level]))
          {
            QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccFileWriteDouble()");
            return(1);
          }
      for (level = 0; level < scalar_quantizer->num_levels - 1; level++)
        if (QccFileWriteDouble(outfile,
                               scalar_quantizer->boundaries[level]))
          {
            QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccFileWriteDouble()");
            return(1);
          }
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:
      if (QccFileWriteDouble(outfile,
                             scalar_quantizer->stepsize))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccFileWriteDouble()");
          return(1);
        }
      break;

    case QCCSQSCALARQUANTIZER_DEADZONE:
      if (QccFileWriteDouble(outfile,
                             scalar_quantizer->stepsize))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccFileWriteDouble()");
          return(1);
        }
      if (QccFileWriteDouble(outfile,
                             scalar_quantizer->deadzone))
        {
          QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccFileWriteDouble()");
          return(1);
        }
      break;
    }
  
  return(0);
}


int QccSQScalarQuantizerWrite(const QccSQScalarQuantizer *scalar_quantizer)
{
  FILE *outfile;
  
  if (scalar_quantizer == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(scalar_quantizer->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccSQScalarQuantizerWriteHeader(outfile, scalar_quantizer))
    {
      QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccSQScalarQuantizerWriteHeader()");
      return(1);
    }
  
  if (QccSQScalarQuantizerWriteData(outfile, scalar_quantizer))
    {
      QccErrorAddMessage("(QccSQScalarQuantizerWrite): Error calling QccSQScalarQuantizerWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


int QccSQScalarQuantizerPrint(const QccSQScalarQuantizer *scalar_quantizer)
{
  int level;
  
  if (scalar_quantizer == NULL)
    return(0);
  
  if (QccFilePrintFileInfo(scalar_quantizer->filename,
                           scalar_quantizer->magic_num,
                           scalar_quantizer->major_version,
                           scalar_quantizer->minor_version))
    return(1);

  printf("Quantizer type: ");
  switch (scalar_quantizer->type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      printf("general\n");
      printf("Num of levels: %d\n", scalar_quantizer->num_levels);
      if (!scalar_quantizer->num_levels)
        return(0);

      printf("Index\t   Level\t Prob\t  Boundary\n\n");
      
      for (level = 0; level < scalar_quantizer->num_levels; level++)
        {
          printf("%4d\t% 10.4f\t%.3f\n", 
                 level, scalar_quantizer->levels[level],
                 scalar_quantizer->probs[level]);
          if (level < scalar_quantizer->num_levels - 1)
            printf("\t\t\t\t% 10.4f\n",
                   scalar_quantizer->boundaries[level]);
        }
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:
      printf("uniform\n");
      printf("Step Size: %.3f\n",
             scalar_quantizer->stepsize);
      break;

    case QCCSQSCALARQUANTIZER_DEADZONE:
      printf("dead zone\n");
      printf("Step Size: %.3f\n",
             scalar_quantizer->stepsize);
      printf("Dead Zone: %.3f\n",
             scalar_quantizer->deadzone);
      break;
    }
  
  return(0);
}



