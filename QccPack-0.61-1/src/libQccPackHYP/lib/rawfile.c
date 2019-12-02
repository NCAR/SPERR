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


static int QccHYPRawReadSample8(FILE *infile,
                                int *sample,
                                int signed_data,
                                int endian)
{
  char ch1;
  
  if (QccFileReadChar(infile, &ch1))
    {
      QccErrorAddMessage("(QccHYPRawReadSample8): Error calling QccFileReadChar()");
      return(1);
    }
  
  if (signed_data)
    *sample = (int)ch1;
  else
    *sample = (int)((unsigned char)ch1);

  return(0);
}


static int QccHYPRawReadSample16(FILE *infile,
                                 int *sample,
                                 int signed_data,
                                 int endian)
{
  char ch1;
  char ch2;
  int msb;
  int lsb;
  
  if (QccFileReadChar(infile, &ch1))
    {
      QccErrorAddMessage("(QccHYPRawReadSample16): Error calling QccFileReadChar()");
      return(1);
    }
  if (QccFileReadChar(infile, &ch2))
    {
      QccErrorAddMessage("(QccHYPRawReadSample16): Error calling QccFileReadChar()");
      return(1);
    }
  
  switch (endian)
    {
    case QCCHYP_RAWENDIAN_BIG:
      if (signed_data)
        msb = ch1 << 8;
      else
        msb = ((unsigned char)ch1) << 8;
      lsb = (unsigned char)ch2;
      break;
      
    case QCCHYP_RAWENDIAN_LITTLE:
      if (signed_data)
        msb = ch2 << 8;
      else
        msb = ((unsigned char)ch2) << 8;
      lsb = (unsigned char)ch1;
      break;
      
    default:
      QccErrorAddMessage("(QccHYPRawReadSample): Unrecognized endian value");
      return(1);
    }
  
  *sample = (int)(msb | lsb);
  
  return(0);
}


static int QccHYPRawReadSample32(FILE *infile,
                                 int *sample,
                                 int signed_data,
                                 int endian)
{
  char ch1;
  char ch2;
  char ch3;
  char ch4;
  int b1;
  int b2;
  int b3;
  int b4;
  
  if (QccFileReadChar(infile, &ch1))
    {
      QccErrorAddMessage("(QccHYPRawReadSample32): Error calling QccFileReadChar()");
      return(1);
    }
  if (QccFileReadChar(infile, &ch2))
    {
      QccErrorAddMessage("(QccHYPRawReadSample32): Error calling QccFileReadChar()");
      return(1);
    }
  if (QccFileReadChar(infile, &ch3))
    {
      QccErrorAddMessage("(QccHYPRawReadSample32): Error calling QccFileReadChar()");
      return(1);
    }
  if (QccFileReadChar(infile, &ch4))
    {
      QccErrorAddMessage("(QccHYPRawReadSample32): Error calling QccFileReadChar()");
      return(1);
    }
  
  switch (endian)
    {
    case QCCHYP_RAWENDIAN_BIG:
      if (signed_data)
        b1 = ch1 << 24;
      else
        b1 = ((unsigned char)ch1) << 24;
      b2 = ((unsigned char)ch2) << 16;
      b3 = ((unsigned char)ch3) << 8;
      b4 = (unsigned char)ch4;
      break;
      
    case QCCHYP_RAWENDIAN_LITTLE:
      if (signed_data)
        b1 = ch4 << 24;
      else
        b1 = ((unsigned char)ch4) << 24;
      b2 = ((unsigned char)ch3) << 16;
      b3 = ((unsigned char)ch2) << 8;
      b4 = (unsigned char)ch1;
      break;
      
    default:
      QccErrorAddMessage("(QccHYPRawReadSample): Unrecognized endian value");
      return(1);
    }
  
  *sample = (int)(b1 | b2 | b3 | b4);
  
  return(0);
}


