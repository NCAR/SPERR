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


int QccIMGImageComponentScalarQuantize(const
                                       QccIMGImageComponent *image_component,
                                       const QccSQScalarQuantizer *quantizer,
                                       QccVector distortion,
                                       QccChannel *channel)
{
  int row, col;
  int channel_index;

  if (image_component == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (channel == NULL)
    return(0);

  for (row = 0, channel_index = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++, channel_index++)
      if (QccSQScalarQuantization(image_component->image[row][col],
                                  quantizer,
                                  ((distortion != NULL) ?
                                   &(distortion[channel_index]) : NULL),
                                  &(channel->channel_symbols[channel_index])))
        {
          QccErrorAddMessage("(QccIMGImageComponentScalarQuantize): Error calling QccSQScalarQuantzization()");
          return(1);
        }

  return(0);
}


int QccIMGImageComponentInverseScalarQuantize(const QccChannel *channel,
                                              const
                                              QccSQScalarQuantizer *quantizer,
                                              QccIMGImageComponent 
                                              *image_component)
{
  int row, col;
  int channel_index;

  if (image_component == NULL)
    return(0);
  if (quantizer == NULL)
    return(0);
  if (channel == NULL)
    return(0);

  for (row = 0, channel_index = 0; row < image_component->num_rows; row++)
    for (col = 0; col < image_component->num_cols; col++, channel_index++)
      if (QccSQInverseScalarQuantization(channel->channel_symbols
                                         [channel_index],
                                         quantizer,
                                         &(image_component->image[row][col])))
        {
          QccErrorAddMessage("(QccIMGImageComponentInverseScalarQuantize): Error calling QccSQInverseScalarQuantzization()");
          return(1);
        }

  QccIMGImageComponentSetMin(image_component);
  QccIMGImageComponentSetMax(image_component);

  return(0);
}


int QccIMGImageScalarQuantize(const QccIMGImage *image,
                              const QccSQScalarQuantizer *quantizers,
                              QccChannel *channels)
{
  if (image == NULL)
    return(0);
  if (quantizers == NULL)
    return(0);
  if (channels == NULL)
    return(0);

  if (QccIMGImageComponentScalarQuantize(&(image->Y),
                                         &(quantizers[0]),
                                         NULL,
                                         &(channels[0])))
    {
      QccErrorAddMessage("(QccIMGImageScalarQuantize): Error calling QccIMGImageComponentScalarQuantize()");
      return(1);
    }
  
  if (QccIMGImageComponentScalarQuantize(&(image->U),
                                         &(quantizers[1]),
                                         NULL,
                                         &(channels[1])))
    {
      QccErrorAddMessage("(QccIMGImageScalarQuantize): Error calling QccIMGImageComponentScalarQuantize()");
      return(1);
    }
  
  if (QccIMGImageComponentScalarQuantize(&(image->V),
                                         &(quantizers[2]),
                                         NULL,
                                         &(channels[2])))
    {
      QccErrorAddMessage("(QccIMGImageScalarQuantize): Error calling QccIMGImageComponentScalarQuantize()");
      return(1);
    }

  return(0);
}


int QccIMGImageInverseScalarQuantize(const QccChannel *channels,
                                     const QccSQScalarQuantizer *quantizers,
                                     QccIMGImage *image)
{
  if (channels == NULL)
    return(0);
  if (quantizers == NULL)
    return(0);
  if (image == NULL)
    return(0);

  if (QccIMGImageComponentInverseScalarQuantize(&(channels[0]),
                                                &(quantizers[0]),
                                                &(image->Y)))
    {
      QccErrorAddMessage("(QccIMGImageInverseScalarQuantize): Error calling QccIMGImageComponentInverseScalarQuantize()");
      return(1);
    }
  
  if (QccIMGImageComponentInverseScalarQuantize(&(channels[1]),
                                                &(quantizers[1]),
                                                &(image->U)))
    {
      QccErrorAddMessage("(QccIMGImageInverseScalarQuantize): Error calling QccIMGImageComponentInverseScalarQuantize()");
      return(1);
    }
  
  if (QccIMGImageComponentInverseScalarQuantize(&(channels[2]),
                                                &(quantizers[2]),
                                                &(image->V)))
    {
      QccErrorAddMessage("(QccIMGImageInverseScalarQuantize): Error calling QccIMGImageComponentInverseScalarQuantize()");
      return(1);
    }

  return(0);
}

