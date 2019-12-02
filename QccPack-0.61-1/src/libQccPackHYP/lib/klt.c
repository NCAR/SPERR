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


int QccHYPkltInitialize(QccHYPklt *klt)
{
  if (klt == NULL)
    return(0);

  QccStringMakeNull(klt->filename);
  QccStringCopy(klt->magic_num, QCCHYPKLT_MAGICNUM);
  QccGetQccPackVersion(&klt->major_version,
                       &klt->minor_version,
                       NULL);
  klt->num_bands = 0;
  klt->mean = NULL;
  klt->matrix = NULL;

  return(0);
}


int QccHYPkltAlloc(QccHYPklt *klt)
{
  if (klt == NULL)
    return(0);
  
  if (klt->mean == NULL)
    {
      if (klt->num_bands > 0)
        {
          if ((klt->mean = 
               QccVectorAlloc(klt->num_bands)) == NULL)
            {
              QccErrorAddMessage("(QccHYPkltAlloc): Error calling QccVectorAlloc()");
              return(1);
            }
        }
      else
        klt->mean = NULL;
    }
  
  if (klt->matrix == NULL)
    {
      if (klt->num_bands > 0)
        {
          if ((klt->matrix = 
               QccMatrixAlloc(klt->num_bands, 
                              klt->num_bands)) == NULL)
            {
              QccErrorAddMessage("(QccHYPkltAlloc): Error calling QccMatrixAlloc()");
              return(1);
            }
        }
      else
        klt->matrix = NULL;
    }
  
  return(0);
}


void QccHYPkltFree(QccHYPklt *klt)
{
  if (klt == NULL)
    return;
  
  if (klt->mean != NULL)
    {
      QccVectorFree(klt->mean);
      klt->mean = NULL;
    }

  if (klt->matrix != NULL)
    {
      QccMatrixFree(klt->matrix, 
                    klt->num_bands);
      klt->matrix = NULL;
    }
}


int QccHYPkltPrint(const QccHYPklt *klt)
{
  int row, col;
  
  if (QccFilePrintFileInfo(klt->filename,
                           klt->magic_num,
                           klt->major_version,
                           klt->minor_version))
    return(1);

  printf("Num Bands: %d\n",
         klt->num_bands);
  printf("\nMean Vector:\n\n");
  
  for (col = 0; col < klt->num_bands; col++)
    printf("% 10.4f ", klt->mean[col]);

  printf("\nTransform matrix:\n\n");
  
  for (row = 0; row < klt->num_bands; row++)
    {
      for (col = 0; col < klt->num_bands; col++)
        printf("% 10.4f ", klt->matrix[row][col]);
      printf("\n");
    }      
  return(0);
}


static int QccHYPkltReadHeader(FILE *infile, 
                               QccHYPklt *klt)
{
  if ((infile == NULL) || (klt == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             klt->magic_num,
                             &klt->major_version,
                             &klt->minor_version))
    {
      QccErrorAddMessage("(QccHYPkltReadHeader): Error reading magic number in %s",
                         klt->filename);
      return(1);
    }
  
  if (strcmp(klt->magic_num, QCCHYPKLT_MAGICNUM))
    {
      QccErrorAddMessage("(QccHYPkltReadHeader): %s is not of klt (%s) type",
                         klt->filename,
                         QCCHYPKLT_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d%*1[\n]", &(klt->num_bands));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccHYPkltReadHeader): Error reading number of bands in %s",
                         klt->filename);
      return(1);
    }
  
  return(0);
}


static int QccHYPkltReadData(FILE *infile, 
                             QccHYPklt *klt)
{
  int row, col;
  
  if ((infile == NULL) ||
      (klt == NULL))
    return(0);
  
  for (col = 0; col < klt->num_bands; col++)
    if (QccFileReadDouble(infile,
                          &(klt->mean[col])))
      {
        QccErrorAddMessage("(QccHYPkltReadData): Error calling QccFileReadDouble()");
        return(1);
      }

  for (row = 0; row < klt->num_bands; row++)
    for (col = 0; col < klt->num_bands; col++)
      if (QccFileReadDouble(infile,
                            &(klt->matrix[row][col])))
        {
          QccErrorAddMessage("(QccHYPkltReadData): Error calling QccFileReadDouble()");
          return(1);
        }
  
  return(0);
}