static int QccHYPRawReadSample(FILE *infile,
                               int *sample,
                               int bps,
                               int signed_data,
                               int endian)
{
  if (bps <= 8)
    {
      if (QccHYPRawReadSample8(infile,
                               sample,
                               signed_data,
                               endian))
        {
          QccErrorAddMessage("(QccHYPRawReadSample): Error calling QccHYPRawReadSample8()");
          return(1);
        }
    }
  else
    if (bps <= 16)
      {
        if (QccHYPRawReadSample16(infile,
                                  sample,
                                  signed_data,
                                  endian))
          {
            QccErrorAddMessage("(QccHYPRawReadSample): Error calling QccHYPRawReadSample16()");
            return(1);
          }
      }
    else
      if (bps <= 32)
        {
          if (QccHYPRawReadSample32(infile,
                                    sample,
                                    signed_data,
                                    endian))
            {
              QccErrorAddMessage("(QccHYPRawReadSample): Error calling QccHYPRawReadSample32()");
              return(1);
            }
        }
      else
        {
          QccErrorAddMessage("(QccHYPRawReadSample): Unrecognized bits per sample value");
          return(1);
        }
  
  return(0);
}


static int QccHYPRawWriteSample8(FILE *outfile,
                                 int sample,
                                 int endian)
{
  if (QccFileWriteChar(outfile, ((unsigned int)sample) & 0xff))
    {
      QccErrorAddMessage("(QccHYPRawWriteSample8): Error calling QccFileWriteChar()");
      return(1);
    }
  
  return(0);
}


static int QccHYPRawWriteSample16(FILE *outfile,
                                  int sample,
                                  int endian)
{
  char msb;
  char lsb;
  
  msb = (((unsigned int)sample) >> 8) & 0xff;
  lsb = ((unsigned int)sample) & 0xff;
  
  switch (endian)
    {
    case QCCHYP_RAWENDIAN_BIG:
      if (QccFileWriteChar(outfile, msb))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample16): Error calling QccFileWriteChar()");
          return(1);
        }
      if (QccFileWriteChar(outfile, lsb))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample16): Error calling QccFileWriteChar()");
          return(1);
        }
      break;
      
    case QCCHYP_RAWENDIAN_LITTLE:
      if (QccFileWriteChar(outfile, lsb))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample16): Error calling QccFileWriteChar()");
          return(1);
        }
      if (QccFileWriteChar(outfile, msb))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample16): Error calling QccFileWriteChar()");
          return(1);
        }
      break;
      
    default:
      QccErrorAddMessage("(QccHYPRawWriteSample16): Unrecognized endian value");
      return(1);
    }
  
  return(0);
}


static int QccHYPRawWriteSample32(FILE *outfile,
                                  int sample,
                                  int endian)
{
  char b1;
  char b2;
  char b3;
  char b4;
  
  b1 = (((unsigned int)sample) >> 24) & 0xff;
  b2 = (((unsigned int)sample) >> 16) & 0xff;
  b3 = (((unsigned int)sample) >> 8) & 0xff;
  b4 = ((unsigned int)sample) & 0xff;
  
  switch (endian)
    {
    case QCCHYP_RAWENDIAN_BIG:
      if (QccFileWriteChar(outfile, b1))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample32): Error calling QccFileWriteChar()");
          return(1);
        }
      if (QccFileWriteChar(outfile, b2))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample32): Error calling QccFileWriteChar()");
          return(1);
        }
      if (QccFileWriteChar(outfile, b3))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample32): Error calling QccFileWriteChar()");
          return(1);
        }
      if (QccFileWriteChar(outfile, b4))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample32): Error calling QccFileWriteChar()");
          return(1);
        }
      break;
      
    case QCCHYP_RAWENDIAN_LITTLE:
      if (QccFileWriteChar(outfile, b4))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample32): Error calling QccFileWriteChar()");
          return(1);
        }
      if (QccFileWriteChar(outfile, b3))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample32): Error calling QccFileWriteChar()");
          return(1);
        }
      if (QccFileWriteChar(outfile, b2))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample32): Error calling QccFileWriteChar()");
          return(1);
        }
      if (QccFileWriteChar(outfile, b1))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample32): Error calling QccFileWriteChar()");
          return(1);
        }
      break;
      
    default:
      QccErrorAddMessage("(QccHYPRawWriteSample32): Unrecognized endian value");
      return(1);
    }
  
  return(0);
}


