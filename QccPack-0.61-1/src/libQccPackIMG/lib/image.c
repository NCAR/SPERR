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

int QccIMGImageInitialize(QccIMGImage *image)
{
  if (image == NULL)
    return(0);
  
  image->image_type = QCCIMGTYPE_UNKNOWN;
  QccStringMakeNull(image->filename);
  QccIMGImageComponentInitialize(&(image->Y));
  QccIMGImageComponentInitialize(&(image->U));
  QccIMGImageComponentInitialize(&(image->V));
  
  return(0);
}


int QccIMGImageGetSize(const QccIMGImage *image,
                       int *num_rows, int *num_cols)
{
  if (image == NULL)
    return(0);
  
  if (num_rows != NULL)
    *num_rows = image->Y.num_rows;
  if (num_cols != NULL)
    *num_cols = image->Y.num_cols;
  
  return(0);
}


int QccIMGImageGetSizeYUV(const QccIMGImage *image,
                          int *num_rows_Y, int *num_cols_Y,
                          int *num_rows_U, int *num_cols_U,
                          int *num_rows_V, int *num_cols_V)
{
  if (image == NULL)
    return(0);
  
  if (num_rows_Y != NULL)
    *num_rows_Y = image->Y.num_rows;
  if (num_cols_Y != NULL)
    *num_cols_Y = image->Y.num_cols;
  
  if (num_rows_U != NULL)
    *num_rows_U = image->U.num_rows;
  if (num_cols_U != NULL)
    *num_cols_U = image->U.num_cols;
  
  if (num_rows_V != NULL)
    *num_rows_V = image->V.num_rows;
  if (num_cols_V != NULL)
    *num_cols_V = image->V.num_cols;
  
  return(0);
}


int QccIMGImageSetSize(QccIMGImage *image,
                       int num_rows, int num_cols)
{
  if (image == NULL)
    return(0);
  
  if (QccIMGImageColor(image))
    {
      image->Y.num_rows = image->U.num_rows = image->V.num_rows = num_rows;
      image->Y.num_cols = image->U.num_cols = image->V.num_cols = num_cols;
    }
  else
    {
      image->Y.num_rows = num_rows;
      image->U.num_rows = image->V.num_rows = 0;
      image->Y.num_cols = num_cols;
      image->U.num_cols = image->V.num_cols = 0;
    }
  
  return(0);
}


int QccIMGImageSetSizeYUV(QccIMGImage *image,
                          int num_rows_Y, int num_cols_Y,
                          int num_rows_U, int num_cols_U,
                          int num_rows_V, int num_cols_V)
{
  if (image == NULL)
    return(0);
  
  image->Y.num_rows = num_rows_Y;
  image->Y.num_cols = num_cols_Y;
  
  image->U.num_rows = num_rows_U;
  image->U.num_cols = num_cols_U;
  
  image->V.num_rows = num_rows_V;
  image->V.num_cols = num_cols_V;
  
  return(0);
}