int QccHYPkltRead(QccHYPklt *klt)
{
  FILE *infile = NULL;
  
  if (klt == NULL)
    return(0);
  
  if ((infile = QccFileOpen(klt->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccHYPkltRead): Error opening %s for reading",
                         klt->filename);
      return(1);
    }
  
  if (QccHYPkltReadHeader(infile, klt))
    {
      QccErrorAddMessage("(QccHYPkltRead): Error calling QccHYPkltReadHeader()");
      return(1);
    }
  
  if (QccHYPkltAlloc(klt))
    {
      QccErrorAddMessage("(QccHYPkltRead): Error calling QccHYPkltAlloc()");
      return(1);
    }
  
  if (QccHYPkltReadData(infile, klt))
    {
      QccErrorAddMessage("(QccHYPkltRead): Error calling QccHYPkltReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


static int QccHYPkltWriteHeader(FILE *outfile, 
                                const QccHYPklt *klt)
{
  if ((outfile == NULL) || (klt == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCHYPKLT_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d\n",
          klt->num_bands);
  if (ferror(outfile))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccHYPkltWriteHeader): Error writing header to %s",
                     klt->filename);
  return(1);
}


static int QccHYPkltWriteData(FILE *outfile,
                              const QccHYPklt *klt)
{
  int row, col;
  
  if ((klt == NULL) ||
      (outfile == NULL))
    return(0);
  
  for (col = 0; col < klt->num_bands; col++)
    if (QccFileWriteDouble(outfile,
                           klt->mean[col]))
      {
        QccErrorAddMessage("(QccHYPkltWriteData): Error calling QccFileWriteDouble()");
        return(1);
      }
  
  for (row = 0; row < klt->num_bands; row++)
    for (col = 0; col < klt->num_bands; col++)
      if (QccFileWriteDouble(outfile,
                             klt->matrix[row][col]))
        {
          QccErrorAddMessage("(QccHYPkltWriteData): Error calling QccFileWriteDouble()");
          return(1);
        }
  
  return(0);
}


int QccHYPkltWrite(const QccHYPklt *klt)
{
  FILE *outfile;
  
  if (klt == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(klt->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccHYPkltWrite): Error opening %s for writing",
                         klt->filename);
      return(1);
    }
  
  if (QccHYPkltWriteHeader(outfile, klt))
    {
      QccErrorAddMessage("(QccHYPkltWrite): Error calling QccHYPkltWriteHeader()");
      return(1);
    }
  if (QccHYPkltWriteData(outfile, klt))
    {
      QccErrorAddMessage("(QccHYPkltWrite): Error calling QccHYPkltWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


static int QccHYPkltMean(const QccIMGImageCube *image,
                         QccVector mean)
{
  int num_bands;
  int row, col, band;

  num_bands = image->num_frames;

  for (band = 0; band < num_bands; band++)
    {
      mean[band] = 0;

      for (row = 0; row < image->num_rows; row++)
        for (col = 0; col < image->num_cols; col++)
          mean[band] +=
            image->volume[band][row][col];

      mean[band] /=
        (image->num_rows * image->num_cols);
    }

  return(0);
}


static int QccHYPkltCovariance(const QccIMGImageCube *image,
                               const QccVector mean,
                               QccMatrix covariance)
{
  int num_bands;
  int row, col;
  int covariance_row, covariance_col;

  num_bands = image->num_frames;

  for (covariance_row = 0; covariance_row < num_bands; covariance_row++)
    for (covariance_col = 0; covariance_col < num_bands; covariance_col++)
      if (covariance_col >= covariance_row)
        {
          covariance[covariance_row][covariance_col] = 0;
          
          for (row = 0; row < image->num_rows; row++)
            for (col = 0; col < image->num_cols; col++)
              covariance[covariance_row][covariance_col] +=
                image->volume[covariance_row][row][col] *
                image->volume[covariance_col][row][col];

          covariance[covariance_row][covariance_col] /=
            (image->num_rows * image->num_cols);
          covariance[covariance_row][covariance_col] -=
            mean[covariance_row] * mean[covariance_col];
          
        }
      else
        covariance[covariance_row][covariance_col] =
          covariance[covariance_col][covariance_row];

  return(0);
}


int QccHYPkltTrain(const QccIMGImageCube *image,
                   QccHYPklt *klt)
{
  int return_value;
  QccMatrix covariance = NULL;
  int num_bands;

  if (image == NULL)
    return(0);
  if (klt == NULL)
    return(0);

  num_bands = image->num_frames;

  if (num_bands != klt->num_bands)
    {
      QccErrorAddMessage("(QccHYPkltTrain): Number of bands in KLT (%d) does not match that of image cube (%d)",
                         klt->num_bands,
                         num_bands);
      goto Error;
    }

  if (QccHYPkltMean(image, klt->mean))
    {
      QccErrorAddMessage("(QccHYPkltTrain): Error calling QccHYPkltMean()");
      goto Error;
    }

  if ((covariance = QccMatrixAlloc(num_bands, num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPkltTrain): Error calling QccMatrixAlloc()");
      goto Error;
    }

  if (QccHYPkltCovariance(image, klt->mean, covariance))
    {
      QccErrorAddMessage("(QccHYPkltTrain): Error calling QccHYPkltCovariance()");
      goto Error;
    }

  if (QccMatrixSVD(covariance,
                   num_bands,
                   num_bands,
                   klt->matrix,
                   NULL,
                   NULL))
    {
      QccErrorAddMessage("(QccHYPkltTrain): Error calling QccMatrixSVD()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(covariance, num_bands);
  return(return_value);
}


int QccHYPkltTransform(QccIMGImageCube *image,
                       const QccHYPklt *klt,
                       int num_pcs)
{
  int return_value;
  int num_bands;
  int band;
  int row, col;
  QccVector pixel_vector1 = NULL;
  QccVector pixel_vector2 = NULL;
  QccMatrix transform_matrix = NULL;

  if (image == NULL)
    return(0);
  if (klt == NULL)
    return(0);
  if (num_pcs <= 0)
    return(0);

  num_bands = image->num_frames;
  
  if (klt->num_bands != num_bands)
    {
      QccErrorAddMessage("(QccHYPkltTransform): Number of bands in KLT (%d) does not match that of image cube (%d)",
                         klt->num_bands,
                         num_bands);
      goto Error;
    }

  if (num_pcs > num_bands)
    {
      QccErrorAddMessage("(QccHYPkltTransform): Number of PCs greater than number of bands");
      goto Error;
    }

  if ((pixel_vector1 = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPkltTransform): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((pixel_vector2 = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPkltTransform): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((transform_matrix = QccMatrixAlloc(num_bands, num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPkltTransform): Error calling QccMatrixAlloc()");
      goto Error;
    }

  if (QccMatrixTranspose(klt->matrix,
                         transform_matrix,
                         num_bands,
                         num_bands))
    {
      QccErrorAddMessage("(QccHYPkltTransform): Error calling QccMatrixTranspose()");
      goto Error;
    }

  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      {
        for (band = 0; band < num_bands; band++)
          pixel_vector1[band] = 
            image->volume[band][row][col] - klt->mean[band];

        if (QccMatrixVectorMultiply(transform_matrix,
                                    pixel_vector1,
                                    pixel_vector2,
                                    num_bands,
                                    num_bands))
          {
            QccErrorAddMessage("(QccHYPkltTransform): Error calling QccMatrixVectorMultiply()");
            goto Error;
          }

        for (band = 0; band < num_bands; band++)
          image->volume[band][row][col] = pixel_vector2[band];
      }

  if (num_pcs != num_bands)
    if (QccIMGImageCubeResize(image,
                              num_pcs,
                              image->num_rows,
                              image->num_cols))
      {
        QccErrorAddMessage("(QccHYPkltTransform): Error calling QccIMGImageCubeResize()");
        goto Error;
      }

  QccIMGImageCubeSetMaxMin(image);

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(pixel_vector1);
  QccVectorFree(pixel_vector2);
  QccMatrixFree(transform_matrix, num_bands);
  return(return_value);
}


int QccHYPkltInverseTransform(QccIMGImageCube *image,
                              const QccHYPklt *klt)
{
  int return_value;
  int num_bands;
  int band;
  int row, col;
  QccVector pixel_vector1 = NULL;
  QccVector pixel_vector2 = NULL;

  if (image == NULL)
    return(0);
  if (klt == NULL)
    return(0);

  num_bands = klt->num_bands;
  
  if (num_bands < image->num_frames)
    {
      QccErrorAddMessage("(QccHYPkltInverseTransform): Image cube has too many bands for KLT");
      goto Error;
    }

  if (num_bands != image->num_frames)
    if (QccIMGImageCubeResize(image,
                              num_bands,
                              image->num_rows,
                              image->num_cols))
    {
      QccErrorAddMessage("(QccHYPkltInverseTransform): Error calling QccIMGImageCubeResize()");
      goto Error;
    }

  if ((pixel_vector1 = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPkltInverseTransform): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((pixel_vector2 = QccVectorAlloc(num_bands)) == NULL)
    {
      QccErrorAddMessage("(QccHYPkltInverseTransform): Error calling QccVectorAlloc()");
      goto Error;
    }

  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      {
        for (band = 0; band < num_bands; band++)
          pixel_vector1[band] = image->volume[band][row][col];

        if (QccMatrixVectorMultiply(klt->matrix,
                                    pixel_vector1,
                                    pixel_vector2,
                                    num_bands,
                                    num_bands))
          {
            QccErrorAddMessage("(QccHYPkltInverseTransform): Error calling QccMatrixVectorMultiply()");
            goto Error;
          }

        for (band = 0; band < num_bands; band++)
          image->volume[band][row][col] =
            pixel_vector2[band] + klt->mean[band];
      }

  QccIMGImageCubeSetMaxMin(image);

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(pixel_vector1);
  QccVectorFree(pixel_vector2);
  return(return_value);
}