static int QccHYPRawWriteSample(FILE *outfile,
                                int sample,
                                int bps,
                                int endian)
{
  if (bps <= 8)
    {
      if (QccHYPRawWriteSample8(outfile,
                                sample,
                                endian))
        {
          QccErrorAddMessage("(QccHYPRawWriteSample): Error calling QccHYPRawWriteSample8()");
          return(1);
        }
    }
  else
    if (bps <= 16)
      {
        if (QccHYPRawWriteSample16(outfile,
                                   sample,
                                   endian))
          {
            QccErrorAddMessage("(QccHYPRawWriteSample): Error calling QccHYPRawWriteSample16()");
            return(1);
          }
      }
    else
      if (bps <= 32)
        {
          if (QccHYPRawWriteSample32(outfile,
                                     sample,
                                     endian))
            {
              QccErrorAddMessage("(QccHYPRawWriteSample): Error calling QccHYPRawWriteSample32()");
              return(1);
            }
        }
      else
        {
          QccErrorAddMessage("(QccHYPRawWriteSample): Unrecognized bits per sample value");
          return(1);
        }
  
  return(0);
}


int QccHYPRawRead2D(QccString filename,
                    QccIMGImageComponent *image_component,
                    int bpp,
                    int signed_data,
                    int endian)
{
  FILE *infile;
  int row, col;
  int sample;
  
  if (image_component == NULL)
    return(1);
  
  if ((infile = QccFileOpen(filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawRead2D): Error calling QccFileOpen()");
      return(1);
    }
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      {
        if (QccHYPRawReadSample(infile,
                                &sample,
                                bpp,
                                signed_data,
                                endian))
          {
            QccErrorAddMessage("(QccHYPRawRead2D): Error calling QccHYPRawReadSample()");
            return(1);
          }
        image_component->image[row][col] = (double)sample;
      }
  
  QccFileClose(infile);
  
  return(0);
}


int QccHYPRawWrite2D(QccString filename,
                     const QccIMGImageComponent *image_component,
                     int bpp,
                     int endian)
{
  FILE *outfile;
  int row, col;
  int sample;
  
  if (image_component == NULL)
    return(1);

  if ((outfile = QccFileOpen(filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawWrite2D): Error calling QccFileOpen()");
      return(1);
    }
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      {
        sample = (int)rint(image_component->image[row][col]);
        if (QccHYPRawWriteSample(outfile,
                                 sample,
                                 bpp,
                                 endian))
          {
            QccErrorAddMessage("(QccHYPRawWrite2D): Error calling QccHYPRawWriteSample()");
            return(1);
          }
      }
  
  QccFileClose(outfile);
  
  return(0);
}


static int QccHYPRawMean2D(QccString filename,
                           double *mean,
                           int num_rows,
                           int num_cols,
                           int bpp,
                           int signed_data,
                           int endian)
{
  FILE *infile;
  int row, col;
  int sample;
  
  *mean = 0;
  
  if ((infile = QccFileOpen(filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawMean2D): Error calling QccFileOpen()");
      return(1);
    }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        if (QccHYPRawReadSample(infile,
                                &sample,
                                bpp,
                                signed_data,
                                endian))
          {
            QccErrorAddMessage("(QccHYPRawMean2D): Error calling QccHYPRawReadSample()");
            return(1);
          }
        
        *mean += (double)sample / num_rows / num_cols;
      }
  
  QccFileClose(infile);
  
  return(0);
}


int QccHYPRawDist2D(QccString filename1,
                    QccString filename2,
                    double *mse,
                    double *mae,
                    double *snr,
                    int num_rows,
                    int num_cols,
                    int bpp,
                    int signed_data,
                    int endian)
{
  FILE *infile1;
  FILE *infile2;
  int row, col;
  double mean;
  int sample1;
  int sample2;
  double signal_power = 0;
  double ae;
  double mse1 = 0;
  double mae1 = 0;
  double snr1 = 0;
    
  if (QccHYPRawMean2D(filename1,
                      &mean,
                      num_rows,
                      num_cols,
                      bpp,
                      signed_data,
                      endian))
    {
      QccErrorAddMessage("(QccHYPRawDist2D): Error calling QccHYPRawMean2D()");
      return(1);
    }
  
  if ((infile1 = QccFileOpen(filename1, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawDist2D): Error calling QccFileOpen()");
      return(1);
    }
  if ((infile2 = QccFileOpen(filename2, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawDist2D): Error calling QccFileOpen()");
      return(1);
    }
  
  for (row = 0; row < num_rows; row++)
    for (col = 0; col < num_cols; col++)
      {
        if (QccHYPRawReadSample(infile1,
                                &sample1,
                                bpp,
                                signed_data,
                                endian))
          {
            QccErrorAddMessage("(QccHYPRawDist2D): Error calling QccHYPRawReadSample()");
            return(1);
          }
        if (QccHYPRawReadSample(infile2,
                                &sample2,
                                bpp,
                                signed_data,
                                endian))
          {
            QccErrorAddMessage("(QccHYPRawDist2D): Error calling QccHYPRawReadSample()");
            return(1);
          }
        mse1 += (double)(sample1 - sample2)*(sample1 - sample2) /
          num_rows / num_cols;
        ae = fabs((double)(sample1 - sample2));
        if (ae > mae1)
          mae1 = ae;
        signal_power += (sample1 - mean)*(sample1 - mean) /
          num_rows / num_cols;
      }
  
  if (mse1 != 0.0)
    snr1 = 10.0*log10(signal_power/(mse1));
  
  QccFileClose(infile1);
  QccFileClose(infile2);
  
  if (mse != NULL)
    *mse = mse1;
  if (mae != NULL)
    *mae = mae1;
  if (snr != NULL)
    *snr = snr1;

  return(0);
}


int QccHYPRawRead3D(QccString filename,
                    QccIMGImageCube *image_cube,
                    int bpv,
                    int signed_data,
                    int format,
                    int endian)
{
  FILE *infile;
  int frame, row, col;
  int sample;
  
  if (image_cube == NULL)
    return(1);
  
  if ((infile = QccFileOpen(filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawRead3D): Error calling QccFileOpen()");
      return(1);
    }
  
  switch (format)
    {
    case QCCHYP_RAWFORMAT_BSQ:
      for (frame = 0; frame < image_cube->num_frames; frame++)
        for (row = 0; row < image_cube->num_rows; row++)
          for (col = 0; col < image_cube->num_cols; col++)
            {
              if (QccHYPRawReadSample(infile,
                                      &sample,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawRead3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              image_cube->volume[frame][row][col] = (double)sample;
            }
      break;
      
    case QCCHYP_RAWFORMAT_BIL:
      for (row = 0; row < image_cube->num_rows; row++)
        for (frame = 0; frame < image_cube->num_frames; frame++)
          for (col = 0; col < image_cube->num_cols; col++)
            {
              if (QccHYPRawReadSample(infile,
                                      &sample,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawRead3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              image_cube->volume[frame][row][col] = (double)sample;
            }
      break;
      
    case QCCHYP_RAWFORMAT_BIP:
      for (row = 0; row < image_cube->num_rows; row++)
        for (col = 0; col < image_cube->num_cols; col++)
          for (frame = 0; frame < image_cube->num_frames; frame++)
            {
              if (QccHYPRawReadSample(infile,
                                      &sample,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawRead3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              image_cube->volume[frame][row][col] = (double)sample;
            }
      break;
      
    default:
      {
        QccErrorAddMessage("(QccHYPRawRead3D): Unrecognized format");
        return(1);
      }
    }
  
  QccFileClose(infile);
  
  return(0);
}


int QccHYPRawWrite3D(QccString filename,
                     const QccIMGImageCube *image_cube,
                     int bpv,
                     int format,
                     int endian)
{
  FILE *outfile;
  int frame, row, col;
  int sample;

  if (image_cube == NULL)
    return(1);

  if ((outfile = QccFileOpen(filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawWrite3D): Error calling QccFileOpen()");
      return(1);
    }
  
  switch (format)
    {
    case QCCHYP_RAWFORMAT_BSQ:
      for (frame = 0; frame < image_cube->num_frames; frame++)
        for (row = 0; row < image_cube->num_rows; row++)
          for (col = 0; col < image_cube->num_cols; col++)
            {
              sample =
                (int)rint(image_cube->volume[frame][row][col]);
              if (QccHYPRawWriteSample(outfile,
                                       sample,
                                       bpv,
                                       endian))
                {
                  QccErrorAddMessage("(QccHYPRawWrite3D): Error calling QccHYPRawWriteSample()");
                  return(1);
                }
            }
      break;
      
    case QCCHYP_RAWFORMAT_BIL:
      for (row = 0; row < image_cube->num_rows; row++)
        for (frame = 0; frame < image_cube->num_frames; frame++)
          for (col = 0; col < image_cube->num_cols; col++)
            {
              sample =
                (int)rint(image_cube->volume[frame][row][col]);
              if (QccHYPRawWriteSample(outfile,
                                       sample,
                                       bpv,
                                       endian))
                {
                  QccErrorAddMessage("(QccHYPRawWrite3D): Error calling QccHYPRawWriteSample()");
                  return(1);
                }
            }
      break;
      
    case QCCHYP_RAWFORMAT_BIP:
      for (row = 0; row < image_cube->num_rows; row++)
        for (col = 0; col < image_cube->num_cols; col++)
          for (frame = 0; frame < image_cube->num_frames; frame++)
            {
              sample =
                (int)rint(image_cube->volume[frame][row][col]);
              if (QccHYPRawWriteSample(outfile,
                                       sample,
                                       bpv,
                                       endian))
                {
                  QccErrorAddMessage("(QccHYPRawWrite3D): Error calling QccHYPRawWriteSample()");
                  return(1);
                }
            }
      break;
      
    default:
      {
        QccErrorAddMessage("(QccHYPRawWrite3D): Unrecognized format");
        return(1);
      }
    }
  
  QccFileClose(outfile);
  
  return(0);
}


static int QccHYPRawMean3D(QccString filename,
                           double *mean,
                           int num_frames,
                           int num_rows,
                           int num_cols,
                           int bpv,
                           int signed_data,
                           int format,
                           int endian)
{
  FILE *infile;
  int frame, row, col;
  int sample;
  
  *mean = 0;
  
  if ((infile = QccFileOpen(filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawMean3D): Error calling QccFileOpen()");
      return(1);
    }
  
  switch (format)
    {
    case QCCHYP_RAWFORMAT_BSQ:
      for (frame = 0; frame < num_frames; frame++)
        for (row = 0; row < num_rows; row++)
          for (col = 0; col < num_cols; col++)
            {
              if (QccHYPRawReadSample(infile,
                                      &sample,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawMean3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              *mean += (double)sample / num_rows / num_cols / num_frames;
            }
      break;
      
    case QCCHYP_RAWFORMAT_BIL:
      for (row = 0; row < num_rows; row++)
        for (frame = 0; frame < num_frames; frame++)
          for (col = 0; col < num_cols; col++)
            {
              if (QccHYPRawReadSample(infile,
                                      &sample,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawMean3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              *mean += (double)sample / num_rows / num_cols / num_frames;
            }
      break;
      
    case QCCHYP_RAWFORMAT_BIP:
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          for (frame = 0; frame < num_frames; frame++)
            {
              if (QccHYPRawReadSample(infile,
                                      &sample,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawMean3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              *mean += (double)sample / num_rows / num_cols / num_frames;
            }
      break;
      
    default:
      {
        QccErrorAddMessage("(QccHYPRawMean3D): Unrecognized format");
        return(1);
      }
    }
  
  QccFileClose(infile);
  
  return(0);
}


int QccHYPRawDist3D(QccString filename1,
                    QccString filename2,
                    double *mse,
                    double *mae,
                    double *snr,
                    int num_frames,
                    int num_rows,
                    int num_cols,
                    int bpv,
                    int signed_data,
                    int format,
                    int endian)
{
  FILE *infile1;
  FILE *infile2;
  int frame, row, col;
  double mean;
  int sample1;
  int sample2;
  double signal_power = 0;
  double ae;
  double mse1 = 0;
  double mae1 = 0;
  double snr1 = 0;
  
  if (QccHYPRawMean3D(filename1,
                      &mean,
                      num_frames,
                      num_rows,
                      num_cols,
                      bpv,
                      signed_data,
                      format,
                      endian))
    {
      QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccHYPRawMean3D()");
      return(1);
    }
  
  if ((infile1 = QccFileOpen(filename1, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccFileOpen()");
      return(1);
    }
  if ((infile2 = QccFileOpen(filename2, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccFileOpen()");
      return(1);
    }
  
  switch (format)
    {
    case QCCHYP_RAWFORMAT_BSQ:
      for (frame = 0; frame < num_frames; frame++)
        for (row = 0; row < num_rows; row++)
          for (col = 0; col < num_cols; col++)
            {
              if (QccHYPRawReadSample(infile1,
                                      &sample1,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              if (QccHYPRawReadSample(infile2,
                                      &sample2,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              mse1 += (double)(sample1 - sample2)*(sample1 - sample2) /
                num_frames / num_rows / num_cols;
              ae = fabs((double)(sample1 - sample2));
              if (ae > mae1)
                mae1 = ae;
              signal_power += (sample1 - mean)*(sample1 - mean) /
                num_frames / num_rows / num_cols;
            }
      break;
      
    case QCCHYP_RAWFORMAT_BIL:
      for (row = 0; row < num_rows; row++)
        for (frame = 0; frame < num_frames; frame++)
          for (col = 0; col < num_cols; col++)
            {
              if (QccHYPRawReadSample(infile1,
                                      &sample1,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              if (QccHYPRawReadSample(infile2,
                                      &sample2,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              mse1 += (double)(sample1 - sample2)*(sample1 - sample2) /
                num_frames / num_rows / num_cols;
              ae = fabs((double)(sample1 - sample2));
              if (ae > mae1)
                mae1 = ae;
              signal_power += (sample1 - mean)*(sample1 - mean) /
                num_frames / num_rows / num_cols;
            }
      break;
      
    case QCCHYP_RAWFORMAT_BIP:
      for (row = 0; row < num_rows; row++)
        for (col = 0; col < num_cols; col++)
          for (frame = 0; frame < num_frames; frame++)
            {
              if (QccHYPRawReadSample(infile1,
                                      &sample1,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              if (QccHYPRawReadSample(infile2,
                                      &sample2,
                                      bpv,
                                      signed_data,
                                      endian))
                {
                  QccErrorAddMessage("(QccHYPRawDist3D): Error calling QccHYPRawReadSample()");
                  return(1);
                }
              mse1 += (double)(sample1 - sample2)*(sample1 - sample2) /
                num_frames / num_rows / num_cols;
              ae = fabs((double)(sample1 - sample2));
              if (ae > mae1)
                mae1 = ae;
              signal_power += (sample1 - mean)*(sample1 - mean) /
                num_frames / num_rows / num_cols;
            }
      break;
      
    default:
      {
        QccErrorAddMessage("(QccHYPRawDist3D): Unrecognized format");
        return(1);
      }
    }
  
  if (mse1 != 0.0)
    snr1 = 10.0*log10(signal_power/(mse1));
  
  QccFileClose(infile1);
  QccFileClose(infile2);
  
  if (mse != NULL)
    *mse = mse1;
  if (mae != NULL)
    *mae = mae1;
  if (snr != NULL)
    *snr = snr1;

  return(0);
}
