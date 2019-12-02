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


static QccIMGImageArray QccIMGImageArrayAlloc(int num_rows, int num_cols)
{
  QccIMGImageArray new_array;
  int row;
  
  if ((num_rows <= 0) && (num_cols <= 0))
    return(NULL);
  
  if ((new_array = (double **)malloc(sizeof(double *)*num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageArrayAlloc): Error allocating memory");
      return(NULL);
    }
  
  for (row = 0; row < num_rows; row++)
    if ((new_array[row] = (double *)malloc(sizeof(double)*num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccIMGImageArrayAlloc): Error allocating memory");
        return(NULL);
      }
  
  return(new_array);
}


static void QccIMGImageArrayFree(QccIMGImageArray image_array, int num_rows)
{
  int row;
  
  if (image_array != NULL)
    {
      for (row = 0; row < num_rows; row++)
        if (image_array[row] != NULL)
          QccFree(image_array[row]);
      QccFree(image_array);
    }
}


int QccIMGImageComponentInitialize(QccIMGImageComponent *image_component)
{
  if (image_component == NULL)
    return(0);

  QccStringMakeNull(image_component->filename);
  QccStringCopy(image_component->magic_num, QCCIMGIMAGECOMPONENT_MAGICNUM);
  QccGetQccPackVersion(&image_component->major_version,
                       &image_component->minor_version,
                       NULL);
  image_component->num_rows = 0;
  image_component->num_cols = 0;
  image_component->min_val = 0.0;
  image_component->max_val = 0.0;
  image_component->image = NULL;

  return(0);
}


int QccIMGImageComponentAlloc(QccIMGImageComponent *image_component)
{
  if (image_component == NULL)
    return(0);
  
  if (image_component->image == NULL)
    {
      if ((image_component->num_rows > 0) ||
          (image_component->num_cols > 0))
        {
          if ((image_component->image = 
               QccIMGImageArrayAlloc(image_component->num_rows, 
                                     image_component->num_cols)) == NULL)
            {
              QccErrorAddMessage("(QccIMGImageComponentAlloc): Error calling QccIMGImageArrayAlloc()");
              return(1);
            }
        }
      else
        image_component->image = NULL;
    }
  
  return(0);
}


void QccIMGImageComponentFree(QccIMGImageComponent *image_component)
{
  if (image_component == NULL)
    return;
  
  if (image_component->image != NULL)
    {
      QccIMGImageArrayFree(image_component->image, 
                           image_component->num_rows);
      image_component->image = NULL;
    }
}


int QccIMGImageComponentPrint(const QccIMGImageComponent *image_component)
{
  int row, col;
  
  if (QccFilePrintFileInfo(image_component->filename,
                           image_component->magic_num,
                           image_component->major_version,
                           image_component->minor_version))
    return(1);

  printf("Size: %dx%d\n",
         image_component->num_cols,
         image_component->num_rows);
  printf("Min. value: % 10.4f\n", image_component->min_val);
  printf("Max. value: % 10.4f\n", image_component->max_val);
  printf("\nImage array:\n\n");
  
  for (row = 0; row < image_component->num_rows; row++)
    {
      for (col = 0; col < image_component->num_cols; col++)
        printf("% 10.4f ", image_component->image[row][col]);
      printf("\n");
    }      
  return(0);
}


int QccIMGImageComponentSetMin(QccIMGImageComponent *image_component)
{
  double min = MAXDOUBLE;
  int row, col;
  
  if (image_component == NULL)
    return(0);
  if (image_component->image == NULL)
    return(0);
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      if (image_component->image[row][col] < min)
        min = image_component->image[row][col];
  
  image_component->min_val = min;
  
  return(0);
}


int QccIMGImageComponentSetMax(QccIMGImageComponent *image_component)
{
  double max = -MAXDOUBLE;
  int row, col;
  
  if (image_component == NULL)
    return(0);
  if (image_component->image == NULL)
    return(0);
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      if (image_component->image[row][col] > max)
        max = image_component->image[row][col];
  
  image_component->max_val = max;
  
  return(0);
}


int QccIMGImageComponentSetMaxMin(QccIMGImageComponent *image_component)
{
  if (QccIMGImageComponentSetMax(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentSetMaxMin): Error calling QccIMGImageComponentSetMax()");
      return(1);
    }

  if (QccIMGImageComponentSetMin(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentSetMaxMin): Error calling QccIMGImageComponentSetMin()");
      return(1);
    }

  return(0);
}


int QccIMGImageComponentResize(QccIMGImageComponent *image_component,
                               int num_rows,
                               int num_cols)
{
  if (image_component == NULL)
    return(0);

  if ((image_component->image =
       (QccIMGImageArray)QccMatrixResize((QccMatrix)image_component->image,
                                         image_component->num_rows,
                                         image_component->num_cols,
                                         num_rows,
                                         num_cols)) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentResize): Error calling QccMatrixResize()");
      return(1);
    }
  
  image_component->num_rows = num_rows;
  image_component->num_cols = num_cols;
  
  QccIMGImageComponentSetMaxMin(image_component);

  return(0);
}


static int QccIMGImageComponentReadHeader(FILE *infile, 
                                          QccIMGImageComponent
                                          *image_component)
{
  if ((infile == NULL) || (image_component == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             image_component->magic_num,
                             &image_component->major_version,
                             &image_component->minor_version))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): Error reading magic number in %s",
                         image_component->filename);
      return(1);
    }
  
  if (strcmp(image_component->magic_num, QCCIMGIMAGECOMPONENT_MAGICNUM))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): %s is not of image_component (%s) type",
                         image_component->filename,
                         QCCIMGIMAGECOMPONENT_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(image_component->num_cols));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): Error reading number of columns in %s",
                         image_component->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): Error reading number of columns in %s",
                         image_component->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(image_component->num_rows));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): Error reading number of rows in %s",
                         image_component->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): Error reading number of rows in %s",
                         image_component->filename);
      return(1);
    }
  
  fscanf(infile, "%lf", &(image_component->min_val));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): Error reading number of rows in %s",
                         image_component->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): Error reading number of rows in %s",
                         image_component->filename);
      return(1);
    }
  
  fscanf(infile, "%lf%*1[\n]", &(image_component->max_val));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccIMGImageComponentReadHeader): Error reading number of rows in %s",
                         image_component->filename);
      return(1);
    }
  
  return(0);
}