int QccIMGImageAlloc(QccIMGImage *image)
{
  if (image == NULL)
    return(0);
  
  if (QccIMGImageComponentAlloc(&(image->Y)))
    {
      QccErrorAddMessage("(QccIMGImageAlloc): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  if (QccIMGImageComponentAlloc(&(image->U)))
    {
      QccErrorAddMessage("(QccIMGImageAlloc): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  if (QccIMGImageComponentAlloc(&(image->V)))
    {
      QccErrorAddMessage("(QccIMGImageAlloc): Error calling QccIMGImageComponentAlloc()");
      goto Error;
    }
  
  return(0);
  
 Error:
  QccIMGImageComponentFree(&(image->Y));
  QccIMGImageComponentFree(&(image->U));
  QccIMGImageComponentFree(&(image->V));
  return(1);
}


void QccIMGImageFree(QccIMGImage *image)
{
  if (image == NULL)
    return;
  
  QccIMGImageComponentFree(&(image->Y));
  QccIMGImageComponentFree(&(image->U));
  QccIMGImageComponentFree(&(image->V));
}


static int QccIMGImageSetMin(QccIMGImage *image)
{
  if (image == NULL)
    return(0);
  
  QccIMGImageComponentSetMin(&(image->Y));
  QccIMGImageComponentSetMin(&(image->U));
  QccIMGImageComponentSetMin(&(image->V));
  
  return(0);
}


static int QccIMGImageSetMax(QccIMGImage *image)
{
  if (image == NULL)
    return(0);
  
  QccIMGImageComponentSetMax(&(image->Y));
  QccIMGImageComponentSetMax(&(image->U));
  QccIMGImageComponentSetMax(&(image->V));
  
  return(0);
}


int QccIMGImageSetMaxMin(QccIMGImage *image)
{
  if (image == NULL)
    return(0);
  
  if (QccIMGImageSetMax(image))
    {
      QccErrorAddMessage("(QccIMGImageSetMaxMin): Error calling QccIMGImageSetMax()");
      return(1);
    }
  
  if (QccIMGImageSetMin(image))
    {
      QccErrorAddMessage("(QccIMGImageSetMaxMin): Error calling QccIMGImageSetMin()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageColor(const QccIMGImage *image)
{
  return(image->image_type == QCCIMGTYPE_PPM);
}


int QccIMGImageDetermineType(QccIMGImage *image)
{
  QccString magic_num;
  QccString extension;
  
  if (image == NULL)
    return(0);
  
  if (image->image_type != QCCIMGTYPE_UNKNOWN)
    return(0);
  
  if (QccFileExists(image->filename))
    {
      if (QccFileGetMagicNumber(image->filename, magic_num))
        {
          QccErrorAddMessage("(QccIMGImageDetermineType): Error calling QccFileGetMagicNumber()");
          goto Return;
        }
      
      if (!strncmp(magic_num, "ICP", QCCSTRINGLEN))
        {
          image->image_type = QCCIMGTYPE_ICP;
          return(0);
        }
      
      QccIMGImagePNMGetType(magic_num, &image->image_type, NULL);
      if (image->image_type != QCCIMGTYPE_UNKNOWN)
        return(0);
    }
  
  if (QccFileGetExtension(image->filename, extension))
    goto Error;
  
  if (!strncmp(extension, "icp", QCCSTRINGLEN))
    {
      image->image_type = QCCIMGTYPE_ICP;
      return(0);
    }
  if (!strncmp(extension, "pbm", QCCSTRINGLEN))
    {
      image->image_type = QCCIMGTYPE_PBM;
      return(0);
    }
  if (!strncmp(extension, "pgm", QCCSTRINGLEN))
    {
      image->image_type = QCCIMGTYPE_PGM;
      return(0);
    }
  if (!strncmp(extension, "ppm", QCCSTRINGLEN))
    {
      image->image_type = QCCIMGTYPE_PPM;
      return(0);
    }
  
 Error:
  image->image_type = QCCIMGTYPE_UNKNOWN;
  QccErrorAddMessage("(QccIMGImageDetermineType): Unknown image type");
 Return:
  return(1);
}


int QccIMGImageRead(QccIMGImage *image)
{
  if (image == NULL)
    return(0);
  
  image->image_type = QCCIMGTYPE_UNKNOWN;
  
  if (QccIMGImageDetermineType(image))
    {
      QccErrorAddMessage("(QccIMGImageRead): Error calling QccIMGImageDetermineType()");
      return(1);
    }
  
  switch (image->image_type)
    {
    case QCCIMGTYPE_ICP:
      QccStringCopy(image->Y.filename, image->filename);
      if (QccIMGImageComponentRead(&(image->Y)))
        {
          QccErrorAddMessage("(QccIMGImageRead): Error calling QccIMGImageComponentRead()");
          return(1);
        }
      break;
      
    case QCCIMGTYPE_PBM:
    case QCCIMGTYPE_PGM:
    case QCCIMGTYPE_PPM:
      if (QccIMGImagePNMRead(image))
        {
          QccErrorAddMessage("(QccIMGImageRead): Error calling QccIMGImagePNMRead()");
          return(1);
        }
      break;
      
    default:
      QccErrorAddMessage("(QccIMGImageRead): Unknown image type");
      return(1);
    }
  
  if (QccIMGImageSetMaxMin(image))
    {
      QccErrorAddMessage("(QccIMGImageRead): Error calling QccIMGImageSetMaxMin()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageWrite(QccIMGImage *image)
{
  if (image == NULL)
    return(0);
  
  if (QccIMGImageDetermineType(image))
    {
      QccErrorAddMessage("(QccIMGImageWrite): Error calling QccIMGImageDetermineType()");
      return(1);
    }
  
  switch (image->image_type)
    {
    case QCCIMGTYPE_ICP:
      QccStringCopy(image->Y.filename, image->filename);
      if (QccIMGImageComponentWrite(&(image->Y)))
        {
          QccErrorAddMessage("(QccIMGImageWrite): Error calling QccIMGImageComponentWrite()");
          return(1);
        }
      break;
      
    case QCCIMGTYPE_PBM:
    case QCCIMGTYPE_PGM:
    case QCCIMGTYPE_PPM:
      if (QccIMGImagePNMWrite(image))
        {
          QccErrorAddMessage("(QccIMGImageWrite): Error calling QccIMGImagePNMWrite()");
          return(1);
        }
      break;
      
    default:
      QccErrorAddMessage("(QccIMGImageWrite): Unknown image type");
      return(1);
    }
  
  return(0);
}


int QccIMGImageCopy(QccIMGImage *image1, const QccIMGImage *image2)
{
  if ((image1 == NULL) || (image2 == NULL))
    return(0);
  
  if (QccIMGImageComponentCopy(&(image1->Y), &(image2->Y)))
    {
      QccErrorAddMessage("(QccIMGImageCopy): Error calling QccIMGImageComponentCopy()");
      return(1);
    }
  if (QccIMGImageComponentCopy(&(image1->U), &(image2->U)))
    {
      QccErrorAddMessage("(QccIMGImageCopy): Error calling QccIMGImageComponentCopy()");
      return(1);
    }
  if (QccIMGImageComponentCopy(&(image1->V), &(image2->V)))
    {
      QccErrorAddMessage("(QccIMGImageCopy): Error calling QccIMGImageComponentCopy()");
      return(1);
    }
  
  return(0);
}


static void QccIMGImageColorMean(const QccIMGImage *image,
                                 double *U_mean,
                                 double *V_mean,
                                 double *W_mean)
{
  int row, col;
  int red, green, blue;
  double U, V, W;
  
  *U_mean = 0.0;
  *V_mean = 0.0;
  *W_mean = 0.0;
  
  for (row = 0; row < image->Y.num_rows; row++)
    for (col = 0; col < image->Y.num_cols; col++)
      {
        QccIMGImageYUVtoRGB(&red, &green, &blue,
                            (image->Y.image[row][col]),
                            (image->U.image[row][col]),
                            (image->V.image[row][col]));
        QccIMGImageRGBtoModifiedUCS(&U, &V, &W,
                                    red, green, blue);
        *U_mean += U;
        *V_mean += V;
        *W_mean += W;
      }
  *U_mean /= (image->Y.num_rows * image->Y.num_cols);
  *V_mean /= (image->Y.num_rows * image->Y.num_cols);
  *W_mean /= (image->Y.num_rows * image->Y.num_cols);
  
}


double QccIMGImageColorSNR(const QccIMGImage *image1,
                           const QccIMGImage *image2)
{
  double distortion = 0.0;
  double signal_power = 0.0;
  double snr;
  int row, col;
  int red1, green1, blue1;
  int red2, green2, blue2;
  double U, V, W;
  double U_mean, V_mean, W_mean;
  int num_rows1_Y = 0;
  int num_cols1_Y = 0;
  int num_rows1_U = 0;
  int num_cols1_U = 0;
  int num_rows1_V = 0;
  int num_cols1_V = 0;
  int num_rows2_Y = 0;
  int num_cols2_Y = 0;
  int num_rows2_U = 0;
  int num_cols2_U = 0;
  int num_rows2_V = 0;
  int num_cols2_V = 0;

  if (image1 == NULL)
    return(0.0);
  if (image2 == NULL)
    return(0.0);
  
  if ((!QccIMGImageColor(image1)) ||
      (!QccIMGImageColor(image2)))
    return(0.0);
  
  if (QccIMGImageGetSizeYUV(image1,
                            &num_rows1_Y, &num_cols1_Y,
                            &num_rows1_U, &num_cols1_U,
                            &num_rows1_V, &num_cols1_V))
    return(0.0);
  if (QccIMGImageGetSizeYUV(image2,
                            &num_rows2_Y, &num_cols2_Y,
                            &num_rows2_U, &num_cols2_U,
                            &num_rows2_V, &num_cols2_V))
    return(0.0);
  
  if ((num_rows1_Y != num_rows2_Y) ||
      (num_cols1_Y != num_cols2_Y) ||
      (num_rows1_U != num_rows2_U) ||
      (num_cols1_U != num_cols2_U) ||
      (num_rows1_V != num_rows2_V) ||
      (num_cols1_V != num_cols2_V))
    return(0.0);
  
  if ((num_rows1_Y != num_rows1_U) ||
      (num_rows1_Y != num_rows1_V) ||
      (num_cols1_Y != num_cols1_U) ||
      (num_cols1_Y != num_cols1_V))
    return(0.0);

  QccIMGImageColorMean(image1,
                       &U_mean, &V_mean, &W_mean);
  
  for (row = 0; row < image1->Y.num_rows; row++)
    for (col = 0; col < image1->Y.num_cols; col++)
      {
        QccIMGImageYUVtoRGB(&red1, &green1, &blue1,
                            (image1->Y.image[row][col]),
                            (image1->U.image[row][col]),
                            (image1->V.image[row][col]));
        QccIMGImageYUVtoRGB(&red2, &green2, &blue2,
                            (image2->Y.image[row][col]),
                            (image2->U.image[row][col]),
                            (image2->V.image[row][col]));
        distortion +=
          QccIMGImageModifiedUCSColorMetric(red1, green1, blue1,
                                            red2, green2, blue2);
        
        QccIMGImageRGBtoModifiedUCS(&U, &V, &W,
                                    red1, green1, blue1);
        signal_power +=
          (U - U_mean) * (U - U_mean) +
          (V - V_mean) * (V - V_mean) +
          (W - W_mean) * (W - W_mean);
      }
  
  snr = 10 * log10(signal_power / distortion);
  
  return(snr);
}
