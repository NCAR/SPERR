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


#ifdef QCC_USE_GSL


int QccFastDCTInitialize(QccFastDCT *transform)
{
  if (transform == NULL)
    return(0);

  transform->signal_workspace = NULL;
  transform->forward_weights = NULL;
  transform->inverse_weights = NULL;
  transform->wavetable = NULL;
  transform->workspace = NULL;

  return(0);
}


int QccFastDCTCreate(QccFastDCT *transform,
                     int length)
{
  int return_value;
  int n;

  if (transform == NULL)
    return(0);
  if (length <= 0)
    {
      QccErrorAddMessage("(QccFastDCTCreate): Invalid DCT length");
      goto Error;
    }

  transform->length = length;

  if ((transform->forward_weights =
       malloc(sizeof(gsl_complex) * length)) == NULL)
    {
      QccErrorAddMessage("(QccFastDCTCreate): Error allocating memory");
      goto Error;
    }

  for (n = 0; n < length; n++)
    transform->forward_weights[n] =
      gsl_complex_div_real(gsl_complex_exp(gsl_complex_rect(0,
                                                            (-n)*M_PI/2/length)),
                           sqrt(2.0 * length));
  transform->forward_weights[0] =
    gsl_complex_div_real(transform->forward_weights[0], M_SQRT2);

  if ((transform->inverse_weights =
       malloc(sizeof(gsl_complex) * length)) == NULL)
    {
      QccErrorAddMessage("(QccFastDCTCreate): Error allocating memory");
      goto Error;
    }

  for (n = 0; n < length; n++)
    transform->inverse_weights[n] =
      gsl_complex_mul_real(gsl_complex_exp(gsl_complex_rect(0,
                                                            n*M_PI/2/length)),
                           sqrt(2.0 * length));
  transform->inverse_weights[0] =
    gsl_complex_div_real(transform->inverse_weights[0], M_SQRT2);

  if (length % 2)
    {
      if ((transform->signal_workspace = QccVectorAlloc(length * 4)) == NULL)
        {
          QccErrorAddMessage("(QccFastDCTCreate): Error calling QccVectorAlloc()");
          goto Error;
        }

      if ((transform->wavetable =
           gsl_fft_complex_wavetable_alloc(length * 2)) == NULL)
        {
          QccErrorAddMessage("(QccFastDCTCreate): Error allocating wavetable");
          goto Error;
        }
      
      if ((transform->workspace =
           gsl_fft_complex_workspace_alloc(length * 2)) == NULL)
        {
          QccErrorAddMessage("(QccFastDCTCreate): Error allocating workspace");
          goto Error;
        }
    }
  else
    {
      if ((transform->signal_workspace = QccVectorAlloc(length * 2)) == NULL)
        {
          QccErrorAddMessage("(QccFastDCTForwardTransform1D): Error calling QccVectorAlloc()");
          goto Error;
        }

      if ((transform->wavetable =
           gsl_fft_complex_wavetable_alloc(length)) == NULL)
        {
          QccErrorAddMessage("(QccFastDCTCreate): Error allocating wavetable");
          goto Error;
        }
      
      if ((transform->workspace =
           gsl_fft_complex_workspace_alloc(length)) == NULL)
        {
          QccErrorAddMessage("(QccFastDCTCreate): Error allocating workspace");
          goto Error;
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
  QccFastDCTFree(transform);
 Return:
  return(return_value);
}


void QccFastDCTFree(QccFastDCT *transform)
{
  if (transform == NULL)
    return;

  QccVectorFree(transform->signal_workspace);
  if (transform->forward_weights != NULL)
    QccFree(transform->forward_weights);
  if (transform->inverse_weights != NULL)
    QccFree(transform->inverse_weights);
  if (transform->wavetable != NULL)
    gsl_fft_complex_wavetable_free(transform->wavetable);
  if (transform->workspace != NULL)
    gsl_fft_complex_workspace_free(transform->workspace);

}


int QccFastDCTForwardTransform1D(const QccVector input_signal,
                                 QccVector output_signal,
                                 int length,
                                 const QccFastDCT *transform)
{
  int return_value;
  int index;

  if (transform == NULL)
    return(0);
  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);

  if (length != transform->length)
    {
      QccErrorAddMessage("(QccFastDCTForwardTransform1D): Transform length must match that of signals");
      goto Error;
    }

  if (length % 2)
    {
      if (QccVectorZero(transform->signal_workspace, length * 4))
        {
          QccErrorAddMessage("(QccFastDCTForwardTransform1D): Error calling QccVectorZero()");
          goto Error;
        }
      for (index = 0; index < length; index++)
        {
          transform->signal_workspace[2 * index] = input_signal[index];
          transform->signal_workspace[2 * index + length * 2] =
            input_signal[length - index - 1];
        }      

      if (gsl_fft_complex_forward((gsl_complex_packed_array)
                                  transform->signal_workspace,
                                  1,
                                  length * 2,
                                  transform->wavetable,
                                  transform->workspace))
        {
          QccErrorAddMessage("(QccFastDCTForwardTransform1D): Error performing fft");
          goto Error;
        }

      for (index = 0; index < length; index++)
        output_signal[index] =
          GSL_REAL(gsl_complex_mul(gsl_complex_rect(transform->signal_workspace
                                                    [2 * index],
                                                    transform->signal_workspace
                                                    [2 * index + 1]),
                                   transform->forward_weights[index]));
    }
  else
    {
      if (QccVectorZero(transform->signal_workspace, length * 2))
        {
          QccErrorAddMessage("(QccFastDCTForwardTransform1D): Error calling QccVectorZero()");
          goto Error;
        }
      
      for (index = 0; index < length; index += 2)
        {
          transform->signal_workspace[index] = input_signal[index];
          transform->signal_workspace[index + length] =
            input_signal[length - index - 1];
        }
      
      if (gsl_fft_complex_forward((gsl_complex_packed_array)
                                  transform->signal_workspace,
                                  1,
                                  length,
                                  transform->wavetable,
                                  transform->workspace))
        {
          QccErrorAddMessage("(QccFastDCTForwardTransform1D): Error performing fft");
          goto Error;
        }
      
      for (index = 0; index < length; index++)
        output_signal[index] =
          GSL_REAL(gsl_complex_mul(gsl_complex_rect(transform->signal_workspace
                                                    [2 * index],
                                                    transform->signal_workspace
                                                    [2 * index + 1]),
                                   transform->forward_weights[index])) * 2;
    }
      
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccFastDCTInverseTransform1D(const QccVector input_signal,
                                 QccVector output_signal,
                                 int length,
                                 const QccFastDCT *transform)
{
  int return_value;
  gsl_complex temp_value;
  int index;

  if (length != transform->length)
    {
      QccErrorAddMessage("(QccFastDCTInverseTransform1D): Transform length must match that of signals");
      goto Error;
    }

  if (length % 2)
    {
      if (QccVectorZero(transform->signal_workspace, length * 4))
        {
          QccErrorAddMessage("(QccFastDCTInverseTransform1D): Error calling QccVectorZero()");
          goto Error;
        }
      for (index = 0; index < length; index++)
        {
          temp_value =
            gsl_complex_mul(gsl_complex_rect(input_signal[index], 0),
                            transform->inverse_weights[index]);
          transform->signal_workspace[2 * index] = GSL_REAL(temp_value);
          transform->signal_workspace[2 * index + 1] = GSL_IMAG(temp_value);
        }      
      transform->signal_workspace[0] *= 2;
      transform->signal_workspace[1] *= 2;

      for (index = 0; index < length - 1; index++)
        {
          temp_value =
            gsl_complex_mul(gsl_complex_rect(0,
                                             -input_signal[length -
                                                           index - 1]),
                            transform->inverse_weights[index + 1]);
          transform->signal_workspace[2 * (index + length) + 2] =
            GSL_REAL(temp_value);
          transform->signal_workspace[2 * (index + length) + 3] =
            GSL_IMAG(temp_value);
        }

      if (gsl_fft_complex_inverse((gsl_complex_packed_array)
                                  transform->signal_workspace,
                                  1,
                                  length * 2,
                                  transform->wavetable,
                                  transform->workspace))
        {
          QccErrorAddMessage("(QccFastDCTInverseTransform1D): Error performing inverse fft");
          goto Error;
        }

      for (index = 0; index < length; index ++)
        output_signal[index] = transform->signal_workspace[2 * index];
    }
  else
    {
      if (QccVectorZero(transform->signal_workspace, length * 2))
        {
          QccErrorAddMessage("(QccFastDCTInverseTransform1D): Error calling QccVectorZero()");
          goto Error;
        }
      
      for (index = 0; index < length; index++)
        {
          temp_value =
            gsl_complex_mul(gsl_complex_rect(input_signal[index], 0),
                            transform->inverse_weights[index]);
          transform->signal_workspace[2 * index] = GSL_REAL(temp_value);
          transform->signal_workspace[2 * index + 1] = GSL_IMAG(temp_value);
        }
      
      if (gsl_fft_complex_inverse((gsl_complex_packed_array)
                                  transform->signal_workspace,
                                  1,
                                  length,
                                  transform->wavetable,
                                  transform->workspace))
        {
          QccErrorAddMessage("(QccFastDCTInverseTransform1D): Error performing inverse fft");
          goto Error;
        }
      
      for (index = 0; index < length; index += 2)
        {
          output_signal[index] = transform->signal_workspace[index];
          output_signal[index + 1] =
            transform->signal_workspace[2*length - index - 2];
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccFastDCTForwardTransform2D(const QccMatrix input_block,
                                 QccMatrix output_block,
                                 int num_rows,
                                 int num_cols,
                                 const QccFastDCT *transform_horizontal,
                                 const QccFastDCT *transform_vertical)
{
  int return_value;
  QccVector col_vector1 = NULL;
  QccVector col_vector2 = NULL;
  int row, col;
  
  if ((input_block == NULL) ||
      (output_block == NULL))
    return(0);
  if ((transform_horizontal == NULL) ||
      (transform_vertical == NULL))
    return(0);

  if ((num_rows <= 0) || (num_cols <= 0))
    return(0);
  
  if ((num_rows != transform_vertical->length) ||
      (num_cols != transform_horizontal->length))
    {
      QccErrorAddMessage("(QccFastDCTForwardTransform2D): Transform lengths must match that of blocks both horizontally and vertically");
      goto Error;
    }

  if ((col_vector1 = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccFastDCTForwardTransform2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((col_vector2 = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccFastDCTForwardTransform2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    if (QccFastDCTForwardTransform1D(input_block[row],
                                     output_block[row],
                                     num_cols,
                                     transform_horizontal))
      {
        QccErrorAddMessage("(QccFastDCTForwardTransform2D): Error calling QccFastDCTForwardTransform1D()");
        goto Error;
      }
  
  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        col_vector1[row] = output_block[row][col];
      if (QccFastDCTForwardTransform1D(col_vector1,
                                       col_vector2,
                                       num_rows,
                                       transform_vertical))
        {
          QccErrorAddMessage("(QccFastDCTForwardTransform2D): Error calling QccFastDCTForwardTransform1D()");
          goto Error;
        }
      for (row = 0; row < num_rows; row++)
        output_block[row][col] = col_vector2[row];
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(col_vector1);
  QccVectorFree(col_vector2);
  return(return_value);
}


int QccFastDCTInverseTransform2D(const QccMatrix input_block,
                                 QccMatrix output_block,
                                 int num_rows,
                                 int num_cols,
                                 const QccFastDCT *transform_horizontal,
                                 const QccFastDCT *transform_vertical)
{
  int return_value;
  QccVector col_vector1 = NULL;
  QccVector col_vector2 = NULL;
  int row, col;
  
  if ((input_block == NULL) ||
      (output_block == NULL))
    return(0);
  if ((transform_horizontal == NULL) ||
      (transform_vertical == NULL))
    return(0);

  if ((num_rows <= 0) || (num_cols <= 0))
    return(0);
  
  if ((num_rows != transform_vertical->length) ||
      (num_cols != transform_horizontal->length))
    {
      QccErrorAddMessage("(QccFastDCTInverseTransform2D): Transform lengths must match that of blocks both horizontally and vertically");
      goto Error;
    }

  if ((col_vector1 = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccFastDCTInverseTransform2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  if ((col_vector2 = QccVectorAlloc(num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccFastDCTInverseTransform2D): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  for (row = 0; row < num_rows; row++)
    if (QccFastDCTInverseTransform1D(input_block[row],
                                     output_block[row],
                                     num_cols,
                                     transform_horizontal))
      {
        QccErrorAddMessage("(QccFastDCTInverseTransform2D): Error calling QccFastDCTInverseTransform1D()");
        goto Error;
      }
  
  for (col = 0; col < num_cols; col++)
    {
      for (row = 0; row < num_rows; row++)
        col_vector1[row] = output_block[row][col];
      if (QccFastDCTInverseTransform1D(col_vector1,
                                       col_vector2,
                                       num_rows,
                                       transform_vertical))
        {
          QccErrorAddMessage("(QccFastDCTInverseTransform2D): Error calling QccFastDCTInverseTransform1D()");
          goto Error;
        }
      for (row = 0; row < num_rows; row++)
        output_block[row][col] = col_vector2[row];
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(col_vector1);
  QccVectorFree(col_vector2);
  return(return_value);
}


#else


int QccFastDCTInitialize(QccFastDCT *transform)
{
  QccErrorAddMessage("(QccFastDCTInitialize): GSL support is not available -- fast DCT is thus not supported");
  return(1);
}


int QccFastDCTCreate(QccFastDCT *transform,
                     int length)
{
  QccErrorAddMessage("(QccFastDCTCreate): GSL support is not available -- fast DCT is thus not supported");
  return(1);
}


void QccFastDCTFree(QccFastDCT *transform)
{
  QccErrorAddMessage("(QccFastDCTFree): GSL support is not available -- fast DCT is thus not supported");
}


int QccFastDCTForwardTransform1D(const QccVector input_signal,
                                 QccVector output_signal,
                                 int length,
                                 const QccFastDCT *transform)
{
  QccErrorAddMessage("(QccFastDCTForwardTransform1D): GSL support is not available -- fast DCT is thus not supported");
  return(1);
}


int QccFastDCTInverseTransform1D(const QccVector input_signal,
                                 QccVector output_signal,
                                 int length,
                                 const QccFastDCT *transform)
{
  QccErrorAddMessage("(QccFastDCTInverseTransform1D): GSL support is not available -- fast DCT is thus not supported");
  return(1);
}


int QccFastDCTForwardTransform2D(const QccMatrix input_block,
                                 QccMatrix output_block,
                                 int num_rows,
                                 int num_cols,
                                 const QccFastDCT *transform_horizontal,
                                 const QccFastDCT *transform_vertical)
{
  QccErrorAddMessage("(QccFastDCTForwardTransform2D): GSL support is not available -- fast DCT is thus not supported");
  return(1);
}


int QccFastDCTInverseTransform2D(const QccMatrix input_block,
                                 QccMatrix output_block,
                                 int num_rows,
                                 int num_cols,
                                 const QccFastDCT *transform_horizontal,
                                 const QccFastDCT *transform_vertical)
{
  QccErrorAddMessage("(QccFastDCTInverseTransform2D): GSL support is not available -- fast DCT is thus not supported");
  return(1);
}


#endif
