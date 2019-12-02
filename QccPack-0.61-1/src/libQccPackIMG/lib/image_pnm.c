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


void QccIMGImagePNMGetType(const QccString magic_number,
                           int *returned_image_type,
                           int *returned_raw)
{
  int image_type;
  int raw =  QCCIMG_PNM_RAW;

  if (!strncmp(magic_number, "P1", QCCSTRINGLEN))
    {
      image_type = QCCIMGTYPE_PBM;
      raw = QCCIMG_PNM_ASCII;
      goto Return;
    }
  if (!strncmp(magic_number, "P4", QCCSTRINGLEN))
    {
      image_type = QCCIMGTYPE_PBM;
      raw = QCCIMG_PNM_RAW;
      goto Return;
    }
  
  if (!strncmp(magic_number, "P2", QCCSTRINGLEN))
    {
      image_type = QCCIMGTYPE_PGM;
      raw = QCCIMG_PNM_ASCII;
      goto Return;
    }
  if (!strncmp(magic_number, "P5", QCCSTRINGLEN))
    {
      image_type = QCCIMGTYPE_PGM;
      raw = QCCIMG_PNM_RAW;
      goto Return;
    }
  
  if (!strncmp(magic_number, "P3", QCCSTRINGLEN))
    {
      image_type = QCCIMGTYPE_PPM;
      raw = QCCIMG_PNM_ASCII;
      goto Return;
    }
  if (!strncmp(magic_number, "P6", QCCSTRINGLEN))
    {
      image_type = QCCIMGTYPE_PPM;
      raw = QCCIMG_PNM_RAW;
      goto Return;
    }
  
  image_type = QCCIMGTYPE_UNKNOWN;
  
 Return:
  if (returned_image_type != NULL)
    *returned_image_type = image_type;
  if (returned_raw != NULL)
    *returned_raw = raw;
}


static int QccIMGImagePNMReadHeader(FILE *infile,
                                    QccIMGImage *image,
                                    int *image_raw)
{
  QccString magic_number;
  int num_rows, num_cols;
  int maxval;

  if (QccFileReadString(infile, magic_number))
    {
      QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                         image->filename);
      return(1);
    }
  if (QccFileSkipWhiteSpace(infile, 1))
    {
      QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                         image->filename);
      return(1);
    }


  QccIMGImagePNMGetType(magic_number, &image->image_type, image_raw);

  if (image->image_type == QCCIMGTYPE_PBM)
    {
      fscanf(infile, "%d", &num_cols);
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }
      if (QccFileSkipWhiteSpace(infile, 1))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }

      if (*image_raw == QCCIMG_PNM_RAW)
        fscanf(infile, "%d%*c", &num_rows);
      else
        {
          fscanf(infile, "%d", &num_rows);
          if (QccFileSkipWhiteSpace(infile, 1))
            {
              QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                                 image->filename);
              return(1);
            }
        }
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }
    }

  if ((image->image_type == QCCIMGTYPE_PGM) ||
      (image->image_type == QCCIMGTYPE_PPM))
    {
      fscanf(infile, "%d", &num_cols);
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }
      if (QccFileSkipWhiteSpace(infile, 1))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }
      
      fscanf(infile, "%d", &num_rows);
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }
      if (QccFileSkipWhiteSpace(infile, 1))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }
      
      if (*image_raw == QCCIMG_PNM_RAW)
        fscanf(infile, "%d%*c", &maxval);
      else
        {
          fscanf(infile, "%d", &maxval);
          if (QccFileSkipWhiteSpace(infile, 1))
            {
              QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                                 image->filename);
              return(1);
            }
        }
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }
      if ((*image_raw == QCCIMG_PNM_RAW) && (maxval != 255))
        {
          QccErrorAddMessage("(QccIMGImagePNMReadheader): Raw PGM and PPM files must have maxval = 255");
          QccErrorAddMessage("(QccIMGImagePNMReadHeader): Error reading %s",
                             image->filename);
          return(1);
        }
    }

  switch (image->image_type)
    {
    case QCCIMGTYPE_PPM:
      image->Y.num_rows = image->U.num_rows = image->V.num_rows = num_rows;
      image->Y.num_cols = image->U.num_cols = image->V.num_cols = num_cols;
      break;
    case QCCIMGTYPE_PBM:
    case QCCIMGTYPE_PGM:
      image->Y.num_rows = num_rows;
      image->U.num_rows = image->V.num_rows = 0;
      image->Y.num_cols = num_cols;
      image->U.num_cols = image->V.num_cols = 0;
      break;
    case QCCIMGTYPE_UNKNOWN:
    default:
      QccErrorAddMessage("(QccIMGImagePNMRead): Unknown image type %d",
                         image->image_type);
      return(1);
    }

  return(0);
}