static int QccIMGImageComponentReadData(FILE *infile, 
                                        QccIMGImageComponent *image_component)
{
  int row, col;
  
  if ((infile == NULL) ||
      (image_component == NULL))
    return(0);
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      if (QccFileReadDouble(infile,
                            &(image_component->image[row][col])))
        {
          QccErrorAddMessage("(QccIMGImageComponentReadData): Error calling QccFileReadDouble()",
                             image_component->filename);
          return(1);
        }
  
  return(0);
}


int QccIMGImageComponentRead(QccIMGImageComponent *image_component)
{
  FILE *infile = NULL;
  
  if (image_component == NULL)
    return(0);
  
  if ((infile = QccFileOpen(image_component->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentRead): Error opening %s for reading",
                         image_component->filename);
      return(1);
    }
  
  if (QccIMGImageComponentReadHeader(infile, image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentRead): Error calling QccIMGImageComponentReadHeader()");
      return(1);
    }
  
  if (QccIMGImageComponentAlloc(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentRead): Error calling QccIMGImageComponentAlloc()");
      return(1);
    }
  
  if (QccIMGImageComponentReadData(infile, image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentRead): Error calling QccIMGImageComponentReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


static int QccIMGImageComponentWriteHeader(FILE *outfile, 
                                           const QccIMGImageComponent 
                                           *image_component)
{
  if ((outfile == NULL) || (image_component == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCIMGIMAGECOMPONENT_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d %d\n",
          image_component->num_cols,
          image_component->num_rows);
  if (ferror(outfile))
    goto Error;
  
  fprintf(outfile, "% 16.9e % 16.9e\n",
          image_component->min_val,
          image_component->max_val);
  if (ferror(outfile))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccIMGImageComponentWriteHeader): Error writing header to %s",
                     image_component->filename);
  return(1);
}


static int QccIMGImageComponentWriteData(FILE *outfile,
                                         const QccIMGImageComponent
                                         *image_component)
{
  int row, col;
  
  if ((image_component == NULL) ||
      (outfile == NULL))
    return(0);
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      if (QccFileWriteDouble(outfile,
                             image_component->image[row][col]))
        {
          QccErrorAddMessage("(QccIMGImageComponentWriteData): Error calling QccFileWriteDouble()");
          return(1);
        }
  
  return(0);
}


int QccIMGImageComponentWrite(const QccIMGImageComponent *image_component)
{
  FILE *outfile;
  
  if (image_component == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(image_component->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentWrite): Error opening %s for writing",
                         image_component->filename);
      return(1);
    }
  
  if (QccIMGImageComponentWriteHeader(outfile, image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentWrite): Error calling QccIMGImageComponentWriteHeader()");
      return(1);
    }
  if (QccIMGImageComponentWriteData(outfile, image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentWrite): Error calling QccIMGImageComponentWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


double QccIMGImageComponentClipPixel(double pixel_value)
{
  double new_value;

  if (pixel_value < 0)
    new_value = 0;
  else if (pixel_value > 255)
    new_value = 255;
  else
    new_value = pixel_value;

  return(new_value);
}


int QccIMGImageComponentClip(QccIMGImageComponent *image_component)
{
  int row, col;

  if (image_component == NULL)
    return(0);
  if (image_component->image == NULL)
    return(0);

  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      image_component->image[row][col] =
        QccIMGImageComponentClipPixel(image_component->image[row][col]);

  if (QccIMGImageComponentSetMaxMin(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentClip): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccIMGImageComponentNormalize(QccIMGImageComponent *image_component)
{
  int row, col;
  double range;
  double min;

  if (image_component == NULL)
    return(0);
  if (image_component->image == NULL)
    return(0);

  range = image_component->max_val - image_component->min_val;
  min = image_component->min_val;

  if (range == 0.0)
    return(0);

  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      image_component->image[row][col] =
        255 * (image_component->image[row][col] - min) /
        range;

  if (QccIMGImageComponentSetMaxMin(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentNormalize): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccIMGImageComponentAbsoluteValue(QccIMGImageComponent *image_component)
{
  int row, col;

  if (image_component == NULL)
    return(0);
  if (image_component->image == NULL)
    return(0);

  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      image_component->image[row][col] =
        fabs(image_component->image[row][col]);

  if (QccIMGImageComponentSetMaxMin(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentAbsoluteValue): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


double QccIMGImageComponentMean(const QccIMGImageComponent *image_component)
{
  double sum = 0.0;
  int row, col;
  
  if (image_component == NULL)
    return(0.0);
  if (image_component->image == NULL)
    return(0.0);

  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      sum += image_component->image[row][col];
  
  sum /= image_component->num_rows * image_component->num_cols;

  return(sum);
}


double QccIMGImageComponentShapeAdaptiveMean(const QccIMGImageComponent
                                             *image_component,
                                             const QccIMGImageComponent
                                             *alpha_mask)
{
  double sum = 0.0;
  int row, col;
  int cnt = 0;

  if (image_component == NULL)
    return(0.0);
  if (image_component->image == NULL)
    return(0.0);

  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      if (!QccAlphaTransparent(alpha_mask->image[row][col]))
        {
          sum += image_component->image[row][col];
          cnt++;
        }
  
  sum /= cnt;

  return(sum);
}


double QccIMGImageComponentVariance(const
                                    QccIMGImageComponent *image_component)
{
  double sum = 0.0;
  double mean;
  int row, col;
  
  if (image_component == NULL)
    return(0.0);
  if (image_component->image == NULL)
    return(0.0);

  mean = QccIMGImageComponentMean(image_component);
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      sum +=
        (image_component->image[row][col] - mean)*
        (image_component->image[row][col] - mean);
  
  sum /= image_component->num_rows * image_component->num_cols;
  return(sum);
}


double QccIMGImageComponentShapeAdaptiveVariance(const QccIMGImageComponent
                                                 *image_component,
                                                 const QccIMGImageComponent
                                                 *alpha_mask)
{
  double sum = 0.0;
  double mean;
  int row, col;
  int cnt = 0;

  if (image_component == NULL)
    return(0.0);
  if (image_component->image == NULL)
    return(0.0);

  if (alpha_mask == NULL)
    return(QccIMGImageComponentVariance(image_component));
  if (alpha_mask->image == NULL)
    return(QccIMGImageComponentVariance(image_component));

  mean = QccIMGImageComponentShapeAdaptiveMean(image_component, alpha_mask);
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      if (!QccAlphaTransparent(alpha_mask->image[row][col]))
        {
          sum +=
            (image_component->image[row][col] - mean)*
            (image_component->image[row][col] - mean);
          cnt++;
        }
  
  sum /= cnt;
  return(sum);
}


int QccIMGImageComponentSubtractMean(QccIMGImageComponent 
                                     *image_component,
                                     double *mean,
                                     const QccSQScalarQuantizer *quantizer)
{
  int row, col;
  int partition;
  
  double mean1;
  
  if (image_component == NULL)
    return(0);
  if (image_component->image == NULL)
    return(0);
  
  mean1 = QccIMGImageComponentMean(image_component);
  
  if (quantizer != NULL)
    {
      if (QccSQScalarQuantization(mean1, quantizer, NULL, &partition))
        {
          QccErrorAddMessage("(QccIMGImageComponentSubtractMean): Error calling QccSQScalarQuantization()");
          return(1);
        }
      if (QccSQInverseScalarQuantization(partition, quantizer, &mean1))
        {
          QccErrorAddMessage("(QccIMGImageComponentSubtractMean): Error calling QccSQInverseScalarQuantization()");
          return(1);
        }
    }
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      image_component->image[row][col] -= mean1;
  
  if (mean != NULL)
    *mean = mean1;
  
  if (QccIMGImageComponentSetMaxMin(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentSubtractMean): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageComponentAddMean(QccIMGImageComponent *image_component,
                                double mean)
{
  int row, col;
  
  if (image_component == NULL)
    return(0);
  if (image_component->image == NULL)
    return(0);
  
  for (row = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++)
      image_component->image[row][col] += mean;
  
  if (QccIMGImageComponentSetMaxMin(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentAddMean): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }
  
  return(0);
}


double QccIMGImageComponentMSE(const QccIMGImageComponent *image_component1,
                               const QccIMGImageComponent *image_component2)
{
  double sum = 0.0;
  int row, col;
  
  if (image_component1 == NULL)
    return(0.0);
  if (image_component2 == NULL)
    return(0.0);
  if (image_component1->image == NULL)
    return(0.0);
  if (image_component2->image == NULL)
    return(0.0);
  
  if ((image_component2->num_rows != image_component1->num_rows) ||
      (image_component2->num_cols != image_component1->num_cols))
    return(0.0);

  for (row = 0; row < image_component1->num_rows; row++)
    for (col = 0; col < image_component1->num_cols; col++)
      sum +=
        (image_component1->image[row][col] - 
         image_component2->image[row][col])*
        (image_component1->image[row][col] - 
         image_component2->image[row][col]);
  
  sum /= image_component1->num_rows * image_component1->num_cols;

  return(sum);
}


double QccIMGImageComponentShapeAdaptiveMSE(const QccIMGImageComponent
                                            *image_component1,
                                            const QccIMGImageComponent
                                            *image_component2,
                                            const QccIMGImageComponent
                                            *alpha_mask)
{
  double sum = 0.0;
  int row, col;
  int cnt = 0;

  if (image_component1 == NULL)
    return(0.0);
  if (image_component2 == NULL)
    return(0.0);
  if ((image_component1->image == NULL) || (image_component2->image == NULL))
    return(0.0);
  
  if ((image_component2->num_rows != image_component1->num_rows) ||
      (image_component2->num_cols != image_component1->num_cols))
    return(0.0);

  if (alpha_mask == NULL)
    return(QccIMGImageComponentMSE(image_component1,
                                   image_component2));
  if (alpha_mask->image == NULL)
    return(QccIMGImageComponentMSE(image_component1,
                                   image_component2));

  if ((alpha_mask->num_rows != image_component1->num_rows) ||
      (alpha_mask->num_cols != image_component1->num_cols))
    return(0.0);

  for (row = 0; row < image_component1->num_rows; row++)
    for (col = 0; col < image_component1->num_cols; col++)
      if (!QccAlphaTransparent(alpha_mask->image[row][col]))
        {
          sum +=
            (image_component1->image[row][col] - 
             image_component2->image[row][col])*
            (image_component1->image[row][col] - 
             image_component2->image[row][col]);
          cnt++;
        }
  
  sum /= cnt;
  return(sum);
}


double QccIMGImageComponentMAE(const QccIMGImageComponent *image_component1,
                               const QccIMGImageComponent *image_component2)
{
  int row, col;
  double mae = 0;

  if (image_component1 == NULL)
    return(0.0);
  if (image_component2 == NULL)
    return(0.0);
  if ((image_component1->image == NULL) || (image_component2->image == NULL))
    return(0.0);
  
  if ((image_component2->num_rows != image_component1->num_rows) ||
      (image_component2->num_cols != image_component1->num_cols))
    return(0.0);

  for (row = 0; row < image_component1->num_rows; row++)
    for (col = 0; col < image_component1->num_cols; col++)
      mae = QccMathMax(mae,
                       fabs(image_component1->image[row][col] - 
                            image_component2->image[row][col]));

  return(mae);
}


double QccIMGImageComponentShapeAdaptiveMAE(const QccIMGImageComponent
                                            *image_component1,
                                            const QccIMGImageComponent
                                            *image_component2,
                                            const QccIMGImageComponent
                                            *alpha_mask)
{
  int row, col;
  double mae = 0;

  if (image_component1 == NULL)
    return(0.0);
  if (image_component2 == NULL)
    return(0.0);
  if ((image_component1->image == NULL) || (image_component2->image == NULL))
    return(0.0);
  
  if ((image_component2->num_rows != image_component1->num_rows) ||
      (image_component2->num_cols != image_component1->num_cols))
    return(0.0);

  if (alpha_mask == NULL)
    return(QccIMGImageComponentMAE(image_component1,
                                   image_component2));
  if (alpha_mask->image == NULL)
    return(QccIMGImageComponentMAE(image_component1,
                                   image_component2));

  if ((alpha_mask->num_rows != image_component1->num_rows) ||
      (alpha_mask->num_cols != image_component1->num_cols))
    return(0.0);

  for (row = 0; row < image_component1->num_rows; row++)
    for (col = 0; col < image_component1->num_cols; col++)
      if (!QccAlphaTransparent(alpha_mask->image[row][col]))
        mae = QccMathMax(mae,
                         fabs(image_component1->image[row][col] - 
                              image_component2->image[row][col]));

  return(mae);
}


int QccIMGImageComponentExtractBlock(const
                                     QccIMGImageComponent *image_component,
                                     int image_row, int image_col,
                                     QccMatrix block,
                                     int block_num_rows, int block_num_cols)
{
  int block_row, block_col;
  int current_image_row, current_image_col;

  for (block_row = 0; block_row < block_num_rows; block_row++)
    for (block_col = 0; block_col < block_num_cols; block_col++)
      {
        current_image_row = image_row + block_row;
        current_image_col = image_col + block_col;
        block[block_row][block_col] =
          ((current_image_row >= 0) && 
           (current_image_row < image_component->num_rows) &&
           (current_image_col >= 0) &&
           (current_image_col < image_component->num_cols)) ?
          image_component->image[current_image_row][current_image_col] : 0;
      }

  return(0);
}


int QccIMGImageComponentInsertBlock(QccIMGImageComponent *image_component,
                                    int image_row, int image_col,
                                    const QccMatrix block,
                                    int block_num_rows, int block_num_cols)
{
  int block_row, block_col;
  int current_image_row, current_image_col;

  for (block_row = 0; block_row < block_num_rows; block_row++)
    for (block_col = 0; block_col < block_num_cols; block_col++)
      {
        current_image_row = image_row + block_row;
        current_image_col = image_col + block_col;
        if ((current_image_row >= 0) && 
            (current_image_row < image_component->num_rows) &&
            (current_image_col >= 0) &&
            (current_image_col < image_component->num_cols)) 
          image_component->image[current_image_row][current_image_col] =
            block[block_row][block_col];
      }

  if (QccIMGImageComponentSetMaxMin(image_component))
    {
      QccErrorAddMessage("(QccIMGImageComponentInsertBlock): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccIMGImageComponentCopy(QccIMGImageComponent *image_component1,
                             const QccIMGImageComponent *image_component2)
{
  int row, col;
  
  if ((image_component1 == NULL) ||
      (image_component2 == NULL))
    return(0);
  
  if ((image_component2->image == NULL) ||
      (image_component2->num_rows <= 0) ||
      (image_component2->num_cols <= 0))
    return(0);

  if (image_component1->image == NULL)
    {
      image_component1->num_rows = image_component2->num_rows;
      image_component1->num_cols = image_component2->num_cols;
      if (QccIMGImageComponentAlloc(image_component1))
        {
          QccErrorAddMessage("(QccIMGImageComponentCopy): Error calling QccIMGImageComponentAlloc()");
          return(1);
        }
    }
  else
    {
      if ((image_component1->num_rows != image_component2->num_rows) ||
          (image_component1->num_cols != image_component2->num_cols))
        {
          QccErrorAddMessage("(QccIMGImageComponentCopy): Image-component arrays have different sizes");
          return(1);
        }
    }

  for (row = 0; row < image_component1->num_rows; row++)
    for (col = 0; col < image_component1->num_cols; col++)
      image_component1->image[row][col] = 
        image_component2->image[row][col];
  
  if (QccIMGImageComponentSetMaxMin(image_component1))
    {
      QccErrorAddMessage("(QccIMGImageComponentCopy): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
  
}


int QccIMGImageComponentAdd(const QccIMGImageComponent *image_component1,
                            const QccIMGImageComponent *image_component2,
                            QccIMGImageComponent *image_component3)
{
  int row, col;

  if (image_component1 == NULL)
    return(0);
  if (image_component2 == NULL)
    return(0);
  if (image_component3 == NULL)
    return(0);

  if (image_component1->image == NULL)
    return(0);
  if (image_component2->image == NULL)
    return(0);
  if (image_component3->image == NULL)
    return(0);

  if ((image_component1->num_rows != image_component2->num_rows) ||
      (image_component1->num_cols != image_component2->num_cols) ||
      (image_component1->num_rows != image_component3->num_rows) ||
      (image_component1->num_cols != image_component3->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentSubtract): Image components must have same number of rows and columns");
      return(1);
    }

  for (row = 0; row < image_component1->num_rows; row++)
    for (col = 0; col < image_component1->num_cols; col++)
      image_component3->image[row][col] =
        image_component1->image[row][col] +
        image_component2->image[row][col];

  if (QccIMGImageComponentSetMaxMin(image_component3))
    {
      QccErrorAddMessage("(QccIMGImageComponentAdd): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccIMGImageComponentSubtract(const QccIMGImageComponent *image_component1,
                                 const QccIMGImageComponent *image_component2,
                                 QccIMGImageComponent *image_component3)
{
  int row, col;

  if (image_component1 == NULL)
    return(0);
  if (image_component2 == NULL)
    return(0);
  if (image_component3 == NULL)
    return(0);

  if (image_component1->image == NULL)
    return(0);
  if (image_component2->image == NULL)
    return(0);
  if (image_component3->image == NULL)
    return(0);

  if ((image_component1->num_rows != image_component2->num_rows) ||
      (image_component1->num_cols != image_component2->num_cols) ||
      (image_component1->num_rows != image_component3->num_rows) ||
      (image_component1->num_cols != image_component3->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentSubtract): Image components must have same number of rows and columns");
      return(1);
    }

  for (row = 0; row < image_component1->num_rows; row++)
    for (col = 0; col < image_component1->num_cols; col++)
      image_component3->image[row][col] =
        image_component1->image[row][col] -
        image_component2->image[row][col];

  if (QccIMGImageComponentSetMaxMin(image_component3))
    {
      QccErrorAddMessage("(QccIMGImageComponentSubtract): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


static int QccIMGImageComponentExtractSubpixelClip(int value,
                                                   int min_value,
                                                   int max_value)
{
  if (value < min_value)
    value = min_value;
  if (value > max_value)
    value = max_value;

  return(value);
}


int QccIMGImageComponentExtractSubpixel(const QccIMGImageComponent
                                        *image_component,
                                        double x,
                                        double y,
                                        double *subpixel)
{
  int top;
  int bottom;
  int left;
  int right;
  double alpha, beta;
  double gamma;

  left =
    QccIMGImageComponentExtractSubpixelClip((int)floor(x),
                                            0,
                                            image_component->num_cols - 1);
  top =
    QccIMGImageComponentExtractSubpixelClip((int)floor(y),
                                            0,
                                            image_component->num_rows - 1);
  right =
    QccIMGImageComponentExtractSubpixelClip((int)ceil(x),
                                            0,
                                            image_component->num_cols - 1);
  bottom =
    QccIMGImageComponentExtractSubpixelClip((int)ceil(y),
                                            0,
                                            image_component->num_rows - 1);

  if (left == right)
    {
      alpha = image_component->image[top][left];
      beta = image_component->image[bottom][left];
    }
  else
    {
      gamma = (x - left) / (right - left);

      alpha =
        (1 - gamma) * image_component->image[top][left] +
        gamma * image_component->image[top][right];

      beta =
        (1 - gamma) * image_component->image[bottom][left] +
        gamma * image_component->image[bottom][right];
    }

  if (top == bottom)
    *subpixel = alpha;
  else
    {
      gamma = (y - top) / (bottom - top);
      *subpixel =
        (1 - gamma) * alpha + gamma * beta;
    }

  return(0);
}


int QccIMGImageComponentInterpolateBilinear(const QccIMGImageComponent
                                            *image_component1,
                                            QccIMGImageComponent
                                            *image_component2)
{
  int row1, col1;
  int row2, col2;

  if (image_component1 == NULL)
    return(0);
  if (image_component2 == NULL)
    return(0);

  if (image_component1->image == NULL)
    return(0);
  if (image_component2->image == NULL)
    return(0);

  if ((image_component2->num_rows != 2 * image_component1->num_rows) ||
      (image_component2->num_cols != 2 * image_component1->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentInterpolateBilinear): Destination image size must be twice that of source image");
      return(1);
    }
      
  for (row1 = 0, row2 = 0; row1 < image_component1->num_rows;
       row1++, row2 += 2)
    for (col1 = 0, col2 = 0; col1 < image_component1->num_cols;
         col1++, col2 += 2)
      image_component2->image[row2][col2] =
        image_component1->image[row1][col1];

  for (row2 = 1; row2 < image_component2->num_rows - 1; row2 += 2)
    for (col2 = 0; col2 < image_component2->num_cols; col2 += 2)
      image_component2->image[row2][col2] =
        (image_component2->image[row2 - 1][col2] +
         image_component2->image[row2 + 1][col2]) / 2;

  for (col2 = 1; col2 < image_component2->num_cols - 1; col2 +=2)
    for (row2 = 0; row2 < image_component2->num_rows; row2++)
      image_component2->image[row2][col2] =
        (image_component2->image[row2][col2 - 1] +
         image_component2->image[row2][col2 + 1]) / 2;

  for (col2 = 0; col2 < image_component2->num_cols; col2++)
    image_component2->image[image_component2->num_rows - 1][col2] =
      image_component2->image[image_component2->num_rows - 2][col2];
  for (row2 = 0; row2 < image_component2->num_rows; row2++)
    image_component2->image[row2][image_component2->num_cols - 1] = 
      image_component2->image[row2][image_component2->num_cols - 2];

  if (QccIMGImageComponentSetMaxMin(image_component2))
    {
      QccErrorAddMessage("(QccIMGImageComponentInterpolateBilinear): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccIMGImageComponentInterpolateFilter(const QccIMGImageComponent
                                          *image_component1,
                                          QccIMGImageComponent
                                          *image_component2,
                                          const QccFilter *filter)
{
  int return_value;
  int row1, col1;
  int row2, col2;
  QccVector input_vector = NULL;
  QccVector output_vector = NULL;

  if (image_component1 == NULL)
    return(0);
  if (image_component2 == NULL)
    return(0);
  if (filter == NULL)
    return(0);

  if (image_component1->image == NULL)
    return(0);
  if (image_component2->image == NULL)
    return(0);

  if (filter->causality != QCCFILTER_SYMMETRICHALF)
    {
      QccErrorAddMessage("(QccIMGImageComponentInterpolateFilter): Filter must be half-sample symmetric");
      goto Error;
    }

  if ((image_component2->num_rows != 2 * image_component1->num_rows) ||
      (image_component2->num_cols != 2 * image_component1->num_cols))
    {
      QccErrorAddMessage("(QccIMGImageComponentInterpolateFilter): Destination image size must be twice that of source image");
      goto Error;
    }
      
  if ((input_vector =
       QccVectorAlloc(QccMathMax(image_component2->num_rows,
                                 image_component2->num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentInterpolateFilter): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((output_vector =
       QccVectorAlloc(QccMathMax(image_component2->num_rows,
                                 image_component2->num_cols))) == NULL)
    {
      QccErrorAddMessage("(QccIMGImageComponentInterpolateFilter): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row1 = 0, row2 = 0; row1 < image_component1->num_rows;
       row1++, row2 += 2)
    for (col1 = 0, col2 = 0; col1 < image_component1->num_cols;
         col1++, col2 += 2)
      image_component2->image[row2][col2] =
        image_component1->image[row1][col1];
  
  for (col1 = 0; col1 < image_component1->num_cols; col1++)
    {
      for (row1 = 0; row1 < image_component1->num_rows; row1++)
        input_vector[row1] = image_component1->image[row1][col1];

      QccVectorZero(output_vector, image_component1->num_rows);

      if (QccFilterVector(input_vector,
                          output_vector,
                          image_component1->num_rows,
                          filter,
                          QCCFILTER_SYMMETRIC_EXTENSION))
        {
          QccErrorAddMessage("(QccIMGImageComponentInterpolateFilter): Error calling QccFilterVector()");
          goto Error;
        }

      for (row2 = 1; row2 < image_component2->num_rows - 1; row2 += 2)
        image_component2->image[row2][col1 * 2] =
          output_vector[row2 / 2];

      image_component2->image[image_component2->num_rows - 1][col1 * 2] =
        image_component2->image[image_component2->num_rows - 2][col1 * 2];
    }
  
  for (row2 = 0; row2 < image_component2->num_rows; row2++)
    {
      for (col1 = 0; col1 < image_component1->num_cols; col1++)
        input_vector[col1] =
          image_component2->image[row2][col1 * 2];

      QccVectorZero(output_vector, image_component1->num_cols);

      if (QccFilterVector(input_vector,
                          output_vector,
                          image_component1->num_cols,
                          filter,
                          QCCFILTER_SYMMETRIC_EXTENSION))
        {
          QccErrorAddMessage("(QccIMGImageComponentInterpolateFilter): Error calling QccFilterVector()");
          goto Error;
        }
      
      for (col2 = 1; col2 < image_component2->num_cols - 1; col2 +=2)
        image_component2->image[row2][col2] =
          output_vector[col2 / 2];

      image_component2->image[row2][image_component2->num_cols - 1] =
        image_component2->image[row2][image_component2->num_cols - 2];
    }
  
  if (QccIMGImageComponentSetMaxMin(image_component2))
    {
      QccErrorAddMessage("(QccIMGImageComponentInterpolateFilter): Error calling QccIMGImageComponentSetMaxMin()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(input_vector);
  QccVectorFree(output_vector);
  return(return_value);
}
