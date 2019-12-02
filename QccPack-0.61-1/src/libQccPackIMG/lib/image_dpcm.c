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


static double QccIMGImageComponentDPCMPredictor0(double A,
                                                 double B,
                                                 double C,
                                                 double D)
{
  return(A);
}


static double QccIMGImageComponentDPCMPredictor1(double A,
                                                 double B,
                                                 double C,
                                                 double D)
{
  return((A + D)/2);
}


static double QccIMGImageComponentDPCMPredictor2(double A,
                                                 double B,
                                                 double C,
                                                 double D)
{
  return((A + C)/2);
}


static double QccIMGImageComponentDPCMPredictor3(double A,
                                                 double B,
                                                 double C,
                                                 double D)
{
  return((A + (C + D)/2)/2);
}


static double QccIMGImageComponentDPCMPredictor4(double A,
                                                 double B,
                                                 double C,
                                                 double D)
{
  return(A + (C - B));
}


static double QccIMGImageComponentDPCMPredictor5(double A,
                                                 double B,
                                                 double C,
                                                 double D)
{
  return(A + (D - B)/2);
}


typedef double (*QccIMGImageComponentDPCMPredictorFunction)(double A, 
                                                            double B, 
                                                            double C,
                                                            double D);


static const QccIMGImageComponentDPCMPredictorFunction
QccIMGImageComponentDPCMPredictors[QCCIMGPREDICTOR_NUMPREDICTORS] =
{
  QccIMGImageComponentDPCMPredictor0,
  QccIMGImageComponentDPCMPredictor1,
  QccIMGImageComponentDPCMPredictor2,
  QccIMGImageComponentDPCMPredictor3,
  QccIMGImageComponentDPCMPredictor4,
  QccIMGImageComponentDPCMPredictor5
};


static void QccIMGImageComponentDPCMGetPreviousPixels(const
                                                      QccIMGImageComponent 
                                                      *reconstructed_image, 
                                                      int row, int col,
                                                      double 
                                                      out_of_bound_value,
                                                      double *A, 
                                                      double *B, 
                                                      double *C, 
                                                      double *D)
{
  *A = col ? reconstructed_image->image[row][col - 1] : out_of_bound_value;
  *B = row ? (col ? reconstructed_image->image[row - 1][col - 1] :
              out_of_bound_value) : out_of_bound_value;
  *C = row ? reconstructed_image->image[row - 1][col] : out_of_bound_value;
  *D = row ? ((col < reconstructed_image->num_cols - 1) ? 
              reconstructed_image->image[row - 1][col + 1] :
              out_of_bound_value) : out_of_bound_value;
}