static int QccIMGImagePBMReadData(FILE *infile,
                                  QccIMGImage *image,
                                  int image_raw)
{
  int row, col;
  int bit_count = 0;
  unsigned char ch = 0;

  if (image_raw == QCCIMG_PNM_RAW)
    {
      for (row = 0; row < image->Y.num_rows; row++)
        for (col = 0; col < image->Y.num_cols; col++)
          {
            if (!bit_count)
              {
                if (QccFileReadChar(infile, (char *)(&ch)))
                  goto Error;
                bit_count = 8;
              }
            image->Y.image[row][col] = 
              ((ch & 0x80) == 0);
            ch <<= 1;
            bit_count--;
          }
    }
  else
    {
      for (row = 0; row < image->Y.num_rows; row++)
        for (col = 0; col < image->Y.num_cols; col++)
          {
            int val;

            fscanf(infile, "%d", &val);
            if (ferror(infile) || feof(infile))
              goto Error;
            image->Y.image[row][col] = (double)val;
          }
    }
      
  return(0);
 Error:
  QccErrorAddMessage("(QccIMGImagePBMReadData): Error reading data in %s",
                     image->filename);
  return(1);

}


static int QccIMGImagePGMReadData(FILE *infile,
                                  QccIMGImage *image,
                                  int image_raw)
{
  int row, col;
  unsigned char ch;

  if (image_raw == QCCIMG_PNM_RAW)
    {
      for (row = 0; row < image->Y.num_rows; row++)
        for (col = 0; col < image->Y.num_cols; col++)
          {
            if (QccFileReadChar(infile, (char *)(&ch)))
              goto Error;
            image->Y.image[row][col] = (int)ch;
          }
    }
  else
    {
      for (row = 0; row < image->Y.num_rows; row++)
        for (col = 0; col < image->Y.num_cols; col++)
          {
            int val;

            fscanf(infile, "%d", &val);
            if (ferror(infile) || feof(infile))
              goto Error;
            image->Y.image[row][col] = (double)val;
          }
    }
      
  return(0);
 Error:
  QccErrorAddMessage("(QccIMGImagePGMReadData): Error reading data in %s",
                     image->filename);
  return(1);
}


static int QccIMGImagePPMReadData(FILE *infile,
                                  QccIMGImage *image,
                                  int image_raw)
{
  int row, col;
  int red, green, blue;
  unsigned char ch;

  if (image_raw == QCCIMG_PNM_RAW)
    {
      for (row = 0; row < image->Y.num_rows; row++)
        for (col = 0; col < image->Y.num_cols; col++)
          {
            if (QccFileReadChar(infile, (char *)(&ch)))
              goto Error;
            red = (int)ch;
            if (QccFileReadChar(infile, (char *)(&ch)))
              goto Error;
            green = (int)ch;
            if (QccFileReadChar(infile, (char *)(&ch)))
              goto Error;
            blue = (int)ch;
            QccIMGImageRGBtoYUV(&(image->Y.image[row][col]),
                                &(image->U.image[row][col]),
                                &(image->V.image[row][col]),
                                red, green, blue);
          }
    }
  else
    {
      for (row = 0; row < image->Y.num_rows; row++)
        for (col = 0; col < image->Y.num_cols; col++)
          {
            fscanf(infile, "%d", &red);
            if (ferror(infile) || feof(infile))
              goto Error;
            fscanf(infile, "%d", &green);
            if (ferror(infile) || feof(infile))
              goto Error;
            fscanf(infile, "%d", &blue);
            if (ferror(infile) || feof(infile))
              goto Error;
            QccIMGImageRGBtoYUV(&(image->Y.image[row][col]),
                                &(image->U.image[row][col]),
                                &(image->V.image[row][col]),
                                red, green, blue);
          }
    }
      
  return(0);
 Error:
  QccErrorAddMessage("(QccIMGImagePPMReadData): Error reading data in %s",
                     image->filename);
  return(1);
}


