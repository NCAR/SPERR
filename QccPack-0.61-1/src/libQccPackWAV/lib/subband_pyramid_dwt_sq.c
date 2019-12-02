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


int QccWAVSubbandPyramidDWTSQEncode(const QccIMGImageComponent *image,
                                    QccChannel *channels,
                                    const QccSQScalarQuantizer *quantizer,
                                    double *image_mean,
                                    int num_levels,
                                    const QccWAVWavelet *wavelet,
                                    const QccWAVPerceptualWeights
                                    *perceptual_weights)
{
  int return_value;
  int subband;
  int num_subbands;
  QccWAVSubbandPyramid subband_pyramid;
  QccIMGImageComponent *subband_images = NULL;
  int subband_num_rows, subband_num_cols;
  
  if (image == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (channels == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&subband_pyramid);
  
  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);
  
  subband_pyramid.num_levels = 0;
  subband_pyramid.num_rows = image->num_rows;
  subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if ((subband_images =
       (QccIMGImageComponent *)malloc(sizeof(QccIMGImageComponent) *
                                      num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error allocating memory");
      goto Error;
    }
  for (subband = 0; subband < num_subbands; subband++)
    {
      QccIMGImageComponentInitialize(&subband_images[subband]);
      if (QccWAVSubbandPyramidSubbandSize(&subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccWAVSubbandPyramidSubbandSize()");
          goto Error;
        }
      subband_images[subband].num_rows = subband_num_rows;
      subband_images[subband].num_cols = subband_num_cols;
      if (QccIMGImageComponentAlloc(&subband_images[subband]))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
    }
  
  if (QccMatrixCopy(subband_pyramid.matrix, image->image,
                    image->num_rows, image->num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccMatrixCopy()");
      goto Error;
    }
  
  if (QccWAVSubbandPyramidSubtractMean(&subband_pyramid,
                                       image_mean,
                                       NULL))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccWAVSubbandPyramidSubtractMean()");
      goto Error;
    }

  if (QccWAVSubbandPyramidDWT(&subband_pyramid,
                              num_levels,
                              wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccWAVSubbandPyramidDWT()");
      goto Error;
    }
  
  if (perceptual_weights != NULL)
    if (QccWAVPerceptualWeightsApply(&subband_pyramid,
                                     perceptual_weights))
      {
        QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccWAVPerceptualWeightsApply()");
        goto Error;
      }
  
  if (QccWAVSubbandPyramidSplitToImageComponent(&subband_pyramid,
                                                subband_images))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccWAVSubbandPyramidSplitToImageComponent()");
      goto Error;
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    if (QccIMGImageComponentScalarQuantize(&subband_images[subband],
                                           quantizer,
                                           NULL,
                                           &channels[subband]))
      {
        QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQEncode): Error calling QccIMGImageComponentScalarQuantize()");
        goto Error;
      }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramidFree(&subband_pyramid);
  if (subband_images != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccIMGImageComponentFree(&subband_images[subband]);
      QccFree(subband_images);
    }
  return(return_value);
}


int QccWAVSubbandPyramidDWTSQDecode(const QccChannel *channels,
                                    QccIMGImageComponent *image,
                                    const QccSQScalarQuantizer *quantizer,
                                    double image_mean,
                                    int num_levels,
                                    const QccWAVWavelet *wavelet,
                                    const QccWAVPerceptualWeights
                                    *perceptual_weights)
{
  int return_value;
  int num_subbands;
  QccIMGImageComponent *subband_images = NULL;
  int subband_num_rows, subband_num_cols;
  QccWAVSubbandPyramid subband_pyramid;
  int subband;
  
  if (channels == NULL)
    return(0);
  if (image == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&subband_pyramid);
  
  num_subbands = QccWAVSubbandPyramidNumLevelsToNumSubbands(num_levels);
  
  subband_pyramid.num_levels = num_levels;
  subband_pyramid.num_rows = image->num_rows;
  subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if ((subband_images =
       (QccIMGImageComponent *)malloc(sizeof(QccIMGImageComponent) *
                                      num_subbands)) == NULL)
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error allocating memory");
      goto Error;
    }
  for (subband = 0; subband < num_subbands; subband++)
    {
      QccIMGImageComponentInitialize(&subband_images[subband]);
      if (QccWAVSubbandPyramidSubbandSize(&subband_pyramid,
                                          subband,
                                          &subband_num_rows,
                                          &subband_num_cols))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccWAVSubbandPyramidSubbandSize()");
          goto Error;
        }
      subband_images[subband].num_rows = subband_num_rows;
      subband_images[subband].num_cols = subband_num_cols;
      if (QccIMGImageComponentAlloc(&subband_images[subband]))
        {
          QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccIMGImageComponentAlloc()");
          goto Error;
        }
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    if (QccIMGImageComponentInverseScalarQuantize(&channels[subband],
                                                  quantizer,
                                                  &subband_images[subband]))
      {
        QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccIMGImageComponentInverseScalarQuantize()");
        goto Error;
      }
  
  if (QccWAVSubbandPyramidAssembleFromImageComponent(subband_images,
                                                     &subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccWAVSubbandPyramidAssembleFromImageComponent()");
      goto Error;
    }
  
  if (perceptual_weights != NULL)
    if (QccWAVPerceptualWeightsRemove(&subband_pyramid,
                                      perceptual_weights))
      {
        QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccWAVPerceptualWeightsRemove()");
        goto Error;
      }
  
  if (QccWAVSubbandPyramidInverseDWT(&subband_pyramid,
                                     wavelet))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccWAVSubbandPyramidInverseDWT()");
      goto Error;
    }
  
  if (QccWAVSubbandPyramidAddMean(&subband_pyramid,
                                  image_mean))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccWAVSubbandPyramidAddMean()");
      goto Error;
    }

  if (QccMatrixCopy(image->image, subband_pyramid.matrix,
                    subband_pyramid.num_rows,
                    subband_pyramid.num_cols))
    {
      QccErrorAddMessage("(QccWAVSubbandPyramidDWTSQDecode): Error calling QccMatrixCopy()");
      goto Error;
    }
  
  QccIMGImageComponentSetMin(image);
  QccIMGImageComponentSetMax(image);
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (subband_images != NULL)
    {
      for (subband = 0; subband < num_subbands; subband++)
        QccIMGImageComponentFree(&subband_images[subband]);
      QccFree(subband_images);
    }
  QccWAVSubbandPyramidFree(&subband_pyramid);
  return(return_value);
}