int QccIMGImageComponentDPCMEncode(const QccIMGImageComponent *image_component,
                                   const QccSQScalarQuantizer *quantizer,
                                   double out_of_bound_value,
                                   int predictor_code,
                                   QccIMGImageComponent *difference_image,
                                   QccChannel *channel)
{
  int row, col;
  double diff;
  int channel_symbol;
  double A, B, C, D;
  double predicted_value;
  int channel_index;
  QccIMGImageComponent reconstructed_image;
  int return_value;

  QccIMGImageComponentInitialize(&reconstructed_image);

  if (image_component == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  
  if ((predictor_code < 0) || 
      (predictor_code >= QCCIMGPREDICTOR_NUMPREDICTORS))
    {
      QccErrorAddMessage("(QccIMGImageComponentDPCMEncode): Predictor code (%d) is invalid",
                         predictor_code);
      goto Error;
    }
  
  reconstructed_image.num_rows = image_component->num_rows;
  reconstructed_image.num_cols = image_component->num_cols;
  if (QccIMGImageComponentAlloc(&reconstructed_image))
    {
      QccErrorAddMessage("(QccIMGImageComponentDPCMEncode): Predictor code (%d) is invalid",
                         predictor_code);
      goto Error;
    }
  
  for (row = 0, channel_index = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++, channel_index++)
      {
        QccIMGImageComponentDPCMGetPreviousPixels(&reconstructed_image, 
                                                  row, col,
                                                  out_of_bound_value,
                                                  &A, &B, &C, &D);

        predicted_value = 
          (QccIMGImageComponentDPCMPredictors[predictor_code])(A, B, C, D);

        diff = image_component->image[row][col] - predicted_value;

        if (difference_image != NULL)
          difference_image->image[row][col] = diff;

        if (QccSQScalarQuantization(diff,
                                    quantizer,
                                    NULL,
                                    &channel_symbol))
          {
            QccErrorAddMessage("(QccIMGImageComponentDPCMEncode): Error calling QccScalarQuantization()");
            goto Error;
          }

        if (QccSQInverseScalarQuantization(channel_symbol,
                                           quantizer,
                                           &(reconstructed_image.image
                                             [row][col])))
          {
            QccErrorAddMessage("(QccIMGImageComponentDPCMEncode): Error calling QccInverseScalarQuantization()");
            goto Error;
          }

        reconstructed_image.image[row][col] += predicted_value;
        
        if (channel != NULL)
          channel->channel_symbols[channel_index] = channel_symbol;
      }
  
  if (quantizer->type != QCCSQSCALARQUANTIZER_GENERAL)
    if (QccChannelNormalize(channel))
      {
        QccErrorAddMessage("(QccIMGImageComponentDPCMEncode): Error calling QccChannelNormalize()");
        goto Error;
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccIMGImageComponentFree(&reconstructed_image);
  return(return_value);
}


int QccIMGImageComponentDPCMDecode(QccChannel *channel,
                                   const QccSQScalarQuantizer *quantizer,
                                   double out_of_bound_value,
                                   int predictor_code,
                                   QccIMGImageComponent *image_component)
{
  int row, col;
  int channel_index;
  double diff;
  double A, B, C, D;
  
  if (channel == NULL)
    return(0);
  if (channel->channel_symbols == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (image_component == NULL)
    return(0);
  if (image_component->image == NULL)
    return(0);

  if (quantizer->type != QCCSQSCALARQUANTIZER_GENERAL)
    if (QccChannelDenormalize(channel))
      {
        QccErrorAddMessage("(QccIMGImageComponentDPCMDecode): Error calling QccChannelDenormalize()");
        return(1);
      }

  for (row = 0, channel_index = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++, channel_index++)
      {
        if (QccSQInverseScalarQuantization(channel->channel_symbols
                                           [channel_index],
                                           quantizer,
                                           &diff))
          {
            QccErrorAddMessage("(QccIMGImageComponentDPCMDecode): Error calling QccSQInverseScalarQuantization()");
            return(1);
          }

        QccIMGImageComponentDPCMGetPreviousPixels(image_component,
                                                  row, col,
                                                  out_of_bound_value,
                                                  &A, &B, &C, &D);

        image_component->image[row][col] = diff +
          (QccIMGImageComponentDPCMPredictors[predictor_code])(A, B, C, D);
      }
  
  QccIMGImageComponentSetMin(image_component);
  QccIMGImageComponentSetMax(image_component);
  
  return(0);
}


int QccIMGImageDPCMEncode(const QccIMGImage *image,
                          const QccSQScalarQuantizer *quantizers,
                          const QccVector out_of_bound_values,
                          int predictor_code,
                          QccIMGImage *difference_image,
                          QccChannel *channels)
{
  if (image == NULL)
    return(0);
  if (quantizers == NULL)
    return(0);
  
  if (QccIMGImageComponentDPCMEncode(&(image->Y),
                                     &(quantizers[0]),
                                     (out_of_bound_values != NULL) ?
                                     out_of_bound_values[0] : (double)0.0,
                                     predictor_code,
                                     (difference_image != NULL) ?
                                     &(difference_image->Y) : NULL,
                                     (channels != NULL) ?
                                     &(channels[0]) : NULL))
    {
      QccErrorAddMessage("(QccIMGImageDPCMEncode): Error calling QccIMGImageComponentDPCMEncode()");
      return(1);
    }
  
  if (QccIMGImageComponentDPCMEncode(&(image->U),
                                     &(quantizers[1]),
                                     (out_of_bound_values != NULL) ?
                                     out_of_bound_values[1] : (double)0.0,
                                     predictor_code,
                                     (difference_image != NULL) ?
                                     &(difference_image->U) : NULL,
                                     (channels != NULL) ?
                                     &(channels[1]) : NULL))
    {
      QccErrorAddMessage("(QccIMGImageDPCMEncode): Error calling QccIMGImageComponentDPCMEncode()");
      return(1);
    }
  
  if (QccIMGImageComponentDPCMEncode(&(image->V),
                                     &(quantizers[2]),
                                     (out_of_bound_values != NULL) ?
                                     out_of_bound_values[2] : (double)0.0,
                                     predictor_code,
                                     (difference_image != NULL) ?
                                     &(difference_image->V) : NULL,
                                     (channels != NULL) ?
                                     &(channels[2]) : NULL))
    {
      QccErrorAddMessage("(QccIMGImageDPCMEncode): Error calling QccIMGImageComponentDPCMEncode()");
      return(1);
    }
  
  return(0);
}


int QccIMGImageDPCMDecode(QccChannel *channels,
                          const QccSQScalarQuantizer *quantizers,
                          const QccVector out_of_bound_values,
                          int predictor_code,
                          QccIMGImage *image)
{
  if (channels == NULL)
    return(0);
  if (channels->channel_symbols == NULL)
    return(0);
  if (quantizers == NULL)
    return(0);
  if (image == NULL)
    return(0);
  
  if (QccIMGImageComponentDPCMDecode(&(channels[0]),
                                     &(quantizers[0]),
                                     (out_of_bound_values != NULL) ?
                                     out_of_bound_values[0] : (double)0.0,
                                     predictor_code,
                                     &(image->Y)))
    
    {
      QccErrorAddMessage("(QccIMGImageDPCMDecode): Error calling QccIMGImageComponentDPCMDecode()");
      return(1);
    }
  
  if (QccIMGImageComponentDPCMDecode(&(channels[1]),
                                     &(quantizers[1]),
                                     (out_of_bound_values != NULL) ?
                                     out_of_bound_values[1] : (double)0.0,
                                     predictor_code,
                                     &(image->U)))
    
    {
      QccErrorAddMessage("(QccIMGImageDPCMDecode): Error calling QccIMGImageComponentDPCMDecode()");
      return(1);
    }
  
  if (QccIMGImageComponentDPCMDecode(&(channels[2]),
                                     &(quantizers[2]),
                                     (out_of_bound_values != NULL) ?
                                     out_of_bound_values[2] : (double)0.0,
                                     predictor_code,
                                     &(image->V)))
    
    {
      QccErrorAddMessage("(QccIMGImageDPCMDecode): Error calling QccIMGImageComponentDPCMDecode()");
      return(1);
    }
  
  return(0);
}