int QccIMGImagePNMRead(QccIMGImage *image)
{
  FILE *infile;
  int image_raw;
  int return_value;

  if ((infile = QccFileOpen(image->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccIMGImagePNMRead): Error calling QccFileOpen()");
      goto Error;
    }

  if (QccIMGImagePNMReadHeader(infile, image, &image_raw))
    {
      QccErrorAddMessage("(QccIMGImagePNMRead): Error calling QccIMGImagePNMReadheader()");
      goto Error;
    }

  if (QccIMGImageAlloc(image))
    {
      QccErrorAddMessage("(QccIMGImagePNMRead): Error calling QccIMGImageAlloc()");
      goto Error;
    }    

  switch (image->image_type)
    {
    case QCCIMGTYPE_PBM:
      if (QccIMGImagePBMReadData(infile, image, image_raw))
        {
          QccErrorAddMessage("(QccIMGImagePNMRead): Error calling QccIMGImagePBMReadData()");
          goto Error;
        }
      break;
    case QCCIMGTYPE_PGM:
      if (QccIMGImagePGMReadData(infile, image, image_raw))
        {
          QccErrorAddMessage("(QccIMGImagePNMRead): Error calling QccIMGImagePGMReadData()");
          goto Error;
        }
      break;
    case QCCIMGTYPE_PPM:
      if (QccIMGImagePPMReadData(infile, image, image_raw))
        {
          QccErrorAddMessage("(QccIMGImagePNMRead): Error calling QccIMGImagePPMReadData()");
          goto Error;
        }
      break;
    }
  
  return_value = 0;
  goto Return;

 Error:
  return_value = 1;
 Return:
  QccFileClose(infile);
  return(return_value);
}


static int QccIMGImagePNMWriteHeader(FILE *outfile, const QccIMGImage *image)
{
  int num_rows, num_cols;
  int major_version, minor_version;
  QccString date;

  switch (image->image_type)
    {
    case QCCIMGTYPE_PBM:
      fprintf(outfile, "P4\n");
      if (ferror(outfile))
        {
          QccErrorAddMessage("(QccIMGImagePNMWriteHeader): Error writing header to %s",
                             image->filename);
          goto Error;
        }
      break;
    case QCCIMGTYPE_PGM:
      fprintf(outfile, "P5\n");
      if (ferror(outfile))
        {
          QccErrorAddMessage("(QccIMGImagePNMWriteHeader): Error writing header to %s",
                             image->filename);
          goto Error;
        }
      break;
    case QCCIMGTYPE_PPM:
      fprintf(outfile, "P6\n");
      if (ferror(outfile))
        {
          QccErrorAddMessage("(QccIMGImagePNMWriteHeader): Error writing header to %s",
                             image->filename);
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccIMGImagePNMWriteHeader): Unrecognized image type");
      goto Error;
    }

  QccGetQccPackVersion(&major_version, &minor_version, date);

  fprintf(outfile, "# Created by QccPack Version %d.%d (%s)\n",
          major_version, minor_version, date);
  if (ferror(outfile))
    {
      QccErrorAddMessage("(QccIMGImagePNMWriteHeader): Error writing header to %s",
                         image->filename);
      goto Error;
    }

  if (QccIMGImageGetSize(image, &num_rows, &num_cols))
    {
      QccErrorAddMessage("(QccIMGImagewritePNM): Error calling QccIMGImageGetSize()");
      goto Error;
    }
  
  fprintf(outfile, "%d %d\n", num_cols, num_rows);
  if (ferror(outfile))
    {
      QccErrorAddMessage("(QccIMGImagePNMWriteHeader): Error writing header to %s",
                         image->filename);
      goto Error;
    }

  if ((image->image_type == QCCIMGTYPE_PGM) ||
      (image->image_type == QCCIMGTYPE_PPM))
    {
      fprintf(outfile, "%d\n", 255);
      if (ferror(outfile))
        {
          QccErrorAddMessage("(QccIMGImagePNMWriteHeader): Error writing header to %s",
                             image->filename);
          goto Error;
        }
    }

  return(0);
 Error:
  return(1);
}


static int QccIMGImagePBMWriteData(FILE *outfile, const QccIMGImage *image)
{
  int row, col;
  int bit_count = 8;
  unsigned char ch = 0;

  for (row = 0; row < image->Y.num_rows; row++)
    for (col = 0; col < image->Y.num_cols; col++)
      {
        ch <<= 1;
        ch |= (image->Y.image[row][col] == 0);
        bit_count--;
        if (!bit_count)
          {
            if (QccFileWriteChar(outfile, ch))
              goto Error;
            ch = 0;
            bit_count = 8;
          }          
      }

  if (bit_count != 8)
    {
      ch >>= bit_count;
      if (QccFileWriteChar(outfile, ch))
        goto Error;
    }

  return(0);
 Error:
  QccErrorAddMessage("(QccIMGImagePBMWriteData): Error writing data to %s",
                     image->filename);
  return(1);
}


static int QccIMGImagePGMWriteData(FILE *outfile, const QccIMGImage *image)
{
  int row, col;

  for (row = 0; row < image->Y.num_rows; row++)
    for (col = 0; col < image->Y.num_cols; col++)
      if (QccFileWriteChar(outfile,
                           (char)(QccIMGImageComponentClipPixel(image->Y.image
                                                       [row][col]) + 0.5)))
        {
          QccErrorAddMessage("(QccIMGImagePGMWriteData): Error writing data to %s",
                             image->filename);
          return(1);
        }

  return(0);
}


static int QccIMGImagePPMWriteData(FILE *outfile, const QccIMGImage *image)
{
  int row, col;
  int red, green, blue;

  for (row = 0; row < image->Y.num_rows; row++)
    for (col = 0; col < image->Y.num_cols; col++)
      {
        QccIMGImageYUVtoRGB(&red, &green, &blue,
                            image->Y.image[row][col],
                            image->U.image[row][col],
                            image->V.image[row][col]);
        if (QccFileWriteChar(outfile,
                             (char)red))
          {
            QccErrorAddMessage("(QccIMGImagePPMWriteData): Error writing data to %s",
                               image->filename);
            return(1);
          }
        if (QccFileWriteChar(outfile,
                             (char)green))
          {
            QccErrorAddMessage("(QccIMGImagePPMWriteData): Error writing data to %s",
                               image->filename);
            return(1);
          }
        if (QccFileWriteChar(outfile,
                             (char)blue))
          {
            QccErrorAddMessage("(QccIMGImagePPMWriteData): Error writing data to %s",
                               image->filename);
            return(1);
          }
      }

  return(0);
}


int QccIMGImagePNMWrite(const QccIMGImage *image)
{
  FILE *outfile;

  if (image == NULL)
    return(0);

  if ((outfile = QccFileOpen(image->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccIMGImagePNMWrite): Error calling QccFileOpen()");
      goto Error;
    }

  if (QccIMGImagePNMWriteHeader(outfile, image))
    {
      QccErrorAddMessage("(QccIMGImagePNMWrite): Error calling QccIMGImagePNMWriteHeader()");
      goto Error;
    }

  switch (image->image_type)
    {
    case QCCIMGTYPE_PBM:
      if (QccIMGImagePBMWriteData(outfile, image))
        {
          QccErrorAddMessage("(QccIMGImagePNMWrite): Error calling QccIMGImagePBMWriteData()");
          goto Error;
        }
      break;
    case QCCIMGTYPE_PGM:
      if (QccIMGImagePGMWriteData(outfile, image))
        {
          QccErrorAddMessage("(QccIMGImagePNMWrite): Error calling QccIMGImagePGMWriteData()");
          goto Error;
        }
      break;
    case QCCIMGTYPE_PPM:
      if (!QccIMGImageColor(image))
        {
          QccErrorAddMessage("(QccIMGImagePNMWrite): Error %s is not a color file",
                             image->filename);
          return(1);
        }
      if (QccIMGImagePPMWriteData(outfile, image))
        {
          QccErrorAddMessage("(QccIMGImagePNMWrite): Error calling QccIMGImagePPMWriteData()");
          goto Error;
        }
      break;
    }

  QccFileClose(outfile);

  return(0);

 Error:
  return(1);
}

